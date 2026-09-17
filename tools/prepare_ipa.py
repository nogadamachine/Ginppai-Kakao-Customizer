#!/usr/bin/env python3
"""Inject public Ginppai dylibs into a user-supplied unencrypted IPA.

The output is unsigned; import it into SideStore to sign/install it.
This tool does not decrypt apps or obtain an IPA from the internet.
"""
import argparse
import hashlib
from pathlib import Path, PurePosixPath
import plistlib
import stat
import struct
import subprocess
import tempfile
import zipfile
from native_layouts import NATIVE_26_8, ORIGINAL_26_8_SHA256

def custom_branding(app, name):
    """Rename only app display labels, preserving executable and signing IDs."""
    name=name.strip()
    if not name or len(name)>80 or any(ord(c)<32 or ord(c)==127 for c in name):
        raise ValueError('App display name must contain 1 to 80 visible characters on one line.')
    targets=[app/'Info.plist', *sorted(app.glob('*.lproj/InfoPlist.strings'))]
    changes=[]
    for path in targets:
        raw=path.read_bytes()
        try:
            values=plistlib.loads(raw)
        except plistlib.InvalidFileException:
            # Apple localized strings also use the OpenStep format. Parse with
            # the system plist tool instead of replacing labels with a regex.
            converted=subprocess.run(['plutil','-convert','xml1','-o','-',str(path)],capture_output=True,check=True)
            values=plistlib.loads(converted.stdout)
        if not isinstance(values,dict): raise ValueError('Expected a property-list dictionary.')
        values['CFBundleDisplayName']=name
        values['CFBundleName']=name
        changes.append((path,plistlib.dumps(values,fmt=plistlib.FMT_BINARY)))
    # Validate every locale before applying the first change in the temp app.
    for path,data in changes: path.write_bytes(data)

def commands(data):
    if len(data)<32 or struct.unpack_from('<II',data)[0:2]!=(0xFEEDFACF,0x100000C):
        raise ValueError('Expected a thin arm64 Mach-O.')
    count, total = struct.unpack_from('<II',data,16)
    offset=32
    for _ in range(count):
        if offset+8>32+total: raise ValueError('Invalid Mach-O command table.')
        command,size=struct.unpack_from('<II',data,offset)
        if size<8 or offset+size>32+total: raise ValueError('Invalid load command size.')
        yield offset,command,size
        offset+=size
    if offset != 32+total: raise ValueError('Invalid load command total.')

def inject(data, name):
    entries=list(commands(data))
    header_limit=0x7000
    if struct.unpack_from('<I',data,12)[0]!=2:
        raise ValueError('Use an original IPA, not an app already modified by LiveContainer.')
    for off,cmd,size in entries:
        if cmd==0x19:
            sections=struct.unpack_from('<I',data,off+64)[0]
            if 72+sections*80>size: raise ValueError('Invalid segment section table.')
            for index in range(sections):
                section=off+72+80*index
                position=struct.unpack_from('<I',data,section+48)[0]
                if position: header_limit=min(header_limit,position)
        if cmd in [0x21,0x2c] and struct.unpack_from('<I',data,off+16)[0]:
            raise ValueError('Encrypted app: supply your own unencrypted IPA. This tool does not decrypt it.')
        if cmd in [0xc,0x80000018]:
            start=off+struct.unpack_from('<I',data,off+8)[0]
            if data[start:off+size].split(b'\0')[0].decode()==name:
                return data
    count,total=struct.unpack_from('<II',data,16)
    encoded=name.encode()+b'\0'
    size=(24+len(encoded)+7)&~7
    end=32+total
    # Do not overwrite executable contents or the optional country patch.
    if end+size>header_limit or any(data[end:end+size]):
        raise ValueError('Not enough empty Mach-O header space for injection.')
    result=bytearray(data)
    result[end:end+size]=struct.pack('<IIIIII',0xc,size,24,0,0,0)+encoded+b'\0'*(size-24-len(encoded))
    struct.pack_into('<II',result,16,count+1,total+size)
    return result

def country_patch(data):
    expected='c2195ceae06edac6086aeca06c1b2a65fcaabee26e20169f739b134d20b0a0eb'
    fingerprint=None
    for off,cmd,size in commands(data):
        if cmd!=0x19: continue
        for index in range(struct.unpack_from('<I',data,off+64)[0]):
            section=off+72+index*80
            if data[section:section+16].split(b'\0')[0]==b'__text':
                length,position=struct.unpack_from('<QI',data,section+40)
                fingerprint=hashlib.sha256(data[position:position+length]).hexdigest()
    current = fingerprint == ORIGINAL_26_8_SHA256
    if fingerprint not in {expected, ORIGINAL_26_8_SHA256} or any(data[0x7000:0x7400]):
        raise ValueError('Country patch supports only verified, unpatched KakaoTalk 26.7.3 and 26.8.0 arm64 executables.')
    b=bytearray(data);base=0x100000000;slot=NATIVE_26_8["COUNTRY_SLOT"] if current else 0x1083e7f00
    def put(address,word): struct.pack_into('<I',b,address-base,word)
    def branch(pc,target):
        delta=target-pc
        if delta%4 or not -(1<<27)<=delta<(1<<27): raise ValueError('Branch out of range.')
        put(pc,0x14000000|((delta//4)&0x3ffffff))
    def load(pc):
        delta=(slot>>12)-(pc>>12)
        put(pc,0x90000000|((delta&3)<<29)|(((delta>>2)&0x7ffff)<<5)|17)
        put(pc+4,0xb9400000|(((slot&0xfff)//4)<<10)|(17<<5)|16)
    def cbz(pc,target): put(pc,0x34000000|((((target-pc)//4)&0x7ffff)<<5)|16)
    pc=base+0x7000;load(pc);cbz(pc+8,pc+28)
    put(pc+12,0x2a1003e0);put(pc+16,0xd2fc4001);put(pc+20,0xd65f03c0);branch(pc+28,NATIVE_26_8["COUNTRY_FALLBACK"] if current else 0x104694c00)
    fallbacks=NATIVE_26_8['COUNTRY_BOOL_FALLBACKS'] if current else [0x104695010,0x104695014]
    for offset,code,fallback in [(0x7080,b'KR',fallbacks[0]),(0x70c0,b'JP',fallbacks[1])]:
        pc=base+offset;load(pc);cbz(pc+8,pc+32)
        put(pc+12,0x52800000|(int.from_bytes(code,'little')<<5)|17)
        put(pc+16,0x6b11021f);put(pc+20,0x1a9f17e0);put(pc+24,0xd65f03c0);branch(pc+32,fallback)
    entries=NATIVE_26_8["COUNTRY_BRANCHES"] if current else [0x104694bfc,0x1046957dc,0x1046957e0]
    for address,target in zip(entries,[0x7000,0x7080,0x70c0]):branch(address,base+target)
    b[0x73f0:0x73f8]=b'KCUICFG3'
    return b

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--dylib',type=Path,action='append',required=True)
    parser.add_argument('--country-ui',action='store_true',help='Apply a verified KakaoTalk UI-only country patch.')
    parser.add_argument('--features',action='store_true',help='Add verified native message-save callbacks for Ginppai features.')
    parser.add_argument('--display-name',help='Set the main app name in all bundled languages before signing.')
    args=parser.parse_args()
    if args.output.exists(): parser.error('Output already exists; choose a new filename.')
    if not all(p.is_file() and p.suffix=='.dylib' for p in args.dylib): parser.error('Each dylib must exist.')
    if len({p.name for p in args.dylib})!=len(args.dylib): parser.error('Duplicate dylib filenames.')
    with tempfile.TemporaryDirectory(prefix='ginppai-ipa-') as temp:
        root=Path(temp)
        with zipfile.ZipFile(args.input) as archive:
            for info in archive.infolist():
                path=PurePosixPath(info.filename)
                if path.is_absolute() or '..' in path.parts or '\\' in info.filename:
                    raise ValueError('Unsafe ZIP path.')
                if stat.S_ISLNK(info.external_attr>>16): raise ValueError('Symlink entries are not supported.')
            archive.extractall(root)
            for info in archive.infolist():
                mode=(info.external_attr>>16)&0o777
                if mode: (root/info.filename).chmod(mode)
        apps=list((root/'Payload').glob('*.app'))
        if len(apps)!=1: raise ValueError('Expected one main app.')
        app=apps[0];info=plistlib.loads((app/'Info.plist').read_bytes())
        if info.get('CFBundleIdentifier')!='com.iwilab.KakaoTalk': raise ValueError('Expected KakaoTalk IPA.')
        if any('Customizer' in p.name for p in args.dylib) and info.get('CFBundleShortVersionString') not in {'26.7.3','26.8.0'}:
            raise ValueError('Customizer supports verified KakaoTalk 26.7.3 and 26.8.0 builds.')
        if args.display_name is not None: custom_branding(app,args.display_name)
        executable=app/info['CFBundleExecutable'];binary=bytearray(executable.read_bytes())
        if args.country_ui:
            if info.get('CFBundleShortVersionString') not in {'26.7.3','26.8.0'}: raise ValueError('Country patch requires 26.7.3 or 26.8.0.')
            binary=country_patch(binary)
        if args.features:
            from native_patch import patch
            binary=patch(binary)
        frameworks=app/'Frameworks';frameworks.mkdir(exist_ok=True)
        for library in args.dylib:
            binary=inject(binary,f'@executable_path/Frameworks/{library.name}')
            (frameworks/library.name).write_bytes(library.read_bytes())
            (frameworks/library.name).chmod(0o755)
        executable.write_bytes(binary);executable.chmod(0o755)
        # SideStore will sign the prepared app with the user's own account.
        with zipfile.ZipFile(args.output,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
            for path in sorted(root.rglob('*')):
                if path.is_file(): archive.write(path,path.relative_to(root).as_posix())
    print(f'Prepared unsigned IPA: {args.output}. Import it into SideStore to sign/install.')

if __name__=='__main__': main()
