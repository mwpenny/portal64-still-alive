import bpy
import math
import mathutils

def should_auto_uv(mat):
    return 'tileSizeS' in mat and 'tileSizeT' in mat

def should_auto_uv_object(obj):
    return obj.type == 'MESH' and len(obj.material_slots) > 0 and obj.material_slots[0].material and should_auto_uv(obj.material_slots[0].material)

def get_tile_sizes(obj):
    mat = obj.material_slots[0].material
    return float(mat['tileSizeS']), float(mat['tileSizeT'])

def vector_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def world_space_verts(obj):
    if obj.type != 'MESH':
        return None

    matrix_world = obj.matrix_world

    vertices = []

    for vertex in obj.data.vertices:
        vertices.append(matrix_world @ vertex.co)

    return vertices

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

def find_number_arg(args, name, default):
    value = find_named_arg(args, name)
    return float(value) if value else default

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

def auto_uv_group(obj, world_verts, group, uv_scale, rotation, translation):
    normal = mathutils.Vector([0, 0, 0])

    # Blender uses up=Z, Skeletool uses up=Y
    flip_y_z = mathutils.Matrix([
        [1, 0, 0],
        [0, 0, 1],
        [0, -1, 0],
    ])
    matrix_rotate = obj.matrix_world.to_3x3() @ rotation @ flip_y_z

    for polygon in group:
        normal = normal + polygon.normal

    normal = matrix_rotate @ normal
    normal.normalize()

    min_left = 1000000000000000
    min_up = 1000000000000000

    max_left = -min_left
    max_up = -min_up

    if abs(normal.y) > 0.7:
        up = mathutils.Vector([0, 0, 1])
        left = mathutils.Vector([1, 0, 0])
    elif abs(normal.z) > 0.7:
        up = mathutils.Vector([0, 1, 0])
        left = mathutils.Vector([1, 0, 0])
    else:
        up = mathutils.Vector([0, 1, 0])
        left = mathutils.Vector([0, 0, 1])

    for polygon in group:
        for loop_index in polygon.loop_indices:
            loop = obj.data.loops[loop_index]

            vertex = matrix_rotate @ world_verts[loop.vertex_index]

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

            vertex = (matrix_rotate @ world_verts[loop.vertex_index]) + translation

            s_coord = vertex @ left - min_left
            t_coord = vertex @ up - min_up

            uv_layer.data[loop_index].uv = [
                s_coord * s_tile - left_half_size,
                t_coord * t_tile - up_half_size
            ]

def auto_uv(obj):
    args = parse_args(obj.name)
    uv_scale = find_number_arg(args, "uvscale", 1)
    rotation = (
        mathutils.Quaternion((1, 0, 0), math.radians(find_number_arg(args, "uvrotx", 0))) @ \
        mathutils.Quaternion((0, 1, 0), math.radians(find_number_arg(args, "uvroty", 0))) @ \
        mathutils.Quaternion((0, 0, 1), math.radians(find_number_arg(args, "uvrotz", 0)))
    ).to_matrix()
    translation = mathutils.Vector([
        find_number_arg(args, "uvtransx", 0),
        find_number_arg(args, "uvtransy", 0),
        find_number_arg(args, "uvtransz", 0)
    ])

    polygon_groups = get_polygon_groups(obj.data)

    world_verts = world_space_verts(obj)

    for group in polygon_groups:
        auto_uv_group(obj, world_verts, group, uv_scale, rotation, translation)

def auto_uv_all():
    for obj in bpy.data.objects:
        if should_auto_uv_object(obj):
            auto_uv(obj)


class AutoUV(bpy.types.Operator):
    bl_idname = "wm.auto_uv"
    bl_label = "Generate UVs"

    def execute(self, context):
        auto_uv_all()
        self.report({'INFO'}, "UVs generated!")

        return {'FINISHED'}

    @classmethod
    def create(cls):
        registered_before = hasattr(bpy.types, bpy.ops.wm.auto_uv.idname())
        bpy.utils.register_class(cls)

        if not registered_before:
            menu_func = lambda self, context: self.layout.operator(cls.bl_idname, text=cls.bl_label)
            bpy.types.VIEW3D_MT_view.append(menu_func)

AutoUV.create()

# Test call the operator
bpy.ops.wm.auto_uv()
