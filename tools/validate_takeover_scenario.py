import sys
from pathlib import Path
t=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
for marker in ['CS6 CONTROL spectating=1','CS6 CONTROL success=1 same_alive=1 health_kept=1 inventory_kept=1','CS6 CONTROL enemy_rejected=1','CS6 CONTROL controlled_wallet_award=1','CS6 CONTROL next_round_restored=1','CS6 CONTROL PASS']:
    assert marker in t,'Missing real runtime evidence: '+marker
if 'CS6 FP DIAG' in t:
    import re
    assert re.search(r'CS6 FP DIAG viewTarget=\S+ armsVisible=0 armsAnim=\d bodyVisible=0',t),'Observed first-person view must hide the body rig it cannot drive'
assert 'Accessed None' not in t,'Blueprint accessed missing object'
print('PASS: friendly spectator, bot takeover with preserved health/inventory/alive count and enemy rejection')
