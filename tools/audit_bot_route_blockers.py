import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();rows=[]
for pos in [(6672,6898,6231),(6606,6834,6238),(6492,6921,6225)]:
 for c in unreal.CS6TemplateBridge.get_pawn_blockers(world,unreal.Vector(*pos)):
  props=dict(unreal.CS6TemplateBridge.read_template_properties(c))
  rows.append(dict(position=pos,actor=c.get_owner().get_name(),component=c.get_name(),props={k:v for k,v in props.items() if any(x in k.lower() for x in ['staticmesh','nav','collision','relativelocation','relativescale','objecttype']) and not k.startswith('Function:')}))
Path('D:/CS6/reports/bot-route-blockers.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
