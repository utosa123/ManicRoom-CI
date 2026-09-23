import json,os
from pathlib import Path
steps=json.loads(os.environ['STEP_RESULTS'])
order=[('compile','1 iOS arm64 core build'),('framework','2 framework'),('binary','3 Mach-O/dependencies'),('custom','4 custom adapters'),('room','5 Room exports'),('app','6 C Manic build'),('ipa','7 unsigned IPA')]
rows=[]
for key,label in order:
    outcome=steps.get(key,{}).get('outcome','skipped')
    rows.append({'stage':label,'実行済み':outcome in ('success','failure'),'結果':{'success':'成功','failure':'失敗'}.get(outcome,'未実行'),'actions_outcome':outcome})
Path('logs').mkdir(exist_ok=True)
Path('logs/C-stage-results.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2))
summary='\n|工程|結果|\n|---|---|\n'+''.join('|'+r['stage']+'|'+r['結果']+'|\n' for r in rows)
with open(os.environ['GITHUB_STEP_SUMMARY'],'a',encoding='utf8') as f:f.write(summary)
