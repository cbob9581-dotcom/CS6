import sys
from pathlib import Path
t=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
for marker in ['CS6 SHOP armor_cash=150 armor=100','CS6 SHOP refund_cash=800 armor=0','CS6 SHOP rifle_equipped=1 cash=3100','CS6 SHOP fired_refund_rejected=1','CS6 SHOP zone_rejected=1','CS6 SHOP PASS']:
    assert marker in t, 'Missing real runtime evidence: '+marker
assert 'Accessed None' not in t, 'Blueprint accessed missing object'
print('PASS: real armor purchase/refund, rifle equip, spent ammunition refund rejection and buy zone')
