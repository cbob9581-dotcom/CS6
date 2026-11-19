"""Register the new playable map without replacing existing menu entries."""
import json
from pathlib import Path
import unreal

path='/Game/MultiplayerFPS/Maps/DT_Maps'
table=unreal.load_asset(path)
if not table:
    raise RuntimeError('Original map menu data table is unavailable')
original=unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
backup=Path('D:/CS6/reports/maps-before-military.json')
if not backup.exists():
    backup.write_text(original,encoding='utf-8')
rows=json.loads(original)
base=next(row for row in rows if row['GameMode']=='Team Deathmatch')
entry=dict(base)
entry.update(Name='MilitaryBaseTDM',LevelName='M_MilitaryBase',
             command='servertravel /Game/CS6/Maps/M_MilitaryBase',
             MapName='Military Base',DayOrNight='DAY',
             Description='Military base tactical combat against bots')
# The standalone build exposes its authored map, rather than advertising
# template maps that are not included in this game's cooked package.
rows=[entry]
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows)):
    raise RuntimeError('Map table import failed; original JSON backup retained')
if not unreal.EditorAssetLibrary.save_loaded_asset(table,only_if_is_dirty=False):
    raise RuntimeError('Military map table could not be saved to disk')
mode_path='/Game/MultiplayerFPS/Game/Blueprints/Customization/Tables/DT_GameModes'
mode_table=unreal.load_asset(mode_path)
mode_json=unreal.DataTableFunctionLibrary.export_data_table_to_json_string(mode_table)
mode_backup=Path('D:/CS6/reports/gamemodes-before-standalone.json')
if not mode_backup.exists(): mode_backup.write_text(mode_json,encoding='utf-8')
modes=[r for r in json.loads(mode_json) if r['Name']=='TDM']
if len(modes)!=1: raise RuntimeError('Template TDM menu entry missing')
if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(mode_table,json.dumps(modes)):
    raise RuntimeError('Standalone mode table import failed')
if not unreal.EditorAssetLibrary.save_loaded_asset(mode_table,only_if_is_dirty=False):
    raise RuntimeError('Standalone mode table could not be saved to disk')
Path('D:/CS6/reports/maps-with-military.json').write_text(
    unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')
unreal.log('CS6_MILITARY_MENU_REGISTERED')
