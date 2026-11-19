import unreal,json
from pathlib import Path
paths=['/Game/MultiplayerFPS/Game/Blueprints/Core/Throwable/Grenade/BP_Grenade','/Game/MultiplayerFPS/Game/Blueprints/Core/Throwable/Smoke/BP_SmokeGrenade','/Game/MultiplayerFPS/Game/Blueprints/Core/AI/Blueprints/Throwables/BP_AIGrenade','/Game/MultiplayerFPS/Game/Blueprints/Core/AI/Blueprints/Throwables/BP_AISmoke']
rows={}
for p in paths:
 bp=unreal.load_asset(p);c=bp.generated_class();rows[p]=dict(properties=dict(unreal.CS6TemplateBridge.read_template_properties(unreal.get_default_object(c))),graphs=list(unreal.CS6TemplateBridge.inspect_blueprint_graph(bp,'')))
Path('D:/CS6/reports/grenade-assets.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
