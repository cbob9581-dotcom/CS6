import unreal,json
from pathlib import Path
paths=['/Game/MultiplayerFPS/Game/Weapons/Secondary/1911/BP_1911',
       '/Game/MultiplayerFPS/Game/Weapons/Primary/AK-74/BP_AK74M',
       '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent']
for path in paths:
    unreal.log('CS6_LOAD_PROBE '+path)
    if not unreal.EditorAssetLibrary.load_blueprint_class(path): raise RuntimeError('Blueprint class missing: '+path)
Path('D:/CS6/reports/migrated-blueprint-load-probe.json').write_text(json.dumps({'passed':True,'assets':paths},indent=2),encoding='utf-8')
unreal.log('CS6_MIGRATED_LOAD_PROBE_PASS')
