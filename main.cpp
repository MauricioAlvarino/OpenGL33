#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

// ========================================
// VERTEX SHADER
// ========================================

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

// ========================================
// FRAGMENT SHADER
// ========================================

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(0.2, 0.6, 1.0, 1.0);
}
)";

// ========================================
// MAIN
// ========================================

int main()
{
    // Inicializar GLFW
    if (!glfwInit())
    {
        std::cerr << "Error al inicializar GLFW\n";
        return -1;
    }

    // Solicitar OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(
        800,
        600,
        "Mi primer triangulo - OpenGL 3.3",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Error al crear la ventana\n";
        glfwTerminate();
        return -1;
    }

    // Hacer el contexto actual
    glfwMakeContextCurrent(window);

    // Inicializar GLAD
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Error al inicializar GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Mostrar versión de OpenGL
    std::cout << "OpenGL: "
              << glGetString(GL_VERSION)
              << std::endl;

    // ========================================
    // CREAR VERTEX SHADER
    // ========================================

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vertexShader,
        1,
        &vertexShaderSource,
        nullptr
    );

    glCompileShader(vertexShader);

    // ========================================
    // CREAR FRAGMENT SHADER
    // ========================================

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fragmentShader,
        1,
        &fragmentShaderSource,
        nullptr
    //=========================================
    // CREAR SHADER PROGRAM
    // ========================================

    );

    glCompileShader(fragmentShader);

    // =================================
    unsigned int shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    // Ya no necesitamos los shaders individuales
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // ========================================
    // VÉRTICES DEL TRIÁNGULO
    // ========================================

    float vertices[] =
    {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    // ========================================
    // VBO
    // ========================================

    unsigned int VBO;

    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // ========================================
    // VAO
    // ========================================

    unsigned int VAO;

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Configurar los atributos de los vértices
    glVertexAttribPointer(
        0, //
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    // ========================================
    // LOOP PRINCIPAL
    // ========================================

   while (!glfwWindowShouldClose(window))
{
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

    // ========================================
    // LIBERAR RECURSOS
    // ========================================

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}