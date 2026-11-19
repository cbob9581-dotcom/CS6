import unreal,json
from pathlib import Path
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows={}
for p in ['/Game/MultiplayerFPS/Game/Weapons/Primary/AK-74/BP_AK74M','/Game/MultiplayerFPS/Game/Weapons/Secondary/1911/BP_1911','/Game/ChamferZone_Weapons/AKM/BP_AKM','/Game/ChamferZone_Weapons/MK18/BP_MK18','/Game/ChamferZone_Weapons/P350/BP_P350','/Game/MultiplayerFPS/Game/Blueprints/Core/BP_FPSCharacter_Parent']:
 c=unreal.EditorAssetLibrary.load_blueprint_class(p);a=actors.spawn_actor_from_class(c,unreal.Vector(0,0,-20000));out=[]
 for x in a.get_components_by_class(unreal.SceneComponent):
  props=dict(unreal.CS6TemplateBridge.read_template_properties(x))
  row=dict(name=x.get_name(),type=x.get_class().get_name(),parent=x.get_attach_parent().get_name() if x.get_attach_parent() else None,socket=str(x.get_attach_socket_name()),loc=str(x.relative_location),rot=str(x.relative_rotation),scale=str(x.relative_scale3d))
  if isinstance(x,unreal.MeshComponent):
   row['bounds']=str(x.get_local_bounds()) if isinstance(x,unreal.StaticMeshComponent) else ''
   row['asset']=str(props.get('SkeletalMesh',props.get('StaticMesh','')))
   if isinstance(x,unreal.SkeletalMeshComponent):row['bones']={str(x.get_bone_name(i)):str(x.get_socket_transform(x.get_bone_name(i),unreal.RelativeTransformSpace.RTS_COMPONENT)) for i in range(x.get_num_bones())}
  out.append(row)
 rows[p]=out;actors.destroy_actor(a)
Path('D:/CS6/reports/weapon-assembly-transforms.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
unreal.log('CS6_ASSEMBLY_AUDITED')
