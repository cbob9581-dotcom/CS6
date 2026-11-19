import unreal,json,shutil
from pathlib import Path
root=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BeforeCS2Overhaul/M_MilitaryBase-before-agent-resize.umap')
if not backup.exists():shutil.copy2(root/'Content/CS6/Maps/M_MilitaryBase.umap',backup)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rows=[]
for a in actors.get_all_level_actors():
 if isinstance(a,unreal.RecastNavMesh):
  a.modify();a.set_editor_property('agent_radius',42.0);a.set_editor_property('agent_height',192.0)
  rows.append(dict(name=a.get_name(),radius=a.get_editor_property('agent_radius'),height=a.get_editor_property('agent_height')))
assert unreal.CS6TemplateBridge.build_navigation(world),'Navigation rebuild incomplete'
assert levels.save_current_level()
Path('D:/CS6/reports/nav-agent-resize.json').write_text(json.dumps(dict(saved=True,built=True,agents=rows),indent=2),encoding='utf-8')
unreal.log('CS6_NAV_AGENT_RESIZED')
