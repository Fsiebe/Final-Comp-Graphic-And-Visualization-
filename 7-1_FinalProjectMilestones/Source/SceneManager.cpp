///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load wood texture for the desk
	if (!CreateGLTexture("textures/dark_wood.jpg", "deskTexture"))
	{
		std::cout << "Failed to load desk wood texture!" << std::endl;
	}

	// Load texture for keyboard base
	if (!CreateGLTexture("textures/black_plastic.jpg", "keyboardBaseTexture"))
	{
		std::cout << "Failed to load keyboard base texture!" << std::endl;
	}

	// Load texture for keyboard keys
	if (!CreateGLTexture("textures/white_plastic.jpg", "keyCapTexture"))
	{
		std::cout << "Failed to load key cap texture!" << std::endl;
	}

	// Load mouse texture
	if (!CreateGLTexture("textures/key_surface.jpg", "mouseTexture"))
	{
		std::cout << "Failed to load mouse texture!" << std::endl;
	}

	// Load Halloween gadget texture (pumpkin pattern)
	if (!CreateGLTexture("textures/pumpkin.jpg", "pumpkinTexture"))
	{
		std::cout << "Failed to load pumpkin texture!" << std::endl;
	}

	// Bind the loaded textures to OpenGL texture slots
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Desk material - polished wood surface
	OBJECT_MATERIAL deskMaterial;
	deskMaterial.tag = "deskMaterial";
	deskMaterial.ambientStrength = 0.3f;
	deskMaterial.ambientColor = glm::vec3(0.5f, 0.35f, 0.2f);    // Dark wood color
	deskMaterial.diffuseColor = glm::vec3(0.6f, 0.45f, 0.3f);    // Medium wood color
	deskMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);    // Polished wood reflection
	deskMaterial.shininess = 32.0f;                              // Moderately polished
	m_objectMaterials.push_back(deskMaterial);

	// Keyboard base material - plastic with slight reflection
	OBJECT_MATERIAL keyboardMaterial;
	keyboardMaterial.tag = "keyboardMaterial";
	keyboardMaterial.ambientStrength = 0.2f;
	keyboardMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);   // Dark plastic
	keyboardMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);   // Black plastic
	keyboardMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Slight plastic shine
	keyboardMaterial.shininess = 16.0f;                            // Low shine
	m_objectMaterials.push_back(keyboardMaterial);

	// Key caps material - plastic with stronger reflection
	OBJECT_MATERIAL keyCapMaterial;
	keyCapMaterial.tag = "keyCapMaterial";
	keyCapMaterial.ambientStrength = 0.2f;
	keyCapMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f);    // Light color
	keyCapMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);    // White plastic
	keyCapMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);   // Glossy plastic shine
	keyCapMaterial.shininess = 64.0f;                             // Medium-high shine
	m_objectMaterials.push_back(keyCapMaterial);

	// Mouse material - smooth plastic with moderate reflection
	OBJECT_MATERIAL mouseMaterial;
	mouseMaterial.tag = "mouseMaterial";
	mouseMaterial.ambientStrength = 0.2f;
	mouseMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);     // Dark gray
	mouseMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);     // Medium gray
	mouseMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);    // Medium plastic shine
	mouseMaterial.shininess = 48.0f;                              // Medium shine
	m_objectMaterials.push_back(mouseMaterial);

	// Pumpkin material - seasonal decoration with unique properties
	OBJECT_MATERIAL pumpkinMaterial;
	pumpkinMaterial.tag = "pumpkinMaterial";
	pumpkinMaterial.ambientStrength = 0.3f;
	pumpkinMaterial.ambientColor = glm::vec3(0.6f, 0.3f, 0.0f);   // Orange-ish ambient
	pumpkinMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.0f);   // Pumpkin orange
	pumpkinMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Matte finish
	pumpkinMaterial.shininess = 8.0f;                             // Low shine (pumpkins aren't shiny)
	m_objectMaterials.push_back(pumpkinMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Primary light source - office/desk lamp style lighting
	// Positioned above the scene with warm white color
	m_pShaderManager->setBoolValue("lightSources[0].isEnabled", true);
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 10.0f, 2.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.3f, 0.3f, 0.3f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.95f, 0.9f)); // Warm white
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));

	// Secondary light source - accent lighting from the pumpkin (Halloween themed)
	// Positioned near the pumpkin with orange glow
	m_pShaderManager->setBoolValue("lightSources[1].isEnabled", true);
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(7.0f, 2.0f, 0.0f)); // Near pumpkin
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.1f, 0.05f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.8f, 0.4f, 0.0f)); // Orange glow
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.6f, 0.3f, 0.0f));

	// Disable unused light sources
	m_pShaderManager->setBoolValue("lightSources[2].isEnabled", false);
	m_pShaderManager->setBoolValue("lightSources[3].isEnabled", false);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Define materials for all objects in the scene
	DefineObjectMaterials();

	// Setup lighting for the scene
	SetupSceneLights();

	// Load textures for the 3D scene
	LoadSceneTextures();

	// Load required meshes for desk, keyboard, mouse, and Halloween gadget
	m_basicMeshes->LoadPlaneMesh();  // for desk surface
	m_basicMeshes->LoadBoxMesh();    // for keyboard and desk components
	m_basicMeshes->LoadSphereMesh(); // for mouse components
	m_basicMeshes->LoadCylinderMesh(); // for Halloween gadget base
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** DESK SURFACE ***/
	scaleXYZ = glm::vec3(20.0f, 0.5f, 10.0f);
	positionXYZ = glm::vec3(0.0f, -0.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("deskMaterial");       // Apply desk material for lighting properties
	SetShaderTexture("deskTexture");         // Apply wood texture to the desk
	SetTextureUVScale(4.0f, 2.0f);           // Adjust UV scale to avoid stretching
	m_basicMeshes->DrawPlaneMesh();

	/*** KEYBOARD ***/
	float keyboardXPosition = -5.0f;

	/*** KEYBOARD - Base ***/
	scaleXYZ = glm::vec3(7.0f, 0.2f, 3.0f);
	positionXYZ = glm::vec3(keyboardXPosition, 0.1f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("keyboardMaterial");    // Apply keyboard material for lighting
	SetShaderTexture("keyboardBaseTexture");  // Apply dark texture to keyboard base
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/*** KEYBOARD - Accent Trim ***/
	scaleXYZ = glm::vec3(7.2f, 0.05f, 3.2f);
	positionXYZ = glm::vec3(keyboardXPosition, 0.05f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("keyboardMaterial");    // Apply keyboard material
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);   // Silver trim without texture
	m_basicMeshes->DrawBoxMesh();

	// Define key dimensions and spacing
	float keyWidth = 0.45f;
	float keyHeight = 0.15f;
	float keyDepth = 0.45f;
	float keySpacingX = 0.55f;
	float keySpacingZ = 0.55f;

	// Calculate keyboard dimensions
	float keyboardWidth = 7.0f;
	float keyboardDepth = 3.0f;

	// Calculate starting position for first key (top left of the keyboard)
	float startX = keyboardXPosition - (keyboardWidth / 2) + (keyWidth / 2) + 0.3f;
	float startZ = -(keyboardDepth / 2) + (keyDepth / 2) + 0.3f;
	float keyY = 0.2f + (keyHeight / 2); // Position on top of keyboard base

	// Number of keys in each row
	int keysPerRow = 12;
	int rows = 4;

	// Set material and texture for keys
	SetShaderMaterial("keyCapMaterial");     // Apply key cap material for lighting
	SetShaderTexture("keyCapTexture");       // Apply texture to keys
	SetTextureUVScale(1.0f, 1.0f);

	// Draw all keys
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < keysPerRow; col++) {
			// Skip positions for spacebar
			if (row == 3 && col > 2 && col < 9) {
				continue;
			}

			scaleXYZ = glm::vec3(keyWidth, keyHeight, keyDepth);
			positionXYZ = glm::vec3(startX + (col * keySpacingX), keyY, startZ + (row * keySpacingZ));

			SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
			m_basicMeshes->DrawBoxMesh();
		}
	}

	// Add a larger spacebar with texture and material
	scaleXYZ = glm::vec3(keyWidth * 6, keyHeight, keyDepth);
	positionXYZ = glm::vec3(startX + (5.5f * keySpacingX), keyY, startZ + (3 * keySpacingZ));
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("keyCapMaterial");     // Apply same material as other keys
	SetTextureUVScale(6.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/*** MOUSE ***/
	float mouseXPosition = 1.0f;
	float mouseZPosition = 0.0f;

	// Mouse base (main body) with material and texture
	scaleXYZ = glm::vec3(1.8f, 0.6f, 2.5f);
	positionXYZ = glm::vec3(mouseXPosition, 0.3f, mouseZPosition);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("mouseMaterial");      // Apply mouse material for lighting
	SetShaderTexture("mouseTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Mouse top with material and texture
	scaleXYZ = glm::vec3(1.8f, 0.4f, 2.5f);
	positionXYZ = glm::vec3(mouseXPosition, 0.65f, mouseZPosition);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("mouseMaterial");      // Apply mouse material for lighting
	SetShaderTexture("mouseTexture");
	SetTextureUVScale(1.0f, 0.5f);
	m_basicMeshes->DrawSphereMesh();

	/*** HALLOWEEN GADGET ***/
	float pumpkinXPosition = 7.0f;
	float pumpkinZPosition = 0.0f;

	// Base cylinder with pumpkin texture and material
	scaleXYZ = glm::vec3(1.2f, 1.5f, 1.2f);
	positionXYZ = glm::vec3(pumpkinXPosition, 0.75f, pumpkinZPosition);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("pumpkinMaterial");    // Apply pumpkin material for lighting
	SetShaderTexture("pumpkinTexture");      // Use pumpkin texture for the base
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Top sphere (pumpkin head) with material and texture
	scaleXYZ = glm::vec3(1.3f, 1.3f, 1.3f);
	positionXYZ = glm::vec3(pumpkinXPosition, 2.0f, pumpkinZPosition);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("pumpkinMaterial");    // Apply pumpkin material for lighting
	SetShaderTexture("mouseTexture");        // Same texture as mouse
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
}