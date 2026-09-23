"""Run ONLY after the ui archive and IPA steps actually succeeded."""
import hashlib,json,os,plistlib
from pathlib import Path
out=Path('app-out'); app=out/'ManicRoom.xcarchive/Products/Applications/ManicEmuSideload.app'
info=plistlib.loads((app/'Info.plist').read_bytes())
assert info['ManicRoomBuildMode']=='ui'
assert (out/'build-mode.txt').read_text().strip()=='ui'
assert list(out.glob('*.ipa'))
sha=hashlib.sha256((app/'Frameworks/azahar.libretro.framework/azahar.libretro').read_bytes()).hexdigest()
assert sha=='183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c'
Path('logs/B-ui-success.json').write_text(json.dumps({
    'schema':1,'mode':'ui','archive_success':True,'communication_verified':False,
    'repository':os.environ['GITHUB_REPOSITORY'],'head_sha':os.environ['GITHUB_SHA'],
    'run_id':os.environ['GITHUB_RUN_ID'],'original_sha256':sha,
    'bundle_id':info['CFBundleIdentifier']},indent=2))
