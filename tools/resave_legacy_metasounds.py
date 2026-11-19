import unreal,json,shutil
from pathlib import Path
project=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
paths=['/Game/MultiplayerFPS/Game/Effects/Sounds/Weapons/AK-74/MSS_AK-74Fire',
       '/Game/MultiplayerFPS/Game/Effects/Sounds/Weapons/SKS/MSS_SKS_Fire']
report=[]
for path in paths:
    relative=Path(path.removeprefix('/Game/')+'.uasset')
    backup=Path('D:/CS6/Backups/MetaSoundMigration54')/relative
    backup.parent.mkdir(parents=True,exist_ok=True)
    if not backup.exists(): shutil.copy2(project/'Content'/relative,backup)
    sound=unreal.load_asset(path)
    if not sound: raise RuntimeError('MetaSound missing: '+path)
    if not unreal.CS6TemplateBridge.rebuild_meta_sound_editor_graph(sound): raise RuntimeError('MetaSound editor graph rebuild failed: '+path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(sound,only_if_is_dirty=False): raise RuntimeError('MetaSound save failed: '+path)
    report.append({'asset':path,'saved':True})
Path('D:/CS6/reports/metasound-migration-54.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('CS6_METASOUND_MIGRATION_SAVED')
unreal.SystemLibrary.quit_editor()
