#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

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
        "OpenGL 3.3",
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

    // Cargar OpenGL mediante GLAD
    int version = gladLoadGL(glfwGetProcAddress);

    if (version == 0)
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

    // Bucle principal
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cerrar
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}