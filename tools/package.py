#!/usr/bin/env python3
"""Wrap the same app-only dylib in rootful/rootless packages."""
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
config = json.loads((ROOT/'package.json').read_text())
dylib = ROOT/'build'/config['dylib']
if not dylib.is_file():
    raise SystemExit('Run python3 build.py first.')
if not shutil.which('dpkg-deb'):
    raise SystemExit('Install dpkg (macOS: brew install dpkg).')
source_id = hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').glob('*')) if p.suffix in ['.m', '.inc', '.swift'])).hexdigest()[:16]
if source_id.encode() not in dylib.read_bytes() or config['version'].encode() not in dylib.read_bytes():
    raise SystemExit('Source or version changed. Rebuild before packaging.')
dist = ROOT/'dist'
dist.mkdir(exist_ok=True)
shutil.copy2(dylib, dist/config['dylib'])
artifacts = [dist/config['dylib']]
for scheme, architecture, prefix in [('rootful', 'iphoneos-arm', ''), ('rootless', 'iphoneos-arm64', 'var/jb')]:
    with tempfile.TemporaryDirectory(prefix='ginppai-package-') as temp:
        stage = Path(temp)
        control = stage/'DEBIAN'
        control.mkdir()
        lib = stage/prefix/'Library/MobileSubstrate/DynamicLibraries'
        lib.mkdir(parents=True)
        shutil.copy2(dylib, lib/config['dylib'])
        (lib/Path(config['dylib']).with_suffix('.plist')).write_bytes(plistlib.dumps({
            'Filter': {'Bundles': ['com.iwilab.KakaoTalk']}
        }))
        license_dir = stage/prefix/'usr/share/doc'/config['id']
        license_dir.mkdir(parents=True)
        for name in ['LICENSE', 'COPYING.MIT', 'NOTICE.md']:
            shutil.copy2(ROOT/name, license_dir/name)
        minimum = config['minOS']
        if scheme == 'rootless' and tuple(map(int, minimum.split('.'))) < (15, 0):
            minimum = '15.0'
        fields = {
            'Package': config['id'], 'Name': config['name'], 'Version': config.get('debVersion', config['version']),
            'Architecture': architecture, 'Section': 'Tweaks', 'Priority': 'optional',
            'Maintainer': 'nogadamachine <78150070+nogadamachine@users.noreply.github.com>',
            'Author': 'nogadamachine',
            'Depends': f'firmware (>= {minimum}), mobilesubstrate | ellekit',
            'Description': config['description'], 'Homepage': config['homepage'],
            'Depiction': config['depiction'],
            'Installed-Size': str(sum(p.stat().st_size for p in lib.iterdir())//1024+1),
        }
        (control/'control').write_text(''.join(f'{key}: {value}\n' for key,value in fields.items()))
        for path in stage.rglob('*'):
            path.chmod(0o755 if path.is_dir() or path.suffix == '.dylib' else 0o644)
        output = dist/f"{config['id']}_{config['version']}_{architecture}.deb"
        subprocess.run(['dpkg-deb', '--root-owner-group', '-Zgzip', '-b', str(stage), str(output)], check=True)
        artifacts.append(output)
bundle=dist/f"{config['name']}-{config['version']}-NonJailbreak.zip"
with zipfile.ZipFile(bundle,'w',compression=zipfile.ZIP_DEFLATED) as archive:
    entries={config['dylib']:dylib, 'README.md':ROOT/'README.md', 'LICENSE':ROOT/'LICENSE',
             'COPYING.MIT':ROOT/'COPYING.MIT', 'NOTICE.md':ROOT/'NOTICE.md',
             'tools/prepare_ipa.py':ROOT/'tools/prepare_ipa.py',
             'tools/native_patch.py':ROOT/'tools/native_patch.py',
             'tools/native_layouts.py':ROOT/'tools/native_layouts.py'}
    for path in (ROOT/'docs').glob('*'):
        if path.suffix in ['.md','.json']: entries['docs/'+path.name]=path
    for name,path in sorted(entries.items()): archive.writestr(name,path.read_bytes())
    archive.writestr('SHA256SUMS',''.join(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n' for name,path in sorted(entries.items())))
artifacts.append(bundle)
(dist/'SHA256SUMS').write_text(''.join(
    f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n'
    for p in sorted(artifacts)))
print(dist)
