import bpy
import sys
import operator
import math
import mathutils

def should_auto_uv(mat):
    return 'tileSizeS' in mat and 'tileSizeT' in mat

def should_auto_uv_object(obj):
    return obj.type == 'MESH' and len(obj.material_slots) > 0 and obj.material_slots[0].material and should_auto_uv(obj.material_slots[0].material)

def get_tile_sizes(obj):
    mat = obj.material_slots[0].material
    return float(mat['tileSizeS']), float(mat['tileSizeT'])

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

    return [
        a[0] * t_inv + b[0] * t,
        a[1] * t_inv + b[1] * t,
        a[2] * t_inv + b[2] * t,
        a[3] * t_inv + b[3] * t
    ]


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

    for vertex in obj.data.vertices:
        vertices.append(matrix_world @ vertex.co)

    return vertices

class BoundingBox:
    def __init__(self, min, max):
        self.min = min
        self.max = max

def parse_args(obj_name):
    return obj_name.split(' ')

def find_named_arg(args, name):
    found = False

    for arg in args:
        if arg == name:
            found = True
        elif found:
            return arg

    return None

def is_coplanar(a, b):
    return vector_dot(a.normal, b.normal) > 0.9

def add_polygon_connection(a, b, result):
    if a.index in result:
        result[a.index].append(b)
    else:
        result[a.index] = [b]

    if b.index in result:
        result[b.index].append(a)
    else:
        result[b.index] = [a]

def find_adjacent_polygons(mesh):
    polygons_for_edges = {}
    result = {}

    for polygon in mesh.polygons:
        for edge_key in polygon.edge_keys:
            if not edge_key in polygons_for_edges:
                polygons_for_edges[edge_key] = []

            previous_polygons = polygons_for_edges[edge_key]

            for previous_polygon in previous_polygons:
                if is_coplanar(polygon, previous_polygon):
                    add_polygon_connection(polygon, previous_polygon, result)

            previous_polygons.append(polygon)

    return result

def mark_polygon_in_group(polygon, adjacent_polygons, index_to_group, group_number, polygons_in_group):
    if polygon.index in index_to_group:
        return

    index_to_group[polygon.index] = group_number
    polygons_in_group.append(polygon)

    if not polygon.index in adjacent_polygons:
        return

    for adjacent in adjacent_polygons[polygon.index]:
        mark_polygon_in_group(adjacent, adjacent_polygons, index_to_group, group_number, polygons_in_group)

def get_polygon_groups(mesh):
    index_to_group = {}
    polygon_groups = []
    current_group = 0

    adjacent_polygons = find_adjacent_polygons(mesh)

    for polygon in mesh.polygons:
        if not polygon.index in index_to_group:
            group = []
            mark_polygon_in_group(polygon, adjacent_polygons, index_to_group, current_group, group)
            polygon_groups.append(group)
            current_group = current_group + 1

    return polygon_groups

def auto_uv_group(obj, world_verts, group, uv_scale):
    normal = mathutils.Vector([0, 0, 0])

    matrix_rotate = obj.matrix_world.to_3x3()

    for polygon in group:
        normal = normal + polygon.normal

    normal = matrix_rotate @ normal
    normal.normalize()

    min_left = 1000000000000000
    min_up = 1000000000000000

    max_left = -min_left
    max_up = -min_up

    if abs(normal.z) > 0.7:
        up = mathutils.Vector([0, 1, 0])
        left = mathutils.Vector([1, 0, 0])
    elif abs(normal.y) > 0.7:
        up = mathutils.Vector([0, 0, 1])
        left = mathutils.Vector([1, 0, 0])
    else:
        up = mathutils.Vector([0, 0, 1])
        left = mathutils.Vector([0, 1, 0])

    for polygon in group:
        for loop_index in polygon.loop_indices:
            loop = obj.data.loops[loop_index]

            vertex = world_verts[loop.vertex_index]

            left_dot = vertex @ left
            up_dot = vertex @ up

            min_left = min(min_left, left_dot)
            min_up = min(min_up, up_dot)

            max_left = max(max_left, left_dot)
            max_up = max(max_up, up_dot)

    s_tile, t_tile = get_tile_sizes(obj)

    s_tile = uv_scale / s_tile
    t_tile = uv_scale / t_tile

    left_half_size = math.floor((max_left - min_left) * 0.5 * s_tile)
    up_half_size = math.floor((max_up - min_up) * 0.5 * t_tile)

    uv_layer = obj.data.uv_layers[0]

    if not uv_layer:
        uv_layer = obj.data.uv_layers.new()

    for polygon in group:
        for loop_index in polygon.loop_indices:
            loop = obj.data.loops[loop_index]

            vertex = world_verts[loop.vertex_index]

            s_coord = vertex @ left - min_left
            t_coord = vertex @ up - min_up

            uv_layer.data[loop_index].uv = [
                s_coord * s_tile - left_half_size,
                t_coord * t_tile - up_half_size
            ]

def auto_uv(obj):
    args = parse_args(obj.name)
    uv_scale = find_named_arg(args, "uvscale")

    if uv_scale:
        uv_scale = float(uv_scale)
    else:
        uv_scale = 1.0

    polygon_groups = get_polygon_groups(obj.data)

    world_verts = world_space_verts(obj)

    for group in polygon_groups:
        auto_uv_group(obj, world_verts, group, uv_scale)


for obj in bpy.data.objects:
    if should_auto_uv_object(obj):
        auto_uv(obj)