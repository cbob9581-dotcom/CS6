import re,sys
from pathlib import Path
text=Path(sys.argv[1]).read_text(encoding='utf-8',errors='replace')
partial=re.search(r'CS6 enrolled ([1-9]) combatants',text)
assert not partial, 'Partial roster was committed and existing pawns repeatedly repositioned'
checks=re.findall(r'CS6 SPAWN CHECK pawn=(\S+).*?gap=([\d.-]+).*?cameraHeight=([\d.-]+).*?penetrating=(\d+)',text)
assert len(checks)>=10, 'Missing ten actual capsule/ground/camera checks'
for pawn,gap,camera,penetrating in checks:
    assert -3<=float(gap)<=35, f'{pawn}: invalid ground clearance {gap}'
    assert penetrating=='0', f'{pawn}: blocked initial capsule'
    if pawn.startswith('BP_FPSCharacter_Parent'):
        assert 100<float(camera)<220, f'{pawn}: first-person camera not at standing height: {camera}'
print('PASS: atomic full roster and ground/camera/capsule placement checks')
