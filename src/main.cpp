 #include <iostream>
 #include <vector>
 #include "glad/glad.h"
 #include "GLFW/glfw3.h"
 #include "common_matrices.hpp"
 #include "cube.hpp"
 #include "mat4f.hpp"
 #include "shader_tools.hpp"
 #include "triangle.hpp"
 #include "vec3f.hpp"
 using namespace math;
 using namespace geometry;

 // GLOBAL Variables
 GLuint g_vaoID = 0;
 GLuint g_vertexBufferID = 0;
 GLuint g_programID = 0;
 math::Mat4f G_M = math::identity();
 int g_totalVertices = 0;
 int depth = 0;
 float zoom = 1.f;
 bool perspective = false;

 GLfloat rotationX = 0.0f;
 GLfloat rotationY = 0.0f;

 // function declaration
 using namespace std;

 // Color vetor
 const Vec3f colors[] =  {
         {1.0, 0.0, 1.0},            // purple
         {0.0, 1.0, 0.0},            // green
         {0.0, 0.0, 1.0},            // blue
         {1.0, 0.0, 0.0},            // red
         {1.0, 1.0, 0.0},            // yellow
         {0.929, 0.435, 0.027}};     // orange

         void setFrameBufferSize(GLFWwindow *window, int width, int height) {
            glViewport(0, 0, width, height);
        }

    void setScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        if(yoffset > 0)
            zoom += 0.05;
        else if(yoffset < 0)
            zoom -= 0.05;
    }
    void setKeyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        // Up = go up one level
        if (key == GLFW_KEY_UP && action == GLFW_PRESS)
            depth++;
        // Down = go down one level
        if(key == GLFW_KEY_DOWN && action == GLFW_PRESS)
            depth--;
        // P = switch to perspective
        if(key == GLFW_KEY_P && action == GLFW_PRESS)
            perspective = true;
        // O = switch to ortho
        if(key == GLFW_KEY_O && action == GLFW_PRESS)
            perspective = false;
    }

    // user defined alias
    GLuint createShaderProgram() {
        auto vertexShaderSource = //
                R"vs(

                #version 330 core
          layout (location = 0) in vec3 position;
          layout (location = 1) in vec3 color;

          struct Data
          {
            vec3 color;
          };

          out Data data;

          uniform mat4 MVP;

                void main()
                {
            gl_Position = MVP * vec4(position, 1.);

            data.color = color;
                }

                )vs";

        auto fragmentShaderSource = //
                R"fs(

                #version 330 core

          struct Data
          {
            vec3 color;
          };

          in Data data;

          out vec4 fragmentColor;
                void main()
                {
            fragmentColor = vec4(data.color, 1.f);
                }

                )fs";

        GLuint vsID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vsID, 1, &vertexShaderSource, NULL);
        glCompileShader(vsID);

        openGL::checkShaderCompileStatus(vsID);

        GLuint fsID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fsID, 1, &fragmentShaderSource, NULL);
        glCompileShader(fsID);

        openGL::checkShaderCompileStatus(fsID);

        GLuint programID = glCreateProgram();
        glAttachShader(programID, vsID);
        glAttachShader(programID, fsID);
        glLinkProgram(programID);

        openGL::checkProgramLinkStatus(programID);

        glDetachShader(programID, vsID);
        glDetachShader(programID, fsID);

        glDeleteShader(vsID);
        glDeleteShader(fsID);

        return programID;
    }

    bool generateOpenGL_IDs() {
        g_programID = createShaderProgram();

        glGenVertexArrays(1, &g_vaoID);
        glGenBuffers(1, &g_vertexBufferID);

        return true;
    }

    void deleteOpenGL_IDs() {
        glDeleteVertexArrays(1, &g_vaoID);
        glDeleteBuffers(1, &g_vertexBufferID);
        glDeleteProgram(g_programID);

        g_vaoID = 0;
        g_vertexBufferID = 0;
        g_programID = 0;
    }

    namespace openGL {

    struct Vertex {
        Vertex() = default;
        Vertex(Vec3f position, Vec3f color) : position(position), color(color) {}

        Vec3f position;
        Vec3f color;
    };

    } // namespace openGL

    bool loadTriangleToGPU(std::vector<Triangle> const &tris) {

        using namespace openGL;
        std::vector<Vertex> vertices;
        vertices.reserve(tris.size() * 3);

        for (int i = 0; i < tris.size(); ++i) {
            auto const &t = tris[i];
            int c = (i/2)%6;
            Vertex vert;

            // vertex a position, red color
            vert.position = t.a();
            vert.color = colors[c];
            vertices.push_back(vert);

            // vertex b position, green color
            vert.position = t.b();
            vert.color = colors[c];
            vertices.push_back(vert);

            // vertex c position, blue color
            vert.position = t.c();
            vert.color = colors[c];
            vertices.push_back(vert);
        }

        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferID);
        glBufferData(
                GL_ARRAY_BUFFER,                  // destination
                sizeof(Vertex) * vertices.size(), // size (bytes) of memcopy to GPU
                vertices.data(),                  // pointer to data (contiguous)
                GL_STATIC_DRAW                    // usage patter of the GPU buffer
        );

        g_totalVertices = vertices.size();

        return true;
    }
    void setupVAO() {
        using namespace openGL;

        glBindVertexArray(g_vaoID);

        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferID);

        // set up position input into vertex shader
        glEnableVertexAttribArray(0);          // match layout # in shader
        glVertexAttribPointer(                 //
                0,                                 // attribute layout # (in shader)
                3,                                 // number of coordinates per vertex
                GL_FLOAT,                          // type
                GL_FALSE,                          // normalized?
                sizeof(Vertex),                    // stride
                (void *)offsetof(Vertex, position) // array buffer offset
        );

        // set up color input into vertex shader
        glEnableVertexAttribArray(1);         // match layout # in shader
        glVertexAttribPointer(                //
                1,                                // attribute layout # (in shader)
                3,                                // number of coordinates per vertex
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                sizeof(Vertex),                   // stride
                (void *)(offsetof(Vertex, color)) // array buffer offset
        );

        glBindVertexArray(0);
    }
    void doMenge(CubeStruct cube, vector<CubeStruct> &Cubes)
    {
        //make a new vector with 20 boxes
        CubeStruct cubes[20];

        float newSize = (cube.edgeSize /3);
        float doubleSize = newSize * 2;

        // Set up each cube individually based on their index

        //     -------------------
        //     |  5  |  6  |  7  |
        //     |------------------
        //     |  3  |     |  4  |
        //     |------------------
        //     |  0  |  1  |  2  |
        //     -------------------   and so on for the other 2 sides

        for(int i = 0; i < 20; i++) {
            cubes[i].edgeSize = newSize;

            if (i == 0 || i == 12 || i == 3 || i == 5 || i == 8 || i == 15 || i == 17 || i == 10) {
                cubes[i].x = cube.x - doubleSize;
            }
            if (i == 1 || i == 6 || i == 13 || i == 18) {
                cubes[i].x = cube.x;
            }
            if (i == 2 || i == 4 || i == 7 || i == 9 || i == 11 || i == 14 || i == 16 || i == 19) {
                cubes[i].x = cube.x + doubleSize;
            }
            if (((0 <= i) && (i <= 2)) || i == 8 || i == 9 || ((12 <= i) && (i <= 14))) {
                cubes[i].y = cube.y - doubleSize;
            }
            if (i == 3 || i == 4 || i == 15 || i == 16) {
                cubes[i].y = cube.y;
            }
            if (((5 <= i) && (i <= 7)) || i == 10 || i == 11 || ((17 <= i) && (i <= 19))) {
                cubes[i].y = cube.y + doubleSize;
            }
            if ((0 <= i) && (i <= 7)) {
                cubes[i].z = cube.z - doubleSize;
            }
            if ((8 <= i) && (i <= 11)) {
                cubes[i].z = cube.z;
            }
            if ((12 <= i) && (i <= 19)) {
                cubes[i].z = cube.z + doubleSize;
            }
            Cubes.push_back(cubes[i]);
        }
    }

    void pushTriangles(CubeStruct cube, vector<Triangle> &Tri)
    {
        // Take in a cube structure and convert it into a triangle
        // Push the triangle onto the Triangle vector
        Triangle subTri;

        // Convert the cube structure into a cube
        Cube temp = convert(cube);

        subTri.a() = temp.a();
        subTri.b() = temp.c();
        subTri.c() = temp.d();
        Tri.push_back(subTri);

        subTri.a() = temp.a();
        subTri.b() = temp.d();
        subTri.c() = temp.b();
        Tri.push_back(subTri);

        subTri.a() = temp.b();
        subTri.b() = temp.d();
        subTri.c() = temp.f();
        Tri.push_back(subTri);

        subTri.a() = temp.b();
        subTri.b() = temp.f();
        subTri.c() = temp.e();
        Tri.push_back(subTri);

        subTri.a() = temp.g();
        subTri.b() = temp.h();
        subTri.c() = temp.f();
        Tri.push_back(subTri);

        subTri.a() = temp.g();
        subTri.b() = temp.f();
        subTri.c() = temp.e();
        Tri.push_back(subTri);

        subTri.a() = temp.a();
        subTri.b() = temp.c();
        subTri.c() = temp.h();
        Tri.push_back(subTri);

        subTri.a() = temp.a();
        subTri.b() = temp.h();
        subTri.c() = temp.g();
        Tri.push_back(subTri);

        subTri.a() = temp.h();
        subTri.b() = temp.c();
        subTri.c() = temp.d();
        Tri.push_back(subTri);

        subTri.a() = temp.h();
        subTri.b() = temp.d();
        subTri.c() = temp.f();
        Tri.push_back(subTri);

        subTri.a() = temp.g();
        subTri.b() = temp.a();
        subTri.c() = temp.b();
        Tri.push_back(subTri);

        subTri.a() = temp.g();
        subTri.b() = temp.b();
        subTri.c() = temp.e();
        Tri.push_back(subTri);
    }

    int main() {
        GLFWwindow *window = nullptr;

        if (!glfwInit()) {
            exit(EXIT_FAILURE);
        }

        // print version
        int glfwMajor, glfwMinor, glfwRevision;
        glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRevision);
        cout << "GLFW Version: " << glfwMajor << "." << glfwMinor << "."
                 << glfwRevision << endl;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(1000,1000,"Asg1 Part 2",NULL,NULL);
        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        glfwSwapInterval(1);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);

        // setup callbacks
        glfwSetFramebufferSizeCallback(window, setFrameBufferSize);
        glfwSetKeyCallback(window, setKeyboard);
        glfwSetScrollCallback(window,setScroll);
        // setup
        generateOpenGL_IDs();
        setupVAO();

        double xpos,
               ypos;
        bool pressed = false,
             skipped = false;
        int Xpos = -1,
            Ypos = -1,
            width,
            height;
        CubeStruct x;

        std::vector<CubeStruct> Cubes;
        std::vector<CubeStruct> temp;
        std::vector<Triangle> Tri;

        while (!glfwWindowShouldClose(window)) {

            // Clear all the vectors
            Tri.clear();
            temp.clear();
            Cubes.clear();

            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            GLint mvpID = glGetUniformLocation(g_programID, "MVP");
            glUniformMatrix4fv(mvpID, 1, true, G_M.data());
            glUseProgram(g_programID);   // binds the shader program for use
            glBindVertexArray(g_vaoID);  // binds vertex buffer
            glDrawArrays(GL_TRIANGLES,   // type of drawing (rendered to back buffer)
                                     0,              // offset into buffer
                                     g_totalVertices); // number of vertices in buffer
            // Set up the initial cube
            x.x = 0.f;
            x.y = 0.f;
            x.z = 0.f;
            x.edgeSize = 1.f;

            glfwGetWindowSize(window, &width, &height);

            // Convert the initial cube structure to cube and then push it onto the vector
            Cube t = convert(x);
            Cubes.push_back(x);

            // Repeat the mengerSponge process
            for (int i = 0; i < depth; i++) {
                temp = Cubes;
                Cubes.clear();
                for(auto tmp: temp) {
                    // Split the cube into 20 sub-cubes
                    doMenge(tmp,Cubes);
                }
            }
            for(auto tmp2:Cubes)
                // Convert each sub-cubes into triangles and push them onto Tri
                pushTriangles(tmp2,Tri);

            // Load the triangle vector and draw it
            loadTriangleToGPU(Tri);

            // Switching between perspective/ortho
            if(perspective)
                G_M = perspectiveProjection(45, (float) width/ (float) height ,0.1,100) *
                        lookAtMatrix(Vec3f{0.f,0.f,zoom/2 - 4}, Vec3f{0.f,0.f,0.f}, Vec3f{0.f,1.f,0.f}) *
                        rotateAboutXMatrix(rotationX) *
                        rotateAboutYMatrix(rotationY) *
                        uniformScaleMatrix(min(width,height)/1400.f);
            else
                G_M = orthographicProjection((-1) * ((float) width/(float) height),(float) width/(float) height, 1,-1,-1,1) *
                        rotateAboutXMatrix(rotationX) *
                        rotateAboutYMatrix(rotationY) *
                        uniformScaleMatrix((min(width,height)/2000.f) + (zoom - 1.f));

            // Get cursor position (used for zooming)
            glfwGetCursorPos(window, &xpos, &ypos);

            // Set up zoom
            if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)) {
                if (!pressed) {
                    skipped = false;
                    pressed = true;
                }
                if (skipped) {
                    rotationX += (ypos - Ypos) / 5;
                    rotationY += (xpos - Xpos) / 5;
            }
                Ypos = ypos;
                Xpos = xpos;
                skipped = true;
            }
            else
                pressed = false;

            glfwSwapBuffers(window); // swaps back buffer to front for drawing to screen
            glfwPollEvents();        // will process event queue and carry on
        }

        deleteOpenGL_IDs();
        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_SUCCESS;
    }
