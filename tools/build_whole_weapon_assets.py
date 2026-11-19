import unreal,json,shutil,re
from pathlib import Path
root=Path('D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit')
backup=Path('D:/CS6/Backups/BeforeModelRegression-20261106/M_MilitaryBase.umap')
if not backup.exists():shutil.copy2(root/'Content/CS6/Maps/M_MilitaryBase.umap',backup)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/CS6/Maps/M_MilitaryBase')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
match=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CS6TacticalMatch));offers=list(match.get_editor_property('weapon_catalog'))
report=Path('D:/CS6/reports/whole-weapon-assets.json');prior={r['item']:r for r in json.loads(report.read_text(encoding='utf-8')).get('offers',[])} if report.exists() else {}
unreal.EditorAssetLibrary.make_directory('/Game/CS6/Weapons/Whole');cache={};rows=[]
for offer in offers:
 key=offer.get_editor_property('item_key');source_class=offer.get_editor_property('visual_class')
 source_path=source_class.get_path_name() if source_class else prior[key]['source']
 if not source_class:source_class=unreal.load_object(None,source_path)
 source=actors.spawn_actor_from_class(source_class,unreal.Vector(0,0,0));temps=[];trigger=None
 for part in source.get_components_by_class(unreal.StaticMeshComponent):
  name=part.get_name().lower();mesh=part.get_editor_property('static_mesh')
  if not mesh or any(t in name for t in ['cal_','reticle','navtech']):continue
  if 'silencer' in name and key!=14:continue
  if 'trigger' in name:trigger=unreal.SystemLibrary.get_component_bounds(part)[0]
  a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));a.set_actor_transform(part.get_world_transform(),False,True)
  c=a.static_mesh_component;c.set_static_mesh(mesh)
  for i in range(part.get_num_materials()):c.set_material(i,part.get_material(i))
  temps.append(a)
 assert temps
 asset_name='SM_Whole_'+re.sub('[^A-Za-z0-9_]','_',source_class.get_name())+('_Suppressed' if key==14 else '')
 asset_path='/Game/CS6/Weapons/Whole/'+asset_name
 existing_path='/Game/CS6/Weapons/Whole/SM_'+asset_name
 whole=unreal.load_asset(existing_path) if unreal.EditorAssetLibrary.does_asset_exist(existing_path) else (unreal.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None)
 merged=None
 if not whole:
  options=unreal.MergeStaticMeshActorsOptions();options.set_editor_property('base_package_name',asset_path);options.set_editor_property('destroy_source_actors',False);options.set_editor_property('spawn_merged_actor',True)
  settings=unreal.MeshMergingSettings();settings.set_editor_property('pivot_point_at_zero',True);settings.set_editor_property('merge_materials',False);settings.set_editor_property('generate_light_map_uv',False);settings.set_editor_property('lod_selection_type',unreal.MeshLODSelectionType.SPECIFIC_LOD);settings.set_editor_property('specific_lod',0)
  options.set_editor_property('mesh_merging_settings',settings)
  actors.set_selected_level_actors([])
  result=meshes.merge_static_mesh_actors(temps,options)
  actors.set_selected_level_actors([])
  merged=next((x for x in result if isinstance(x,unreal.StaticMeshActor)),None) if isinstance(result,tuple) else result
  assert merged,'Whole-gun merge failed for '+str(key)
  whole=merged.static_mesh_component.get_editor_property('static_mesh');assert whole
  assert unreal.EditorAssetLibrary.save_loaded_asset(whole,only_if_is_dirty=False)
  for a in temps:actors.destroy_actor(a)
 else:
  for a in temps:actors.destroy_actor(a)
 gun=actors.spawn_actor_from_class(offer.get_editor_property('weapon_class'),unreal.Vector(0,0,0));fp=next(c for c in gun.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='FPMesh')
 kit_origin,kit_extent,_=unreal.SystemLibrary.get_component_bounds(fp);kit_length=2*max(kit_extent.x,kit_extent.y,kit_extent.z)
 # Read bounds from the merged actor, or a temporary actor if this asset was already built.
 bounds_actor=merged
 if not bounds_actor:
  bounds_actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));bounds_actor.static_mesh_component.set_static_mesh(whole)
 pack_origin,pack_extent=bounds_actor.get_actor_bounds(False);pack_length=2*max(pack_extent.x,pack_extent.y,pack_extent.z)
 scale=max(.01,min(4,kit_length/pack_length));kit_trigger=fp.get_socket_transform('Trigger',unreal.RelativeTransformSpace.RTS_COMPONENT).translation
 authored=offer.get_editor_property('visual_offset');rotation=authored.rotation;pack_trigger=trigger if trigger else pack_origin
 rotated=rotation.rotate_vector(pack_trigger*scale)
 offset=unreal.Transform(location=kit_trigger-rotated,rotation=rotation.rotator(),scale=unreal.Vector(scale,scale,scale))
 offer.set_editor_property('visual_mesh',whole);offer.set_editor_property('visual_offset',offset);offer.set_editor_property('visual_class',None)
 rows.append(dict(item=key,source=source_path,mesh=whole.get_path_name(),parts_merged=len(temps),scale=scale,trigger=str(kit_trigger),offset=str(offset)))
 actors.set_selected_level_actors([])
 actors.destroy_actor(bounds_actor);actors.destroy_actor(gun);actors.destroy_actor(source)
 report.write_text(json.dumps(dict(saved=False,offers=rows),indent=2),encoding='utf-8')
match.modify();match.set_editor_property('weapon_catalog',offers);assert levels.save_current_level()
report.write_text(json.dumps(dict(saved=True,offers=rows),indent=2),encoding='utf-8')
unreal.log('CS6_WHOLE_WEAPONS_SAVED')
