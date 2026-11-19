import json
from pathlib import Path
import unreal

paths = [
 '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent',
 '/Game/MultiplayerFPS/Game/Blueprints/Components/C_Health',
 '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/BP_GMBase',
 '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/Instances/BP_GameInstance',
 '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/BP_Player_State',
 '/Game/MultiplayerFPS/Game/Blueprints/Core/BP_BasePlayerController',
 '/Game/MultiplayerFPS/Game/Blueprints/Core/AI/Blueprints/BP_AISpawnable',
 '/Game/MultiplayerFPS/Game/Blueprints/Gamemodes/TeamDeathmatch/BP_TDM_GS',
]
report={}
for path in paths:
    bp=unreal.load_asset(path)
    cls=unreal.EditorAssetLibrary.load_blueprint_class(path)
    default=unreal.get_default_object(cls) if cls else None
    fields=[{'name':str(name),'default':str(value)} for name,value in
            unreal.CS6TemplateBridge.read_template_properties(default).items()]
    report[path]=fields
Path('D:/CS6/reports/blueprint-variables.json').write_text(
    json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
