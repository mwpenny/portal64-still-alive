from glob import glob
from math import cos, sin, sqrt
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
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return "Vertex(" + str(self.x) + ", " + str(self.y) + ", " + str(self.z) + ")"


    def __str__(self):
        return "(" + str(self.x) + ", " + str(self.y) + ", " + str(self.z) + ")"

    def midpoint(self, to):
        return Vertex((self.x + to.x) / 2, (self.y + to.y) / 2, (self.z + to.z) / 2)

    def add(self, other):
        return Vertex(self.x + other.x, self.y + other.y, self.z + other.z)

    def sub(self, other):
        return Vertex(self.x - other.x, self.y - other.y, self.z - other.z)

    def scale(self, scalar):
        return Vertex(self.x * scalar, self.y * scalar, self.z * scalar)

class Triangle:
    def __init__(self, a, b, c):
        self.a = a
        self.b = b
        self.c = c

    def __repr__(self):
        return "Edge(" + str(self.aIndex) + ", " + str(self.bIndex) + ", " + str(self.nextEdge) + ", " + str(self.prevEdge) + ", " + str(self.nextEdgeReverse) + ", " + str(self.prevEdgeReverse) + ")"

class Simplex:
    def __init__(self, vertices, triangles, closestFace):
        self.vertices = vertices
        self.triangles = triangles
        self.closestFace = closestFace

    def build_vertex_buffer(self, raycastDir):
        vertex_array_id = gl.glGenVertexArrays(1)
        gl.glBindVertexArray(vertex_array_id)

        attr_id = 0

        vertex_buffer = gl.glGenBuffers(1)
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertex_buffer)

        vertex_pos = []
        vertex_color = []
        indices = []

        def append_vertex(pos, color):
            vertex_pos.append(pos.x)
            vertex_pos.append(pos.y)
            vertex_pos.append(pos.z)

            vertex_color.append(color[0])
            vertex_color.append(color[1])
            vertex_color.append(color[2])


        for vertex in self.vertices:
            append_vertex(vertex, [0, 1, 1])

        for triangle in self.triangles:
            indices.append(triangle.a)
            indices.append(triangle.b)

            indices.append(triangle.b)
            indices.append(triangle.c)

            indices.append(triangle.c)
            indices.append(triangle.a)

        indices.append(len(self.vertices))
        indices.append(len(self.vertices) + 1)

        append_vertex(Vertex(0, 0, 0), [0, 1, 0])
        append_vertex(raycastDir, [0, 1, 0])

        if self.closestFace:
            append_vertex(self.vertices[self.closestFace.a], [1, 0, 0])
            append_vertex(self.vertices[self.closestFace.b], [1, 0, 0])
            append_vertex(self.vertices[self.closestFace.c], [1, 0, 0])

            indices.append(len(self.vertices) + 2)
            indices.append(len(self.vertices) + 3)

            indices.append(len(self.vertices) + 3)
            indices.append(len(self.vertices) + 4)

            indices.append(len(self.vertices) + 4)
            indices.append(len(self.vertices) + 2)
            

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

def buildUniforms(program_id, scale, yaw, pitch):
    global window_height
    global window_width

    gl.glUseProgram(program_id)
    transform_matrix = gl.glGetUniformLocation(program_id, "transformMatrix")

    cosH = cos(yaw)
    sinH = sin(yaw)

    cosP = cos(pitch)
    sinP = sin(pitch)

    gl.glUniformMatrix4fv(transform_matrix, 1, gl.GL_FALSE, [
        scale * cosH, 0, -scale * sinH, 0,
        scale * sinH * sinP, scale * cosP, scale * cosH * sinP, 0,
        scale * sinH * cosP, scale * -sinP, scale * cosH * cosP, 0,
        0, 0, 0, 1
    ])

def extract_simplex_data(simplex, closestFace):
    vertex_count = simplex["pointCount"]
    triangle_count = simplex["triangleCount"]

    input_vertices = simplex["points"]
    output_vertices = []

    for vertex_index in range(0, vertex_count):
        input_vertex = input_vertices[vertex_index]
        output_vertices.append(Vertex(float(input_vertex["x"]), float(input_vertex["y"]), float(input_vertex["z"])))

    input_triangles = simplex["triangles"]
    output_triangles = []

    for edge_index in range(0, triangle_count):
        input_triangle = input_triangles[edge_index]
        output_triangles.append(Triangle(
            int(input_triangle["indexData"]["indices"][0]),
            int(input_triangle["indexData"]["indices"][1]),
            int(input_triangle["indexData"]["indices"][2])
        ))

    if closestFace:
        closestFaceTriangle = Triangle(
            int(closestFace["indexData"]["indices"][0]),
            int(closestFace["indexData"]["indices"][1]),
            int(closestFace["indexData"]["indices"][2])
        )
    else:
        closestFaceTriangle = None

    return Simplex(output_vertices, output_triangles, closestFaceTriangle)

def main():
    print("main")
    print(gl.glGetString(gl.GL_VENDOR), gl.glGetString(gl.GL_RENDERER))
    frame = gdb.selected_frame()
    print("Getting simplex")
    try:
        closestFace = frame.read_var("closestFace")
    except:
        closestFace = None

    simplexVar = None

    try:
        simplexVar = frame.read_var("simplex")
    except:
        simplexVar = frame.read_var("expandingSimplex")

    simplex = extract_simplex_data(simplexVar, closestFace)

    try:
        raycastDir = frame.read_var("raycastDir")
    except:
        raycastDir = {"x": 0, "y": 0, "z": 0}


    with create_main_window() as window:
        print("Building vertex buffer")
        index_count = simplex.build_vertex_buffer(Vertex(float(raycastDir["x"]), float(raycastDir["y"]), float(raycastDir["z"])))

        print("Building shaders")
        program_id = build_shaders()
        if program_id is None:
            return

        max_extent = 0

        for vertex in simplex.vertices:
            max_extent = max(max_extent, abs(vertex.x))
            max_extent = max(max_extent, abs(vertex.y))

        max_extent = max_extent + 10

        scale = float(1) / float(max_extent)
        yaw = 0
        pitch = 0

        buildUniforms(program_id, scale, yaw, pitch)

        while (
            glfw.get_key(window, glfw.KEY_ESCAPE) != glfw.PRESS and
            not glfw.window_should_close(window)
        ):
            if glfw.get_key(window, glfw.KEY_MINUS):
                scale = scale * 0.98

            if glfw.get_key(window, glfw.KEY_EQUAL):
                scale = scale / 0.98

            if glfw.get_key(window, glfw.KEY_LEFT):
                yaw = yaw + 0.01

            if glfw.get_key(window, glfw.KEY_RIGHT):
                yaw = yaw - 0.01

            if glfw.get_key(window, glfw.KEY_UP):
                pitch = pitch - 0.01

            if glfw.get_key(window, glfw.KEY_DOWN):
                pitch = pitch + 0.01

            buildUniforms(program_id, scale, yaw, pitch)

            gl.glClear(gl.GL_COLOR_BUFFER_BIT)
            gl.glDrawElements(gl.GL_LINES, index_count, gl.GL_UNSIGNED_SHORT, None)

            glfw.swap_buffers(window)
            glfw.poll_events()

try:
    main()
except:
    print("An error happened")
    print(traceback.format_exc())