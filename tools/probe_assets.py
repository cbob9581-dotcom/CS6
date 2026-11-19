import json
import unreal
from pathlib import Path

root = Path('D:/CS6/reports')
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
names = [
    '/Game/MultiplayerFPS/Game/Weapons/Primary/AK74U/Mesh/Magazine/SKM_AK74U_Magazine',
    '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_WeaponBase',
    '/Game/MultiplayerFPS/Game/Weapons/Secondary/1911/BP_1911',
]
with (root / 'probe-progress.jsonl').open('w', encoding='utf-8') as out:
    for path in names:
        out.write(json.dumps({'loading': path}) + '\n')
        out.flush()
        asset = unreal.load_asset(path)
        out.write(json.dumps({'loaded': path, 'ok': asset is not None}) + '\n')
        out.flush()
