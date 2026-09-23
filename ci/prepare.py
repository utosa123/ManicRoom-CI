"""Fresh fixed-source checkouts for the manual CI harness (no pushes).
Run from the harness root: python ci/prepare.py windows|core|baseline|ui|experimental
"""
import hashlib, json, os, pathlib, shutil, subprocess, sys
ROOT = pathlib.Path.cwd()
LOG = ROOT / 'logs'
LOG.mkdir(exist_ok=True)
MODE = sys.argv[1]
assert MODE in ('windows', 'core', 'baseline', 'ui', 'experimental')
env = os.environ.copy()
env['GIT_LFS_SKIP_SMUDGE'] = '1'
def run(args, cwd=ROOT):
    with (LOG/'prepare.log').open('a', encoding='utf8') as f:
        f.write(json.dumps([str(a) for a in args])+'\n'); f.flush()
        subprocess.run(args, cwd=cwd, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)
def clone(url, sha, name):
    dst=ROOT/'src'/name
    if dst.exists(): raise RuntimeError('Use a fresh checkout: '+str(dst))
    dst.mkdir(parents=True)
    run(['git','init',str(dst)])
    run(['git','remote','add','origin',url],dst)
    run(['git','fetch','--depth=1','origin',sha],dst)
    run(['git','checkout','--detach','FETCH_HEAD'],dst)
    actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=dst,text=True).strip()
    assert actual==sha
    return dst
def patch(dst, name):
    path=ROOT/'patches'/(name+'.patch')
    (LOG/(name+'-patch-sha256.txt')).write_text(hashlib.sha256(path.read_bytes()).hexdigest()+'\n')
    run(['git','apply','--check',str(path)],dst)
    run(['git','apply',str(path)],dst)
def inventory(dst,name):
    with (LOG/(name+'-submodules.txt')).open('w') as f:
        subprocess.run(['git','submodule','status','--recursive'],cwd=dst,stdout=f,check=True)
    run(['git','diff','--check'],dst)
(LOG/'python-version.txt').write_text(sys.version+'\n')
run(['git','--version'])
if MODE in ('windows','core'):
    dst=clone('https://github.com/Daiuno/azahar.git','03da4a354cc10012418c28a935dd0c627e37a963','azahar')
    deps=['externals/enet','externals/fmt','externals/boost','externals/libretro-common/libretro-common']
    run(['git','submodule','update','--init','--recursive',*(deps if MODE=='windows' else [])],dst)
    patch(dst,'azahar-experimental'); inventory(dst,'azahar')
else:
    dst=clone('https://github.com/Manic-EMU/ManicEMU.git','fbaeab79c214d5920bb51afa6f2d786fb2b12a58','ManicEMU')
    run(['git','submodule','update','--init','--recursive'],dst)
    # LFS data is stored both in .git/lfs and in the checkout. Build/archive/SPM
    # also need space. This is a conservative preflight, not a capacity guarantee.
    pointers=[]
    for p in dst.rglob('*'):
        if p.is_file() and '.git' not in p.parts and p.stat().st_size<1024:
            data=p.read_bytes()
            if data.startswith(b'version https://git-lfs.github.com/spec/v1'):
                size=int(next(x[5:] for x in data.splitlines() if x.startswith(b'size ')))
                pointers.append((str(p.relative_to(dst)),size))
    required=2*sum(x[1] for x in pointers)+8*1024**3
    free=shutil.disk_usage(dst).free
    (LOG/'capacity.json').write_text(json.dumps({'lfs_files':pointers,'estimated_required_bytes':required,'free_bytes':free},indent=2))
    if free<required: raise RuntimeError('Insufficient estimated disk capacity; see logs/capacity.json. No paid runner is selected automatically.')
    run(['git','lfs','install','--local','--skip-smudge'],dst)
    run(['git','lfs','pull'],dst)
    if MODE!='baseline':
        patch(dst,'ManicEMU'); patch(dst/'Dependencies/Libretro','Libretro')
        patch(dst,'B-ui-message-only')
        patch(dst,'ManicUI-swift-types')
        patch(dst/'Dependencies/Libretro','Libretro-header-scope')
    inventory(dst,'ManicEMU'); inventory(dst/'Dependencies/Libretro','Libretro')
print('Prepared fixed sources:', MODE, 'See logs/prepare.log')
