
filegroup(
  name = "importer_files",
  srcs = [
    "__init__.py",
    "import_op.py",
    "import_panel.py",
  ],
)

genrule(
  name = "importer",
  srcs = [
    ":importer_files",
  ],
  outs = ["importer_copy.log"],
  cmd = "cp -vr /home/oz/work/me/cooking_ar/src/blender/importer /home/oz/.config/blender/2.92/scripts/addons/ > $@",
)
