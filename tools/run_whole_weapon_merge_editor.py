import unreal,traceback
try:
    exec(compile(open('D:/CS6/tools/build_whole_weapon_assets.py',encoding='utf-8').read(),'D:/CS6/tools/build_whole_weapon_assets.py','exec'))
except Exception:
    unreal.log_error(traceback.format_exc())
finally:
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).set_selected_level_actors([])
    unreal.SystemLibrary.quit_editor()
