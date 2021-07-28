import bpy

from os import path


class CookingImporterSettings(bpy.types.PropertyGroup):
  recording_path: bpy.props.StringProperty(
    name = "Recording path",
    description = "Path to the root directory of the recording.",
    default = path.join(path.expanduser("~"), "cooking_ar"),
    maxlen = 1024,
    subtype = "FILE_PATH",
  )
