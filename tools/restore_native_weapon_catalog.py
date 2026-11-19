import unreal,json,shutil
from pathlib import Path
root=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BeforeNativeWeaponRestore-20261106');backup.mkdir(exist_ok=True)
source=root/'Content/CS6/Maps/M_MilitaryBase.umap'
if not (backup/source.name).exists():shutil.copy2(source,backup/source.name)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
match=next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.CS6TacticalMatch))
base='/Game/MultiplayerFPS/Game/Weapons/'
paths={1:('1911',base+'Secondary/1911/BP_1911'),2:('1911',base+'Secondary/1911/BP_1911'),8:('AK-74U',base+'Primary/AK74U/BP_AK74U'),12:('AK-74M',base+'Primary/AK-74/BP_AK74M'),13:('AK-74M',base+'Primary/AK-74/BP_AK74M'),16:('SKS',base+'Primary/SKS/BP_SKS')}
offers=[]
for offer in match.get_editor_property('weapon_catalog'):
 key=offer.get_editor_property('item_key')
 if key not in paths:continue
 label,path=paths[key];offer.set_editor_property('weapon_class',unreal.EditorAssetLibrary.load_blueprint_class(path));offer.set_editor_property('display_name',label);offer.set_editor_property('visual_class',None);offer.set_editor_property('visual_mesh',None);offers.append(offer)
match.modify();match.set_editor_property('weapon_catalog',offers);assert levels.save_current_level()
Path('D:/CS6/reports/native-weapon-restore.json').write_text(json.dumps(dict(saved=True,offers=[dict(item=k,name=v[0],asset=v[1]) for k,v in paths.items()],runtime_tests='not_run_user_requested_manual_validation'),indent=2),encoding='utf-8')
