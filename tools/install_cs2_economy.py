import unreal,json,shutil
from pathlib import Path
root=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BeforeCS2Overhaul')
bp_path='/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent'
bp_file=root/'Content/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent.uasset'
if not (backup/bp_file.name).exists():shutil.copy2(bp_file,backup/bp_file.name)
bp=unreal.load_asset(bp_path)
unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.CS6Character)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
status=unreal.CS6TemplateBridge.read_template_properties(bp).get('Status','')
if 'Error' in status:raise RuntimeError('Character parent compilation failed: '+status)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
base='/Game/MultiplayerFPS/Game/Weapons/'
rifle=base+'Primary/AK-74/BP_AK74M';smg=base+'Primary/AK74U/BP_AK74U';marksman=base+'Primary/SKS/BP_SKS';pistol=base+'Secondary/1911/BP_1911'
# IDs are the checked EconomyRules::Item enum. Only actual, compiled combat classes are offered.
specs=[
 (1,'Glock-18',pistol,'C1911',28,.15,.47,240,20,120,False),
 (2,'USP-S',pistol,'P350',35,.17,.505,240,12,24,False),
 (3,'P250',pistol,'P350',38,.16,.64,240,13,26,False),
 (5,'MAC-10',smg,'UMP',29,.075,.57,240,30,100,False),
 (6,'MP9',smg,'MP5',26,.07,.6,240,30,120,False),
 (7,'MP5-SD',smg,'MP5',27,.08,.625,235,30,120,False),
 (8,'UMP-45',smg,'UMP',35,.09,.65,230,25,100,False),
 (9,'P90',smg,'P90',26,.07,.69,230,50,100,False),
 (10,'Galil AR',rifle,'M16',30,.09,.775,215,35,90,False),
 (11,'FAMAS',rifle,'M16',30,.09,.7,220,25,90,False),
 (12,'AK-47',rifle,'AKM',36,.1,.775,215,30,90,False),
 (13,'M4A4',rifle,'MK18',33,.09,.7,225,30,90,False),
 (14,'M4A1-S',rifle,'MK18',38,.1,.7,225,20,80,False),
 (15,'AWP',marksman,'Sako85',115,1.45,.975,200,5,30,True),
 (16,'SSG 08',marksman,'Sako85',88,1.25,.85,230,10,90,True),
]
unreal.EditorAssetLibrary.make_directory('/Game/CS6/Weapons')
tools=unreal.AssetToolsHelpers.get_asset_tools();offers=[];rows=[]
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/ChamferZone_Weapons'],force_rescan=True)
for key,name,parent,visual,damage,rate,armor,speed,mag,reserve,scoped in specs:
    asset_name='BP_CS6_'+str(key);path='/Game/CS6/Weapons/'+asset_name
    child=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not child:
        factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.EditorAssetLibrary.load_blueprint_class(parent))
        child=tools.create_asset(asset_name,'/Game/CS6/Weapons',unreal.Blueprint,factory)
    assert child
    cdo=unreal.get_default_object(child.generated_class())
    for prop,value in [('GameName',name),('Damage',damage),('FireRate',rate),('MagazineCapacity',mag),('CurrentAmmo',mag)]:cdo.set_editor_property(prop,value)
    unreal.BlueprintEditorLibrary.compile_blueprint(child)
    child_status=unreal.CS6TemplateBridge.read_template_properties(child).get('Status','')
    if 'Error' in child_status:raise RuntimeError('Weapon compile failed '+path+':'+child_status)
    assert unreal.EditorAssetLibrary.save_loaded_asset(child,only_if_is_dirty=False)
    offer=unreal.CS6WeaponOffer();offer.set_editor_property('item_key',key);offer.set_editor_property('display_name',name);offer.set_editor_property('weapon_class',child.generated_class())
    candidates=[a for a in registry.get_assets_by_path('/Game/ChamferZone_Weapons/'+visual,recursive=False) if str(a.asset_class_path.asset_name)=='Blueprint']
    visual_class=unreal.EditorAssetLibrary.load_blueprint_class(str(candidates[0].package_name)) if candidates else None
    if visual_class:offer.set_editor_property('visual_class',visual_class)
    # Pack models face +Y; kit combat meshes face +X. Align the trigger/grip first.
    offsets={'AKM':(26,0,14),'MK18':(36,0,17),'M16':(30,0,15),'UMP':(22,0,13),'MP5':(20,0,13),'P90':(25,0,12),'Sako85':(25,0,15),'P350':(0,0,-2.5),'C1911':(0,0,0)}
    offset=unreal.Transform(location=unreal.Vector(*offsets[visual]),rotation=unreal.Rotator(pitch=0,yaw=-90,roll=0),scale=unreal.Vector(1,1,1))
    offer.set_editor_property('visual_offset',offset)
    for prop,value in [('damage',damage),('fire_interval',rate),('armor_penetration',armor),('move_speed',speed),('magazine',mag),('reserve',reserve),('scoped',scoped)]:offer.set_editor_property(prop,value)
    offers.append(offer);rows.append(dict(item=key,name=name,weapon=path,visual=visual,scoped=scoped))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
matches=[a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.CS6TacticalMatch)]
assert len(matches)==1
match=matches[0];match.modify();match.set_editor_property('weapon_catalog',offers);match.set_editor_property('economy_enabled',True)
assert levels.save_current_level()
sample=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,-10000))
movement=sample.get_component_by_class(unreal.CS6CharacterMovement)
if not movement:raise RuntimeError('Character movement native parent did not migrate')
unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(sample)
Path('D:/CS6/reports/cs2-economy-install.json').write_text(json.dumps(dict(saved=True,native_movement=True,offers=rows),indent=2),encoding='utf-8')
unreal.log('CS6_ECONOMY_INSTALLED')
