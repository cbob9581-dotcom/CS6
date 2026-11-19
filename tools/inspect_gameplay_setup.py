"""Read authored template setup without switching away from the working map."""
import json
from pathlib import Path
import unreal

world_name='template inspection'
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')
source=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors=unreal.GameplayStatics.get_all_actors_of_class(source,unreal.Actor)
report=[]
for actor in actors:
    cls=actor.get_class().get_name()
    if any(x in cls for x in ['Customization','PlayerStart','EndGameCamera','SpawnPoint','NavMeshBounds','RecastNavMesh']):
        loc=actor.get_actor_location()
        origin,extent=actor.get_actor_bounds(False)
        report.append({'path':actor.get_path_name(),'class':cls,'label':actor.get_actor_label(),
                       'location':[loc.x,loc.y,loc.z],'extent':[extent.x,extent.y,extent.z]})
settings={}
gi=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(
    '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/Instances/BP_GameInstance'))
for name in ['TeamABotCount','Team ABot Count','TeamBBotCount','Team BBot Count','HardcoreBots?','Hardcore Bots?']:
    try:
        settings[name]=str(gi.get_editor_property(name))
    except Exception:
        pass
Path('D:/CS6/reports/gameplay-setup.json').write_text(json.dumps(
    {'current_map':world_name,'actors':report,'game_instance_settings':settings},indent=2),encoding='utf-8')
unreal.log('CS6_GAMEPLAY_SETUP_INSPECTED')


