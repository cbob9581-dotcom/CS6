import unreal,json
from pathlib import Path
table=unreal.load_asset('/Game/MultiplayerFPS/Game/Blueprints/Customization/Data/DT_WeaponSelection')
Path('D:/CS6/reports/weapon-selection.json').write_text(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')
paths=['/Game/MultiplayerFPS/Game/HUD/Loadout/WB_WeaponComponnet','/Game/MultiplayerFPS/Game/HUD/Player/WB_PlayerHud']
for path in paths:
    bp=unreal.load_asset(path)
    Path('D:/CS6/reports/'+path.rsplit('/',1)[-1]+'-graphs.txt').write_text('\n\n'.join(unreal.CS6TemplateBridge.inspect_blueprint_graph(bp,'')),encoding='utf-8')
unreal.log('CS6_DEFAULT_LOADOUT_AUDIT_COMPLETE')
