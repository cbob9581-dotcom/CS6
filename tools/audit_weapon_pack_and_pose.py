import unreal,json
from pathlib import Path
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/ChamferZone_Weapons'],force_rescan=True)
assets=registry.get_assets_by_path('/Game/ChamferZone_Weapons',recursive=True)
rows=[dict(path=str(a.package_name),kind=str(a.asset_class_path.asset_name)) for a in assets]
bp=unreal.load_asset('/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent')
cdo=unreal.get_default_object(bp.generated_class())
pose={}
for c in cdo.get_components_by_class(unreal.SceneComponent):
    pose[c.get_name()]=dict(type=c.get_class().get_name(),relative_location=str(c.relative_location),relative_scale=str(c.relative_scale3d),props=dict(unreal.CS6TemplateBridge.read_template_properties(c)))
samples={}
for path in ['/Game/ChamferZone_Weapons/M4/BP_M4','/Game/ChamferZone_Weapons/AKM/BP_AKM']:
    item=unreal.load_asset(path)
    if item:
        samples[path]={'properties':dict(unreal.CS6TemplateBridge.read_template_properties(unreal.get_default_object(item.generated_class()))),
                       'graphs':list(unreal.CS6TemplateBridge.inspect_blueprint_graph(item,''))}
Path('D:/CS6/reports/weapon-pack-pose-audit.json').write_text(json.dumps(dict(assets=rows,pose=pose,samples=samples),ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CS6_WEAPON_PACK_POSE_AUDITED')
