
#include <GL/glew.h>

// STB_IMAGE for loading images of many filetypes
#include <stb_image.h>
#include<iostream>
#include <cstdlib>

#include <labhelper.h>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////

// The window we'll be rendering to
SDL_Window* g_window = nullptr;

int mag = 1;
int mini = 5;
float anisotropy = 16.0f;
float camera_pan = 0.f;
bool showUI = false;


///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////

// The shaderProgram holds the vertexShader and fragmentShader
GLuint shaderProgram;


///////////////////////////////////////////////////////////////////////////////
// Scene objects
///////////////////////////////////////////////////////////////////////////////

// The vertexArrayObject here will hold the pointers to
// the vertex data (in positionBuffer) and color data per vertex (in colorBuffer)
GLuint positionBuffer, colorBuffer, indexBuffer, texcoordBuffer, vertexArrayObject;
GLuint texture;
GLuint vertBuffer_exp, indexBuffer_exp, texcoordBuffer_exp, vertexArrayObject_exp;
GLuint texture_exp;


///////////////////////////////////////////////////////////////////////////////
/// This function is called once at the start of the program and never again
///////////////////////////////////////////////////////////////////////////////
void initialize()
{
	ENSURE_INITIALIZE_ONLY_ONCE();

	///////////////////////////////////////////////////////////////////////////
	// Create the vertex array object
	///////////////////////////////////////////////////////////////////////////
	// Create a handle for the vertex array object
	glGenVertexArrays(1, &vertexArrayObject);
	// Set it as current, i.e., related calls will affect this object
	glBindVertexArray(vertexArrayObject);
	///////////////////////////////////////////////////////////////////////////
	// Create the positions buffer object
	///////////////////////////////////////////////////////////////////////////
	const float positions[] = {
		// X      Y       Z
		-10.0f, 0.0f, -10.0f,  // v0
		-10.0f, 0.0f, -330.0f, // v1
		10.0f,  0.0f, -330.0f, // v2
		10.0f,  0.0f, -10.0f   // v3
	};
	// Create a handle for the vertex position buffer
	glGenBuffers(1, &positionBuffer);
	// Set the newly created buffer as the current one
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	// Send the vetex position data to the current buffer
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(positions) * sizeof(float), positions,
		GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	// Enable the attribute
	glEnableVertexAttribArray(0);

	///////////////////////////////////////////////////////////////////////////
	// Task 1 : Create the texture coordinates.
	//			Create the texture coordinates' buffer object.
	//			Set up the attrib pointer.
	//			Enable the vertex attrib array.
	///////////////////////////////////////////////////////////////////////////
	float texcoords[] = {
		0.0f,0.0f,//(u,v) for v0
		0.0f,15.0f,//(u,v) for v1
		1.0f,15.0f,//(u.v) for v2
		1.0f,0.0f//(u.v) for v3
	};
	glGenBuffers(1, &texcoordBuffer);//Create a handle for the texcoord buffer
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);//Set the newly created buffer as the current one
	//Send the texcoord data to the current buffer
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(texcoords) * sizeof(float), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);
	//Enable the new vertex attribute array
	glEnableVertexAttribArray(1);
	///////////////////////////////////////////////////////////////////////////
	// Create the element array buffer object
	///////////////////////////////////////////////////////////////////////////
	//const int indices[] = {
	//	0, 1, 3, // Triangle 1
	//	1, 2, 3  // Triangle 2
	//};
	const int indices[] = {
	3,1,0,
	3,2,1
	};
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, labhelper::array_length(indices) * sizeof(float), indices,
		GL_STATIC_DRAW);

	// Create the explosion quad
///////////////////////////////////////////////////////////////////////////
	const float positions_exp[] = {
		2.0f,  0.0f,  -50.0f, // v0
		2.0f,  10.0f, -50.0f, // v1
		12.0f, 10.0f, -50.0f, // v2
		12.0f, 0.0f,  -50.0f  // v3
	};
	float texcoords_exp[] = {
		0.0f, 0.0f, // (u,v) for v0
		0.0f, 1.0f, // (u,v) for v1
		1.0f, 1.0f, // (u,v) for v2
		1.0f, 0.0f  // (u,v) for v3
	};
	//const int indices_exp[] = {
	//	0, 1, 3, // Triangle 1
	//	1, 2, 3  // Triangle 2
	//};
	//invert the render order
	const int indices_exp[] = {
		3,1,0,
		3,2,1
	};
	// Create the buffer objects
	glGenBuffers(1, &vertBuffer_exp);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer_exp);
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(positions_exp) * sizeof(float), positions_exp, GL_STATIC_DRAW);

	glGenBuffers(1, &texcoordBuffer_exp);
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer_exp);
	glBufferData(GL_ARRAY_BUFFER, labhelper::array_length(texcoords_exp) * sizeof(float), texcoords_exp,
		GL_STATIC_DRAW);

	glGenBuffers(1, &indexBuffer_exp);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_exp);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, labhelper::array_length(indices_exp) * sizeof(float), indices_exp,
		GL_STATIC_DRAW);

	glGenVertexArrays(1, &vertexArrayObject_exp);
	glBindVertexArray(vertexArrayObject_exp);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer_exp);
	glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer_exp);
	glVertexAttribPointer(1, 2, GL_FLOAT, false /*normalized*/, 0 /*stride*/, 0 /*offset*/);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_exp);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	// The loadShaderProgram and linkShaderProgam functions are defined in glutil.cpp and
	// do exactly what we did in lab1 but are hidden for convenience
	shaderProgram = labhelper::loadShaderProgram("../lab2-textures/simple.vert",
		"../lab2-textures/simple.frag");

	//**********************************************

	//************************************
	//			Load Texture
	//************************************
	// Task 2
	int w, h, comp;
	unsigned char* image = stbi_load("../scenes/textures/asphalt.jpg", &w, &h, &comp, STBI_rgb_alpha);
	//Generate OpenGL texture
	glGenTextures(1, &texture);
	//Bind this texture and allocate storage
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	//clamp the texure coordinate to this range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//set up texture filtering
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	glBindTexture(GL_TEXTURE_2D, 0);//Unbind the texture,avoid being modified
	stbi_image_free(image);
	//Task 7:

	//Bind and Render Texture


	unsigned char* image2 = stbi_load("../scenes/textures/explosion.png", &w, &h, &comp, STBI_rgb_alpha);
	glGenTextures(1, &texture_exp);
	glBindTexture(GL_TEXTURE_2D, texture_exp);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2);
	stbi_image_free(image2);
	//Indicates that the active texture should be repeated,
	//instead of for instance clamped, for texture coordinates > 1 or <-1.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
	// Sets the type of mipmap interpolation to be used on magnifying and
	// minifying the active texture. These are the nicest available options.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}


///////////////////////////////////////////////////////////////////////////////
/// This function will be called once per frame, so the code to set up
/// the scene for rendering should go here
///////////////////////////////////////////////////////////////////////////////
void display(void)
{
	// The viewport determines how many pixels we are rasterizing to
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);
	// Set viewport
	glViewport(0, 0, w, h);

	// Set clear color
	glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
	// Clears the color buffer and the z-buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// We disable backface culling for this tutorial, otherwise care must be taken with the winding order
	// of the vertices. It is however a lot faster to enable culling when drawing large scenes.
	//glDisable(GL_CULL_FACE); //
	glEnable(GL_CULL_FACE);
	// Disable depth testing
	glDisable(GL_DEPTH_TEST);

	// Set the shader program to use for this draw call
	glUseProgram(shaderProgram);

	// Set up a projection matrix
	float fovy = radians(45.0f);
	float aspectRatio = float(w) / float(h);
	float nearPlane = 0.01f;
	float farPlane = 400.0f;
	mat4 projectionMatrix = perspective(fovy, aspectRatio, nearPlane, farPlane);
	// Send it to the vertex shader
	int loc = glGetUniformLocation(shaderProgram, "projectionMatrix");
	glUniformMatrix4fv(loc, 1, false, &projectionMatrix[0].x);

	loc = glGetUniformLocation(shaderProgram, "cameraPosition");
	glUniform3f(loc, camera_pan, 10, 0);


	// Task 3.1
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(vertexArrayObject);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	//BindTexture
	//set alpha channel
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_exp);
	glBindVertexArray(vertexArrayObject_exp);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glDisable(GL_BLEND);

	glUseProgram(0); // "unsets" the current shader program. Not really necessary.
}


///////////////////////////////////////////////////////////////////////////////
/// This function is to hold the general GUI logic
///////////////////////////////////////////////////////////////////////////////
void gui()
{
	// ----------------- Set variables --------------------------
	ImGui::PushID("mag");
	ImGui::Text("Magnification");
	ImGui::RadioButton("GL_NEAREST", &mag, 0);
	ImGui::RadioButton("GL_LINEAR", &mag, 1);
	ImGui::PopID();

	ImGui::PushID("mini");
	ImGui::Text("Minification");
	ImGui::RadioButton("GL_NEAREST", &mini, 0);
	ImGui::RadioButton("GL_LINEAR", &mini, 1);
	ImGui::RadioButton("GL_NEAREST_MIPMAP_NEAREST", &mini, 2);
	ImGui::RadioButton("GL_NEAREST_MIPMAP_LINEAR", &mini, 3);
	ImGui::RadioButton("GL_LINEAR_MIPMAP_NEAREST", &mini, 4);
	ImGui::RadioButton("GL_LINEAR_MIPMAP_LINEAR", &mini, 5);
	ImGui::PopID();

	ImGui::SliderFloat("Anisotropic filtering", &anisotropy, 1.0, 16.0, "Number of samples: %.0f");
	ImGui::Dummy({ 0, 20 });
	ImGui::SliderFloat("Camera Panning", &camera_pan, -1.0, 1.0);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	switch (mag) {
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
	switch (mini) {
	case 0:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		break;
	case 3:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		break;
	case 4:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 5:
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	}
	//Anisotropic filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

}

int main(int argc, char* argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 2");

	initialize();

	// render-loop
	bool stopRendering = false;
	while (!stopRendering)
	{
		// Inform imgui of new frame
		ImGui_ImplSdlGL3_NewFrame(g_window);

		// check events (keyboard among other)
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			// Allow ImGui to capture events.
			ImGui_ImplSdlGL3_ProcessEvent(&event);

			if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
			{
				stopRendering = true;
			}
			else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
			{
				showUI = !showUI;
			}
			else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_PRINTSCREEN)
			{
				labhelper::saveScreenshot();
			}
		}

		// render to window
		display();

		// Render overlay GUI.
		if (showUI)
		{
			gui();
		}

		// Render the GUI.
		ImGui::Render();

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);
	}

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
