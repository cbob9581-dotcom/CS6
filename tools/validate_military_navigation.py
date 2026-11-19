import json
from pathlib import Path
import unreal

def main():
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CS6/Maps/M_MilitaryBase'):
        raise RuntimeError('Saved military map did not load')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    setup=json.loads(Path('D:/CS6/reports/military-gameplay-setup.json').read_text(encoding='utf-8'))
    a=unreal.Vector(*setup['spawns']['A'][0])+unreal.Vector(0,0,90)
    b=unreal.Vector(*setup['spawns']['B'][0])+unreal.Vector(0,0,90)
    path=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,a,b)
    result={'valid':bool(path and path.is_valid()),'partial':bool(path and path.is_partial()),
            'points':[[p.x,p.y,p.z] for p in path.path_points] if path else []}
    Path('D:/CS6/reports/military-navigation-validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    if not result['valid'] or result['partial'] or len(result['points'])<2:
        raise RuntimeError('Teams do not have a complete navigation route')
main()
unreal.SystemLibrary.quit_editor()
