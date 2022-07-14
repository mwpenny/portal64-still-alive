from glob import glob
from math import sqrt
from re import L
import re
import sys
import traceback
import gdb
import contextlib
import ctypes
from OpenGL import GL as gl
import glfw

window_width = 800
window_height = 600

@contextlib.contextmanager
def create_main_window():
    global window_height
    global window_width

    if not glfw.init():
        sys.exit(1)
    try:
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)

        title = 'Surface Generator Debug'
        window = glfw.create_window(window_width, window_height, title, None, None)
        if not window:
            sys.exit(2)
        glfw.make_context_current(window)

        glfw.set_input_mode(window, glfw.STICKY_KEYS, True)
        gl.glClearColor(0, 0, 0, 0)

        yield window

    finally:
        glfw.terminate()

def dot_product(a, b):
    return a.x * b.x + a.y * b.y
    
class Vertex:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __repr__(self):
        return "Vertex(" + str(self.x) + ", " + str(self.y) + ")"


    def __str__(self):
        return "(" + str(self.x) + ", " + str(self.y) + ")"

    def midpoint(self, to):
        return Vertex((self.x + to.x) / 2, (self.y + to.y) / 2)

    def add(self, other):
        return Vertex(self.x + other.x, self.y + other.y)

    def sub(self, other):
        return Vertex(self.x - other.x, self.y - other.y)

    def scale(self, scalar):
        return Vertex(self.x * scalar, self.y * scalar)

class Edge:
    def __init__(self, aIndex, bIndex, nextEdge, prevEdge, nextEdgeReverse, prevEdgeReverse):
        self.aIndex = aIndex
        self.bIndex = bIndex
        self.nextEdge = nextEdge
        self.prevEdge = prevEdge
        self.nextEdgeReverse = nextEdgeReverse
        self.prevEdgeReverse = prevEdgeReverse

    def __repr__(self):
        return "Edge(" + str(self.aIndex) + ", " + str(self.bIndex) + ", " + str(self.nextEdge) + ", " + str(self.prevEdge) + ", " + str(self.nextEdgeReverse) + ", " + str(self.prevEdgeReverse) + ")"

next_color = [0.1, 0.2, 0]
prev_color = [0.2, 0.1, 0]

next_color_reverse = [0.15, 0.2, 0]
prev_color_reverse = [0.2, 0.15, 0]

class SurfaceConnections:
    def __init__(self, vertices, edges):
        self.vertices = vertices
        self.edges = edges

    def generate_connection(self, fromEdge, toEdge, color, vertex_pos, vertex_color, indices):
        fromMidpoint = self.vertices[fromEdge.aIndex].midpoint(self.vertices[fromEdge.bIndex])
        toMidpoint = self.vertices[toEdge.aIndex].midpoint(self.vertices[toEdge.bIndex])

        midpointMidpoint = fromMidpoint.midpoint(toMidpoint)

        curr_vertex = int(len(vertex_pos) / 3)

        indices.append(curr_vertex)
        indices.append(curr_vertex + 1)

        vertex_pos.append(fromMidpoint.x)
        vertex_pos.append(fromMidpoint.y)
        vertex_pos.append(0)

        vertex_pos.append(midpointMidpoint.x)
        vertex_pos.append(midpointMidpoint.y)
        vertex_pos.append(0)

        vertex_color.append(color[0])
        vertex_color.append(color[1])
        vertex_color.append(color[2])

        vertex_color.append(color[0])
        vertex_color.append(color[1])
        vertex_color.append(color[2])

    def build_vertex_buffer(self):
        vertex_array_id = gl.glGenVertexArrays(1)
        gl.glBindVertexArray(vertex_array_id)

        attr_id = 0

        vertex_buffer = gl.glGenBuffers(1)
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertex_buffer)

        vertex_pos = []
        vertex_color = []
        indices = []

        current_edge = 0

        for edge in self.edges:
            lerp = float(current_edge) / float(len(self.edges))

            curr_vertex = int(len(vertex_pos) / 3)

            pointA = self.vertices[edge.aIndex]
            pointB = self.vertices[edge.bIndex]

            vertex_pos.append(pointA.x)
            vertex_pos.append(pointA.y)
            vertex_pos.append(0)

            vertex_color.append(0)
            vertex_color.append(lerp)
            vertex_color.append(1)

            vertex_pos.append(pointB.x)
            vertex_pos.append(pointB.y)
            vertex_pos.append(0)

            vertex_color.append(1)
            vertex_color.append(lerp)
            vertex_color.append(0)

            indices.append(curr_vertex)
            indices.append(curr_vertex + 1)

            current_edge = current_edge + 1

            if edge.nextEdge != 255:
                self.generate_connection(edge, self.edges[edge.nextEdge], next_color, vertex_pos, vertex_color, indices)

            if edge.prevEdge != 255:
                self.generate_connection(edge, self.edges[edge.prevEdge], prev_color, vertex_pos, vertex_color, indices)

            if edge.nextEdgeReverse != 255:
                self.generate_connection(edge, self.edges[edge.nextEdgeReverse], next_color_reverse, vertex_pos, vertex_color, indices)

            if edge.prevEdgeReverse != 255:
                self.generate_connection(edge, self.edges[edge.prevEdgeReverse], prev_color_reverse, vertex_pos, vertex_color, indices)


        array_type = (gl.GLfloat * len(vertex_pos))

        gl.glBufferData(gl.GL_ARRAY_BUFFER, len(vertex_pos) * ctypes.sizeof(ctypes.c_float), array_type(*vertex_pos), gl.GL_STATIC_DRAW)

        gl.glVertexAttribPointer(
            attr_id,
            3,
            gl.GL_FLOAT,
            False,
            0,
            None
        )
        gl.glEnableVertexAttribArray(0)

        attr_id = 1

        color_buffer = gl.glGenBuffers(1)
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, color_buffer)
        array_type = (gl.GLfloat * len(vertex_color))
        gl.glBufferData(gl.GL_ARRAY_BUFFER,
            len(vertex_color) * ctypes.sizeof(ctypes.c_float),
            array_type(*vertex_color),
            gl.GL_STATIC_DRAW)
        gl.glVertexAttribPointer(
            attr_id,
            3,
            gl.GL_FLOAT,
            False,
            0,
            None
        )
        gl.glEnableVertexAttribArray(attr_id)

        index_buffer = gl.glGenBuffers(1)
        array_type = (gl.GLshort * len(indices))
        gl.glBindBuffer(gl.GL_ELEMENT_ARRAY_BUFFER, index_buffer)
        gl.glBufferData(
            gl.GL_ELEMENT_ARRAY_BUFFER,
            len(indices) * ctypes.sizeof(ctypes.c_short),
            array_type(*indices),
            gl.GL_STATIC_DRAW
        )

        return len(indices)

    def distance_to_edge(self, edge_index, from_point):
        edge = self.edges[edge_index]
        a = self.vertices[edge.aIndex]
        b = self.vertices[edge.bIndex]

        edge_dir = b.sub(a)
        point_dir = from_point.sub(a)

        edge_mag_sqr = dot_product(edge_dir, edge_dir)

        if edge_mag_sqr == 0:
            return sqrt(dot_product(point_dir, point_dir))

        lerp = dot_product(edge_dir, point_dir) / edge_mag_sqr

        if lerp < 0:
            lerp = 0
        elif lerp > 1:
            lerp = 1

        compare_point = a.add(edge_dir.scale(lerp))

        offset = compare_point.sub(from_point)

        return sqrt(dot_product(offset, offset))
        

    def find_closest_edge(self, from_point):
        result = 0
        distance = self.distance_to_edge(0, from_point)

        for index in range(1, len(self.edges)):
            distance_check = self.distance_to_edge(index, from_point)

            if distance_check < distance:
                distance = distance_check
                result = index

        return result
        
def build_shaders():
    shaders = {
        gl.GL_VERTEX_SHADER: '''\
            #version 330 core
            layout(location = 0) in vec3 vertexPosition_modelspace;
            layout(location = 1) in vec3 vertexColor;

            varying vec3 pixelColor;

            uniform mat4 transformMatrix;

            void main(){
              gl_Position = transformMatrix * vec4(vertexPosition_modelspace, 1);
              pixelColor = vertexColor;
            }
            ''',
        gl.GL_FRAGMENT_SHADER: '''\
            #version 330 core

            varying vec3 pixelColor;

            out vec3 color;
            void main(){
              color = pixelColor;
            }
            '''
        }
    program_id = gl.glCreateProgram()

    shader_ids = []
    for shader_type, shader_src in shaders.items():
        shader_id = gl.glCreateShader(shader_type)
        gl.glShaderSource(shader_id, shader_src)

        gl.glCompileShader(shader_id)

        # check if compilation was successful
        result = gl.glGetShaderiv(shader_id, gl.GL_COMPILE_STATUS)
        info_log_len = gl.glGetShaderiv(shader_id, gl.GL_INFO_LOG_LENGTH)
        if info_log_len:
            logmsg = gl.glGetShaderInfoLog(shader_id)
            print(logmsg)
            return None

        gl.glAttachShader(program_id, shader_id)
        shader_ids.append(shader_id)

    gl.glLinkProgram(program_id)

    # check if linking was successful
    result = gl.glGetProgramiv(program_id, gl.GL_LINK_STATUS)
    info_log_len = gl.glGetProgramiv(program_id, gl.GL_INFO_LOG_LENGTH)
    if info_log_len:
        logmsg = gl.glGetProgramInfoLog(program_id)
        print(logmsg)
        return None

    gl.glUseProgram(program_id)

    return program_id

def buildUniforms(program_id, scale, xOffset, yOffset):
    global window_height
    global window_width

    gl.glUseProgram(program_id)
    transform_matrix = gl.glGetUniformLocation(program_id, "transformMatrix")

    gl.glUniformMatrix4fv(transform_matrix, 1, gl.GL_FALSE, [
        scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        xOffset, yOffset, 0, 1
    ])

def extract_surface_data(surfaceBuilder):
    vertex_count = surfaceBuilder["currentVertex"]
    edge_count = surfaceBuilder["currentEdge"]

    input_vertices = surfaceBuilder["vertices"]
    output_vertices = []

    for vertex_index in range(0, vertex_count):
        input_vertex = input_vertices[vertex_index]
        output_vertices.append(Vertex(float(input_vertex["x"]), float(input_vertex["y"])))

    input_edges = surfaceBuilder["edges"]
    output_edges = []

    for edge_index in range(0, edge_count):
        input_edge = input_edges[edge_index]
        output_edges.append(Edge(
            input_edge["aIndex"],
            input_edge["bIndex"],
            input_edge["nextEdge"],
            input_edge["prevEdge"],
            input_edge["nextEdgeReverse"],
            input_edge["prevEdgeReverse"]
        ))

    return SurfaceConnections(output_vertices, output_edges)

def main():
    print("main")
    print(gl.glGetString(gl.GL_VENDOR), gl.glGetString(gl.GL_RENDERER))
    frame = gdb.selected_frame()
    print("Getting surfaceBuilder")
    surface = extract_surface_data(frame.read_var("surfaceBuilder"))

    with create_main_window() as window:
        print("Building vertex buffer")
        index_count = surface.build_vertex_buffer()

        print("Building shaders")
        program_id = build_shaders()
        if program_id is None:
            return

        max_extent = 0

        for vertex in surface.vertices:
            max_extent = max(max_extent, abs(vertex.x))
            max_extent = max(max_extent, abs(vertex.y))

        max_extent = max_extent + 10

        scale = float(1) / float(max_extent)
        xOffset = 0
        yOffset = 0

        last_button = 0

        buildUniforms(program_id, scale, xOffset, yOffset)

        while (
            glfw.get_key(window, glfw.KEY_ESCAPE) != glfw.PRESS and
            not glfw.window_should_close(window)
        ):
            if glfw.get_key(window, glfw.KEY_MINUS):
                scale = scale * 0.98

            if glfw.get_key(window, glfw.KEY_EQUAL):
                scale = scale / 0.98

            if glfw.get_key(window, glfw.KEY_LEFT):
                xOffset = xOffset + 0.01

            if glfw.get_key(window, glfw.KEY_RIGHT):
                xOffset = xOffset - 0.01

            if glfw.get_key(window, glfw.KEY_UP):
                yOffset = yOffset - 0.01

            if glfw.get_key(window, glfw.KEY_DOWN):
                yOffset = yOffset + 0.01

            current_button = glfw.get_mouse_button(window, glfw.MOUSE_BUTTON_1)

            if current_button and not last_button:
                cursor_pos = glfw.get_cursor_pos(window)
                world_pos = Vertex(
                    ((2 * cursor_pos[0] / window_width - 1) - xOffset) / scale,
                    ((1 - 2 * cursor_pos[1] / window_height) - yOffset) / scale
                )

                print(f"Checking for edges closest to")
                print(world_pos)
                closest_edge_index = surface.find_closest_edge(world_pos)

                closest_edge = surface.edges[closest_edge_index]
                print(f"Edge index {closest_edge_index}")
                print(closest_edge)
                print(surface.vertices[closest_edge.aIndex])
                print(surface.vertices[closest_edge.bIndex])

            last_button = current_button

            buildUniforms(program_id, scale, xOffset, yOffset)

            gl.glClear(gl.GL_COLOR_BUFFER_BIT)
            gl.glDrawElements(gl.GL_LINES, index_count, gl.GL_UNSIGNED_SHORT, None)

            glfw.swap_buffers(window)
            glfw.poll_events()

try:
    main()
except:
    print("An error happened")
    print(traceback.format_exc())