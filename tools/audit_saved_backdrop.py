import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
rows=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,unreal.StaticMeshActor) and actor.get_actor_label().startswith('SM_Mountain'):
        c=actor.static_mesh_component
        rows.append(dict(actor=actor.get_name(),label=actor.get_actor_label(),enabled=str(c.get_collision_enabled()),profile=str(c.get_collision_profile_name()),actor_enabled=actor.get_actor_enable_collision()))
Path('D:/CS6/reports/saved-backdrop-collision.json').write_text(json.dumps(rows,indent=2))
unreal.log('CS6_SAVED_BACKDROP_AUDITED')
