from pathlib import Path
import hashlib,shutil,json,subprocess
root=Path.cwd().resolve()
source=root/'src/ManicEMU'
stage=root/'stage-C/ManicEMU'
candidate=root/'core-out/azahar.libretro.framework'
original=source/'Cores/azahar.libretro.framework'
assert not stage.exists() and candidate.is_dir()
def manifest(p):
    return {str(x.relative_to(p)):hashlib.sha256(x.read_bytes()).hexdigest() for x in p.rglob('*') if x.is_file()}
before=manifest(original)
assert before['azahar.libretro']=='183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c'
# Verify every original LFS object before making the separate staging copy.
subprocess.run(['bash',str(source/'Scripts/VerifyLFS.sh')],check=True)
required=sum(x.stat().st_size for x in source.rglob('*') if x.is_file())+4*1024**3
assert shutil.disk_usage(root).free>required, 'Insufficient space for separate C staging copy'
# Exclude only the original framework during copy; never delete or move it.
def ignore(directory,names):
    return ['azahar.libretro.framework'] if Path(directory)==source/'Cores' else []
shutil.copytree(source,stage,ignore=ignore,symlinks=True)
shutil.copytree(candidate,stage/'Cores/azahar.libretro.framework')
# The unsigned candidate has no original signature. LFS also enumerates HEAD,
# so removing an index entry is insufficient. Apply a staging-only exception
# for this one obsolete signature, after validating all original LFS assets.
signature='Cores/azahar.libretro.framework/_CodeSignature/CodeResources'
assert not (stage/signature).exists()
staging_patch=root/'patches/Manic-C-unsigned-signature.patch'
subprocess.run(['git','-C',str(stage),'apply','--check',str(staging_patch)],check=True)
subprocess.run(['git','-C',str(stage),'apply',str(staging_patch)],check=True)
subprocess.run(['bash',str(stage/'Scripts/VerifyLFS.sh')],check=True)
assert manifest(original)==before
(root/'logs/core-replacement.json').write_text(json.dumps({'original_unchanged':before,'candidate':manifest(candidate),'staging':str(stage),
    'staging_only_lfs_exception':signature,'staging_patch_sha256':hashlib.sha256(staging_patch.read_bytes()).hexdigest()},indent=2))
