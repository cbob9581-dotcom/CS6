import re,sys,json,math
from pathlib import Path
text=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
samples={};age=0.;phase=0;current=None
for line in text.splitlines():
 m=re.search(r'CS6 AUDIT age=([\d.]+) phase=(\d+)',line)
 if m:age=float(m[1]);phase=int(m[2]);current=None
 m=re.search(r'CS6 MEMBER pawn=(\S+) player=0 alive=(\d).*?pos=V\(X=([\d.-]+), Y=([\d.-]+), Z=([\d.-]+)\)',line)
 if m and m[1].startswith('BP_AISpawnable'):
  current=dict(age=age,phase=phase,alive=bool(int(m[2])),position=list(map(float,m.group(3,4,5))),mode=None)
  samples.setdefault(m[1],[]).append(current)
 m=re.search(r'CS6 BOT STATUS pawn=(\S+) mode=(\S+).*?recoveries=(\d+)',line)
 if m and current and m[1] in samples:current['mode']=m[2];current['recoveries']=int(m[3])
assert len(samples)>=9,'Missing individual audit of nine bots'
failures=[];result={}
for name,rows in samples.items():
 active=[r for r in rows if r['alive'] and r['phase'] in (1,2)]
 stationary=[]
 for r in active:
  if r['mode'] in ('moving','recovering','path-failed'):
   stationary.append(r)
   while stationary and r['age']-stationary[0]['age']>25:stationary.pop(0)
   if len(stationary)>=3 and r['age']-stationary[0]['age']>=18:
    origin=stationary[0]['position'];travel=max(math.dist(q['position'][:2],origin[:2]) for q in stationary)
    if travel<75:failures.append(name+': moving objective but stationary for >=18s')
  else:stationary=[]
 if active and any(r['mode'] is None for r in active):failures.append(name+': missing active tactical state')
 travel=max((math.dist(r['position'][:2],rows[0]['position'][:2]) for r in rows),default=0)
 result[name]=dict(observations=len(rows),distance_cm=round(travel),states=sorted({r['mode'] for r in active if r['mode']}))
assert any(r['states'] for r in result.values()),'No autonomous tactical updates observed'
Path('D:/CS6/reports/bot-individual-validation.json').write_text(json.dumps(dict(bots=result,failures=failures),indent=2),encoding='utf-8')
assert not failures,'; '.join(failures[:10])
print('PASS: individual audits of',len(samples),'bot lives; no unclassified active bot or sustained moving-objective stall')
