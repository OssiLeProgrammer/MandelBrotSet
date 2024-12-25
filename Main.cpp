#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <thread>

// Vertex Shader
const char* vertText = "#version 330 core\n"
"layout (location = 0) in vec3 aPOS;\n"
"layout (location = 1) in vec2 aTEX;\n"
"out vec2 TEX;\n"
"void main() {\n"
"   TEX = aTEX;\n"
"   gl_Position = vec4(aPOS, 1.0);\n"
"}";

// Fragment Shader
const char* FragTEXT = "#version 330 core\n"
"in vec2 TEX;\n"
"out vec4 FragCOLOR;\n"
"uniform sampler2D sampler;\n"
"void main() {\n"
"   FragCOLOR = texture(sampler, TEX);\n"
"}\n";

void makeWhite(unsigned char* buffer, int size) {
    for (int i = 0; i < size; i++) {
        buffer[i] = 255;
    }
}

void computeCSET(unsigned char* buffer, std::vector<std::thread>& threads, const int threadCOUNT, int width, int height);
void computeMandelbrot(int startY, int endY, int width, int height, unsigned char* pixels);

double minReal = -2.0, maxReal = 2.0;
double minImag = -2.0, maxImag = 2.0;

void process(GLFWwindow* window, double& minReal, double& maxReal, double& minImag, double& maxImag);
void scrollCallback(GLFWwindow* window, double xOff, double yOff);

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Mandelbrot Set", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetScrollCallback(window, scrollCallback);
    // Set up the pixel buffer for Mandelbrot
    int width = 800, height = 600;
    unsigned char* buffer = new unsigned char[width * height * 3];
    std::vector<std::thread> threads;
    const int threadCount = 15;
    computeCSET(buffer, threads, threadCount, width, height); // Compute Mandelbrot Set

    // Vertex data for a quad (two triangles)
    GLfloat data[] = {
        // Vertex Positions         // Texture Coordinates
        -1.0f, -1.0f, 0.0f,          0.0f, 0.0f,   // Bottom Left
         1.0f, -1.0f, 0.0f,          1.0f, 0.0f,   // Bottom Right
        -1.0f,  1.0f, 0.0f,          0.0f, 1.0f,   // Top Left

         1.0f, -1.0f, 0.0f,          1.0f, 0.0f,   // Bottom Right
         1.0f,  1.0f, 0.0f,          1.0f, 1.0f,   // Top Right
        -1.0f,  1.0f, 0.0f,          0.0f, 1.0f    // Top Left
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Compile shaders and link program
    GLuint shaderProg = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertText, nullptr);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FragTEXT, nullptr);
    glCompileShader(fragmentShader);
    glAttachShader(shaderProg, vertexShader);
    glAttachShader(shaderProg, fragmentShader);
    glLinkProgram(shaderProg);
    glUseProgram(shaderProg);

    // Texture setup
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUniform1i(glGetUniformLocation(shaderProg, "sampler"), 0); // Set sampler to texture unit 0

    // Main loop
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set clear color to black
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
        glBindTexture(GL_TEXTURE_2D, texture); // Bind the texture
        process(window, minReal, maxReal, minImag, maxImag);
        computeCSET(buffer, threads, threadCount, width, height); // Compute Mandelbrot Set
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);

        glUseProgram(shaderProg);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // Draw the quad

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete[] buffer;
    glfwDestroyWindow(window);
    glfwTerminate();
}

void computeCSET(unsigned char* buffer, std::vector<std::thread>& threads, const int threadCOUNT, int width, int height) {
    int chunkSize = height / threadCOUNT;
    for (int i = 0; i < threadCOUNT; ++i) {
        int startY = i * chunkSize;
        int endY = (i == threadCOUNT - 1) ? height : startY + chunkSize;
        threads.emplace_back(computeMandelbrot, startY, endY, width, height, buffer);
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    threads.clear();
}

void computeMandelbrot(int startY, int endY, int width, int height, unsigned char* pixels) {
    for (int y = startY; y < endY; ++y) {
        for (int x = 0; x < width; ++x) {
            double cx = minReal + (maxReal - minReal) * x / width;
            double cy = minImag + (maxImag - minImag) * y / height;

            double zx = 0.0, zy = 0.0;
            int iteration = 0, maxIteration = 1000;
            while (zx * zx + zy * zy < 4 && iteration < maxIteration) {
                double temp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = temp;
                iteration++;
            }

            // Set pixel color based on the number of iterations
            int offset = (y * width + x) * 3;
            pixels[offset] = 255 * iteration / maxIteration;        // Red
            pixels[offset + 1] = 255 * iteration / maxIteration;    // Green
            pixels[offset + 2] = 255 * iteration / maxIteration;    // Blue
        }
    }
}


void process(GLFWwindow* window, double& minReal, double& maxReal, double& minImag, double& maxImag)
{
    float rangeY = (maxImag - minImag) / 2;
    float rangeX = (maxReal - minReal) / 2;
    if (glfwGetKey(window, GLFW_KEY_UP)) {
        minImag += rangeY * 0.1f;
        maxImag += rangeY * 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN)) {
        minImag -= rangeY * 0.1f;
        maxImag -= rangeY * 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT)) {
        minReal -= rangeX * 0.1f;
        maxReal -= rangeX * 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
        minReal += rangeX * 0.1f;
        maxReal += rangeX * 0.1f;
    }
}

void scrollCallback(GLFWwindow* window, double xOff, double yOff) {
    double zoomFactor = yOff > 0 ? 0.9 : 1.1; // Zoom in or out based on scroll direction
    double aspectRatio = 800.0 / 600.0;       // Aspect ratio (width / height)

    double realRange = maxReal - minReal;
    double imagRange = maxImag - minImag;

    double realCenter = (minReal + maxReal) / 2.0;
    double imagCenter = (minImag + maxImag) / 2.0;

    // Adjust ranges while keeping the center fixed
    realRange *= zoomFactor;
    imagRange = realRange / aspectRatio;

    minReal = realCenter - realRange / 2.0;
    maxReal = realCenter + realRange / 2.0;
    minImag = imagCenter - imagRange / 2.0;
    maxImag = imagCenter + imagRange / 2.0;
}