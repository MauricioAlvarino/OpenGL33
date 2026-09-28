#include <iostream>
#include <vector>
#include <cmath>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;
uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";

const int DIVISIONES = 30;

bool arrastrando = false;
int filaCentro = 0, colCentro = 0;
std::vector<std::pair<int, int>> celdasFigura;

unsigned int VBOFigura = 0;
unsigned int VAOFigura = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void pixelACelda(GLFWwindow* window, double xpos, double ypos, int& fila, int& col)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float ndcX = (float)(2.0 * xpos / width - 1.0);
    float ndcY = (float)(1.0 - 2.0 * ypos / height);

    float paso = 2.0f / DIVISIONES;

    col = (int)((ndcX + 1.0f) / paso);
    fila = (int)((ndcY + 1.0f) / paso);

    if (col < 0) col = 0;
    if (col >= DIVISIONES) col = DIVISIONES - 1;
    if (fila < 0) fila = 0;
    if (fila >= DIVISIONES) fila = DIVISIONES - 1;
}

// Algoritmo del punto medio para circunferencias (Midpoint Circle Algorithm),
// aplicado sobre indices de celda en vez de pixeles. Formulas identicas a la diapositiva.
std::vector<std::pair<int, int>> bresenhamCirculo(int filaC, int colC, int radio)
{
    std::vector<std::pair<int, int>> celdas;

    int x = 0;
    int y = radio;
    int p = 1 - radio;

    auto agregarOctantes = [&](int x, int y)
    {
        celdas.push_back({ filaC + y, colC + x });
        celdas.push_back({ filaC + x, colC + y });
        celdas.push_back({ filaC + x, colC - y });
        celdas.push_back({ filaC + y, colC - x });
        celdas.push_back({ filaC - y, colC - x });
        celdas.push_back({ filaC - x, colC - y });
        celdas.push_back({ filaC - x, colC + y });
        celdas.push_back({ filaC - y, colC + x });
    };

    agregarOctantes(x, y);

    while (x < y)
    {
        x = x + 1;

        if (p < 0)
        {
            p = p + 2 * x + 1;
        }
        else
        {
            y = y - 1;
            p = p + 2 * x - 2 * y + 1;
        }

        agregarOctantes(x, y);
    }

    return celdas;
}

// Descarta celdas que caigan fuera de la grilla (el circulo puede salirse del rango)
std::vector<std::pair<int, int>> filtrarCeldasValidas(const std::vector<std::pair<int, int>>& celdas)
{
    std::vector<std::pair<int, int>> validas;

    for (const auto& c : celdas)
    {
        if (c.first >= 0 && c.first < DIVISIONES && c.second >= 0 && c.second < DIVISIONES)
        {
            validas.push_back(c);
        }
    }

    return validas;
}

std::vector<float> CeldasATriangulos(const std::vector<std::pair<int, int>>& celdas)
{
    std::vector<float> vertices;
    float tam = 2.0f / DIVISIONES;

    for (const auto& c : celdas)
    {
        int fila = c.first;
        int col = c.second;

        float xIzq = -1.0f + col * tam;
        float xDer = xIzq + tam;
        float yAbajo = -1.0f + fila * tam;
        float yArriba = yAbajo + tam;

        vertices.push_back(xIzq); vertices.push_back(yArriba); vertices.push_back(0.0f);
        vertices.push_back(xIzq); vertices.push_back(yAbajo);  vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yAbajo);  vertices.push_back(0.0f);

        vertices.push_back(xIzq); vertices.push_back(yArriba); vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yAbajo);  vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yArriba); vertices.push_back(0.0f);
    }

    return vertices;
}

void actualizarBufferFigura()
{
    std::vector<float> vertices = CeldasATriangulos(celdasFigura);

    glBindBuffer(GL_ARRAY_BUFFER, VBOFigura);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.empty() ? nullptr : vertices.data(),
        GL_DYNAMIC_DRAW
    );
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (action == GLFW_PRESS)
    {
        arrastrando = true;
        pixelACelda(window, xpos, ypos, filaCentro, colCentro);

        celdasFigura = { { filaCentro, colCentro } };
        actualizarBufferFigura();
    }
    else if (action == GLFW_RELEASE)
    {
        arrastrando = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!arrastrando)
        return;

    int filaActual, colActual;
    pixelACelda(window, xpos, ypos, filaActual, colActual);

    int dFila = filaActual - filaCentro;
    int dCol = colActual - colCentro;
    int radio = (int)std::round(std::sqrt((double)(dFila * dFila + dCol * dCol)));

    if (radio <= 0)
    {
        celdasFigura = { { filaCentro, colCentro } };
    }
    else
    {
        celdasFigura = filtrarCeldasValidas(bresenhamCirculo(filaCentro, colCentro, radio));
    }

    actualizarBufferFigura();
}

std::vector<float> CreateGridSquaresForViewport(int divisiones)
{
    std::vector<float> vertices;
    float paso = 2.0f / divisiones;

    for (int i = 0; i <= divisiones; i++)
    {
        float x = -1.0f + i * paso;
        vertices.push_back(x); vertices.push_back(1.0f);  vertices.push_back(0.0f);
        vertices.push_back(x); vertices.push_back(-1.0f); vertices.push_back(0.0f);
    }

    for (int i = 0; i <= divisiones; i++)
    {
        float y = -1.0f + i * paso;
        vertices.push_back(-1.0f); vertices.push_back(y); vertices.push_back(0.0f);
        vertices.push_back(1.0f);  vertices.push_back(y); vertices.push_back(0.0f);
    }

    return vertices;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Error al inicializar GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        800, 600,
        "Bresenham Circulo sobre grilla - OpenGL 3.3",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Error al crear la ventana\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Error al inicializar GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int uColorLocation = glGetUniformLocation(shaderProgram, "uColor");

    std::vector<float> verticesGrilla = CreateGridSquaresForViewport(DIVISIONES);

    unsigned int VBOGrilla, VAOGrilla;
    glGenBuffers(1, &VBOGrilla);
    glGenVertexArrays(1, &VAOGrilla);

    glBindVertexArray(VAOGrilla);
    glBindBuffer(GL_ARRAY_BUFFER, VBOGrilla);
    glBufferData(GL_ARRAY_BUFFER, verticesGrilla.size() * sizeof(float), verticesGrilla.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int numVerticesGrilla = verticesGrilla.size() / 3;

    glGenBuffers(1, &VBOFigura);
    glGenVertexArrays(1, &VAOFigura);

    glBindVertexArray(VAOFigura);
    glBindBuffer(GL_ARRAY_BUFFER, VBOFigura);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniform4f(uColorLocation, 0.0f, 0.0f, 0.0f, 1.0f);
        glBindVertexArray(VAOGrilla);
        glDrawArrays(GL_LINES, 0, numVerticesGrilla);

        if (!celdasFigura.empty())
        {
            glUniform4f(uColorLocation, 1.0f, 0.2f, 0.2f, 1.0f);
            glBindVertexArray(VAOFigura);
            glDrawArrays(GL_TRIANGLES, 0, (int)celdasFigura.size() * 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAOGrilla);
    glDeleteBuffers(1, &VBOGrilla);
    glDeleteVertexArrays(1, &VAOFigura);
    glDeleteBuffers(1, &VBOFigura);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}