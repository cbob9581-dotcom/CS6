import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();rows=[]
for a in actors.get_all_level_actors():
 if isinstance(a,unreal.RecastNavMesh):
  rows.append(dict(name=a.get_name(),radius=a.get_editor_property('agent_radius'),height=a.get_editor_property('agent_height')))
p=unreal.EditorAssetLibrary.load_blueprint_class('/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent');sample=actors.spawn_actor_from_class(p,unreal.Vector(0,0,-20000));cap=sample.get_component_by_class(unreal.CapsuleComponent);dims=dict(radius=cap.get_unscaled_capsule_radius(),half=cap.get_unscaled_capsule_half_height());actors.destroy_actor(sample)
Path('D:/CS6/reports/nav-agent-fit.json').write_text(json.dumps(dict(nav=rows,capsule=dims),indent=2),encoding='utf-8')
