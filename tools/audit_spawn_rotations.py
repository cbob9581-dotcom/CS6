import unreal,json
from pathlib import Path
r=unreal.Rotator(0,180,0)
rows={'positional_rotator':dict(pitch=r.pitch,yaw=r.yaw,roll=r.roll),'spawns':[]}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label().startswith('CS6_Spawn_'):
        r=a.get_actor_rotation()
        rows['spawns'].append(dict(label=a.get_actor_label(),pitch=r.pitch,yaw=r.yaw,roll=r.roll))
Path('D:/CS6/reports/spawn-rotation-audit.json').write_text(json.dumps(rows,indent=2))
