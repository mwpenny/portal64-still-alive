import bpy
import sys
import operator
import math
import mathutils

debug = False

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

def vector_sub(a, b):
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]

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

def vector_div(a, b):
    if isinstance(a, float):
        return [a / x for x in b]
    
    if isinstance(b, float):
        return [x / b for x in a]

    return [a[0] / b[0], a[1] / b[1], a[2] / b[2]]

def vector_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def vector_lerp(a, b, t):
    if isinstance(t, float):
        t_inv = 1 - t
    else:
        t_inv = vector_sub([1, 1, 1], t)

    t_inv = 1 - t
    return vector_add(vector_mul(a, t_inv), vector_mul(b, t))

def vector_unlerp(a, b, pos):
    return vector_div(vector_sub(pos, a), vector_sub(b, a))
    

def color_lerp(a, b, t):
    t_inv = 1 - t

    return mathutils.Color([
        a[0] * t_inv + b[0] * t,
        a[1] * t_inv + b[1] * t,
        a[2] * t_inv + b[2] * t
    ])


def calc_midpoint(vertices):
    result = [0, 0, 0]

    for vertex in vertices:
        result = vector_add(result, vertex)

    return vector_mul(result, 1 / len(vertices))

def world_space_verts(obj):
    if obj.type != 'MESH':
        return None

    matrix_world = obj.matrix_world
    rotation = obj.matrix_world.to_3x3()

    vertices = []
    normals = []

    for vertex in obj.data.vertices:
        vertices.append(matrix_world @ vertex.co)
        normals.append(rotation @ vertex.normal)

    return vertices, normals

def calculate_point_light(point_light, pos, normal):

    light_pos = point_light.matrix_world @ mathutils.Vector([0, 0, 0])
    offset = light_pos - pos
    distance_sqrd = offset.length_squared

    offset.normalize()

    scalar = offset.dot(normal)

    if scalar <= 0:
        return mathutils.Color([0, 0, 0])

    scalar = point_light.data.energy * scalar / distance_sqrd

    return point_light.data.color * scalar

class BoundingBox:
    def __init__(self, min, max):
        self.min = min
        self.max = max

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

        for loop in obj.data.loops:
            corner_index = 0
            vertex = vertices[loop.vertex_index]

            bb_min = vector_min(bb_min, vertex)
            bb_max = vector_max(bb_max, vertex)

            if vertex[0] > midpoint[0]:
                corner_index = corner_index + 1

            if vertex[1] > midpoint[1]:
                corner_index = corner_index + 2

            if vertex[2] > midpoint[2]:
                corner_index = corner_index + 4

            corner_colors[corner_index] = colors.data[loop.index].color

        self.corner_colors = corner_colors
        self.bb = BoundingBox(bb_min, bb_max)
        self.point_lights = []

    def determine_distance(self, pos):
        closest_point = vector_max(self.bb.min, vector_min(self.bb.max, pos))
        diff = vector_sub(closest_point, pos)
        distance_sqrd = vector_dot(diff, diff)
        return math.sqrt(distance_sqrd)

    def determine_color(self, pos, normal):
        lerp_values = vector_unlerp(self.bb.min, self.bb.max, pos)

        lerp_values = vector_min(lerp_values, [1, 1, 1])
        lerp_values = vector_max(lerp_values, [0, 0, 0])

        x0 = color_lerp(self.corner_colors[0], self.corner_colors[1], lerp_values[0])
        x1 = color_lerp(self.corner_colors[2], self.corner_colors[3], lerp_values[0])
        x2 = color_lerp(self.corner_colors[4], self.corner_colors[5], lerp_values[0])
        x3 = color_lerp(self.corner_colors[6], self.corner_colors[7], lerp_values[0])

        y0 = color_lerp(x0, x1, lerp_values[1])
        y1 = color_lerp(x2, x3, lerp_values[1])

        result = color_lerp(y0, y1, lerp_values[2])

        for point_light in self.point_lights:
            light_contribution = calculate_point_light(point_light, pos, normal)
            result = result + light_contribution

        return result

def build_ambient_blocks():
    ambient_blocks = []

    for obj in bpy.data.objects:
        if obj.name.startswith('@ambient '):
            ambient_blocks.append(AmbientBlock(obj))

    for point_light in bpy.data.objects:
        if point_light.type != 'LIGHT' or not point_light.name.startswith('@point_light'):
            continue

        pos = point_light.matrix_world @ mathutils.Vector([0, 0, 0])

        distances = [block.determine_distance(pos) for block in ambient_blocks]
        block_index = min_indices(distances, 1)

        if len(block_index) == 0:
            continue

        ambient_blocks[block_index[0]].point_lights.append(point_light)

    return ambient_blocks

def min_indices(elements, count):
    result = []
    
    for index in range(len(elements)):
        insert_index = len(result)

        curr_value = elements[index]

        while insert_index > 0 and elements[result[insert_index - 1]] > curr_value:
            insert_index = insert_index - 1

        if insert_index < count:
            result.insert(insert_index, index)

        if len(result) > count:
            result.pop()

    return result


def determine_vertex_color(ambient_blocks, pos, normal):
    distances = [block.determine_distance(pos) for block in ambient_blocks]
    two_closest = min_indices(distances, 2)

    if len(two_closest) == 0:
        return mathutils.Color([1, 1, 1])

    if len(two_closest) == 1 or distances[two_closest[0]] < 0.0001:
        return ambient_blocks[two_closest[0]].determine_color(pos, normal)

    total_weight = distances[two_closest[0]] + distances[two_closest[1]]

    if total_weight == 0:
        return ambient_blocks[two_closest[0]].determine_color(pos)

    return color_lerp(
        ambient_blocks[two_closest[0]].determine_color(pos, normal),
        ambient_blocks[two_closest[1]].determine_color(pos, normal),
        distances[two_closest[0]] / total_weight
    )


def bake_object(obj, ambient_blocks):
    if obj.data.users > 1:
        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.make_single_user(obdata = True)

    global debug
    color_layer = get_or_make_color_layer(obj.data)

    vertices, normals = world_space_verts(obj)

    rotation = obj.matrix_world.to_3x3()

    for polygon in obj.data.polygons:
        for loop_index in polygon.loop_indices:
            loop = obj.data.loops[loop_index]
            vertex_index = loop.vertex_index
            
            if polygon.use_smooth:
                normal = normals[vertex_index]
            else:
                normal = rotation @ polygon.normal

            vertex_color = determine_vertex_color(
                ambient_blocks,
                vertices[vertex_index],
                normal
            )

            color_layer.data[loop.index].color = [
                min(vertex_color[0], 1), 
                min(vertex_color[1], 1), 
                min(vertex_color[2], 1),
                1
            ]

def bake_scene():
    bpy.ops.object.mode_set(mode='OBJECT', toggle=False)

    ambient_blocks = build_ambient_blocks()

    for obj in bpy.data.objects:
        if should_bake_object(obj):
            bake_object(obj, ambient_blocks)


bake_scene()

class VertexBake(bpy.types.Operator):
    bl_idname = "wm.vertex_bake"
    bl_label = "Vertex Bake Lighting"

    def execute(self, context):
        bake_scene()
        self.report({'INFO'}, "Baked!")

        return {'FINISHED'}

def vertex_bake_func(self, context):
    self.layout.operator(VertexBake.bl_idname, text="Vertex Bake Lighting")

bpy.utils.register_class(VertexBake)
bpy.types.VIEW3D_MT_view.append(vertex_bake_func)

# test call the operator
bpy.ops.wm.vertex_bake()