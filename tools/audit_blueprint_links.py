import unreal,json
from pathlib import Path

def warm():
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level('/Game/MultiplayerFPS/Maps/Levels/TDM/DAY/M_TDM_Aztec_Day')
    levels.load_level('/Game/CS6/Maps/M_MilitaryBase')

warm()
result=dict(unreal.CS6TemplateBridge.audit_loaded_blueprint_links())
Path('D:/CS6/reports/broken-blueprint-links.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
unreal.log('CS6_LINK_AUDIT '+str(result))
unreal.SystemLibrary.quit_editor()
