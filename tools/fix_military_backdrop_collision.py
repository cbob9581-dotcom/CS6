import unreal,json,shutil
from pathlib import Path
project=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BeforeBackdropCollision/M_MilitaryBase.umap')
backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists(): shutil.copy2(project/'Content/CS6/Maps/M_MilitaryBase.umap',backup)
setup_backup=backup.parent/'military-gameplay-setup.json'
if not setup_backup.exists(): shutil.copy2('D:/CS6/reports/military-gameplay-setup.json',setup_backup)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/CS6/Maps/M_MilitaryBase'): raise RuntimeError('Military map load failed')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
all_actors=actors.get_all_level_actors()
changed=[]
for actor in all_actors:
    if isinstance(actor,unreal.StaticMeshActor) and actor.get_actor_label().startswith('SM_Mountain'):
        component=actor.static_mesh_component
        changed.append({'actor':actor.get_name(),'label':actor.get_actor_label(),
                        'mesh':component.static_mesh.get_path_name(),'before':str(component.get_collision_enabled())})
        actor.modify()
        component.modify()
        component.set_collision_profile_name('NoCollision')
        actor.set_actor_enable_collision(False)
if len(changed)!=10: raise RuntimeError('Unexpected backdrop actor count: '+str(len(changed)))
if not levels.save_current_level(): raise RuntimeError('Backdrop collision map save failed')
if not unreal.CS6TemplateBridge.build_navigation(world): raise RuntimeError('Navigation build failed')
sites=[actor.get_actor_location() for actor in all_actors if actor.get_actor_label().startswith('CS6_BombSite_')]
if len(sites)!=2: raise RuntimeError('Both bomb sites are required')
def reachable(point):
    for site in sites:
        route=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,unreal.Vector(*point)+unreal.Vector(0,0,96),site)
        if not route or not route.is_valid() or route.is_partial(): return False
    return True
spawns={}
for side,anchor in [('A',(3900,5300)),('B',(10000,7500))]:
    markers=sorted([a for a in all_actors if a.get_actor_label().startswith('CS6_Spawn_'+side+'_')],key=lambda a:a.get_actor_label())
    if len(markers)!=5: raise RuntimeError('Expected five spawn markers for '+side)
    chosen=[]
    candidates=[(anchor[0]+dx,anchor[1]+dy) for dx in [0,600,1200,-600,1800,-1200,-1800] for dy in [0,600,-600,1200,-1200,1800,-1800]]
    for x,y in candidates:
        ground=unreal.CS6TemplateBridge.find_ground(world,unreal.Vector(x,y,6500),unreal.Vector(x,y,5500))
        p=(ground.x,ground.y,ground.z+12)
        if not 5500<ground.z<6500 or not reachable(p): continue
        if any((p[0]-q[0])**2+(p[1]-q[1])**2<350**2 for q in chosen): continue
        chosen.append(p)
        if len(chosen)==5: break
    if len(chosen)!=5: raise RuntimeError('No connected five-player spawn set for '+side)
    for marker,p in zip(markers,chosen): marker.set_actor_location(unreal.Vector(*p),False,False)
    spawns[side]=chosen
if not levels.save_current_level(): raise RuntimeError('Revised spawn and navigation save failed')
report={'saved':True,'backdrop_components':changed,'navigation_built':True,'spawns':spawns,
        'all_ten_spawns_reach_both_sites':True,'runtime_physics_validation':'pending'}
Path('D:/CS6/reports/military-collision-fix.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
setup_path=Path('D:/CS6/reports/military-gameplay-setup.json')
setup=json.loads(setup_path.read_text(encoding='utf-8'))
setup.update(spawns=spawns,navigation_built=True,backdrop_collision_fixed=True)
setup_path.write_text(json.dumps(setup,indent=2),encoding='utf-8')
unreal.log('CS6_BACKDROP_COLLISION_FIX_SAVED')
