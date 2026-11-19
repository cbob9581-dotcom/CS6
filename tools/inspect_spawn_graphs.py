import unreal,json
from pathlib import Path
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')
root='/Game/MultiplayerFPS/Game/Blueprints/'
paths=[root+'Gamemodes/BP_BaseGS',root+'Gamemodes/TeamDeathmatch/BP_TDM_GM']
report={}
for path in paths:
    report[path]=list(unreal.CS6TemplateBridge.inspect_blueprint_graph(unreal.load_asset(path),''))
Path('D:/CS6/reports/spawn-blueprint-graphs.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CS6_SPAWN_GRAPH_AUDIT_COMPLETE')
unreal.SystemLibrary.quit_editor()
