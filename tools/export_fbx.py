import bpy
import sys

print("Blender export scene in FBX Format in file "+sys.argv[-1])

# Doc can be found here: https://docs.blender.org/api/current/bpy.ops.export_scene.html
bpy.ops.export_scene.fbx(filepath=sys.argv[-1])