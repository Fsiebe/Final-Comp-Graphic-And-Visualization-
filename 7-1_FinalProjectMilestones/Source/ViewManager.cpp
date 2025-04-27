///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// Movement speed factor adjusted by mouse scroll
	float g_MovementSpeedMultiplier = 1.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/*******
 *  ViewManager()
 *
 *  The constructor for the class
 *******/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 10.0f;
}

/*******
 *  ~ViewManager()
 *
 *  The destructor for the class
 *******/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/*******
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 *******/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Register the scroll callback for movement speed adjustment
	glfwSetScrollCallback(window, &ViewManager::MouseScrollCallback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/*******
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 *******/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// Reduce mouse sensitivity for smoother camera control
	float sensitivity = 0.05f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/*******
 *  MouseScrollCallback()
 *
 *  This method is called from GLFW when the mouse scroll wheel
 *  is moved within the active GLFW display window.
 *******/
void ViewManager::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	g_MovementSpeedMultiplier += static_cast<float>(yOffset) * 0.1f;

	// Ensure speed multiplier stays within reasonable bounds
	if (g_MovementSpeedMultiplier < 0.1f)
		g_MovementSpeedMultiplier = 0.1f;
	if (g_MovementSpeedMultiplier > 3.0f)
		g_MovementSpeedMultiplier = 3.0f;

	// Update camera movement speed with the new multiplier
	g_pCamera->MovementSpeed = 10.0f * g_MovementSpeedMultiplier;

	std::cout << "Camera speed: " << g_pCamera->MovementSpeed << std::endl;
}

/*******
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 *******/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Toggle between perspective and orthographic projection 
	// with the P and O keys
	static bool pKeyPressed = false;
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS && !pKeyPressed)
	{
		bOrthographicProjection = false;
		pKeyPressed = true;
		std::cout << "Switched to Perspective Projection" << std::endl;
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE)
	{
		pKeyPressed = false;
	}

	static bool oKeyPressed = false;
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS && !oKeyPressed)
	{
		bOrthographicProjection = true;
		oKeyPressed = true;
		std::cout << "Switched to Orthographic Projection" << std::endl;
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE)
	{
		oKeyPressed = false;
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// Process up and down camera movement with Q and E keys
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		// Move camera up
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		// Move camera down
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}
}

/*******
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 *******/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// Define the projection matrix based on current projection mode
	if (bOrthographicProjection)
	{
		// Orthographic projection for 2D view
		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		float orthoSize = 10.0f; // Controls the size of the orthographic view

		projection = glm::ortho(
			-orthoSize * aspectRatio, orthoSize * aspectRatio,
			-orthoSize, orthoSize,
			0.1f, 100.0f);

		// For orthographic view, we'll use a fixed camera position
		// looking down at the scene from above
		view = glm::lookAt(
			glm::vec3(0.0f, 15.0f, 0.1f),  // Position high above the scene
			glm::vec3(0.0f, 0.0f, 0.0f),   // Look at the origin/center
			glm::vec3(0.0f, 0.0f, -1.0f)   // Up vector (inverted Z as up for top-down view)
		);
	}
	else
	{
		// Use the normal camera for perspective projection
		view = g_pCamera->GetViewMatrix();

		// Perspective projection for 3D view
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the projection matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position for lighting calculations
		if (bOrthographicProjection) {
			// Use fixed position for orthographic view
			m_pShaderManager->setVec3Value("viewPosition", glm::vec3(0.0f, 15.0f, 0.1f));
		}
		else {
			// Use camera position for perspective view
			m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
		}
	}
}