"""Run via -ExecutePythonScript in a full editor; preserve source maps."""
import gc
import json
from pathlib import Path
import unreal

ROOT='/Game/MultiplayerFPS/Game/Blueprints/'
def warm_template():
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day'):
        raise RuntimeError('Template gameplay level failed to initialize')
    # Complete template class initialization before switching levels. Never
    # retain source UWorld/Actor wrappers across the switch.
    paths=[ROOT+'Core/Spawnpoints/BP_SpawnPoint_A',ROOT+'Core/Spawnpoints/BP_SpawnPoint_B',
           ROOT+'Customization/Blueprints/BP_CustomizationSpawnPoints',ROOT+'Gamemodes/BP_EndGameCamera',
           ROOT+'Gamemodes/TeamDeathmatch/BP_TDM_GM']
    for path in paths:
        if not unreal.EditorAssetLibrary.load_blueprint_class(path):
            raise RuntimeError('Missing template class: '+path)
    room=[]
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        p=a.get_actor_location()
        center,_=a.get_actor_bounds(False)
        if isinstance(a,unreal.StaticMeshActor) and center.z < -3000 and abs(center.x)<3000 and abs(center.y)<3000:
            c=a.static_mesh_component
            mesh=c.get_editor_property('static_mesh')
            if mesh:
                r=a.get_actor_rotation(); s=a.get_actor_scale3d()
                room.append({'mesh':mesh.get_path_name(),'location':[p.x+6716.019545,p.y+8468.833148,p.z+29241.795291],
                    'rotation':[r.pitch,r.yaw,r.roll],'scale':[s.x,s.y,s.z],
                    'materials':[c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials())]})
    return room

def configure(room):
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    target='/Game/CS6/Maps/M_MilitaryBase'
    if not levels.load_level(target): raise RuntimeError('Working map failed to load')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
    def place(cls,label,position,rotation=unreal.Rotator()):
        actor=existing.get(label)
        if actor is None:
            actor=actors.spawn_actor_from_class(cls,unreal.Vector(*position),rotation)
            if not actor: raise RuntimeError('Could not place '+label)
            actor.set_actor_label(label); existing[label]=actor
        actor.set_actor_location(unreal.Vector(*position),False,False)
        actor.set_actor_rotation(rotation,False)
        return actor
    gm=unreal.EditorAssetLibrary.load_blueprint_class(ROOT+'Gamemodes/TeamDeathmatch/BP_TDM_GM')
    world.get_world_settings().set_editor_property('default_game_mode',gm)
    start=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlayerStart))
    start.set_actor_location(unreal.Vector(6475,7704,25091),False,False)
    start.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
    showroom=unreal.EditorAssetLibrary.load_blueprint_class(ROOT+'Customization/Blueprints/BP_CustomizationSpawnPoints')
    place(showroom,'CS6_LoadoutSpawnPoints',(6500,7500,25000))
    for i,piece in enumerate(room):
        mesh_actor=place(unreal.StaticMeshActor,'CS6_LoadoutGeometry_'+str(i),piece['location'],unreal.Rotator(*piece['rotation']))
        mesh_actor.static_mesh_component.set_static_mesh(unreal.load_asset(piece['mesh']))
        mesh_actor.set_actor_scale3d(unreal.Vector(*piece['scale']))
        for j,material in enumerate(piece['materials']):
            if material: mesh_actor.static_mesh_component.set_material(j,unreal.load_asset(material))
    if not room:
        floor=place(unreal.StaticMeshActor,'CS6_LoadoutFloor',(6500,7500,24980))
        floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
        floor.set_actor_scale3d(unreal.Vector(40,40,1))
    camera=unreal.EditorAssetLibrary.load_blueprint_class(ROOT+'Gamemodes/BP_EndGameCamera')
    place(camera,'CS6_EndGameCamera',(6500,6500,8500),unreal.Rotator(pitch=-25,yaw=30,roll=0))
    def ground(x,y):
        p=unreal.CS6TemplateBridge.find_ground(world,unreal.Vector(x,y,12000),unreal.Vector(x,y,4500))
        if not 5500<p.z<6500: return None
        return (p.x,p.y,p.z+12)
    placed={}
    for side,cx,cy,yaw in [('A',4500,5900,30),('B',8500,9800,210)]:
        cls=unreal.EditorAssetLibrary.load_blueprint_class(ROOT+'Core/Spawnpoints/BP_SpawnPoint_'+side)
        candidates=[(cx+dx,cy+dy) for dx in [-600,0,600,1200] for dy in [-600,0,600,1200]]
        points=[]
        for x,y in candidates:
            p=ground(x,y)
            if p and all((p[0]-q[0])**2+(p[1]-q[1])**2>250**2 for q in points): points.append(p)
            if len(points)==5: break
        if len(points)<5: raise RuntimeError('Insufficient grounded team '+side+' spawn locations')
        for i,p in enumerate(points):
            actor=place(cls,'CS6_Spawn_'+side+'_'+str(i+1),p,unreal.Rotator(pitch=0,yaw=yaw,roll=0))
            actor.set_editor_property('tags',[unreal.Name('CS6.Team'+side)])
        placed[side]=points
    nav=place(unreal.NavMeshBoundsVolume,'CS6_NavBounds',(6500,7600,6200))
    _,extent=nav.get_actor_bounds(False)
    scale=nav.get_actor_scale3d()
    if min(extent.x,extent.y,extent.z)<=0: raise RuntimeError('Navigation volume has no brush bounds')
    nav.set_actor_scale3d(unreal.Vector(scale.x*6000/extent.x,scale.y*5000/extent.y,scale.z*3500/extent.z))
    if not levels.save_current_level(): raise RuntimeError('Could not save military gameplay actors')
    nav_ready=unreal.CS6TemplateBridge.build_navigation(world)
    if not levels.save_current_level(): raise RuntimeError('Could not save military navigation')
    gi=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(ROOT+'Gamemodes/Instances/BP_GameInstance'))
    gi.set_editor_property('TeamABotCount',4)
    gi.set_editor_property('TeamBBotCount',5)
    if not unreal.EditorAssetLibrary.save_asset(ROOT+'Gamemodes/Instances/BP_GameInstance',only_if_is_dirty=False):
        raise RuntimeError('Could not persist default Bot configuration')
    Path('D:/CS6/reports/military-gameplay-setup.json').write_text(json.dumps(
        {'map':target,'spawns':placed,'navigation_built':nav_ready,'bots':[4,5],'reused_loadout_meshes':len(room)},indent=2),encoding='utf-8')

room=warm_template()
exec(open('D:/CS6/tools/inspect_blueprint_variables.py',encoding='utf-8').read())
gc.collect()
configure(room)
exec(open('D:/CS6/tools/register_military_map.py',encoding='utf-8').read())
unreal.SystemLibrary.quit_editor()
