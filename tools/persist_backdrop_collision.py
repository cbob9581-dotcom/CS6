import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
changed=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,unreal.StaticMeshActor) and actor.get_actor_label().startswith('SM_Mountain'):
        actor.modify()
        c=actor.static_mesh_component
        c.modify()
        c.set_collision_profile_name('NoCollision')
        actor.set_actor_enable_collision(False)
        changed.append(actor.get_name())
assert len(changed)==10
assert levels.save_current_level()
Path('D:/CS6/reports/backdrop-persist-save.json').write_text(json.dumps({'saved':True,'actors':changed},indent=2))
unreal.log('CS6_BACKDROP_PROFILE_SAVED')
