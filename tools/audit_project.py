import json
import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets = registry.get_assets_by_path('/Game/MultiplayerFPS', recursive=True)
report = {'assets': len(assets), 'maps': [], 'blueprints': [], 'load_errors': [], 'note': 'Registry inventory only; loading validation is a separate step.'}
for asset in assets:
    path = str(asset.package_name)
    kind = str(asset.asset_class_path.asset_name)
    if kind == 'World':
        report['maps'].append(path)
    if kind == 'Blueprint':
        report['blueprints'].append({'path': path, 'tags': {str(k): str(v) for k, v in asset.tags_and_values.items()} if hasattr(asset, 'tags_and_values') else {}})
with open('D:/CS6/reports/template-audit.json', 'w', encoding='utf-8') as handle:
    json.dump(report, handle, indent=2, ensure_ascii=False)
unreal.log('CS6_AUDIT_COMPLETE assets=' + str(len(assets)))
