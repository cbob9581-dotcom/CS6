import unreal,json
from pathlib import Path

def warm():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')

def install():
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/CS6/Maps/M_MilitaryBase'): raise RuntimeError('Military map load failed')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith(('CS6_BombMatch','CS6_BombSite','CS6_SiteLabel')):
            actors.destroy_actor(actor)
    setup=json.loads(Path('D:/CS6/reports/military-gameplay-setup.json').read_text(encoding='utf-8'))
    starts=[unreal.Vector(*setup['spawns'][side][0])+unreal.Vector(0,0,90) for side in ['A','B']]
    sites=[]
    routes=[]
    for label,candidates in [('A',[(6500,7000),(6200,7500),(6800,8000)]),('B',[(9000,8500),(9000,7500),(8500,8500)])]:
        chosen=None
        for x,y in candidates:
            p=unreal.CS6TemplateBridge.find_ground(world,unreal.Vector(x,y,6500),unreal.Vector(x,y,5500))
            if not 5500<p.z<6500: continue
            valid=True
            for start in starts:
                route=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,start,p+unreal.Vector(0,0,90))
                if not route or not route.is_valid() or route.is_partial(): valid=False;break
            if valid: chosen=p;break
        if chosen is None: raise RuntimeError('No complete navigation to site '+label)
        target=actors.spawn_actor_from_class(unreal.TargetPoint,chosen+unreal.Vector(0,0,90))
        target.set_actor_label('CS6_BombSite_'+label)
        target.set_editor_property('tags',[unreal.Name('CS6.BombSite'+label)])
        sites.append(target)
        marker=actors.spawn_actor_from_class(unreal.TextRenderActor,chosen+unreal.Vector(0,0,180))
        marker.set_actor_label('CS6_SiteLabel_'+label)
        text=marker.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(label)
        text.set_world_size(100)
        text.set_text_render_color(unreal.Color(255,170,40,255))
        routes.append({'site':label,'position':[chosen.x,chosen.y,chosen.z],'both_teams_reachable':True})
    match=actors.spawn_actor_from_class(unreal.CS6TacticalMatch,unreal.Vector(6500,7500,6300))
    match.set_actor_label('CS6_BombMatch')
    root='/Game/MultiplayerFPS/Game/'
    match.set_editor_property('template_pawn',unreal.EditorAssetLibrary.load_blueprint_class(root+'Blueprints/Core/BP_FPSCharacter_Parent'))
    match.set_editor_property('starting_rifle',unreal.EditorAssetLibrary.load_blueprint_class(root+'Weapons/Primary/AK-74/BP_AK74M'))
    match.set_editor_property('starting_pistol',unreal.EditorAssetLibrary.load_blueprint_class(root+'Weapons/Secondary/1911/BP_1911'))
    match.set_editor_property('bomb_mesh',unreal.load_asset('/Game/FC_MilitaryCamp/Models/MilitaryModels/SM_ArmorBox_1'))
    match.set_editor_property('bomb_sites',sites)
    match.set_editor_property('plant_radius',250)
    if not levels.save_current_level(): raise RuntimeError('Bomb match map save failed')
    Path('D:/CS6/reports/bomb-match-map-setup.json').write_text(json.dumps({'saved':True,'sites':routes,'adapter':'CS6TacticalMatch'},indent=2),encoding='utf-8')
    unreal.log('CS6_BOMB_MATCH_MAP_SAVED')

warm()
install()
unreal.SystemLibrary.quit_editor()
