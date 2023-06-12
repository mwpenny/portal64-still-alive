import bpy
import sys
import operator

print("Blender export scene in FBX Format in file "+sys.argv[-1])

def should_bake_material(mat):
    return 'bakeType' in mat and mat['bakeType'] == 'lit'

def should_bake_object(obj):
    return obj.type == 'MESH' and len(obj.material_slots) > 0 and obj.material_slots[0].material and should_bake_material(obj.material_slots[0].material)

def get_or_make_color_layer(mesh):
    for layer in mesh.vertex_colors:
        if layer.name == 'Col':
            return layer

    result = mesh.vertex_colors.new()

    return result

def vector_add(a, b):
    return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]

def vector_min(a, b):
    return [min(a[0], b[0]), min(a[1], b[1]), min(a[2], b[2])]

def vector_max(a, b):
    return [max(a[0], b[0]), max(a[1], b[1]), max(a[2], b[2])]

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
        if obj.type != 'MESH':
            raise Exception('Can only make ambient block from mesh')

        vertices, normals = world_space_verts(obj)
        colors = get_or_make_color_layer(obj.data)

        midpoint = calc_midpoint(vertices)

        corner_colors = [None] * 8

        bb_min = vertices[0]
        bb_max = vertices[0]

        for index in range(len(vertices)):
            corner_index = 0
            vertex = vertices[index]

            bb_min = vector_min(bb_min, vertex)
            bb_max = vector_max(bb_max, vertex)

            if vertex[0] > midpoint[0]:
                corner_index = corner_index + 1

            if vertex[1] > midpoint[1]:
                corner_index = corner_index + 2

            if vertex[2] > midpoint[2]:
                corner_index = corner_index + 4

            corner_colors[corner_index] = colors.data[index].color
            
        self.corner_colors = corner_colors
        self.bb = [bb_min, bb_max]

    
ambient_blocks = []

for obj in bpy.data.objects:
    if obj.name.startswith('@ambient '):
        ambient_blocks.append(AmbientBlock(obj))

print("Found ambient_blocks count: " + str(len(ambient_blocks)))

for obj in bpy.data.objects:
    if should_bake_object(obj):
        get_or_make_color_layer(obj.data)
