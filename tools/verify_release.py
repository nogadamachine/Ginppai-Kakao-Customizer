#!/usr/bin/env python3
"""Verify final source identity, signatures and every distributed package."""
from pathlib import Path, PurePosixPath
import hashlib, json, plistlib, subprocess, tempfile, zipfile
r=Path(__file__).resolve().parents[1]
c=json.loads((r/'package.json').read_text());d=r/'dist';v=c['version'];raw=(r/'build'/c['dylib']).read_bytes()
hash=lambda b:hashlib.sha256(b).hexdigest()
source_id=hash(b''.join(p.read_bytes() for p in sorted((r/'src').glob('*')) if p.suffix in ['.m','.inc','.swift']))[:16]
assert source_id.encode() in raw, 'Built dylib does not match current source'
assert c['version'].encode() in raw, 'Built dylib version does not match package metadata'
assert json.loads((r/'docs/validation.json').read_text())['version']==v, 'Validation metadata is stale'
assert (d/c['dylib']).read_bytes()==raw
subprocess.run(['codesign','--verify','--strict',str(d/c['dylib'])],check=True)
for arch,prefix in [('iphoneos-arm',''),('iphoneos-arm64','var/jb')]:
 deb=d/f"{c['id']}_{v}_{arch}.deb"
 version=subprocess.check_output(['dpkg-deb','-f',str(deb),'Version'],text=True).strip();assert version==c['debVersion']
 with tempfile.TemporaryDirectory() as t:
  subprocess.run(['dpkg-deb','-x',str(deb),t],check=True)
  p=Path(t)/prefix/'Library/MobileSubstrate/DynamicLibraries'
  assert (p/c['dylib']).read_bytes()==raw
  assert plistlib.loads((p/Path(c['dylib']).with_suffix('.plist')).read_bytes())=={'Filter':{'Bundles':['com.iwilab.KakaoTalk']}}
with zipfile.ZipFile(d/f"{c['name']}-{v}-NonJailbreak.zip") as z:
 for name in z.namelist():
  assert not PurePosixPath(name).is_absolute() and '..' not in PurePosixPath(name).parts
  assert not name.lower().endswith(('.ipa','.p12','.mobileprovision'))
 for line in z.read('SHA256SUMS').decode().splitlines():
  digest,name=line.split('  ',1);assert hash(z.read(name))==digest
 assert z.read(c['dylib'])==raw
for line in (d/'SHA256SUMS').read_text().splitlines():
 digest,name=line.split('  ',1);assert hash((d/name).read_bytes())==digest
print('Verified',v,': strict code signature, both DEBs, bundle filters, ZIP content and all SHA256 digests')
