#include "utilities.h"
#include "Camera.h"
#include "SimpleModel.h"
#include "Texture.h"

// global variables
// settings
unsigned int gWindowWidth = 800;
unsigned int gWindowHeight = 600;
const float gCamMoveSensitivity = 1.0f;
const float gCamRotateSensitivity = 0.1f;

// frame stats
float gFrameRate = 60.0f;
float gFrameTime = 1 / gFrameRate;

// scene content
GLuint gVBO = 0;		// vertex buffer object identifier
GLuint gVAO = 0;		// vertex array object identifier

//GLuint gVBO[2];
//GLuint gVAO[2];

std::map<std::string, ShaderProgram> gShaders; // holds multiple shaders
std::map<std::string, Texture> gTextures; // holds multiple textures
std::map <std::string, SimpleModel> gModels; // holds multiple models

Camera gCamera;					// camera object
std::map<std::string, glm::mat4> gModelMatrix;	// object matrix

Light gLight;					// light properties
std::map<std::string, Material>  gMaterial;		// material properties
//SimpleModel gModel;				// object model

// controls
bool gWireframe = false;	// wireframe control
float gAlpha = 0.5f;		// reflective amount

// function initialise scene and render settings
static void init(GLFWwindow* window)
{
	// set the color the color buffer should be cleared to
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glEnable(GL_DEPTH_TEST);	// enable depth buffer test

	// compile and link a vertex and fragment shader pair
	gShaders["Reflection"].compileAndLink("lighting.vert", "reflection.frag");
	gShaders["NormalMap"].compileAndLink("normalMap.vert", "normalMap.frag");

	// initialise view matrix
	gCamera.setViewMatrix(glm::vec3(0.0f, 2.0f, 4.0f),
		glm::vec3(0.0f, 0.0f, 0.0f));

	// initialise projection matrix
	gCamera.setProjMatrix(glm::perspective(glm::radians(45.0f),
		static_cast<float>(gWindowWidth) / gWindowHeight, 0.1f, 10.0f));

	// initialise point light properties
	gLight.pos = glm::vec3(0.0f, 2.0f, 1.0f);
	gLight.La = glm::vec3(0.3f);
	gLight.Ld = glm::vec3(1.0f);
	gLight.Ls = glm::vec3(1.0f);
	gLight.att = glm::vec3(1.0f, 0.0f, 0.0f);

	// initialise material properties
	gMaterial["Floor"].Ka = glm::vec3(0.2f);
	gMaterial["Floor"].Kd = glm::vec3(0.2f, 0.7f, 1.0f);
	gMaterial["Floor"].Ks = glm::vec3(0.2f, 0.7f, 1.0f);
	gMaterial["Floor"].shininess = 40.0f;

	gMaterial["Cube"].Ka = glm::vec3(0.2f);
	gMaterial["Cube"].Kd = glm::vec3(1.0f, 0.7f, 0.2f);
	gMaterial["Cube"].Ks = glm::vec3(1.0f, 0.7f, 0.2f);
	gMaterial["Cube"].shininess = 10.0f;

	// initialise model matrices
	gModelMatrix["Floor"] = glm::mat4(1.0f) * glm::scale(glm::vec3(1.5f, 1.5f, 1.5f));
	gModelMatrix["Cube"] = glm::translate(glm::vec3(-1.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));

	// load model
	//gModel.loadModel("./models/cube.obj");
	gModels["Cube"].loadModel("./models/cube.obj");
	gModels["Torus"].loadModel("./models/torus.obj");

	// vertex positions and normals
	std::vector<GLfloat> vertices =
	{
		-1.5f, 0.0f, 1.5f,	// vertex 0: position
		0.0f, 1.0f, 0.0f,	// vertex 0: normal
		1.5f, 0.0f, 1.5f,	// vertex 1: position
		0.0f, 1.0f, 0.0f,	// vertex 1: normal
		-1.5f, 0.0f, -1.5f,	// vertex 2: position
		0.0f, 1.0f, 0.0f,	// vertex 2: normal
		1.5f, 0.0f, -1.5f,	// vertex 3: position
		0.0f, 1.0f, 0.0f,	// vertex 3: normal
	};

	// create VBO
	glGenBuffers(1, &gVBO);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO);			// generate unused VAO identifier
	glBindVertexArray(gVAO);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormal),
		reinterpret_cast<void*>(offsetof(VertexNormal, position)));		// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormal),
		reinterpret_cast<void*>(offsetof(VertexNormal, normal)));		// specify format of colour data

	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
}

// function used to update the scene
static void update_scene(GLFWwindow* window)
{
	// stores camera forward/back, up/down and left/right movements
	float moveForward = 0.0f;
	float moveRight = 0.0f;

	// update movement variables based on keyboard input
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		moveForward += gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		moveForward -= gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		moveRight -= gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		moveRight += gCamMoveSensitivity * gFrameTime;

	// update camera position and direction
	gCamera.update(moveForward, moveRight);
}

void draw_floor(float alpha)
{
	ShaderProgram *gShader = &gShaders["Reflection"];

	// use the shaders associated with the shader program
	gShader->use();

	// set light properties
	gShader->setUniform("uLight.pos", gLight.pos);
	gShader->setUniform("uLight.La", gLight.La);
	gShader->setUniform("uLight.Ld", gLight.Ld);
	gShader->setUniform("uLight.Ls", gLight.Ls);
	gShader->setUniform("uLight.att", gLight.att);

	// set viewing position
	gShader->setUniform("uViewpoint", gCamera.getPosition());

	// set material properties
	gShader->setUniform("uMaterial.Ka", gMaterial["Floor"].Ka);
	gShader->setUniform("uMaterial.Kd", gMaterial["Floor"].Kd);
	gShader->setUniform("uMaterial.Ks", gMaterial["Floor"].Ks);
	gShader->setUniform("uMaterial.shininess", gMaterial["Floor"].shininess);

	// calculate matrices
	glm::mat4 MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * gModelMatrix["Floor"];
	glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(gModelMatrix["Floor"])));

	// set uniform variables
	gShader->setUniform("uModelViewProjectionMatrix", MVP);
	gShader->setUniform("uModelMatrix", gModelMatrix["Floor"]);
	gShader->setUniform("uNormalMatrix", normalMatrix);

	// set blending amount
	gShader->setUniform("uAlpha", alpha);

	glBindVertexArray(gVAO);				// make VAO active
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	// render the vertices
}

void draw_objects(bool reflection)
{
	glm::mat4 MVP = glm::mat4(1.0f);
	glm::mat3 normalMatrix = glm::mat3(1.0f);
	glm::mat4 reflectMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::vec3 lightPosition = gLight.pos;

	if (reflection)
	{
		// create reflection matrix about the horizontal plane
		reflectMatrix = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));
		// reposition the point light when rendering the reflection
		lightPosition = glm::vec3(reflectMatrix * glm::vec4(lightPosition, 1.0f));
	}

	ShaderProgram* gShader = &gShaders["Reflection"];

	// use the shaders associated with the shader program
	gShader->use();

	// set light properties
	gShader->setUniform("uLight.pos", lightPosition);
	gShader->setUniform("uLight.La", gLight.La);
	gShader->setUniform("uLight.Ld", gLight.Ld);
	gShader->setUniform("uLight.Ls", gLight.Ls);
	gShader->setUniform("uLight.att", gLight.att);

	// set viewing position
	gShader->setUniform("uViewpoint", gCamera.getPosition());

	// set material properties
	gShader->setUniform("uMaterial.Ka", gMaterial["Cube"].Ka);
	gShader->setUniform("uMaterial.Kd", gMaterial["Cube"].Kd);
	gShader->setUniform("uMaterial.Ks", gMaterial["Cube"].Ks);
	gShader->setUniform("uMaterial.shininess", gMaterial["Cube"].shininess);

	// calculate matrices
	modelMatrix = reflectMatrix * gModelMatrix["Cube"];
	MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * modelMatrix;
	normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

	// set uniform variables
	gShader->setUniform("uModelViewProjectionMatrix", MVP);
	gShader->setUniform("uModelMatrix", modelMatrix);
	gShader->setUniform("uNormalMatrix", normalMatrix);

	gShader->setUniform("uAlpha", 1.0f);

	// draw model
	gModels["Cube"].drawModel();
}

// function to render the scene
static void render_scene()
{
	/************************************************************************************
	 * Clear colour buffer, depth buffer and stencil buffer
	 ************************************************************************************/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	/************************************************************************************
	 * Disable colour buffer and depth buffer, and draw reflective surface into stencil buffer
	 ************************************************************************************/
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // disable any modification to all colour components
	glDepthMask(GL_FALSE);                                // disable any modification to depth value
	glEnable(GL_STENCIL_TEST);                            // enable stencil testing

	// setup the stencil buffer with a reference value
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	// draw the reflective surface into the stencil buffer
	draw_floor(1.0f);

	/************************************************************************************
	 * Enable colour buffer and depth buffer, draw reflected geometry where stencil test passes
	 ************************************************************************************/
	// only render where stencil buffer equals to 1
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);   // allow all colour components to be modified 
	glDepthMask(GL_TRUE);                              // allow depth value to be modified

	// draw the reflected objects
	draw_objects(true);

	glDisable(GL_STENCIL_TEST);		// disable stencil testing

	/************************************************************************************
	 * Draw the scene
	 ************************************************************************************/
	// draw reflective surface by blending with reflection
	glEnable(GL_BLEND);		//enable blending            
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	// blend reflective surface with reflection
	draw_floor(gAlpha);

	glDisable(GL_BLEND);	//disable blending

	// draw the normal scene
	draw_objects(false);

	// flush the graphics pipeline
	glFlush();
}

// key press or release callback function
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// close the window when the ESCAPE key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
}

// mouse movement callback function
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// pass cursor position to tweak bar
	TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));

	// previous cursor coordinates
	static glm::vec2 previousPos = glm::vec2(xpos, ypos);
	static int counter = 0;

	// allow camera rotation when right mouse button held
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		// stablise cursor coordinates for 5 updates
		if (counter < 5)
		{
			// set previous cursor coordinates
			previousPos = glm::vec2(xpos, ypos);
			counter++;
		}

		// change based on previous cursor coordinates
		float deltaYaw = (previousPos.x - xpos) * gCamRotateSensitivity * gFrameTime;
		float deltaPitch = (previousPos.y - ypos) * gCamRotateSensitivity * gFrameTime;

		// update camera's yaw and pitch
		gCamera.updateRotation(deltaYaw, deltaPitch);

		// set previous cursor coordinates
		previousPos = glm::vec2(xpos, ypos);
	}
	else
	{
		counter = 0;
	}
}

// mouse button callback function
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// pass mouse button status to tweak bar
	TwEventMouseButtonGLFW(button, action);
}

// error callback function
static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;	// output error description
}

// create and populate tweak bar elements
TwBar* create_UI(const std::string name)
{
	// create a tweak bar
	TwBar* twBar = TwNewBar(name.c_str());

	// give tweak bar the size of graphics window
	TwWindowSize(gWindowWidth, gWindowHeight);
	TwDefine(" TW_HELP visible=false ");	// disable help menu
	TwDefine(" GLOBAL fontsize=3 ");		// set large font size

	TwDefine(" Main label='User Interface' refresh=0.02 text=light size='250 250' position='10 10' ");

	// create frame stat entries
	TwAddVarRO(twBar, "Frame Rate", TW_TYPE_FLOAT, &gFrameRate, " group='Frame Stats' precision=2 ");
	TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT, &gFrameTime, " group='Frame Stats' ");

	// scene controls
	TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe, " group='Controls' ");

	// light control
	TwAddVarRW(twBar, "Position X", TW_TYPE_FLOAT, &gLight.pos.x, " group='Light' min=-3 max=3 step=0.01 ");
	TwAddVarRW(twBar, "Position Y", TW_TYPE_FLOAT, &gLight.pos.y, " group='Light' min=-3 max=3 step=0.01 ");
	TwAddVarRW(twBar, "Position Z", TW_TYPE_FLOAT, &gLight.pos.z, " group='Light' min=-3 max=3 step=0.01 ");

	// reflective amount
	TwAddVarRW(twBar, "Blend", TW_TYPE_FLOAT, &gAlpha, " group='Reflection' min=0.2 max=1 step=0.01 ");

	return twBar;
}

int main(void)
{
	GLFWwindow* window = nullptr;	// GLFW window handle

	glfwSetErrorCallback(error_callback);	// set GLFW error callback function

	// initialise GLFW
	if (!glfwInit())
	{
		// if failed to initialise GLFW
		exit(EXIT_FAILURE);
	}

	// minimum OpenGL version 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create a window and its OpenGL context
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Lab", nullptr, nullptr);

	// check if window created successfully
	if (window == nullptr)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);	// set window context as the current context
	glfwSwapInterval(1);			// swap buffer interval

	// initialise GLEW
	if (glewInit() != GLEW_OK)
	{
		// if failed to initialise GLEW
		std::cerr << "GLEW initialisation failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// set GLFW callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// initialise scene and render settings
	init(window);

	// initialise AntTweakBar
	TwInit(TW_OPENGL_CORE, nullptr);
	TwBar* tweakBar = create_UI("Main");		// create and populate tweak bar elements

	// timing data
	double lastUpdateTime = glfwGetTime();	// last update time
	double elapsedTime = lastUpdateTime;	// time since last update
	int frameCount = 0;						// number of frames since last update

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window);	// update the scene

		// if wireframe set polygon render mode to wireframe
		if (gWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		render_scene();			// render the scene

		// set polygon render mode to fill
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		TwDraw();				// draw tweak bar

		glfwSwapBuffers(window);	// swap buffers
		glfwPollEvents();			// poll for events

		frameCount++;
		elapsedTime = glfwGetTime() - lastUpdateTime;	// time since last update

		// if elapsed time since last update > 1 second
		if (elapsedTime > 1.0)
		{
			gFrameTime = elapsedTime / frameCount;	// average time per frame
			gFrameRate = 1 / gFrameTime;			// frames per second
			lastUpdateTime = glfwGetTime();			// set last update time to current time
			frameCount = 0;							// reset frame counter
		}
	}

	// clean up
	glDeleteBuffers(1, &gVBO);
	glDeleteVertexArrays(1, &gVAO);

	// uninitialise tweak bar
	TwDeleteBar(tweakBar);
	TwTerminate();

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}