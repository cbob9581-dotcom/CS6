import unreal,json
from pathlib import Path
root='/Game/MultiplayerFPS/Game/'
paths=[root+'Weapons/Primary/AK-74/BP_AK74M',root+'Blueprints/Core/BP_WeaponBase',root+'HUD/Loadout/Customization/Primary/WB_WeaponListPrimary']
report={}
for path in paths:
    bp=unreal.load_asset(path)
    cls=unreal.EditorAssetLibrary.load_blueprint_class(path)
    report[path]={'properties':dict(unreal.CS6TemplateBridge.read_template_properties(unreal.get_default_object(cls))),
                  'graphs':list(unreal.CS6TemplateBridge.inspect_blueprint_graph(bp,''))}
Path('D:/CS6/reports/weapon-defaults.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CS6_WEAPON_DEFAULTS_AUDIT_COMPLETE')
