"""Approve only the already pinned R.swift resource generator on a disposable CI Mac.
Keep Xcode fingerprint validation enabled; do not change Package.resolved.
"""
import json,os,subprocess,sys
from pathlib import Path
assert sys.platform=='darwin' and os.environ.get('GITHUB_ACTIONS')=='true'
resolved,packages,log=map(Path,sys.argv[1:])
revision='a9abc6b0afe0fc4a5a71e1d7d2872143dff2d2f1'
pin=next(p for p in json.loads(resolved.read_text())['pins'] if p['identity']=='r.swift')
assert pin['location'].rstrip('/')=='https://github.com/mac-cain13/R.swift'
assert pin['state']['revision']==revision and pin['state']['version']=='7.8.0'
checkout=next(p for p in (packages/'checkouts').iterdir() if p.name.lower()=='r.swift')
assert subprocess.check_output(['git','-C',str(checkout),'rev-parse','HEAD'],text=True).strip()==revision
subprocess.run(['git','-C',str(checkout),'diff','--exit-code','HEAD'],check=True)
entry={'fingerprint':revision,'packageIdentity':'r.swift','targetName':'RswiftGenerateInternalResources'}
written=[]
# Xcode and SwiftPM may use the compatibility path or the Library path.
# resolve() coalesces them when the runner already provides a symlink.
paths={p.resolve() for p in (Path.home()/'.swiftpm/security/plugins.json',Path.home()/'Library/org.swift.swiftpm/security/plugins.json')}
for path in sorted(paths):
    old=json.loads(path.read_text()) if path.exists() else []
    assert isinstance(old,list)
    matching=[x for x in old if x.get('packageIdentity')=='r.swift' and x.get('targetName')==entry['targetName']]
    assert not matching or matching==[entry], 'Different existing R.swift trust record; refusing overwrite'
    if not matching:
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps(old+[entry],indent=2)+'\n')
    written.append(str(path))
log.write_text(json.dumps({'entry':entry,'trust_stores':written,'global_validation_disabled':False},indent=2))
print('Approved pinned R.swift 7.8.0 resource plugin only; fingerprint validation remains enabled')
