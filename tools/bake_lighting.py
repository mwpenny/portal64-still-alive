import bpy
import sys
import operator

print("Blender export scene in FBX Format in file "+sys.argv[-1])

def shouldBakeMaterial(mat):
    return 'bakeType' in mat and mat['bakeType'] == 'lit'

def shouldBakeObject(obj):
    return obj.type == 'MESH' and len(obj.material_slots) > 0 and obj.material_slots[0].material and shouldBakeMaterial(obj.material_slots[0].material)

def ensureColorLayer(mesh):
    for layer in mesh.vertex_colors:
        if layer.name == 'Col':
            return layer

    result = mesh.vertex_colors.new()

    return result

def vector_add(a, b):
    return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]

def vector_mul(a, b):
    if isinstance(a, float):
        return [x * a for x in b]
    
    if isinstance(b, float):
        return [x * b for x in a]

    return [a[0] * b[0], a[1] * b[1], a[2] * b[2]]
    

def calc_midpoint(vertices):
    result = [0, 0, 0]

    for vertex in vertices:
        result = vector_add(result, vertex)

    return vector_mul(result, 1 / len(vertices))

def world_space_verts(obj):
    if obj.type != 'MESH':
        return None

    matrix_world = obj.matrix_world

    vertices = []
    normals = []

    for vertex in obj.data.vertices:
        vertices.append(matrix_world @ vertex.co)
        # todo transform normal correctly
        normals.append(matrix_world @ vertex.normal)

    return vertices, normals


class AmbientBlock:
    def __init__(self, obj):
        vertices, normals = world_space_verts(obj)

        midpoint = calc_midpoint(vertices)

        self.midpoint = midpoint
        print(midpoint)

    
ambient_blocks = []

for obj in bpy.data.objects:
    if obj.name.startswith('@ambient '):
        ambient_blocks.append(AmbientBlock(obj))

print("Found ambient_blocks count: " + str(len(ambient_blocks)))

for obj in bpy.data.objects:
    if shouldBakeObject(obj):
        ensureColorLayer(obj.data)
