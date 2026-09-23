from pathlib import Path
import hashlib,shutil,json
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
required=sum(x.stat().st_size for x in source.rglob('*') if x.is_file())+4*1024**3
assert shutil.disk_usage(root).free>required, 'Insufficient space for separate C staging copy'
# Exclude only the original framework during copy; never delete or move it.
def ignore(directory,names):
    return ['azahar.libretro.framework'] if Path(directory)==source/'Cores' else []
shutil.copytree(source,stage,ignore=ignore,symlinks=True)
shutil.copytree(candidate,stage/'Cores/azahar.libretro.framework')
assert manifest(original)==before
(root/'logs/core-replacement.json').write_text(json.dumps({'original_unchanged':before,'candidate':manifest(candidate),'staging':str(stage)},indent=2))
