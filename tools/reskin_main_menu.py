import json
import re
from pathlib import Path
import unreal

def resolve(value,outer=None):
    match=re.search(r"'([^']+)'",str(value))
    return unreal.find_object(None,match.group(1)) if match else unreal.find_object(outer,str(value))

path='/Game/MultiplayerFPS/Game/HUD/Menu/MainMenu/WB_MainMenu'
bp=unreal.load_asset(path)
changed=[]
nodes=[]
# Edit the authored tree; the compiler owns the generated class tree.
for outer in [bp]:
    tree=unreal.find_object(outer,'WidgetTree')
    if not tree: continue
    props=unreal.CS6TemplateBridge.read_template_properties(tree)
    root=resolve(props['RootWidget'],tree)
    stack=[root]
    while stack:
        widget=stack.pop()
        name=widget.get_name()
        nodes.append({'name':name,'class':widget.get_class().get_name(),
                      'text':str(widget.get_text()) if isinstance(widget,unreal.TextBlock) else ''})
        if isinstance(widget,unreal.TextBlock):
            replacements={'KitNameText':'CS6 TACTICAL','TitleText':'CS6 TACTICAL','TemplateText':'OPERATIONS',
                          'LocalText':'PLAY','SettingsText':'SETTINGS','ExitGameText':'EXIT'}
            if name in replacements:
                widget.set_text(replacements[name]); changed.append(name)
            old=str(widget.get_text()).strip().upper()
            if old=='ULTIMATE FPS':
                widget.set_text('CS6 TACTICAL'); changed.append('Title:'+name)
            elif old=='TEMPLATE':
                widget.set_text('OPERATIONS'); changed.append('Subtitle:'+name)
            if name=='TemplateText':
                widget.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(0.95,0.7,0.2,1)))
        if name in ['PlayButton','PlayText','OpenURLButton','OpenURLText',
                    'TemplateBackgroundImage','TemplateBackgroundShadowImage']:
            widget.set_visibility(unreal.SlateVisibility.COLLAPSED); changed.append(name)
        if isinstance(widget,unreal.PanelWidget): stack.extend(widget.get_all_children())
Path('D:/CS6/reports/main-menu-node-audit.json').write_text(json.dumps(nodes,indent=2),encoding='utf-8')
if not any(n in ['KitNameText','TitleText'] or n.startswith('Title:') for n in changed):
    raise RuntimeError('Main menu title widget was not found')
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
status=unreal.CS6TemplateBridge.read_template_properties(bp).get('Status','')
if 'Error' in status:
    raise RuntimeError('Main menu compilation failed: '+status)
if not unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False):
    raise RuntimeError('Standalone main menu could not be saved')
Path('D:/CS6/reports/main-menu-reskin.json').write_text(json.dumps({'changed':changed,'saved':True}),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
