import json
import unreal
paths = [
 '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent',
 '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/Skirmish/BP_Skirmish_GM',
 '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_WeaponBase',
]
result = []
for path in paths:
    unreal.log('CS6_INSPECT_LOADING ' + path)
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    default = unreal.get_default_object(cls) if cls else None
    result.append({'path':path, 'class':str(cls), 'members':dir(default) if default else []})
    with open('D:/CS6/reports/template-interfaces.json','w',encoding='utf-8') as f:
        json.dump(result,f,indent=2)
