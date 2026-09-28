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

// Ahora el fragment shader recibe el color como uniform,
// para poder pintar la grilla de un color y las celdas de otro
// usando el MISMO shader.
const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;
uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";

// ---------- Configuracion global de la grilla ----------
const int DIVISIONES = 30;

// ---------- Estado del dibujo con el mouse ----------
bool arrastrando = false;
int filaInicio = 0, colInicio = 0;
std::vector<std::pair<int, int>> celdasLinea; // celdas (fila, col) resultado de Bresenham

// ---------- IDs de OpenGL que necesitamos actualizar desde el callback ----------
unsigned int VBOCeldas = 0;
unsigned int VAOCeldas = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Convierte una posicion de mouse en pixeles a una celda (fila, col) de la grilla
void pixelACelda(GLFWwindow* window, double xpos, double ypos, int& fila, int& col)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Paso 1: convertir de pixeles a coordenadas normalizadas (NDC)
    // xpos va de 0 (izquierda) a width (derecha)   -> ndcX de -1 a 1
    // ypos va de 0 (arriba)    a height (abajo)     -> ndcY de 1 a -1 (se invierte)
    float ndcX = (float)(2.0 * xpos / width - 1.0);
    float ndcY = (float)(1.0 - 2.0 * ypos / height);

    // Paso 2: convertir de NDC a indice de celda
    float paso = 2.0f / DIVISIONES;

    col = (int)((ndcX + 1.0f) / paso);
    fila = (int)((ndcY + 1.0f) / paso);

    // Paso 3: asegurar que la celda quede dentro del rango valido
    if (col < 0) col = 0;
    if (col >= DIVISIONES) col = DIVISIONES - 1;
    if (fila < 0) fila = 0;
    if (fila >= DIVISIONES) fila = DIVISIONES - 1;
}

// Algoritmo de Bresenham clasico, aplicado sobre indices de celda (columna, fila)
// en vez de pixeles de imagen. La logica es identica.
std::vector<std::pair<int, int>> bresenham(int col0, int fila0, int col1, int fila1)
{
    std::vector<std::pair<int, int>> celdas;

    int dx = std::abs(col1 - col0);
    int dy = std::abs(fila1 - fila0);

    // sx y sy indican la DIRECCION en la que se mueve la linea:
    // +1 si la columna/fila debe aumentar, -1 si debe disminuir
    int sx = (col0 < col1) ? 1 : -1;
    int sy = (fila0 < fila1) ? 1 : -1;

    int err = dx - dy;

    int col = col0;
    int fila = fila0;

    while (true)
    {
        celdas.push_back({ fila, col });

        if (col == col1 && fila == fila1)
            break;

        int e2 = 2 * err;

        // Si el error acumulado lo indica, avanzamos en columna (eje X)
        if (e2 > -dy)
        {
            err -= dy;
            col += sx;
        }

        // Si el error acumulado lo indica, avanzamos en fila (eje Y)
        if (e2 < dx)
        {
            err += dx;
            fila += sy;
        }
    }

    return celdas;
}

// Genera los vertices (triangulos) de un conjunto de celdas, para dibujarlas rellenas
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

        // Triangulo 1
        vertices.push_back(xIzq); vertices.push_back(yArriba); vertices.push_back(0.0f);
        vertices.push_back(xIzq); vertices.push_back(yAbajo);  vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yAbajo);  vertices.push_back(0.0f);

        // Triangulo 2
        vertices.push_back(xIzq); vertices.push_back(yArriba); vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yAbajo);  vertices.push_back(0.0f);
        vertices.push_back(xDer); vertices.push_back(yArriba); vertices.push_back(0.0f);
    }

    return vertices;
}

// Sube al VBO de celdas los vertices actuales de "celdasLinea"
void actualizarBufferCeldas()
{
    std::vector<float> vertices = CeldasATriangulos(celdasLinea);

    glBindBuffer(GL_ARRAY_BUFFER, VBOCeldas);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.empty() ? nullptr : vertices.data(),
        GL_DYNAMIC_DRAW
    );
}

// ---------- Callbacks de mouse ----------

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (action == GLFW_PRESS)
    {
        arrastrando = true;
        pixelACelda(window, xpos, ypos, filaInicio, colInicio);

        celdasLinea = { { filaInicio, colInicio } };
        actualizarBufferCeldas();
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

    int filaFin, colFin;
    pixelACelda(window, xpos, ypos, filaFin, colFin);

    celdasLinea = bresenham(colInicio, filaInicio, colFin, filaFin);
    actualizarBufferCeldas();
}

// Genera las lineas de la grilla de fondo (igual que antes)
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
        "Bresenham sobre grilla - OpenGL 3.3",
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

    // Ubicacion del uniform "uColor" dentro del shader, para poder cambiarlo en runtime
    int uColorLocation = glGetUniformLocation(shaderProgram, "uColor");

    // ---------- Buffers de la grilla (estatica, no cambia) ----------
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

    // ---------- Buffers de las celdas dibujadas (dinamica, cambia con el mouse) ----------
    glGenBuffers(1, &VBOCeldas);
    glGenVertexArrays(1, &VAOCeldas);

    glBindVertexArray(VAOCeldas);
    glBindBuffer(GL_ARRAY_BUFFER, VBOCeldas);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // --- Dibujar la grilla en negro ---
        glUniform4f(uColorLocation, 0.0f, 0.0f, 0.0f, 1.0f);
        glBindVertexArray(VAOGrilla);
        glDrawArrays(GL_LINES, 0, numVerticesGrilla);

        // --- Dibujar las celdas de la linea en azul ---
        if (!celdasLinea.empty())
        {
            glUniform4f(uColorLocation, 0.2f, 0.6f, 1.0f, 1.0f);
            glBindVertexArray(VAOCeldas);
            glDrawArrays(GL_TRIANGLES, 0, (int)celdasLinea.size() * 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAOGrilla);
    glDeleteBuffers(1, &VBOGrilla);
    glDeleteVertexArrays(1, &VAOCeldas);
    glDeleteBuffers(1, &VBOCeldas);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
//g++ -std=c++17 -I.\include cuadrado.cpp .\src\gl.c -o Cuadrado.exe -lglfw3 -lopengl32 -lgdi32
//.\Cuadrado.exe