import unreal,json,shutil
from pathlib import Path
map_file=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/Content/CS6/Maps/M_MilitaryBase.umap')
backup=Path('D:/CS6/Backups/BeforeCS2Overhaul/M_MilitaryBase.umap')
if not backup.exists():shutil.copy2(map_file,backup)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
rows=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    label=a.get_actor_label()
    if label.startswith('CS6_Spawn_A_') or label.startswith('CS6_Spawn_B_'):
        before=a.get_actor_rotation();yaw=30 if label.startswith('CS6_Spawn_A_') else 210
        a.modify();a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False)
        r=a.get_actor_rotation()
        rows.append(dict(label=label,before=dict(pitch=before.pitch,yaw=before.yaw,roll=before.roll),after=dict(pitch=r.pitch,yaw=r.yaw,roll=r.roll)))
assert len(rows)==10 and levels.save_current_level()
Path('D:/CS6/reports/spawn-rotation-fix.json').write_text(json.dumps(dict(saved=True,actors=rows),indent=2))
