"""Collect B provenance after success OR failure; no core mutation or execution."""
import hashlib,json,os,plistlib,subprocess
from pathlib import Path
out=Path('logs');out.mkdir(exist_ok=True)
root=Path('src/ManicEMU')
def git(path,*args):
    if not path.exists():return None
    p=subprocess.run(['git','-C',str(path),*args],capture_output=True,text=True)
    return p.stdout.strip() if p.returncode==0 else None
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
def manifest(path):
    return {p.relative_to(path).as_posix():sha(p) for p in sorted(path.rglob('*')) if p.is_file()}
original=root/'Cores/azahar.libretro.framework'
app=Path('app-out/ManicRoom.xcarchive/Products/Applications/ManicEmuSideload.app')
archived=app/'Frameworks/azahar.libretro.framework'
info=plistlib.loads((app/'Info.plist').read_bytes()) if (app/'Info.plist').is_file() else {}
evidence={'run_id':os.getenv('GITHUB_RUN_ID'),'harness_commit':os.getenv('GITHUB_SHA'),
 'runner_os':os.getenv('RUNNER_OS'),'runner_arch':os.getenv('RUNNER_ARCH'),
 'ImageOS':os.getenv('ImageOS'),'ImageVersion':os.getenv('ImageVersion'),
 'Manic_commit':git(root,'rev-parse','HEAD'),
 'Libretro_commit':git(root/'Dependencies/Libretro','rev-parse','HEAD'),
 'original_framework':manifest(original),'archived_framework':manifest(archived),
 'original_expected_sha256':'183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c',
 'archive_bundle_id':info.get('CFBundleIdentifier'),'archive_build_mode':info.get('ManicRoomBuildMode'),
 'unsigned_ipas':{p.name:sha(p) for p in Path('app-out').glob('*.ipa')},
 'steps':json.loads(os.environ.get('B_STEP_RESULTS','{}')),'C_executed':False}
(out/'B-execution-evidence.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2))
