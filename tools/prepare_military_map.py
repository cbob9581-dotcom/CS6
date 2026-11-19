"""Run inside the live UE editor, after stopping PIE."""
import json
from pathlib import Path
import unreal

out = Path('D:/CS6/reports')
source = '/Game/FC_MilitaryCamp/Maps/Map_MilitaryCamp'
target = '/Game/CS6/Maps/M_MilitaryBase'
unreal.EditorAssetLibrary.make_directory('/Game/CS6/Maps')
subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(target):
    if not subsystem.load_level(target):
        raise RuntimeError('Could not load saved military working map')
else:
    # World assets require the dedicated map-template path. Generic asset
    # duplication leaves a standalone World that trips LoadMap's leak check.
    if not subsystem.new_level_from_template(target, source):
        raise RuntimeError('Could not create military working map from template')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
summary=[]
for actor in actors:
    loc=actor.get_actor_location()
    origin,extent=actor.get_actor_bounds(False)
    summary.append({'name':actor.get_actor_label(),'class':actor.get_class().get_name(),
                    'location':[loc.x,loc.y,loc.z], 'origin':[origin.x,origin.y,origin.z],
                    'extent':[extent.x,extent.y,extent.z]})
with (out/'military-scene.json').open('w',encoding='utf-8') as f:
    json.dump({'map':target,'game_mode':'pending gameplay setup','actors':summary},f,ensure_ascii=False,indent=2)
subsystem.save_current_level()
unreal.log('CS6_MILITARY_MAP_PREPARED ' + target)
