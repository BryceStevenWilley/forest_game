#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>

#define GLEW_STATIC // Depending on how you built/installed GLEW, you may want to change this
#include <GL/glew.h>

// #define GLFW_DLL // Depending on how you built/installed GLFW, you may want to change this
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "Player.hpp"
#include "SceneObjects/SceneObject.hpp"
#include "World.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unistd.h>

// TODO: finish adding 2d text on the screen.

using namespace ParamWorld;

GLFWwindow *window;

// FreeType Helper struct (https://learnopengl.com/#!In-Practice/Text-Rendering)
struct Character {
    GLuint TextureID; // ID handle of the glyph texture.
    glm::ivec2 Size; // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to the left/top of glyph
    GLuint Advance; // Offset to advance to the next glyph.
};

std::map<GLchar, Character> Characters;

GLuint VAO, VBO;

/**
 * Renders a line of text.
 */
void RenderText(GLuint shaderProgramID, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
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

        GLfloat vertices[6][4] = {
            {xpos,     ypos + h, 0.0, 0.0},
            {xpos,     ypos,     0.0, 1.0},
            {xpos + w, ypos,     1.0, 1.0},
            {xpos,     ypos + h, 0.0, 0.0},
            {xpos + w, ypos,     0.0, 1.0},
            {xpos + w, ypos + h, 1.0, 1.0}
        };
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6 ) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


int main(void) {
    int windowWidth = 1024;
    int windowHeight = 768;

    // Initialise GLFW
    if( !glfwInit() ) {
	    fprintf( stderr, "Failed to initialize GLFW\n" );
  	    getchar();
  	    return -1;
  	}

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE,GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For Mac stuff
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // new OpenGL

    // Initialize Free Type.
    
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeTypeLibrary" << std::endl;
    }
    FT_Face face;
    if (FT_New_Face(ft, "../fonts/arial.ttf", 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }
    // TODO: Resize based on window size.
    FT_Set_Pixel_Sizes(face, 0, 48);
    
  	// Open a window and create its OpenGL context
    window = glfwCreateWindow(windowWidth, windowHeight, "Forest" , NULL, NULL);
  	if(window == NULL) {
	    fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU,"
                " they are not 3.3 compatible.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}

    glfwMakeContextCurrent(window);
    glewExperimental=true;

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
	    fprintf(stderr, "Failed to initialize GLEW\n");
	    getchar();
	    glfwTerminate();
	    return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetCursorPos(window, windowWidth/2, windowHeight/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, windowWidth/2, windowHeight/2);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Disable byte-alignment restriction
    
    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D( // Specifies a 2D texture image.
            GL_TEXTURE_2D,  // Target
            0, // level
            GL_RED, // internal format
            face->glyph->bitmap.width, 
            face->glyph->bitmap.rows, 
            0, // border
            GL_RED, // format
            GL_UNSIGNED_BYTE, // type 
            face->glyph->bitmap.buffer // data
        );
        // St texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
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

    do{
        // Measure speed
        #ifdef COMPILE_WITH_PERFORMANCE_TOOLS
            double currentTime = glfwGetTime();
            nbFrames++;
            if (currentTime - lastTime >= 1.0 ){ // If last print was more than 1s ago
                // printf and reset timer
                printf("%f ms/frame\n", 1000.0/double(nbFrames));
                nbFrames = 0;
                lastTime += 1.0;
            }
        #endif

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programID);
        player.updateCameraFromInputs(window);
        w.updateExploredSquares(window, player.getPosition(), player.horizontalAngle);
        w.Render(player.getProjectionMatrix(), player.getPosition(), player.getDirection(), player.getUp());

        // Render Text.
        // TODO

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) ==  0);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();
    return 0;
}
