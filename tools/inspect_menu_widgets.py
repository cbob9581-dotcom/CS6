import json
import re
from pathlib import Path
import unreal

def ref_object(value):
    path=re.search(r"'([^']+)'",value)
    return unreal.find_object(None,path.group(1) if path else value) if value and value!='None' else None

report={}
paths=['/Game/MultiplayerFPS/Game/HUD/Menu/MainMenu/WB_MainMenu',
       '/Game/MultiplayerFPS/Game/HUD/Menu/CreateMatch/WB_CreateMatchMenuTDM',
       '/Game/MultiplayerFPS/Game/HUD/Menu/Offline/WB_PVPOffline']
for path in paths:
    bp=unreal.load_asset(path)
    cls=unreal.EditorAssetLibrary.load_blueprint_class(path)
    data={}
    for label,obj in [('blueprint',bp),('generated_class',cls)]:
        props=dict(unreal.CS6TemplateBridge.read_template_properties(obj))
        data[label]={str(k):str(v) for k,v in props.items() if 'WidgetTree' in str(k)}
        for name,value in list(data[label].items()):
            tree=ref_object(value)
            if tree:
                data[label][name+'_properties']=dict(unreal.CS6TemplateBridge.read_template_properties(tree))
    data['defaults']={str(k):str(v) for k,v in unreal.CS6TemplateBridge.read_template_properties(unreal.get_default_object(cls)).items()
                      if not str(k).startswith('Function:')}
    report[path]=data
Path('D:/CS6/reports/menu-widget-structure.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
