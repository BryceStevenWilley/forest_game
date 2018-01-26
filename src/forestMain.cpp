#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

#define GLEW_STATIC  // Depending on how you built/installed GLEW, you may want to change this
#include "headers.hpp"

// #define GLFW_DLL // Depending on how you built/installed GLFW, you may want to change this
#include <glm/gtc/matrix_transform.hpp>
#include "Player.hpp"
#include "SceneObjects/SceneObject.hpp"
#include "World.hpp"
#include "shader.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unistd.h>

// TODO: finish adding 2d text on the screen.

using namespace ParamWorld;

GLFWwindow *window;

// FreeType Helper struct (https://learnopengl.com/#!In-Practice/Text-Rendering)
struct Character {
    GLuint TextureID;    // ID handle of the glyph texture.
    glm::ivec2 Size;     // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to the left/top of glyph
    GLuint Advance;      // Offset to advance to the next glyph.
};

std::map<GLchar, Character> Characters;

GLuint VAO, VBO;
   
FT_Library ft;
FT_Face face;

/**
 * Renders a line of text.
 */
void RenderText(GLuint shaderProgramID, std::string text, GLfloat x, GLfloat y, GLfloat scale,
                glm::vec3 color)
{
    glUseProgram(shaderProgramID);
    glUniform3f(glGetUniformLocation(shaderProgramID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];
        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        GLfloat vertices[6][4] = {{xpos, ypos + h, 0.0, 0.0}, {xpos, ypos, 0.0, 1.0},
                                  {xpos + w, ypos, 1.0, 1.0}, {xpos, ypos + h, 0.0, 0.0},
                                  {xpos + w, ypos, 0.0, 1.0}, {xpos + w, ypos + h, 1.0, 1.0}};
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    //glBindVertexArray(0);
    //glBindTexture(GL_TEXTURE_2D, 0);
}

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE)
    {
        std::cout << "Released key " << key << std::endl;
    }
}

int init_resources()
{
    // Initialize Free Type.
    if (FT_Init_FreeType(&ft) != 0) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeTypeLibrary: "
                     "Remember to run from the same directory as the binary."
                  << std::endl;
    }
    if (FT_New_Face(ft, "../fonts/arial.ttf", 0, &face) != 0) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }
}

int init_glfw()
{
    // Initialise GLFW
    if (glfwInit() == 0) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }
  
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // For Mac stuff
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // new OpenGL
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    // Open a window and create its OpenGL context
    //window = glfwCreateWindow(windowWidth, windowHeight, "Forest", glfwGetPrimaryMonitor(), nullptr);
    int windowWidth = mode->width;
    int windowHeight = mode->height;
    window = glfwCreateWindow(windowWidth, windowHeight, "Forest", primary, nullptr);
    if (window == nullptr) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU,"
                " they are not 3.3 compatible.\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

    glfwSetKeyCallback(window, key_callback);
}


int main()
{
    init_glfw();

    glewExperimental = 1u;

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    init_resources();
    // TODO: Resize based on window size.
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Disable byte-alignment restriction

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(       // Specifies a 2D texture image.
            GL_TEXTURE_2D,  // Target
            0,              // level
            GL_RED,         // internal format
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            0,                          // border
            GL_RED,                     // format
            GL_UNSIGNED_BYTE,           // type
            face->glyph->bitmap.buffer  // data
            );
        // St texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = {texture,
                               glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                               static_cast<GLuint>(face->glyph->advance.x)};
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // REquired for Font fragment shader.
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint programID = LoadShaders("SimpleVertexShader.glsl", "SimpleFragmentShader.glsl");
    // Shaders for fonts.
    GLuint fontID = LoadShaders("FontVertexShader.glsl", "FontFragmentShader.glsl");

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    World w(299.0, MatrixID);

    Player player(glm::vec3(0, 1.7, 0));

#ifdef COMPILE_WITH_PERFORMANCE_TOOLS
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    printf("Showing performance and debug tools. Printing average ms per frame.\n");
    fflush(stdout);
#endif

    do {
// Measure speed
#ifdef COMPILE_WITH_PERFORMANCE_TOOLS
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0) {  // If last print was more than 1s ago
            // printf and reset timer
            printf("%f ms/frame\n", 1000.0 / double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }
#endif

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programID);
        glfwPollEvents();
        player.updateCameraFromInputs(window);
        w.updateExploredSquares(window, player.getPosition(), player.horizontalAngle);
        w.Render(player.getProjectionMatrix(), player.getPosition(), player.getDirection(),
                 player.getUp());

        // Render Text.
        // TODO: follow OpenGL_programming: Modern_OpenGL_Tutorial_Text_Rendering front to back when you have time.
        //RenderText(fontID, "Hi", 0.0, 0.0, 10.0, glm::vec3(1.0, 0.0, 0.0));

        // Swap buffers
        glfwSwapBuffers(window);

    }  // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    // Close OpenGL window and terminate GLFW
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}
