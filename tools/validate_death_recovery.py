import sys
from pathlib import Path
log=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
for marker in ['CS6 DEATH original_dead=1 dropped=1 spectator=1',
               'CS6 DEATH recovered=1 no_early_respawn=1',
               'CS6 DEATH reset_alive=10 player_replaced=1 score=1:0']:
    assert marker in log, 'Missing actual runtime evidence: '+marker
print('PASS: real player death, spectator, dropped device recovery and round revival')
