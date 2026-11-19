import unreal,json,shutil
from pathlib import Path

project=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BlueprintMigration54')
root='/Game/MultiplayerFPS/Game/'
paths=[root+p for p in ['Blueprints/Core/BP_WeaponBaseParent','Blueprints/Core/BP_WeaponBase',
       'Blueprints/Core/BP_FPSCharacter_Parent','Blueprints/Core/AI/Blueprints/BP_AISpawnable',
       'Blueprints/Core/BP_BasePlayerController','Weapons/Primary/AK-74/BP_AK74M','Weapons/Secondary/1911/BP_1911']]
for path in paths:
    relative=Path(path.removeprefix('/Game/')+'.uasset')
    destination=backup/relative
    destination.parent.mkdir(parents=True,exist_ok=True)
    if not destination.exists(): shutil.copy2(project/'Content'/relative,destination)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')
report=[]
for path in paths:
    bp=unreal.load_asset(path)
    if not unreal.CS6TemplateBridge.rebuild_blueprint(bp): raise RuntimeError('Blueprint rebuild failed: '+path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False): raise RuntimeError('Blueprint save failed: '+path)
    report.append({'asset':path,'rebuilt':True,'saved':True})
Path('D:/CS6/reports/blueprint-migration-54.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('CS6_BLUEPRINT_MIGRATION_SAVED')
unreal.SystemLibrary.quit_editor()
