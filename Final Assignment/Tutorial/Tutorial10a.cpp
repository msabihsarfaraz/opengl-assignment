#include <cstdio>		// for C++ i/o
#include <iostream>
#include <string>
#include <cstddef>
using namespace std;	// to avoid having to use std::

#include <GLEW/glew.h>	// include GLEW
#include <GLFW/glfw3.h>	// include GLFW (which includes the OpenGL header)
#include <glm/glm.hpp>	// include GLM (ideally should only use the GLM headers that are actually used)
#include <glm/gtx/transform.hpp>
using namespace glm;	// to avoid having to use glm::

#include <AntTweakBar.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "bmpfuncs.h"
#include "Camera.h"

#define MOVEMENT_SENSITIVITY 3.0f		// camera movement sensitivity
#define ROTATION_SENSITIVITY 0.3f		// camera rotation sensitivity

// struct for vertex attributes
typedef struct Vertex
{
	GLfloat position[3];
	GLfloat normal[3];
	GLfloat tangent[3];
	GLfloat texCoord[2];
} Vertex;

typedef struct Vertex2
{
	GLfloat position[3];
	GLfloat normal[3];
} Vertex2;

// light and material structs
typedef struct Light
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	int type;
} Light;

// struct for mesh properties
typedef struct Mesh
{
	Vertex* pMeshVertices;		// pointer to mesh vertices
	GLint numberOfVertices;		// number of vertices in the mesh
	GLint* pMeshIndices;		// pointer to mesh indices
	GLint numberOfFaces;		// number of faces in the mesh
} Mesh;

typedef struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
} Material;

// Global variables
Vertex g_vertices[] = {
	// Front: triangle 1
	// vertex 1
	-1.0f, 1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	0.0f, 1.0f,			// texture coordinate
	// vertex 2
	-1.0f, -1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	0.0f, 0.0f,			// texture coordinate
	// vertex 3
	1.0f, 1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	1.0f, 1.0f,			// texture coordinate

	// triangle 2
	// vertex 1
	1.0f, 1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	1.0f, 1.0f,			// texture coordinate
	// vertex 2
	-1.0f, -1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	0.0f, 0.0f,			// texture coordinate
	// vertex 3
	1.0f, -1.0f, 0.0f,	// position
	0.0f, 0.0f, -1.0f,	// normal
	1.0f, 0.0f, 0.0f,	// tangent
	1.0f, 0.0f,			// texture coordinate
};

Mesh g_mesh;					// mesh

GLuint g_IBO = 0;				// index buffer object identifier
GLuint g_VBO[3];				// vertex buffer object identifier
GLuint g_VAO[3];				// vertex array object identifier
GLuint g_shaderProgramID = 0;	// shader program identifier

// locations in shader
GLuint g_MVP_Index = 0;
GLuint g_MV_Index = 0;
GLuint g_V_Index = 0;
GLuint g_texSamplerIndex;
GLuint g_normalSamplerIndex;
GLuint g_envMapSamplerIndex;
GLuint g_alphaIndex;
GLuint g_lightPositionIndex = 0;
GLuint g_lightDirectionIndex = 0;
GLuint g_lightAmbientIndex = 0;
GLuint g_lightDiffuseIndex = 0;
GLuint g_lightSpecularIndex = 0;
GLuint g_lightTypeIndex = 0;
GLuint g_materialAmbientIndex = 0;
GLuint g_materialDiffuseIndex = 0;
GLuint g_materialSpecularIndex = 0;
GLuint g_materialShininessIndex = 0;
GLuint g_surfaceIsReflective = 0;


glm::mat4 g_modelMatrix[14];		// object's model matrix

enum CUBE_FACE { FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM };

Light g_lightPoint;				// light properties
Light g_lightDirectional;		// light properties
Material g_material[2];			// material properties
bool g_directional = false;		// directional light source on or off

unsigned char* g_texImage[3];	//image data
unsigned char* cube_texImage[6];	//image data
unsigned char* floorImage[2];
GLuint g_textureID[6];			//texture id

GLuint g_windowWidth = 800;		// window dimensions
GLuint g_windowHeight = 600;

float g_frameTime = 0.0f;
float g_alpha = 0.5f;
Camera g_camera;
bool g_moveCamera = false;

bool load_mesh(const char* fileName, Mesh* mesh);

static void init(GLFWwindow* window)
{
	glEnable(GL_DEPTH_TEST);	// enable depth buffer test

	// create and compile our GLSL program from the shader files
	g_shaderProgramID = loadShaders("NormalMapVS.vert", "NormalMapFS.frag");

	// find the location of shader variables
	GLuint positionIndex = glGetAttribLocation(g_shaderProgramID, "aPosition");
	GLuint normalIndex = glGetAttribLocation(g_shaderProgramID, "aNormal");
	GLuint tangentIndex = glGetAttribLocation(g_shaderProgramID, "aTangent");
	GLuint texCoordIndex = glGetAttribLocation(g_shaderProgramID, "aTexCoord");

	g_MVP_Index = glGetUniformLocation(g_shaderProgramID, "uModelViewProjectionMatrix");
	g_MV_Index = glGetUniformLocation(g_shaderProgramID, "uModelViewMatrix");
	g_V_Index = glGetUniformLocation(g_shaderProgramID, "uViewMatrix");

	g_surfaceIsReflective = glGetUniformLocation(g_shaderProgramID, "isReflective");
	g_envMapSamplerIndex = glGetUniformLocation(g_shaderProgramID, "uEnvironmentMap");
	g_alphaIndex = glGetUniformLocation(g_shaderProgramID, "uAlpha");

	g_texSamplerIndex = glGetUniformLocation(g_shaderProgramID, "uTextureSampler");
	g_normalSamplerIndex = glGetUniformLocation(g_shaderProgramID, "uNormalSampler");

	g_lightPositionIndex = glGetUniformLocation(g_shaderProgramID, "uLight.position");
	g_lightDirectionIndex = glGetUniformLocation(g_shaderProgramID, "uLight.direction");
	g_lightAmbientIndex = glGetUniformLocation(g_shaderProgramID, "uLight.ambient");
	g_lightDiffuseIndex = glGetUniformLocation(g_shaderProgramID, "uLight.diffuse");
	g_lightSpecularIndex = glGetUniformLocation(g_shaderProgramID, "uLight.specular");
	g_lightTypeIndex = glGetUniformLocation(g_shaderProgramID, "uLight.type");

	g_materialAmbientIndex = glGetUniformLocation(g_shaderProgramID, "uMaterial.ambient");
	g_materialDiffuseIndex = glGetUniformLocation(g_shaderProgramID, "uMaterial.diffuse");
	g_materialSpecularIndex = glGetUniformLocation(g_shaderProgramID, "uMaterial.specular");
	g_materialShininessIndex = glGetUniformLocation(g_shaderProgramID, "uMaterial.shininess");

	// initialise model matrix to the identity matrix
	g_modelMatrix[0] = glm::mat4(1.0f);
	g_modelMatrix[1] = glm::mat4(1.0f);
	g_modelMatrix[2] = glm::mat4(1.0f);
	g_modelMatrix[3] = glm::mat4(1.0f);
	g_modelMatrix[4] = glm::mat4(1.0f);
	g_modelMatrix[5] = glm::mat4(1.0f);
	g_modelMatrix[6] = glm::mat4(1.0f);
	g_modelMatrix[7] = glm::mat4(1.0f);
	g_modelMatrix[8] = glm::mat4(1.0f);
	g_modelMatrix[9] = glm::mat4(1.0f);
	g_modelMatrix[10] = glm::mat4(1.0f);
	g_modelMatrix[11] = glm::mat4(1.0f);
	g_modelMatrix[12] = glm::mat4(1.0f);
	g_modelMatrix[13] = glm::mat4(1.0f);
	//floor
	g_modelMatrix[0] = translate(vec3(0.0f, -6.0f, 12.0f)) * rotate(radians(90.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(12.0f, 12.0f, 1.0f));
	//walls
	g_modelMatrix[1] = scale(vec3(12.0f,6.0f,1.0f));
	g_modelMatrix[2] = translate(vec3(12.0f, 0.0f, 12.0f)) * rotate(radians(90.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(12.0f, 6.0f, 1.0f));
	g_modelMatrix[3] = translate(vec3(-12.0f, 0.0f, 12.0f)) * rotate(radians(-90.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(12.0f, 6.0f, 1.0f));
	g_modelMatrix[4] = translate(vec3(0.0f, 0.0f, 24.0f)) * rotate(radians(180.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(12.0f, 6.0f, 1.0f));
	//model
	g_modelMatrix[5] = translate(vec3(0.0f, -2.5f, 12.0f)) * rotate(radians(90.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	//pedestal
	g_modelMatrix[6] = translate(vec3(0.0f, -5.0f, 11.0f)) * rotate(radians(0.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	g_modelMatrix[7] = translate(vec3(0.0f, -5.0f, 13.0f)) * rotate(radians(0.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	g_modelMatrix[8] = translate(vec3(-1.0f, -5.0f, 12.0f)) * rotate(radians(90.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	g_modelMatrix[9] = translate(vec3(1.0f, -5.0f, 12.0f)) * rotate(radians(-90.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	g_modelMatrix[10] = translate(vec3(0.0f, -4.0f, 12.0f)) * rotate(radians(90.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(1.0f, 1.0f, 1.0f));
	//mirror
	g_modelMatrix[11] = translate(vec3(0.0f, 0.0f, 6.0f)) * rotate(radians(0.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(6.0f, 6.0f, 1.0f));
	//wallpaper
	g_modelMatrix[12] = translate(vec3(0.0f, 0.0f, 0.1f)) * rotate(radians(180.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(3.0f, 3.0f, 1.0f));
	g_modelMatrix[13] = translate(vec3(0.0f, 0.0f, 23.9f)) * rotate(radians(0.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(vec3(3.0f, 3.0f, 1.0f));

	// initialise view matrix
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspectRatio = static_cast<float>(width) / height;

	g_camera.setViewMatrix(glm::vec3(0.0f, -2.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	g_camera.setProjection(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

	// load mesh
	//	load_mesh("models/sphere.obj", &g_mesh);
	load_mesh("models/torus.obj", &g_mesh);

	// initialise point light properties
	g_lightPoint.position = glm::vec3(1.0f, 1.0f, 1.0f);
	g_lightPoint.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
	g_lightPoint.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	g_lightPoint.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	g_lightPoint.type = 0;


	// initialise material properties
	g_material[0].ambient = glm::vec3(0.3f, 0.3f, 0.3f);
	g_material[0].diffuse = glm::vec3(0.2f, 0.7f, 1.0f);
	g_material[0].specular = glm::vec3(1.0f, 1.0f, 1.0f);
	g_material[0].shininess = 40.0f;

	g_material[1].ambient = glm::vec3(0.3f, 0.3f, 0.3f);
	g_material[1].diffuse = glm::vec3(0.2f, 0.8f, 0.4f);
	g_material[1].specular = glm::vec3(1.0f, 1.0f, 1.0f);
	g_material[1].shininess = 40.0f;
	
	g_material[2].ambient = glm::vec3(0.3f, 0.3f, 0.3f);
	g_material[2].diffuse = glm::vec3(0.2f, 0.7f, 1.0f);
	g_material[2].specular = glm::vec3(2.0f, 0.7f, 1.0f);
	g_material[2].shininess = 40.0f;

	// read the image data
	GLint imageWidth[6];			//image width info
	GLint imageHeight[6];			//image height info
	g_texImage[0] = readBitmapRGBImage("images/Fieldstone.bmp", &imageWidth[0], &imageHeight[0]);
	g_texImage[1] = readBitmapRGBImage("images/FieldstoneBumpDOT3.bmp", &imageWidth[1], &imageHeight[1]);
	g_texImage[2] = readBitmapRGBImage("images/White.bmp", &imageWidth[5], &imageHeight[5]);

	floorImage[0] = readBitmapRGBImage("images/Tile4.bmp", &imageWidth[2], &imageHeight[2]);
	floorImage[1] = readBitmapRGBImage("images/Tile4BumpDOT3.bmp", &imageWidth[3], &imageHeight[3]);

	cube_texImage[FRONT] = readBitmapRGBImage("images/cm_front.bmp", &imageWidth[4], &imageHeight[4]);
	cube_texImage[BACK] = readBitmapRGBImage("images/cm_back.bmp", &imageWidth[4], &imageHeight[4]);
	cube_texImage[LEFT] = readBitmapRGBImage("images/cm_left.bmp", &imageWidth[4], &imageHeight[4]);
	cube_texImage[RIGHT] = readBitmapRGBImage("images/cm_right.bmp", &imageWidth[4], &imageHeight[4]);
	cube_texImage[TOP] = readBitmapRGBImage("images/cm_top.bmp", &imageWidth[4], &imageHeight[4]);
	cube_texImage[BOTTOM] = readBitmapRGBImage("images/cm_bottom.bmp", &imageWidth[4], &imageHeight[4]);


	// generate identifier for texture object and set texture properties
	glGenTextures(5, g_textureID);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[0], imageHeight[0], 0, GL_BGR, GL_UNSIGNED_BYTE, g_texImage[0]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[1], imageHeight[1], 0, GL_BGR, GL_UNSIGNED_BYTE, g_texImage[1]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, g_textureID[2]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[2], imageHeight[2], 0, GL_BGR, GL_UNSIGNED_BYTE, floorImage[0]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, g_textureID[3]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[3], imageHeight[3], 0, GL_BGR, GL_UNSIGNED_BYTE, floorImage[1]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_CUBE_MAP, g_textureID[4]);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[RIGHT]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[LEFT]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[TOP]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[BOTTOM]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[BACK]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, imageWidth[4], imageHeight[4], 0, GL_BGR, GL_UNSIGNED_BYTE, cube_texImage[FRONT]);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, g_textureID[5]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[5], imageHeight[5], 0, GL_BGR, GL_UNSIGNED_BYTE, g_texImage[2]);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// generate identifier for VBOs and copy data to GPU
	glGenBuffers(3, g_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertices), g_vertices, GL_STATIC_DRAW);

	// generate identifiers for VAO
	glGenVertexArrays(3, g_VAO);

	// create VAO and specify VBO data
	glBindVertexArray(g_VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[0]);
	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
	glVertexAttribPointer(normalIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glVertexAttribPointer(tangentIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tangent)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);	// enable vertex attributes
	glEnableVertexAttribArray(normalIndex);
	glEnableVertexAttribArray(tangentIndex);
	glEnableVertexAttribArray(texCoordIndex);


	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertices), g_vertices, GL_STATIC_DRAW);
	glBindVertexArray(g_VAO[1]);

	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[1]);
	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
	glVertexAttribPointer(normalIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glVertexAttribPointer(tangentIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tangent)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);	// enable vertex attributes
	glEnableVertexAttribArray(normalIndex);
	glEnableVertexAttribArray(tangentIndex);
	glEnableVertexAttribArray(texCoordIndex);


	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*g_mesh.numberOfVertices, g_mesh.pMeshVertices, GL_STATIC_DRAW);

	// generate identifier for IBO and copy data to GPU
	glGenBuffers(1, &g_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLint) * 3 * g_mesh.numberOfFaces, g_mesh.pMeshIndices, GL_STATIC_DRAW);

	glBindVertexArray(g_VAO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);
	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
	glVertexAttribPointer(normalIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glVertexAttribPointer(tangentIndex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tangent)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);	// enable vertex attributes
	glEnableVertexAttribArray(normalIndex);
	glEnableVertexAttribArray(tangentIndex);
	glEnableVertexAttribArray(texCoordIndex);
}

// function used to update the scene
static void update_scene(GLFWwindow* window, float frameTime)
{
	// variables to store forward/back and strafe movement
	float moveForward = 0;
	float strafeRight = 0;

	// update movement variables based on keyboard input
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		moveForward += 1 * MOVEMENT_SENSITIVITY * frameTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		moveForward -= 1 * MOVEMENT_SENSITIVITY * frameTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		strafeRight -= 1 * MOVEMENT_SENSITIVITY * frameTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		strafeRight += 1 * MOVEMENT_SENSITIVITY * frameTime;

	g_modelMatrix[5] *= rotate(radians(ROTATION_SENSITIVITY), vec3(0.0f, 0.0f, 1.0f));



	g_camera.update(moveForward, strafeRight);	// update camera
}

static void draw_walls(bool isReflect) {

	glUseProgram(g_shaderProgramID);	// use the shaders associated with the shader program

	glBindVertexArray(g_VAO[0]);		// make VAO active

	mat4 MVP[12];
	mat4 MV[12];
	glm::mat4 reflectMatrix = mat4(1.0f);
	glm::mat4 modelMatrix = mat4(1.0f);

	glm::vec3 lightPosition = g_lightPoint.position;

	if (isReflect) {
		reflectMatrix = glm::scale(vec3(1.0f, -1.0f, 1.0f));
		lightPosition = vec3(reflectMatrix * vec4(lightPosition, 1.0f));
	}
	else {
		reflectMatrix = mat4(1.0f);
	}

	// set uniform shader variables
	modelMatrix = reflectMatrix * g_modelMatrix[1];
	MVP[0] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[0] = g_camera.getViewMatrix() * modelMatrix;
	glm::mat4 V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[0][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[0][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// set uniform shader variables
	modelMatrix = reflectMatrix * g_modelMatrix[2];
	MVP[1] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[1] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[1][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[1][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);
	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// set uniform shader variables
	modelMatrix = reflectMatrix * g_modelMatrix[3];
	MVP[2] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[2] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[2][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[2][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);
	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// set uniform shader variables
	modelMatrix = reflectMatrix * g_modelMatrix[4];
	MVP[3] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[3] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[3][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[3][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);
	

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);
	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[6];
	MVP[4] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[4] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[4][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[4][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[7];
	MVP[5] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[5] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[5][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[5][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[8];
	MVP[6] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[6] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[6][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[6][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[9];
	MVP[7] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[7] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[7][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[7][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[10];
	MVP[8] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[8] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[8][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[8][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[12];
	MVP[10] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[10] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[10][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[10][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[5]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[5]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = reflectMatrix * g_modelMatrix[13];
	MVP[11] = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * modelMatrix;
	MV[11] = g_camera.getViewMatrix() * modelMatrix;
	V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[11][0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[11][0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[5]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[5]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(g_VAO[2]);		// make VAO active

	glm::mat4 MVP2 = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * g_modelMatrix[5];
	glm::mat4 MV2 = g_camera.getViewMatrix() * g_modelMatrix[5];
	glm::mat4 V2 = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP2[0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV2[0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V2[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[2].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[2].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[2].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[2].shininess);

	glUniform1i(g_surfaceIsReflective, true);

	glUniform1i(g_envMapSamplerIndex, 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_textureID[4]);

	glDrawElements(GL_TRIANGLES, g_mesh.numberOfFaces * 3, GL_UNSIGNED_INT, 0);

}

void draw_mirror() {
	glUseProgram(g_shaderProgramID);	// use the shaders associated with the shader program

	glBindVertexArray(g_VAO[0]);		// make VAO active

	glEnable(GL_BLEND);		//enable blending            
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	mat4 MVP = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * g_modelMatrix[11];
	mat4 MV = g_camera.getViewMatrix() * g_modelMatrix[11];
	mat4 V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[0].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[0].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[0].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[0].shininess);

	glUniform1i(g_surfaceIsReflective, false);
	glUniform1f(g_alphaIndex, g_alpha);

	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_textureID[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[1]);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisable(GL_BLEND);
}

void draw_floor() {

	glUseProgram(g_shaderProgramID);	// use the shaders associated with the shader program

	glBindVertexArray(g_VAO[1]);		// make VAO active

									// set shader variables
	glm::mat4 MVP = g_camera.getProjectionMatrix() * g_camera.getViewMatrix() * g_modelMatrix[0];
	glm::mat4 MV = g_camera.getViewMatrix() * g_modelMatrix[0];
	glm::mat4 V = g_camera.getViewMatrix();
	glUniformMatrix4fv(g_MVP_Index, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(g_MV_Index, 1, GL_FALSE, &MV[0][0]);
	glUniformMatrix4fv(g_V_Index, 1, GL_FALSE, &V[0][0]);

	glUniform3fv(g_lightPositionIndex, 1, &g_lightPoint.position[0]);
	glUniform3fv(g_lightAmbientIndex, 1, &g_lightPoint.ambient[0]);
	glUniform3fv(g_lightDiffuseIndex, 1, &g_lightPoint.diffuse[0]);
	glUniform3fv(g_lightSpecularIndex, 1, &g_lightPoint.specular[0]);
	glUniform1i(g_lightTypeIndex, g_lightPoint.type);

	glUniform3fv(g_materialAmbientIndex, 1, &g_material[1].ambient[0]);
	glUniform3fv(g_materialDiffuseIndex, 1, &g_material[1].diffuse[0]);
	glUniform3fv(g_materialSpecularIndex, 1, &g_material[1].specular[0]);
	glUniform1fv(g_materialShininessIndex, 1, &g_material[1].shininess);

	glUniform1i(g_surfaceIsReflective, false);
	glUniform1i(g_texSamplerIndex, 0);
	glUniform1i(g_normalSamplerIndex, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, g_textureID[2]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_textureID[3]);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

// function used to render the scene
static void render_scene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);	// clear colour buffer and depth buffer
	/*
	// disable depth buffer and draw mirror surface to stencil buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  //disable any modification of all color components
	glDepthMask(GL_FALSE);                                //disable any modification to depth buffer
	glEnable(GL_STENCIL_TEST);                            //enable stencil testing

														  //setup the stencil buffer for a function reference value
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 1);

	//draw_floor();

	//enable depth buffer, draw reflected geometry where stencil buffer passes

	//render where the stencil buffer equal to 1
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);   //allow all color components to be modified 
	glDepthMask(GL_TRUE);                              //allow depth buffer contents to be modified

	draw_walls(true);

	glDisable(GL_STENCIL_TEST);		//disable stencil testing
	
									//blend in reflection
	*/

	draw_floor();

	

	draw_walls(false);
	draw_mirror();



	glFlush();	// flush the pipeline
}

// key press or release callback function
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// quit if the ESCAPE key was press
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// variables to store mouse cursor coordinates
	static double previous_xpos = xpos;
	static double previous_ypos = ypos;
	double delta_x = previous_xpos - xpos;
	double delta_y = previous_ypos - ypos;

	if (g_moveCamera)
	{
		// pass mouse movement to camera class to update its yaw and pitch
		g_camera.updateRotation(delta_x * ROTATION_SENSITIVITY * g_frameTime, delta_y * ROTATION_SENSITIVITY * g_frameTime);
	}

	// update previous mouse coordinates
	previous_xpos = xpos;
	previous_ypos = ypos;

	// pass mouse data to tweak bar
	TwEventMousePosGLFW(xpos, ypos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// pass mouse data to tweak bar
	TwEventMouseButtonGLFW(button, action);

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		// use mouse to move camera, hence use disable cursor mode
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		g_moveCamera = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		// use mouse to move camera, hence use disable cursor mode
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		g_moveCamera = false;
	}
}

// error callback function
static void error_callback(int error, const char* description)
{
	cerr << description << endl;	// output error description
}

int main(void)
{
	GLFWwindow* window = NULL;	// pointer to a GLFW window handle
	TwBar *TweakBar;			// pointer to a tweak bar
	double lastUpdateTime = glfwGetTime();	// last update time
	double elapsedTime = lastUpdateTime;	// time elapsed since last update
	int frameCount = 0;						// number of frames since last update
	int FPS = 0;							// frames per second

	glfwSetErrorCallback(error_callback);	// set error callback function

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
	window = glfwCreateWindow(g_windowWidth, g_windowHeight, "Tutorial", NULL, NULL);

	// if failed to create window
	if (window == NULL)
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
		cerr << "GLEW initialisation failed" << endl;
		exit(EXIT_FAILURE);
	}

	// set key callback function
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// initialise AntTweakBar
	TwInit(TW_OPENGL_CORE, NULL);

	// give tweak bar the size of graphics window
	TwWindowSize(g_windowWidth, g_windowHeight);
	TwDefine(" TW_HELP visible=false ");	// disable help menu
	TwDefine(" GLOBAL fontsize=3 ");		// set large font size

	// create a tweak bar
	TweakBar = TwNewBar("Main");
	TwDefine(" Main label='Controls' refresh=0.02 text=light size='220 300' ");

	// create light position
	TwAddVarRW(TweakBar, "LightPos: x", TW_TYPE_FLOAT, &g_lightPoint.position[0], " group='Light Position' min=-10.0 max=10.0 step=0.1");
	TwAddVarRW(TweakBar, "LightPos: y", TW_TYPE_FLOAT, &g_lightPoint.position[1], " group='Light Position' min=-10.0 max=10.0 step=0.1");
	TwAddVarRW(TweakBar, "LightPos: z", TW_TYPE_FLOAT, &g_lightPoint.position[2], " group='Light Position' min=-10.0 max=10.0 step=0.1");

	TwAddVarRW(TweakBar, "Alpha", TW_TYPE_FLOAT, &g_alpha, " group='Glass' min=0.0 max=1.0 step=0.01 ");

	// initialise rendering states
	init(window);

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window, g_frameTime);		// update the scene
		render_scene();		// render the scene

		TwDraw();			// draw tweak bar(s)

		glfwSwapBuffers(window);	// swap buffers
		glfwPollEvents();			// poll for events

		frameCount++;
		elapsedTime = glfwGetTime() - lastUpdateTime;	// current time - last update time

		if (elapsedTime >= 1.0f)	// if time since last update >= to 1 second
		{
			g_frameTime = 1.0f / frameCount;	// calculate frame time

			string str = "FPS = " + to_string(frameCount) + "; FT = " + to_string(g_frameTime);

			glfwSetWindowTitle(window, str.c_str());	// update window title

			FPS = frameCount;
			frameCount = 0;					// reset frame count
			lastUpdateTime += elapsedTime;	// update last update time
		}
	}

	// clean up
	if (g_texImage[0])
		delete[] g_texImage[0];
	if (g_texImage[1])
		delete[] g_texImage[1];

	glDeleteProgram(g_shaderProgramID);
	glDeleteBuffers(2, g_VBO);
	glDeleteVertexArrays(2, g_VAO);
	glDeleteTextures(4, g_textureID);

	// uninitialise tweak bar
	TwTerminate();

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}

bool load_mesh(const char* fileName, Mesh* mesh)
{
	// load file with assimp 
	const aiScene* pScene = aiImportFile(fileName, aiProcess_Triangulate
		| aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

	// check whether scene was loaded
	if (!pScene)
	{
		cout << "Could not load mesh." << endl;
		return false;
	}

	// get pointer to mesh 0
	const aiMesh* pMesh = pScene->mMeshes[0];

	// store number of mesh vertices
	mesh->numberOfVertices = pMesh->mNumVertices;

	// if mesh contains vertex coordinates
	if (pMesh->HasPositions())
	{
		// allocate memory for vertices
		mesh->pMeshVertices = new Vertex[pMesh->mNumVertices];

		// read vertex coordinates and store in the array
		for (int i = 0; i < pMesh->mNumVertices; i++)
		{
			const aiVector3D* pVertexPos = &(pMesh->mVertices[i]);

			mesh->pMeshVertices[i].position[0] = (GLfloat)pVertexPos->x;
			mesh->pMeshVertices[i].position[1] = (GLfloat)pVertexPos->y;
			mesh->pMeshVertices[i].position[2] = (GLfloat)pVertexPos->z;
		}
	}

	// if mesh contains normals
	if (pMesh->HasNormals())
	{
		// read normals and store in the array
		for (int i = 0; i < pMesh->mNumVertices; i++)
		{
			const aiVector3D* pVertexNormal = &(pMesh->mNormals[i]);

			mesh->pMeshVertices[i].normal[0] = (GLfloat)pVertexNormal->x;
			mesh->pMeshVertices[i].normal[1] = (GLfloat)pVertexNormal->y;
			mesh->pMeshVertices[i].normal[2] = (GLfloat)pVertexNormal->z;
		}
	}

	// if mesh contains faces
	if (pMesh->HasFaces())
	{
		// store number of mesh faces
		mesh->numberOfFaces = pMesh->mNumFaces;

		// allocate memory for vertices
		mesh->pMeshIndices = new GLint[pMesh->mNumFaces * 3];

		// read normals and store in the array
		for (int i = 0; i < pMesh->mNumFaces; i++)
		{
			const aiFace* pFace = &(pMesh->mFaces[i]);

			mesh->pMeshIndices[i * 3] = (GLint)pFace->mIndices[0];
			mesh->pMeshIndices[i * 3 + 1] = (GLint)pFace->mIndices[1];
			mesh->pMeshIndices[i * 3 + 2] = (GLint)pFace->mIndices[2];
		}
	}

	// release the scene
	aiReleaseImport(pScene);

	return true;
}
