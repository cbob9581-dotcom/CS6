import re,sys
from pathlib import Path
log=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
assert 'Accessed None' not in log, 'Blueprint accessed a missing runtime object'
assert 'CS6 FULL clock fixedHz=60 audio=1' in log, 'Full match did not preserve audio initialization'
rounds=re.findall(r'CS6 round ended: reason=(\d+) score=(\d+):(\d+)',log)
assert len(rounds)>=13, 'Fewer than thirteen real rounds ended'
assert 'CS6 enrolled 10 combatants; swapped=1' in log, 'No actual side-swap roster rebuild'
scores=list(map(int,rounds[-1][1:]));assert max(scores)==13 or scores==[12,12], 'Match did not end at thirteen wins or a regulation draw'
assert 'CS6 FULL MATCH PASS' in log, 'Runtime full-match checks did not pass'
counts=[int(n) for n in re.findall(r'CS6 INVENTORY weapons=(\d+)',log)]
assert counts and max(counts)<=20, 'Firearms accumulated across rounds'
print('PASS: '+str(len(rounds))+' rounds, MR12 side swap, regulation finish, bounded firearm count')
