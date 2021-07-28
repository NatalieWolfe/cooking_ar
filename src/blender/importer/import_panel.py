import bpy

from . import_op import CookingImportOperator


class CookingImportPanel(bpy.types.Panel):
  bl_idname = "CookingImportPanel"
  bl_label = "Cooking Importer Panel"
  bl_category = "Cooking Importer"
  bl_space_type = "VIEW_3D"
  bl_region_type = "UI"

  def draw(self, context):
    row = self.layout.row()
    row.prop(context.scene.cooking_ar, "recording_path")

    row = self.layout.row()
    row.operator(CookingImportOperator.bl_idname, text=CookingImportOperator.bl_label)
