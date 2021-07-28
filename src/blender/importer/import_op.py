"""
From a blank scene, imports all data recorded as well as camera positions.
"""
import bpy
import yaml

from os import listdir, path
from . property_group import CookingImporterSettings


class CookingImportOperator(bpy.types.Operator):
  bl_idname = "cooking_ar.import_recording"
  bl_label = "Import cooking recording"
  bl_description = "Import all extracted animation data and camera positions."

  def execute(self, context):
    anim_dir = path.join(context.scene.cooking_ar.recording_path, "animation")

    frame_files = [f for f in listdir(anim_dir) if f.endswith(".yml")]

    test_frame = frame_files[0]
    with open(path.join(anim_dir, test_frame), "r") as stream:
      frame = yaml.safe_load(stream)
      for person in frame.people:
        person_name = "cooking_ar_person_" + person.person_id
        for point in person.body:
          point_name =  person_name + "_point_" + point.point_id
          bpy.data.objects.new(point_name, None)

    return {"FINISHED"}
