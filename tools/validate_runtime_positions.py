import re,sys
from pathlib import Path
log=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
match=re.search(r'CS6 AUDIT age=10\.\d[^\n]*\n(.*?)(?=CS6 AUDIT|\Z)',log,re.S)
if not match: raise SystemExit('FAIL: first-round ten-member audit missing')
positions=re.findall(r'CS6 MEMBER[^\n]*?pos=V\(X=([\d.-]+), Y=([\d.-]+), Z=([\d.-]+)\)',match.group(1))[:10]
if len(positions)!=10: raise SystemExit('FAIL: incomplete position audit')
bad=[(float(x),float(y),float(z)) for x,y,z in positions if not 5500<float(z)<6600]
if bad: raise SystemExit('FAIL: combatants outside playable height: '+str(bad))
first_capsules=re.findall(r'^.*CS6 CAPSULE.*$',log,re.M)[:10]
if first_capsules and any('penetrating=1' in line for line in first_capsules): raise SystemExit('FAIL: initial character capsules penetrate world collision')
print('PASS: all ten combatants start at military-base ground height')
