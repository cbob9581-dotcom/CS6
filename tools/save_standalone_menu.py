import json
from pathlib import Path
import unreal
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(
    '/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')
gi_path='/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/Instances/BP_GameInstance'
gi=unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(gi_path))
gi.set_editor_property('TeamABotCount',4)
gi.set_editor_property('TeamBBotCount',5)
gi.set_editor_property('PointsToWin',100)
if not unreal.EditorAssetLibrary.save_asset(gi_path,only_if_is_dirty=False):
    raise RuntimeError('Game instance save failed')
exec(open('D:/CS6/tools/register_military_map.py',encoding='utf-8').read())
Path('D:/CS6/reports/standalone-menu-save.json').write_text(json.dumps({'saved':True,'bots':[4,5]}),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
