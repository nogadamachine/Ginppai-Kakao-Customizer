"""Link the installed jailbreak hook provider in DEB payloads only.

The standalone LiveContainer dylib never gains a jailbreak dependency.
"""
import struct

LIBRARY = '@rpath/CydiaSubstrate.framework/CydiaSubstrate'
RPATHS = {
    'rootful': ['/Library/Frameworks'],
    'rootless': ['/var/jb/Library/Frameworks', '@loader_path/.jbroot/Library/Frameworks'],
}


def link_provider(data, scheme):
    if scheme not in RPATHS:
        raise ValueError('Unsupported jailbreak scheme.')
    if len(data) < 32 or struct.unpack_from('<I', data)[0] != 0xfeedfacf:
        raise ValueError('Expected a thin 64-bit Mach-O.')
    if struct.unpack_from('<I', data, 12)[0] != 6:
        raise ValueError('Expected a dylib.')
    count, size = struct.unpack_from('<II', data, 16)
    end = 32 + size
    if end > len(data):
        raise ValueError('Truncated load commands.')
    pos = 32
    first_content = len(data)
    strings = set()
    for _ in range(count):
        if pos + 8 > end:
            raise ValueError('Truncated command.')
        cmd, length = struct.unpack_from('<II', data, pos)
        if length < 8 or pos + length > end:
            raise ValueError('Invalid command size.')
        if cmd == 0x19:
            if length < 72:
                raise ValueError('Truncated segment.')
            sections = struct.unpack_from('<I', data, pos + 64)[0]
            if 72 + 80 * sections > length:
                raise ValueError('Truncated sections.')
            for n in range(sections):
                sec = pos + 72 + 80 * n
                offset = struct.unpack_from('<I', data, sec + 48)[0]
                if offset:
                    first_content = min(first_content, offset)
        if cmd == 0x2c:
            if length < 24 or struct.unpack_from('<I', data, pos + 16)[0]:
                raise ValueError('Encrypted dylib is unsupported.')
        if cmd in (0xc, 0x80000018, 0x8000001c):
            if length < 12:
                raise ValueError('Truncated string command.')
            offset = struct.unpack_from('<I', data, pos + 8)[0]
            if offset < 12 or offset >= length:
                raise ValueError('Invalid string offset.')
            strings.add((cmd, data[pos+offset:pos+length].split(b'\0')[0].decode()))
        pos += length
    if pos != end or first_content == len(data) or first_content < end:
        raise ValueError('Invalid header padding boundary.')
    additions = []
    for cmd, name in [(0xc, LIBRARY)] + [(0x8000001c, p) for p in RPATHS[scheme]]:
        if (cmd, name) in strings:
            continue
        header = 24 if cmd == 0xc else 12
        value = name.encode() + b'\0'
        length = (header + len(value) + 7) & ~7
        blob = bytearray(length)
        struct.pack_into('<III', blob, 0, cmd, length, header)
        blob[header:header+len(value)] = value
        additions.append(blob)
    payload = b''.join(additions)
    if end + len(payload) > first_content or any(data[end:end+len(payload)]):
        raise ValueError('Insufficient empty load-command padding.')
    result = bytearray(data)
    result[end:end+len(payload)] = payload
    struct.pack_into('<II', result, 16, count + len(additions), size + len(payload))
    return bytes(result)
