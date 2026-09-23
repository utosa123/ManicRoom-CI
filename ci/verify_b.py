import json,os,re,sys,urllib.request
from pathlib import Path
ORIGINAL='183159290d777d42a68c17f5f4d90d8b88f7aa0281e4788bad4e5954a6df940c'
def validate_run(run,repo,sha,runid):
    assert str(run['id'])==str(runid),'wrong run'
    assert run['repository']['full_name']==repo,'wrong repository'
    assert run['head_sha']==sha,'B was built from another harness revision; rerun ui'
    assert run['status']=='completed' and run['conclusion']=='success','B workflow not successful'
    assert run['event']=='workflow_dispatch','not the manual B workflow'
    assert run['path']=='.github/workflows/room-experiment.yml','wrong workflow'
def validate_attestation(a,repo,sha,runid):
    assert a['schema']==1 and a['mode']=='ui' and a['archive_success'] is True
    assert a['repository']==repo and a['head_sha']==sha and str(a['run_id'])==str(runid)
    assert a['original_sha256']==ORIGINAL
    assert a['communication_verified'] is False
    assert a['bundle_id']=='local.manicroom.ui'
def main():
    runid=os.environ['B_RUN_ID'];assert re.fullmatch(r'[0-9]+',runid)
    repo=os.environ['GITHUB_REPOSITORY'];sha=os.environ['GITHUB_SHA']
    Path('logs').mkdir(exist_ok=True)
    if len(sys.argv)>1:
        a=json.loads(Path(sys.argv[1]).read_text());validate_attestation(a,repo,sha,runid)
        print('PASS B ui archive attestation; NOT Room success');return
    request=urllib.request.Request(f'https://api.github.com/repos/{repo}/actions/runs/{runid}',headers={
        'Accept':'application/vnd.github+json','Authorization':'Bearer '+os.environ['GH_TOKEN'],
        'X-GitHub-Api-Version':'2022-11-28'})
    with urllib.request.urlopen(request,timeout=30) as response:run=json.load(response)
    validate_run(run,repo,sha,runid)
    # No token, endpoint identity, or complete API response in logs.
    Path('logs/B-run-verification.json').write_text(json.dumps({k:run[k] for k in ('id','head_sha','status','conclusion','event','path')},indent=2))
    print('PASS completed B workflow on this exact harness revision')
if __name__=='__main__':main()
