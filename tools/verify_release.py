#!/usr/bin/env python3
"""Verify final source identity, signatures and every distributed package."""
from pathlib import Path, PurePosixPath
import hashlib, json, plistlib, struct, subprocess, tempfile, zipfile
from jailbreak_link import link_provider
r=Path(__file__).resolve().parents[1]
c=json.loads((r/'package.json').read_text());d=r/'dist';v=c['version'];raw=(r/'build'/c['dylib']).read_bytes()
hash=lambda b:hashlib.sha256(b).hexdigest()
source_id=hash(b''.join(p.read_bytes() for p in sorted((r/'src').glob('*')) if p.suffix in ['.m','.inc','.swift']))[:16]
assert source_id.encode() in raw, 'Built dylib does not match current source'
assert c['version'].encode() in raw, 'Built dylib version does not match package metadata'
assert json.loads((r/'docs/validation.json').read_text())['version']==v, 'Validation metadata is stale'
assert (d/c['dylib']).read_bytes()==raw
subprocess.run(['codesign','--verify','--strict',str(d/c['dylib'])],check=True)
def signature_start(data):
 pos=32
 for _ in range(struct.unpack_from('<I',data,16)[0]):
  command,size=struct.unpack_from('<II',data,pos)
  if command==0x1d:
   offset,length=struct.unpack_from('<II',data,pos+8)
   assert offset+length==len(data), 'Unexpected code-signature layout'
   return offset
  pos+=size
 raise AssertionError('Missing code signature')

for scheme,arch,prefix in [('rootful','iphoneos-arm',''),('rootless','iphoneos-arm64','var/jb')]:
 deb=d/f"{c['id']}_{c['debVersion']}_{arch}.deb"
 version=subprocess.check_output(['dpkg-deb','-f',str(deb),'Version'],text=True).strip();assert version==c['debVersion']
 assert subprocess.check_output(['dpkg-deb','-f',str(deb),'Architecture'],text=True).strip()==arch
 with tempfile.TemporaryDirectory() as t:
  subprocess.run(['dpkg-deb','-x',str(deb),t],check=True)
  p=Path(t)/prefix/'Library/MobileSubstrate/DynamicLibraries'
  payload=p/c['dylib']; packed=payload.read_bytes(); expected=link_provider(raw,scheme)
  offset=signature_start(expected)
  assert signature_start(packed)==offset
  assert packed[:offset]==expected[:offset], 'Unexpected change outside code signature'
  subprocess.run(['codesign','--verify','--strict',str(payload)],check=True)
  assert payload.stat().st_mode & 0o777==0o755
  assert plistlib.loads((p/Path(c['dylib']).with_suffix('.plist')).read_bytes())=={'Filter':{'Bundles':['com.iwilab.KakaoTalk']}}
with zipfile.ZipFile(d/f"{c['name']}-{v}-NonJailbreak.zip") as z:
 for name in z.namelist():
  assert not PurePosixPath(name).is_absolute() and '..' not in PurePosixPath(name).parts
  assert not name.lower().endswith(('.ipa','.p12','.mobileprovision'))
 for line in z.read('SHA256SUMS').decode().splitlines():
  digest,name=line.split('  ',1);assert hash(z.read(name))==digest
 assert z.read(c['dylib'])==raw
sumfiles=[d/'SHA256SUMS']
if c['debVersion']!=v: sumfiles.append(d/f"Jailbreak-{c['debVersion']}-SHA256SUMS")
for sums in sumfiles:
 for line in sums.read_text().splitlines():
  digest,name=line.split('  ',1);assert hash((d/name).read_bytes())==digest
print('Verified',v,'/ DEB',c['debVersion'],': signatures, hook-provider links, unchanged code, bundle filters, ZIP and SHA256 digests')
