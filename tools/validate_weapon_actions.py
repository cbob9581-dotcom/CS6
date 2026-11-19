import re,sys
from pathlib import Path
log=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
shot=re.search(r'CS6 WEAPON fired before=(\d+) after=(\d+)',log)
assert shot and 0<=int(shot[2])<int(shot[1]), 'No verified ammunition consumption from player fire input'
reload=re.search(r'CS6 WEAPON reloaded ammo=(\d+) capacity=(\d+)',log)
assert reload and int(reload[1])==int(reload[2])>int(shot[2]), 'Reload did not restore magazine'
assert 'CS6 WEAPON switched pistol=1' in log, 'Secondary weapon selection failed'
assert 'CS6 WEAPON switched rifle=1' in log, 'Primary weapon selection failed'
print('PASS: player fire, reload, pistol and rifle selection via actual input')
