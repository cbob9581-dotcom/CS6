import unreal,json
from pathlib import Path
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CS6/Maps/M_MilitaryBase'): raise RuntimeError('Map load failed')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
setup=json.loads(Path('D:/CS6/reports/military-gameplay-setup.json').read_text(encoding='utf-8'))
report=[]
for side in ['A','B']:
    for p in setup['spawns'][side]:
        location=unreal.Vector(*p)+unreal.Vector(0,0,98)
        hits=[]
        for component in unreal.CS6TemplateBridge.get_pawn_blockers(world,location):
            actor=component.get_owner()
            _,extent=actor.get_actor_bounds(False)
            mesh=component.get_editor_property('static_mesh') if isinstance(component,unreal.StaticMeshComponent) else None
            hits.append({'actor':actor.get_name(),'label':actor.get_actor_label(),'component':component.get_name(),
                         'mesh':mesh.get_path_name() if mesh else '',
                         'extent':[extent.x,extent.y,extent.z],
                         'profile':str(component.get_collision_profile_name())})
        report.append({'side':side,'location':[location.x,location.y,location.z],'blockers':hits})
Path('D:/CS6/reports/military-capsule-collision.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('CS6_MILITARY_COLLISION_AUDIT_COMPLETE')
