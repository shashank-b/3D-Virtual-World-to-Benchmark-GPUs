// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "shader.hpp"
#include "obj.hpp"
#include "texture.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#define STEP_SIZE 1
#define GRASS_STEP 2
#define GRASS_DIR 0.1f

struct Object {
    GLuint vertexbuffer;
    GLuint texbuffer;
	std::vector<GLfloat> g_vertex_buffer_data;
	std::vector<GLfloat> g_vertex_tex_data;
	GLuint TextureID;
	GLuint texture;
	SDL_Surface *img;
	GLuint programID;

	Object(std::string objfile, std::string texfile, GLuint programid, glm::vec3 position, float scale = 1) {
		programID = programid;

		auto lobj = LoadOBJToTriangles(objfile.c_str(), position, scale);
		g_vertex_buffer_data = lobj.first;
		g_vertex_tex_data = lobj.second;

        glGenBuffers(1, &vertexbuffer);
	    glGenBuffers(1, &texbuffer);
	    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	    glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer_data.size() * sizeof(g_vertex_buffer_data[0]), g_vertex_buffer_data.data(), GL_DYNAMIC_DRAW);
	    glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
	    glBufferData(GL_ARRAY_BUFFER, g_vertex_tex_data.size() * sizeof(g_vertex_tex_data[0]), g_vertex_tex_data.data(), GL_DYNAMIC_DRAW);

		img = IMG_Load(texfile.c_str());
		if (img == NULL) {
			std::cerr << "FAILED TO LOAD TEXTURE" << std::endl;
			exit(1);
		}

		// Bind our texture in Texture Unit 0
		glGenTextures(1, &TextureID);
		glBindTexture(GL_TEXTURE_2D, TextureID);

		int Mode = GL_RGB;
	
		if(img->format->BytesPerPixel == 4) {
			Mode = GL_RGBA;
		}
		
		glTexImage2D(GL_TEXTURE_2D, 0, Mode, img->w, img->h, 0, Mode, GL_UNSIGNED_BYTE, img->pixels);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

	void draw() {
		GLuint texLoc = glGetUniformLocation(programID, "myTextureSampler");	

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(texLoc, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, g_vertex_buffer_data.size() / 3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	~Object() {
		glDeleteBuffers(1, &vertexbuffer);
		glDeleteBuffers(1, &texbuffer);
	}
};

int main( void )
{
	srand (static_cast <unsigned> (time(0)));
	// Initialise GLFW
	if( !glfwInit() )
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Graphic Benchmark", NULL, NULL);
	if( window == NULL ){
		std::cerr << "Failed to open window; are you sure you support OpenGL 3?" << std::endl;
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GL extensions" << std::endl;
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// BLack background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 90 radian field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 1000.0f);
	// Camera matrix
	glm::mat4 View       = glm::lookAt(
								glm::vec3(2,4,-10), // Camera location
								glm::vec3(0,0,0), // camera looks at
								glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
						   );
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model      = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around

	std::vector<Object*> objects = {
		new Object("objs/ant.obj", "objs/ant.jpg", programID, glm::vec3(-50,0,0)),
		new Object("objs/car.obj", "objs/car.jpg", programID, glm::vec3(-300,0,0), 0.07f),
		new Object("objs/grass.obj", "objs/grass.jpg", programID, glm::vec3(-50,0,0), 0.1f),
		new Object("objs/cabin.obj", "objs/cabin.jpg", programID, glm::vec3(-200,0,-3)),
	};

	float current_pos = 0;

        float c_x = 10, c_z = 0, r_y = 0, r_z = 0;

	int count = 0;

	float grasscx = -50;
	float grasscz = 0;

	clock_t time_a = clock();

	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		current_pos += 0.03f;


		// Camera position
		if (glfwGetKey(window, GLFW_KEY_UP ) == GLFW_PRESS) {
			c_x -= STEP_SIZE * std::cos(r_z);
			c_z -= STEP_SIZE * std::sin(r_z);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN ) == GLFW_PRESS) {
			c_x += STEP_SIZE * std::cos(r_z);
			c_z += STEP_SIZE * std::sin(r_z);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS) {
			c_z += STEP_SIZE * std::cos(r_z);
			c_x -= STEP_SIZE * std::sin(r_z);
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT ) == GLFW_PRESS) {
			c_z -= STEP_SIZE * std::cos(r_z);
			c_x += STEP_SIZE * std::sin(r_z);
		}
		if (glfwGetKey(window, GLFW_KEY_C ) == GLFW_PRESS) {
			r_z -= 0.01;
		}
		if (glfwGetKey(window, GLFW_KEY_X ) == GLFW_PRESS) {
			r_z += 0.01;
		}

		View  = glm::lookAt(
		            glm::vec3(c_x, 0, c_z),
			        glm::vec3(c_x - std::cos(r_z), 0, c_z - std::sin(r_z)),
			        glm::vec3(0,1,0)
		        );

		MVP   = Projection * View * Model;


		// Reset sine value
		if (current_pos >= 20 * M_PI) {
			current_pos = 0;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Draw objects
		for (auto obj : objects) {
			obj->draw();
		}

		if (count++ % GRASS_STEP == 0) {
			grasscx -= static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5 + GRASS_DIR;
			grasscz -= static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5 + GRASS_DIR;
			auto obj = new Object("objs/grass.obj", "objs/grass.jpg", programID, glm::vec3(grasscx,0,grasscz), 0.1f);
			objects.push_back(obj);

			int cc = 0;
			for (auto obj : objects) {
				cc += obj->g_vertex_buffer_data.size() / 3;
			}

			clock_t time_b = clock();
			unsigned int total_time_ticks = (unsigned int)(time_b - time_a);
			std::cout << "Triangle count: " << cc << " FPS: " << float(GRASS_STEP) / (total_time_ticks / 1000000.0f) << std::endl;
			time_a = time_b;
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Delete objects
	for (auto obj : objects) {
		delete obj;
	}

	// Cleanup VBO and shader
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

