"""Opt-in, exact-build AdFitPM callbacks, installed before IPA signing.

The two Swift functions take only their Swift context and return Void. The
disabled path executes the displaced instruction and continues unchanged.
No writable executable memory or JIT is used at runtime.
"""
import hashlib
import struct

SDK_SHA256 = '3f4cfaffcf29328567476741adfeecf70120a4b10629d2c9e3bc4ef6dd2b285b'
SDK_UUID = bytes.fromhex('3bfc9c526714378e8b5e847b0fc15290')
CAVE = 0x3000
MARKER = 0x33F0
MAGIC = b'GCADS001'
FLAGS = 0x507FE0
CALLBACK = FLAGS + 8
ENTRIES = ((0x131B40, 0xA9BC5FF8), (0x28B4D0, 0xA9BA6FFC))


def branch(pc, target):
    delta = target - pc
    if delta % 4 or not -(1 << 27) <= delta < (1 << 27):
        raise ValueError('Invalid advertisement branch.')
    return 0x14000000 | ((delta // 4) & 0x3FFFFFF)


def load_slot(pc, address, register, wide=False):
    delta = (address >> 12) - (pc >> 12)
    if not -(1 << 20) <= delta < (1 << 20):
        raise ValueError('Advertisement slot is outside ADRP range.')
    scale = 8 if wide else 4
    if address % scale:
        raise ValueError('Unaligned advertisement slot.')
    return [0x90000000 | ((delta & 3) << 29) |
            (((delta >> 2) & 0x7FFFF) << 5) | register,
            (0xF9400000 if wide else 0xB9400000) |
            (((address & 0xFFF) // scale) << 10) | (register << 5) | register]


def patch(data):
    from prepare_ipa import commands
    if hashlib.sha256(data).hexdigest() != SDK_SHA256:
        raise ValueError('Advertisement callbacks require the exact original AdFitPM bundled with the verified 26.8.0 IPA.')
    uuid = None
    text_padding = False
    data_padding = False
    for off, cmd, size in commands(data):
        if cmd == 0x1B and size == 24:
            uuid = data[off + 8:off + 24]
        if cmd in (0x21, 0x2C) and struct.unpack_from('<I', data, off + 16)[0]:
            raise ValueError('Encrypted advertisement SDK is unsupported.')
        if cmd != 0x19:
            continue
        va, length, fileoff, filesize = struct.unpack_from('<QQQQ', data, off + 24)
        protection, count = struct.unpack_from('<II', data, off + 60)
        if 72 + count * 80 > size:
            raise ValueError('Invalid advertisement section table.')
        if va == 0 and fileoff == 0 and protection & 5 == 5:
            first = min((struct.unpack_from('<I', data, off + 72 + i * 80 + 48)[0]
                         for i in range(count)), default=0)
            text_padding = 32 + struct.unpack_from('<I', data, 20)[0] <= CAVE and MARKER + 8 <= first
        if va <= FLAGS and CALLBACK + 8 <= va + length and protection & 3 == 3:
            data_padding = True
            for i in range(count):
                address, section_size = struct.unpack_from('<QQ', data, off + 72 + i * 80 + 32)
                if address < CALLBACK + 8 and address + section_size > FLAGS:
                    raise ValueError('Advertisement slots overlap SDK data.')
    if uuid != SDK_UUID or not text_padding or not data_padding:
        raise ValueError('Advertisement SDK layout is unsupported.')
    if any(data[CAVE:MARKER + 8]):
        raise ValueError('Advertisement header space is occupied.')
    result = bytearray(data)
    for route, (entry, original) in enumerate(ENTRIES):
        if struct.unpack_from('<I', data, entry)[0] != original:
            raise ValueError('Unexpected advertisement function prologue.')
        pc = CAVE + route * 0x80
        # x16/x17 are scratch registers; the native Void methods have no
        # arguments other than Swift self (x20). Preserve self, LR and SP over
        # the optional C callback. No object or payload is passed to it.
        words = load_slot(pc, FLAGS, 16) + [
            0x36000000 | (10 << 5) | 16,  # tbz w16,#0,pc+48
            0xA9BF7BF4,                 # stp x20,x30,[sp,#-16]!
            *load_slot(pc + 16, CALLBACK, 17, wide=True),
            0xB4000000 | (3 << 5) | 17, # cbz x17,pc+36
            0x52800000 | (route << 5),   # mov w0,#route
            0xD63F0220,                 # blr x17
            0xA8C17BF4,                 # ldp x20,x30,[sp],#16
            0xD65F03C0,                 # ret
            0xD503201F,                 # nop
            original,
            branch(pc + 52, entry + 4),
        ]
        for i, word in enumerate(words):
            struct.pack_into('<I', result, pc + i * 4, word)
        struct.pack_into('<I', result, entry, branch(entry, pc))
    result[MARKER:MARKER + 8] = MAGIC
    return result
