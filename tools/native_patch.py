"""Version-checked native callbacks for an already decrypted KakaoTalk executable.

Code is prepared on disk before iOS signs it. No JIT, executable memory writes,
downloaded machine code, encryption bypass or network interception is used.
"""
import hashlib
import struct
from types import SimpleNamespace
from native_layouts import NATIVE_26_8

BASE = 0x100000000
SAVE_ENTRY = 0x104631934
CALLBACK_SLOT = 0x1083E7F08
CAVE = 0x7400
MARKER = 0x77F0
MAGIC = b'GCAMPLE1'
FEATURE_CAVE = 0x7800
FEATURE_MARKER = 0x7FF0
FEATURE_MAGIC = b'GCOPT003'
FEATURE_FLAGS = 0x1083E7F10
UNREAD_CLAMP = 0x1017BECC8
UNREAD_CONTINUE = 0x1017BECCC
UNREAD_RETURN = 0x1017BECDC
# Native Bool getters whose first instruction is relocatable SUB SP, SP, #128.
# bit 1: outgoing markdown attachment; bit 2: leverage forwarding policy.
BOOL_GETTERS = [(0x1046ADE0C, 1), (0x1046ACEE0, 2)]
COVER_ENTRY = 0x100B153B4
COVER_PROLOGUE = 0x0A010008  # and w8,w0,w1
ALIMTALK_GETTER = 0x104679D4C
ALIMTALK_CALLSITES = [0x10078C178, 0x1007B752C]
MORE_TAB_AVAILABLE = 0x1008133D0
MORE_TAB_PROLOGUE = 0x72001C08  # ands w8,w0,#0xff
ORIGINAL_TEXT_SHA256 = 'c2195ceae06edac6086aeca06c1b2a65fcaabee26e20169f739b134d20b0a0eb'


def default_layout():
    names = ['SAVE_ENTRY', 'CALLBACK_SLOT', 'FEATURE_FLAGS', 'UNREAD_CLAMP', 'UNREAD_CONTINUE', 'UNREAD_RETURN', 'BOOL_GETTERS', 'COVER_ENTRY', 'ALIMTALK_GETTER', 'ALIMTALK_CALLSITES', 'MORE_TAB_AVAILABLE', 'VERIFIED_TEXT_HASHES']
    return SimpleNamespace(**{name: globals()[name] for name in names})


def text_section(data):
    if len(data) < 32 or struct.unpack_from('<II', data) != (0xFEEDFACF, 0x100000C):
        raise ValueError('Expected a thin arm64 Mach-O executable.')
    offset = 32
    count, total = struct.unpack_from('<II', data, 16)
    for _ in range(count):
        if offset + 8 > 32 + total:
            raise ValueError('Invalid load commands.')
        command, size = struct.unpack_from('<II', data, offset)
        if size < 8 or offset + size > 32 + total:
            raise ValueError('Invalid load command size.')
        if command == 0x19:
            sections = struct.unpack_from('<I', data, offset + 64)[0]
            if 72 + sections * 80 > size:
                raise ValueError('Invalid section table.')
            for index in range(sections):
                section = offset + 72 + index * 80
                if data[section:section + 16].split(b'\0')[0] == b'__text':
                    length, position = struct.unpack_from('<QI', data, section + 40)
                    if position + length > len(data):
                        raise ValueError('Text section extends past file.')
                    return position, length
        offset += size
    raise ValueError('Missing text section.')


def branch(pc, target):
    delta = target - pc
    if delta % 4 or not -(1 << 27) <= delta < (1 << 27):
        raise ValueError('Branch is unaligned or out of range.')
    return 0x14000000 | ((delta // 4) & 0x3FFFFFF)


def flag_load(pc, layout=None):
    layout = layout or default_layout()
    delta=(layout.FEATURE_FLAGS>>12)-(pc>>12)
    if not -(1<<20)<=delta<(1<<20): raise ValueError('Flag address is outside ADRP range.')
    return [0x90000010|((delta&3)<<29)|(((delta>>2)&0x7ffff)<<5),
            0xb9400000|(((layout.FEATURE_FLAGS&0xfff)//4)<<10)|(16<<5)|16]


def bit_branch(pc,target,bit,nonzero=False):
    delta=target-pc
    if delta%4 or not -(1<<15)<=delta<(1<<15) or not 0<=bit<32:
        raise ValueError('Invalid local flag branch.')
    return (0x37000000 if nonzero else 0x36000000)|(bit<<19)|(((delta//4)&0x3fff)<<5)|16


def patch_features(binary, layout=None):
    layout = layout or default_layout()
    if any(binary[FEATURE_CAVE:FEATURE_MARKER+8]):
        raise ValueError('Native feature header space is not empty.')
    if struct.unpack_from('<I',binary,layout.UNREAD_CLAMP-BASE)[0]!=0x540000AC:
        raise ValueError('Unexpected unread count clamp instruction.')
    pc=BASE+FEATURE_CAVE
    # ADRP, LDR and test-bit branches preserve NZCV from the native CMP. The
    # leaf calculator never uses x16. Disabled follows the original B.GT path.
    words=flag_load(pc, layout)+[bit_branch(pc+8,pc+20,0,True),0x5400004D,
                         branch(pc+16,layout.UNREAD_RETURN),branch(pc+20,layout.UNREAD_CONTINUE)]
    for i,word in enumerate(words):struct.pack_into('<I',binary,FEATURE_CAVE+4*i,word)
    struct.pack_into('<I',binary,layout.UNREAD_CLAMP-BASE,branch(layout.UNREAD_CLAMP,pc))
    for index,(entry,bit) in enumerate(layout.BOOL_GETTERS):
        original=struct.unpack_from('<I',binary,entry-BASE)[0]
        if original!=0xD10203FF:raise ValueError('Unexpected native Bool getter prologue.')
        offset=FEATURE_CAVE+32+index*32;pc=BASE+offset
        if offset+28>FEATURE_MARKER:raise ValueError('Native feature header space exhausted.')
        words=flag_load(pc, layout)+[bit_branch(pc+8,pc+20,bit),0x52800020,0xD65F03C0,
                             original,branch(pc+24,entry+4)]
        for i,word in enumerate(words):struct.pack_into('<I',binary,offset+i*4,word)
        struct.pack_into('<I',binary,entry-BASE,branch(entry,pc))
    # Universal cover policy takes six Bool arguments. Suppress only the
    # sub-device consent and mobile-only conditions (w2,w3); validity, version,
    # locked-content and adult-content checks keep their native behavior.
    original=struct.unpack_from('<I',binary,layout.COVER_ENTRY-BASE)[0]
    if original!=COVER_PROLOGUE:raise ValueError('Unexpected Universal cover policy entry.')
    offset=FEATURE_CAVE+32+len(layout.BOOL_GETTERS)*32;pc=BASE+offset
    if offset+28>FEATURE_MARKER:raise ValueError('Native feature header space exhausted.')
    words=flag_load(pc, layout)+[bit_branch(pc+8,pc+20,3),0x52800002,0x52800003,
                         original,branch(pc+24,layout.COVER_ENTRY+4)]
    for i,word in enumerate(words):struct.pack_into('<I',binary,offset+4*i,word)
    struct.pack_into('<I',binary,layout.COVER_ENTRY-BASE,branch(layout.COVER_ENTRY,pc))
    # Only message rendering reads are changed. The same preference getter is
    # also used by settings; those call sites keep their actual saved value.
    offset+=32;pc=BASE+offset
    if offset+24>FEATURE_MARKER:raise ValueError('Native feature header space exhausted.')
    words=flag_load(pc, layout)+[bit_branch(pc+8,pc+20,3),0x52800020,0xD65F03C0,branch(pc+20,layout.ALIMTALK_GETTER)]
    for i,word in enumerate(words):struct.pack_into('<I',binary,offset+4*i,word)
    for site in layout.ALIMTALK_CALLSITES:
        if struct.unpack_from('<I',binary,site-BASE)[0]!=(branch(site,layout.ALIMTALK_GETTER)|0x80000000):
            raise ValueError('Unexpected Alimtalk message rendering call.')
        struct.pack_into('<I',binary,site-BASE,branch(site,pc)|0x80000000)
    # Shared More-tab availability policy: enum 0=home, 1=wallet, 2=game.
    # Both chip construction and the native selected-tab fallback use it.
    # Leave home/wallet and the original saved selection entirely unchanged.
    original=struct.unpack_from('<I',binary,layout.MORE_TAB_AVAILABLE-BASE)[0]
    if original!=MORE_TAB_PROLOGUE:raise ValueError('Unexpected More tab availability policy.')
    offset+=32;pc=BASE+offset
    if offset+40>FEATURE_MARKER:raise ValueError('Native feature header space exhausted.')
    words=flag_load(pc, layout)+[bit_branch(pc+8,pc+32,4),
        0x12001C10,  # and w16,w0,#0xff
        0x71000A1F,  # cmp w16,#2
        0x54000061,  # b.ne original
        0x52800000,0xD65F03C0,original,branch(pc+36,layout.MORE_TAB_AVAILABLE+4)]
    for i,word in enumerate(words):struct.pack_into('<I',binary,offset+4*i,word)
    struct.pack_into('<I',binary,layout.MORE_TAB_AVAILABLE-BASE,branch(layout.MORE_TAB_AVAILABLE,pc))
    binary[FEATURE_MARKER:FEATURE_MARKER+8]=FEATURE_MAGIC


def patch(data, feature_patches=True):
    from prepare_ipa import commands
    binary = bytearray(data)
    start, length = text_section(binary)
    fingerprint = hashlib.sha256(binary[start:start + length]).hexdigest()
    layout = default_layout()
    if fingerprint in NATIVE_26_8["VERIFIED_TEXT_HASHES"]:
        layout = SimpleNamespace(**{**vars(layout), **NATIVE_26_8})
    padding_found = False
    for offset, command, size in commands(binary):
        if command != 0x19 or size < 72:
            continue
        va, vm_size = struct.unpack_from('<QQ', binary, offset + 24)
        protection = struct.unpack_from('<I', binary, offset + 60)[0]
        padding_end=layout.FEATURE_FLAGS+8 if feature_patches else layout.CALLBACK_SLOT+8
        if va <= layout.CALLBACK_SLOT and padding_end <= va + vm_size:
            if protection & 3 != 3:
                raise ValueError('Callback data segment must be readable and writable.')
            padding_found = True
            count = struct.unpack_from('<I', binary, offset + 64)[0]
            if 72 + 80 * count > size:
                raise ValueError('Invalid data section table.')
            for index in range(count):
                section = offset + 72 + 80 * index
                address, section_size = struct.unpack_from('<QQ', binary, section + 32)
                if address < padding_end and address + section_size > layout.CALLBACK_SLOT:
                    raise ValueError('Callback slot overlaps an existing data section.')
    if not padding_found:
        raise ValueError('Expected unused callback data padding is missing.')
    if binary[MARKER:MARKER + 8] == MAGIC:
        raise ValueError('Ginppai native callbacks are already installed.')
    # The caller can supply either the original or our exact country-patched form.
    fingerprint = hashlib.sha256(binary[start:start + length]).hexdigest()
    if fingerprint not in layout.VERIFIED_TEXT_HASHES:
        raise ValueError('Native callbacks support only verified KakaoTalk 26.7.3 and 26.8.0 executables.')
    if any(binary[CAVE:MARKER + len(MAGIC)]):
        raise ValueError('Native callback header space is not empty.')
    if struct.unpack_from('<I', binary, layout.SAVE_ENTRY - BASE)[0] != 0xAA1E03E0:
        raise ValueError('Unexpected ChatMessage.save entry instruction.')
    pc = BASE + CAVE
    page_delta = (layout.CALLBACK_SLOT >> 12) - ((pc + 4) >> 12)
    adrp = 0x90000010 | ((page_delta & 3) << 29) | (((page_delta >> 2) & 0x7FFFF) << 5)
    ldr = 0xF9400000 | (((layout.CALLBACK_SLOT & 0xFFF) // 8) << 10) | (16 << 5) | 16
    words = [
        0xA9BF7BF4,  # stp x20, x30, [sp, #-16]!
        adrp, ldr,
        0xB4000000 | (3 << 5) | 16,  # cbz x16, restore
        0xAA1403E0,  # mov x0, x20 (Swift self to C argument)
        0xD63F0200,  # blr x16
        0xA8C17BF4,  # ldp x20, x30, [sp], #16
        0xAA1E03E0,  # original instruction: mov x0, x30
        branch(pc + 32, layout.SAVE_ENTRY + 4),
    ]
    for index, word in enumerate(words):
        struct.pack_into('<I', binary, CAVE + 4 * index, word)
    struct.pack_into('<I', binary, layout.SAVE_ENTRY - BASE, branch(layout.SAVE_ENTRY, pc))
    binary[MARKER:MARKER + len(MAGIC)] = MAGIC
    if feature_patches: patch_features(binary, layout)
    return binary


VERIFIED_TEXT_HASHES = {
    ORIGINAL_TEXT_SHA256,
    '772de6aab4f629ad3b92a19cd566761a83993e6efc2aee4f2ca98a2db04bd736',
}
