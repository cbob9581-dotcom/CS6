import unreal,json
from pathlib import Path
root='/Game/MultiplayerFPS/Game/Blueprints/'
paths=[root+'Gamemodes/BP_Player_State',root+'Core/BP_FPSCharacter_Parent',root+'Core/BP_BasePlayerController']
report={}
for path in paths:
    bp=unreal.load_asset(path)
    cls=unreal.EditorAssetLibrary.load_blueprint_class(path)
    report[path]={'properties':dict(unreal.CS6TemplateBridge.read_template_properties(unreal.get_default_object(cls))),
                  'graphs':list(unreal.CS6TemplateBridge.inspect_blueprint_graph(bp,''))}
Path('D:/CS6/reports/team-equipment-graphs.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CS6_TEAM_EQUIPMENT_AUDIT_COMPLETE')
