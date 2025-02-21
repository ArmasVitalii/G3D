#include <Windows.h>
#include <locale>
#include <codecvt>

#include <stdlib.h>
#include <stdio.h>
#include <math.h> 

#include <GL/glew.h>

#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Shader.h"
#include "Mesh.h"
#include "Model.h"

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

struct BoundingSphere {
	glm::vec3 center;
	float     radius;
};

struct SceneObject {
	Model* model;
	BoundingSphere bounding;
	glm::vec3 position;
	glm::vec3 scale;
};

std::vector<SceneObject> sceneObjects;

BoundingSphere GetWorldBoundingSphere(const SceneObject& obj)
{
	float maxScale = std::max({ obj.scale.x, obj.scale.y, obj.scale.z });
	BoundingSphere worldSphere;
	worldSphere.radius = obj.bounding.radius * maxScale;
	worldSphere.center = obj.position + (obj.bounding.center * obj.scale);
	return worldSphere;
}

bool CheckSphereCollision(const BoundingSphere& s1, const BoundingSphere& s2)
{
	float distance = glm::length(s1.center - s2.center);
	float combinedRadius = s1.radius + s2.radius;
	return (distance <= combinedRadius);
}

float timeOfDay = 6.0f;
const float dayDuration = 60.0f;
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

enum ECameraMovementType
{
	UNKNOWN,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	ROLL_LEFT,    
	ROLL_RIGHT,   
	PITCH_UP,     
	PITCH_DOWN    
};


float skyboxVertices[] =
{
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
	// Right
	1, 2, 6,
	6, 5, 1,
	// Left
	0, 4, 7,
	7, 3, 0,
	// Top
	4, 5, 6,
	6, 7, 4,
	// Bottom
	0, 3, 2,
	2, 1, 0,
	// Back
	0, 1, 5,
	5, 4, 0,
	// Front
	3, 7, 6,
	6, 2, 3
};

float planeVertices[] = {
100.0f, -0.5f,  100.0f,  4.0f, 0.0f,   // Top Right of the plane
-100.0f, -0.5f,  100.0f,  0.0f, 0.0f,   // Top Left of the plane
-100.0f, -0.5f, -100.0f,  0.0f, 4.0f,   // Bottom Left of the plane

 100.0f, -0.5f,  100.0f,  4.0f, 0.0f,   // Top Right of the plane
-100.0f, -0.5f, -100.0f,  0.0f, 4.0f,   // Bottom Left of the plane
 100.0f, -0.5f, -100.0f,  4.0f, 4.0f    // Bottom Right of the plane
};


glm::mat4 g_planeFix = glm::rotate(
	glm::mat4(1.0f),
	glm::radians(180.0f),
	glm::vec3(0.f, 90.f, 90.f)
);

glm::mat4 g_parkedPlaneFix = glm::rotate(
	glm::mat4(1.0f),
	glm::radians(180.0f),
	glm::vec3(48.f, 90.f, 110.f)
);


std::vector<std::string> faces{
	"path/to/nx.jpg",
	"path/to/ny.jpg",
	"path/to/nz.jpg",
	"path/to/px.jpg",
	"path/to/py.jpg",
	"path/to/pz.jpg"
};

void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale);
void renderModelPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale);
void renderModelParkedPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale);
void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale);
void renderModelParkedPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale);
void renderModelPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale);
void renderTerrain(Shader& terrainShader, Model& terrainModel, const glm::vec3& position, const glm::vec3& scale, int terrainTexture);



unsigned int CreateTexture(const std::string& strTexturePath)
{
	unsigned int textureId = -1;

	int width, height, nrChannels;
	unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture: " << strTexturePath << std::endl;
		std::cerr << "Failed to load texture: " << stbi_failure_reason() << std::endl;
	}
	stbi_image_free(data);

	return textureId;
}

unsigned int LoadSkybox(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	int width, height, nrChannels;
	for (unsigned int i{ 0 }; i < faces.size(); ++i)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else
			std::cerr << "Failed to load texture: " << stbi_failure_reason() << std::endl;
		//std::cerr << "Failed to load texture: " << stbi_failure_reason() << std::endl;
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return textureID;
}

struct Cloud {
	glm::vec3 position;
	glm::vec3 scale;
	float rotation;
	float speed;
};

class Camera
{
private:
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;
	const float MAX_YAW = 0.0f;  
	const float MIN_YAW = -180.0f;
	const float takeoffSpeed = 1.0f;

public:
	Camera(const int width, const int height, const glm::vec3& position)
	{
		startPosition = position;
		Set(width, height, position);

		boundingSphere.radius = 2.0f;  
		boundingSphere.center = position;
	}

	void Set(const int width, const int height, const glm::vec3& position)
	{
		this->isPerspective = true;
		this->yaw = YAW;
		this->pitch = PITCH;

		this->FoVy = FOV;
		this->width = width;
		this->height = height;
		this->zNear = zNEAR;
		this->zFar = zFAR;

		this->worldUp = glm::vec3(0, 1, 0);
		this->position = position;

		lastX = width / 2.0f;
		lastY = height / 2.0f;
		bFirstMouseMove = true;

		UpdateCameraVectors();
	}

	void UpdateBoundingVolume()
	{
		boundingSphere.center = position;
	}

	BoundingSphere boundingSphere;

	float GetRoll() const
	{
		return roll;
	}
	float GetTakeoffSpeed() const
	{
		return takeoffSpeed;
	}

	float GetPitch() const
	{
		return pitch;
	}

	float GetYaw() const
	{
		return yaw;
	}

	void SetPosition(const glm::vec3& pos)
	{
		position = pos;
	}


	void SetRoll(float r) { roll = r; }
	void SetPitch(float p) { pitch = p; }

	void Accelerate(float deltaTime) {
		speed += acceleration * deltaTime;
		if (speed > maxSpeed) speed = maxSpeed;
	}

	void Decelerate(float deltaTime) {
		speed -= 0.05f;
		if (speed <= 0.0f) speed = 0.0f;
	}

	float GetSpeed() const { return speed; }
	void SetSpeed(float s) { speed = s; }

	void Reset(const int width, const int height)
	{
		Set(width, height, startPosition);
	}

	void Reshape(int windowWidth, int windowHeight)
	{
		width = windowWidth;
		height = windowHeight;

		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::mat4 GetViewMatrix() const
	{
		return glm::lookAt(position, position + forward, up);
	}

	const glm::vec3 GetPosition() const
	{
		return position;
	}

	const glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 Proj = glm::mat4(1);
		if (isPerspective) {
			float aspectRatio = ((float)(width)) / height;
			Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
		}
		else {
			float scaleFactor = 2000.f;
			Proj = glm::ortho<float>(
				-width / scaleFactor, width / scaleFactor,
				-height / scaleFactor, height / scaleFactor, -zFar, zFar);
		}
		return Proj;
	}

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime) {
		float rotationSpeed = 45.0f * deltaTime;
		float movementSpeed = 10.0f * deltaTime;

		if (grounded) {
			switch (direction) {
			case FORWARD:
				Accelerate(deltaTime);
				position += forward * GetSpeed() * deltaTime * 10.0f;
				break;
			case BACKWARD:
				Decelerate(deltaTime);
				position -= forward * GetSpeed() * deltaTime * 10.0f;
				break;
			default:
				break;
			}
		}
		else {
			switch (direction) {
			case FORWARD:
				Accelerate(deltaTime);
				position += forward * GetSpeed() * deltaTime * 10.0f;
				break;
			case BACKWARD:
				Decelerate(deltaTime);
				position -= forward * GetSpeed() * deltaTime * 10.0f;
				break;
			case LEFT:
				yaw -= rotationSpeed;  // Rotate left
				break;
			case RIGHT:
				yaw += rotationSpeed;  // Rotate right
				break;
			case UP:
				pitch += rotationSpeed; // Pitch up
				break;
			case DOWN:
				pitch -= rotationSpeed; // Pitch down
				break;
			case ROLL_LEFT:
				roll += rotationSpeed;  // Roll left
				break;
			case ROLL_RIGHT:
				roll -= rotationSpeed;  // Roll right
				break;
			default:
				break;
			}
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
			UpdateCameraVectors();
			position += forward * GetSpeed() * deltaTime * 10.0f;
		}
	}



	void MouseControl(float xPos, float yPos)
	{
		if (bFirstMouseMove) {
			lastX = xPos;
			lastY = yPos;
			bFirstMouseMove = false;
		}

		float xChange = xPos - lastX;
		float yChange = lastY - yPos;
		lastX = xPos;
		lastY = yPos;

		if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6) {
			return;
		}
		xChange *= mouseSensitivity;
		yChange *= mouseSensitivity;

		if (!grounded)
		{
			yaw += xChange;
			pitch += yChange;
		}

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
		if (yaw > MAX_YAW)
			yaw = MAX_YAW;
		if (yaw < MIN_YAW)
			yaw = MIN_YAW;

		UpdateCameraVectors();
	}

	void ProcessMouseScroll(float yOffset)
	{
		if (FoVy >= 1.0f && FoVy <= 90.0f) {
			FoVy -= yOffset;
		}
		if (FoVy <= 1.0f)
			FoVy = 1.0f;
		if (FoVy >= 90.0f)
			FoVy = 90.0f;
	}

	glm::vec3 GetForward() const
	{
		return forward;
	}

	glm::vec3 GetUp() const { return up; }


	void StartFlying() {
		isFlying = true;
		grounded = false;
	}


	void UpdateFlight(float deltaTime) {
		if (isFlying) {
			speed += acceleration * deltaTime;
			if (speed > maxSpeed) speed = maxSpeed;

			pitch += pitchIncrement * deltaTime;
			if (pitch > 20.0f) pitch = 20.0f;
			glm::vec3 newPosition = position + forward * GetSpeed() * deltaTime * 10.0f;

			if (newPosition.y < -20.0f) {
				newPosition.y = 0.0F; 
			}

			position = newPosition;
		}
	}
	void SetGrounded(bool g) {
		grounded = g;
		if (g) {
			pitch = 0.0f;
			roll = 0.0f;
			speed = 0.0f;
		}
	}

	bool IsGrounded() const { return grounded; }

	void StopFlying() {
		isFlying = false;
	}

	bool IsFlying() const { return isFlying; }


private:
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
	{
		yaw += xOffset;
		pitch += yOffset;

		if (constrainPitch) {
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		if (yaw > MAX_YAW)
			yaw = MAX_YAW;
		if (yaw < MIN_YAW)
			yaw = MIN_YAW;

		UpdateCameraVectors();
	}

	void UpdateCameraVectors() {
		this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward.y = sin(glm::radians(pitch));
		this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward = glm::normalize(this->forward);

		this->right = glm::normalize(glm::cross(forward, worldUp));
		this->up = glm::normalize(glm::cross(right, forward));

		glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(roll), forward);
		this->right = glm::normalize(glm::vec3(rollMatrix * glm::vec4(this->right, 0.0f)));
		this->up = glm::normalize(glm::cross(right, forward));
	}

protected:
	const float cameraSpeedFactor = 2.5f;
	const float mouseSensitivity = 0.1f;

	float zNear;
	float zFar;
	float FoVy;
	int width;
	int height;
	bool isPerspective;
	float roll = 0.0f;  // Roll angle in degrees
	float speed = 0.01f;       
	float acceleration = 0.1f; 
	float maxSpeed = 10.0f;     
	float minSpeed = 0.0f;     
	bool isFlying = false;
	bool grounded = true;
	float pitchIncrement = 0.1f;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	// Euler Angles
	float yaw;
	float pitch;

	bool bFirstMouseMove = true;
	float lastX = 0.f, lastY = 0.f;
};

GLuint ProjMatrixLocation, ViewMatrixLocation, WorldMatrixLocation;
Camera* pCamera = nullptr;

void Cleanup()
{
	delete pCamera;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

double deltaTime = 0.0f;	
double lastFrame = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {

	}
}

Model airplane, parkedAirplane, tower, skybox, terrain, road, hangare, highFlyingAirplane, highFlyingAirplane2, landingPlane, cloud, building;

glm::vec3 getSkyColor(float timeOfDay, std::string& skyboxPath) {
	glm::vec3 dayColor(0.5f, 0.7f, 1.0f); 
	glm::vec3 nightColor(0.0f, 0.0f, 0.1f); 

	if (timeOfDay < 6.0f || timeOfDay > 18.0f) {
		// Night time
		skyboxPath += "\\Models\\skyboxNight\\";
		return nightColor;
	}
	else if (timeOfDay >= 6.0f && timeOfDay <= 12.0f) {
		// Morning to noon
		skyboxPath += "\\Models\\skybox\\";
		float t = (timeOfDay - 6.0f) / 6.0f;
		return glm::mix(nightColor, dayColor, t);
	}
	else {
		// Noon to evening
		skyboxPath += "\\Models\\skybox\\";
		float t = (timeOfDay - 12.0f) / 6.0f;
		return glm::mix(dayColor, nightColor, t);
	}
}

float getLightIntensity(float timeOfDay) {
	if (timeOfDay < 6.0f || timeOfDay > 18.0f) {
		// Night time
		return 0.1f;
	}
	else if (timeOfDay >= 6.0f && timeOfDay <= 12.0f) {
		// Morning to noon
		float t = (timeOfDay - 6.0f) / 6.0f;
		return glm::mix(0.1f, 1.0f, t);
	}
	else {
		// Noon to evening
		float t = (timeOfDay - 12.0f) / 6.0f;
		return glm::mix(1.0f, 0.1f, t);
	}
}


struct Particle {
	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;  // Time remaining until the particle is 
	bool active;
};

std::vector<Particle> particles;
int numParticles = 1000;

void initParticles() {
	for (int i = 0; i < numParticles; ++i) {
		Particle p;
		p.position = glm::vec3(rand() % 100 - 50, rand() % 10 + 10, rand() % 100 - 50);
		p.velocity = glm::vec3(0, -0.1, 0);  // Falling down
		p.lifetime = rand() % 10 + 5;
		p.active = true;
		particles.push_back(p);
	}
}


void updateParticles(float deltaTime) {
	for (auto& p : particles) {
		if (!p.active) continue;

		p.position += p.velocity * deltaTime;
		p.lifetime -= deltaTime;

		if (p.lifetime <= 0) {
			p.position.y = 10;
			p.lifetime = rand() % 10 + 5;
		}
	}
}

GLuint sphereVAO;
int sphereVertexCount;

void renderParticles(Shader& shader) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(sphereVAO); 


	for (const auto& p : particles) {
		if (!p.active) continue;

		glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
		model = glm::scale(model, glm::vec3(0.1f));  

		shader.use();
		shader.setMat4("model", model);
		shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		shader.setVec3("objectColor", glm::vec3(1.0f));  

		glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
	}

	glBindVertexArray(0);
	glDisable(GL_BLEND);
}


std::vector<Cloud> clouds;

void initClouds(int numClouds, const glm::vec3& areaMin, const glm::vec3& areaMax) {
	clouds.resize(numClouds);
	for (auto& cloud : clouds) {
		cloud.position = glm::vec3(
			areaMin.x + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (areaMax.x - areaMin.x))),
			areaMin.y + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (areaMax.y - areaMin.y))),
			areaMin.z + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (areaMax.z - areaMin.z)))
		);
		float scaleFactor = 20.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (30.0f - 20.0f)));
		cloud.scale = glm::vec3(scaleFactor);

		cloud.rotation = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 360.0f;
		cloud.speed = 10.1f;
	}
}

void InitSceneObjects(const glm::vec3& initialPosition)
{
	// 1) Tower
	{
		SceneObject towerObj;
		towerObj.model = &tower;
		towerObj.position = initialPosition + glm::vec3(-4.0f, -19.4f, -217.0f);
		towerObj.scale = glm::vec3(1.3f);
		towerObj.bounding.center = glm::vec3(0.0f);
		towerObj.bounding.radius = 6.05f;
		sceneObjects.push_back(towerObj);
	}

	// 2) Hangar
	{
		SceneObject hangarObj;
		hangarObj.model = &hangare;
		hangarObj.position = initialPosition + glm::vec3(-28.0f, -19.4f, -10.0f);
		hangarObj.scale = glm::vec3(0.3f);
		hangarObj.bounding.center = glm::vec3(0.0f);
		sceneObjects.push_back(hangarObj);
	}

	// 3) Road
	{
		SceneObject roadObj;
		roadObj.model = &road;
		roadObj.position = initialPosition + glm::vec3(45.0f, -19.0f, -7.0f);
		roadObj.scale = glm::vec3(0.7f, 0.3f, 1.0f);
		roadObj.bounding.center = glm::vec3(0.0f);
		roadObj.bounding.radius = 20.0f;
		sceneObjects.push_back(roadObj);
	}

	// 4) Building
	{
		SceneObject buildingObj;
		buildingObj.model = &building;
		buildingObj.position = initialPosition + glm::vec3(-22.0f, -19.4f, -207.0f);
		buildingObj.scale = glm::vec3(0.03f);
		buildingObj.bounding.center = glm::vec3(0.0f);
		buildingObj.bounding.radius = 10.0f; // <-- Tweak
		sceneObjects.push_back(buildingObj);
	}

	{
		SceneObject terrainObj;
		terrainObj.model = &terrain;
		terrainObj.position = initialPosition + glm::vec3(10.0f, -30.0f, 0.0f);
		terrainObj.scale = glm::vec3(0.01f);
		terrainObj.bounding.center = glm::vec3(0.0f);
		terrainObj.bounding.radius = 1500.0f;
		sceneObjects.push_back(terrainObj);
	}


	{
		SceneObject groundAirplaneObj;
		groundAirplaneObj.model = &airplane;
		groundAirplaneObj.position = glm::vec3(-50.0f, -19.8f, -55.0f);
		groundAirplaneObj.scale = glm::vec3(0.0088f);
		groundAirplaneObj.bounding.center = glm::vec3(0.0f);
		groundAirplaneObj.bounding.radius = 5.0f; // <-- Tweak
		sceneObjects.push_back(groundAirplaneObj);
	}

	{
		SceneObject landingPlaneObj;
		landingPlaneObj.model = &landingPlane;
		landingPlaneObj.position = initialPosition + glm::vec3(-14.0f, 5.4f, -150.0f);
		landingPlaneObj.scale = glm::vec3(0.005f);
		landingPlaneObj.bounding.center = glm::vec3(0.0f);
		landingPlaneObj.bounding.radius = 5.0f; // <-- Tweak
		sceneObjects.push_back(landingPlaneObj);
	}
}

int main() 
{

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Plane Simulator", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	glEnable(GL_DEPTH_TEST);

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glGenBuffers(1, &skyboxEBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glm::vec3 initialPosition(0.0f, 0.0f, 0.0f);

	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, initialPosition + glm::vec3(-26.0f, -18.f, -4.0f));

	glm::vec3 lightPos(0.0f, 2.0f, 1.0f);

	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);

	std::wstring executablePath(buffer);
	std::wstring wscurrentPath = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string currentPath = converter.to_bytes(wscurrentPath);

	//Paths
	std::string PlanePath = currentPath + "\\Models\\Airplane\\";
	std::string AirportPath = currentPath + "\\Models\\Airport\\";
	std::string MapPath = currentPath + "\\Models\\Map\\";
	std::string TowerPath = currentPath + "\\Models\\Tower\\";

	//Shaders
	Shader programShader((currentPath + "\\PlaneSimulator\\default.vs").c_str(), (currentPath + "\\PlaneSimulator\\default.fs").c_str());
	Shader lightingShader((currentPath + "\\Shaders\\PhongLight.vs").c_str(), (currentPath + "\\Shaders\\PhongLight.fs").c_str());
	Shader lampShader((currentPath + "\\Shaders\\Lamp.vs").c_str(), (currentPath + "\\Shaders\\Lamp.fs").c_str());
	Shader skyboxShader((currentPath + "\\PlaneSimulator\\skybox.vs").c_str(), (currentPath + "\\PlaneSimulator\\skybox.fs").c_str());
	Shader terrainShader((currentPath + "\\PlaneSimulator\\terrain.vs").c_str(), (currentPath + "\\PlaneSimulator\\terrain.fs").c_str());
	Shader aiportShader((currentPath + "\\PlaneSimulator\\default.vs").c_str(), (currentPath + "\\PlaneSimulator\\default.fs").c_str());

	glm::vec4 lightColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	terrainShader.use();
	terrainShader.SetVec3("lightColor", lightColor);
	terrainShader.SetVec3("objectColor", glm::vec3(0.f));

	// Models
	airplane = Model(currentPath + "\\Models\\Airplane\\IAR-93B.obj");
	parkedAirplane = Model(currentPath + "\\Models\\Parked Plane\\ImageToStl.com_iar_80_romanian_ww_ii_low-wing_monoplane.obj");
	terrain = Model(currentPath + "\\Models\\Map\\Map.obj");
	tower = Model(currentPath + "\\Models\\Tower\\Tower_Control.obj");
	road = Model(currentPath + "\\Models\\Road\\Road.obj");
	hangare = Model(currentPath + "\\Models\\Hangar\\uploads_files_852157_Shelter_simple.obj");
	highFlyingAirplane = Model(currentPath + "\\Models\\Airplane\\IAR-93B.obj");
	highFlyingAirplane2 = Model(currentPath + "\\Models\\Airplane\\IAR-93B.obj");
	landingPlane = Model(currentPath + "\\Models\\Airplane\\IAR-93B.obj");
	cloud = Model(currentPath + "\\Models\\Cloud\\Cloud_Polygon_Blender_1.obj");
	building = Model(currentPath + "\\Models\\Building\\10079_Office Building - Brick_V1_iterations-0.obj");

	unsigned int terrainTexture = CreateTexture(currentPath + "\\Models\\Map\\Map.jpg");

	InitSceneObjects(initialPosition);

	glm::vec3 initialPositionTerrain = initialPosition + glm::vec3(10.0f, -30.0f, 0.0f);
	glm::vec3 highFlyingAirplanePosition = initialPosition + glm::vec3(-22.0f, 5.4f, -200.0f);
	glm::vec3 highFlyingAirplanePosition2 = initialPosition + glm::vec3(-26.0f, 8.4f, -200.0f);
	glm::vec3 landingPLanePosition = highFlyingAirplanePosition + glm::vec3(-14.0f, 5.4f, -150.0f);

	initParticles();

	glm::vec3 cloudAreaMin(-400.0f, 160.0f, -2000.0f);
	glm::vec3 cloudAreaMax(400.0f, 220.0f, -200.0f);
	initClouds(100, cloudAreaMin, cloudAreaMax);

	while (!glfwWindowShouldClose(window)) {

		std::string skyBoxPath = currentPath;

		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (pCamera->GetPosition().z < -1400.0f)
			pCamera->SetPosition(glm::vec3(0, 20, 1400.0f));

		updateParticles(deltaTime);

		timeOfDay += deltaTime * (24.0f / dayDuration);
		if (timeOfDay > 24.0f) timeOfDay -= 24.0f;

		// Calculate sky color and light intensity based on time of day
		glm::vec3 skyColor = getSkyColor(timeOfDay, skyBoxPath);
		float lightIntensity = getLightIntensity(timeOfDay);

		std::vector<std::string> facesCubemap =
		{
			skyBoxPath + "px.png",
			skyBoxPath + "nx.png",
			skyBoxPath + "py.png",
			skyBoxPath + "ny.png",
			skyBoxPath + "pz.png",
			skyBoxPath + "nz.png"
		};

		unsigned int cubemapTexture = LoadSkybox(facesCubemap);

		processInput(window);
		pCamera->UpdateFlight(deltaTime);

		if (pCamera->IsFlying()) {
			glm::vec3 newVector = pCamera->GetPosition();
			if (newVector.y <= initialPositionTerrain.y + 11)
				newVector.y += 1.0f;
			pCamera->SetPosition(newVector);
		}
		else if (pCamera->IsGrounded()) {
			glm::vec3 pos = pCamera->GetPosition();
			pos.y = -19.1f;
			pCamera->SetPosition(pos);
		}

		glEnable(GL_DEPTH_TEST);
		glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::vec3 oldPosition = pCamera->GetPosition();
		pCamera->UpdateBoundingVolume();
		bool collided = false;
		for (auto& obj : sceneObjects)
		{
			BoundingSphere worldSphere = GetWorldBoundingSphere(obj);
			if (CheckSphereCollision(pCamera->boundingSphere, worldSphere))
			{
				collided = true;
				break;
			}
		}

		if (collided)
		{
			pCamera->SetPosition(oldPosition);
			pCamera->SetSpeed(0.0f);
		}

		glm::vec3 cameraPosition = pCamera->GetPosition();
		glm::vec3 cameraForward = pCamera->GetForward();
		glm::vec3 rotationAngles = glm::vec3(pCamera->GetPitch(), pCamera->GetYaw(), pCamera->GetRoll()) + glm::vec3(-90.0f, 90.f, 0.0f);


		renderParticles(terrainShader);


		glm::vec3 airplanePosition = cameraPosition + cameraForward + glm::vec3(0.0f, -0.1f, -0.5f);
		
		// render planes
		renderModelPlane(terrainShader, airplane, airplanePosition, /*glm::vec3(0.0f, 180.f, 0.0f)*/rotationAngles, glm::vec3(0.1005f));
		renderModelParkedPlane(terrainShader, parkedAirplane, glm::vec3(-43.0f, -18.8f, -55.0f), glm::vec3(-90.0f, 20.f, -120.0f), glm::vec3(0.4188f));

		highFlyingAirplanePosition += glm::vec3(deltaTime * 2.0f, deltaTime, deltaTime);
		renderModelPlane(terrainShader, highFlyingAirplane, highFlyingAirplanePosition, glm::vec3(-90.0f, 0.f, -90.0f), glm::vec3(0.105f));

		highFlyingAirplanePosition2 += glm::vec3(-2.0f * deltaTime, deltaTime, 2.0f * deltaTime);
		renderModelPlane(terrainShader, highFlyingAirplane2, highFlyingAirplanePosition2, glm::vec3(-75.0f, 0.f, 45.0f), glm::vec3(0.1015f));

		renderModelPlane(terrainShader, landingPlane, landingPLanePosition, glm::vec3(-90.0f, 0.0f, 180.0f), glm::vec3(0.405f));
		if (landingPLanePosition.y > -19.5f)
			landingPLanePosition += glm::vec3(0.0f, -0.1f, 1.0f);

		if (landingPLanePosition.y <= -19.5f && landingPLanePosition.z < -45.0f)
			landingPLanePosition += glm::vec3(0.0f, 0.0f, 0.5f);

		if (landingPLanePosition.y <= -19.5f && landingPLanePosition.z < -40.0f && landingPLanePosition.z > -45.5f)
			landingPLanePosition += glm::vec3(0.0f, 0.0f, 0.25f);

		if (landingPLanePosition.y <= -19.5f && landingPLanePosition.z < -35.0f && landingPLanePosition.z > -40.5f)
			landingPLanePosition += glm::vec3(0.0f, 0.0f, 0.1f);

		// render turn
		renderModel(terrainShader, tower, initialPosition + glm::vec3(-4.0f, -21.4f, -217.0f), 90.0f, glm::vec3(1.3f));

		renderModel(terrainShader, road, initialPosition + glm::vec3(45.0f, -19.5f, -7.0f), 90.0f, glm::vec3(0.7f, 0.3f, 1.0f));

		renderModel(terrainShader, hangare, initialPosition + glm::vec3(-28.0f, -19.4f, -10.0f), 0.0f, glm::vec3(0.3f));


		// render teren
		renderTerrain(terrainShader, terrain, initialPositionTerrain + glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.01), terrainTexture);

		renderModel(terrainShader, hangare, initialPosition + glm::vec3(-55.0f, -19.4f, -55.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.3f));


		for (const auto& cloud1 : clouds) {
			renderModel(terrainShader, cloud, initialPosition + cloud1.position, glm::vec3(0.0f, cloud1.rotation, 0.0f), cloud1.scale);
		}
		for (auto& cloud : clouds) {
			cloud.position.z += cloud.speed * deltaTime;
		}


		lightingShader.SetVec3("objectColor", 0.5f, 1.0f, 0.31f);
		lightingShader.SetVec3("lightColor", glm::vec3(lightIntensity)); 
		lightingShader.SetVec3("lightPos", lightPos);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());
		lightingShader.setMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.setMat4("view", pCamera->GetViewMatrix());

		glm::mat4 projection = pCamera->GetProjectionMatrix();
		glm::mat4 view = pCamera->GetViewMatrix();

		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);

		glm::mat4 lightModel = glm::mat4(1.0f);
		lightModel = glm::translate(lightModel, glm::vec3(0.0f, 50.0f, 0.0f)); 
		lightModel = glm::scale(lightModel, glm::vec3(3.0f));                 

		lampShader.setMat4("model", lightModel);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		glBindTexture(GL_TEXTURE_2D, 0);

		//render skybox
		glDepthFunc(GL_LEQUAL);
		skyboxShader.use();
		projection = pCamera->GetProjectionMatrix();
		view = glm::mat4(glm::mat3(pCamera->GetViewMatrix()));
		skyboxShader.setMat4("projection", projection);
		skyboxShader.setMat4("view", view);

		skyboxShader.setVec3("skyColor", skyColor);

		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	Cleanup();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ROLL_LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ROLL_RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && pCamera->GetSpeed() >= pCamera->GetTakeoffSpeed()) {
		if (pCamera->IsGrounded()) {
			pCamera->StartFlying();
		}
	}
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}

void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale)
{

	ourShader.use();
	ourShader.SetVec3("objectColor", glm::vec3(0.0f));


	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, scale);

	glm::mat4 viewMatrix = pCamera->GetViewMatrix();
	glm::mat4 projectionMatrix = pCamera->GetProjectionMatrix();

	ourShader.setMat4("model", model);
	ourShader.setMat4("view", viewMatrix);
	ourShader.setMat4("projection", projectionMatrix);

	ourModel.Draw(ourShader);
}

void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale) {
	ourShader.use();
	ourShader.SetVec3("objectColor", glm::vec3(0.0f));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	model = glm::rotate(model, glm::radians(rotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotation around X axis
	model = glm::rotate(model, glm::radians(rotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotation around Y axis
	model = glm::rotate(model, glm::radians(rotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotation around Z axis

	model = glm::scale(model, scale);

	glm::mat4 viewMatrix = pCamera->GetViewMatrix();
	glm::mat4 projectionMatrix = pCamera->GetProjectionMatrix();

	ourShader.setMat4("model", model);
	ourShader.setMat4("view", viewMatrix);
	ourShader.setMat4("projection", projectionMatrix);

	ourModel.Draw(ourShader);
}

void renderModelPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale) {
	ourShader.use();
	ourShader.SetVec3("objectColor", glm::vec3(0.0f));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	model = glm::rotate(model, glm::radians(rotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); 
	model = glm::rotate(model, glm::radians(rotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); 
	model = glm::rotate(model, glm::radians(rotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); 

	model = glm::scale(model, scale);

	glm::mat4 viewMatrix = pCamera->GetViewMatrix();
	glm::mat4 projectionMatrix = pCamera->GetProjectionMatrix();

	model = model * g_planeFix;

	ourShader.setMat4("model", model);
	ourShader.setMat4("view", viewMatrix);
	ourShader.setMat4("projection", projectionMatrix);

	ourModel.Draw(ourShader);
}

void renderModelParkedPlane(Shader& ourShader, Model& ourModel, const glm::vec3& position, const glm::vec3& rotationAngles, const glm::vec3& scale) {
	ourShader.use();
	ourShader.SetVec3("objectColor", glm::vec3(0.0f));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	model = glm::rotate(model, glm::radians(rotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f)); 
	model = glm::rotate(model, glm::radians(rotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f)); 
	model = glm::rotate(model, glm::radians(rotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f)); 

	model = glm::scale(model, scale);

	glm::mat4 viewMatrix = pCamera->GetViewMatrix();
	glm::mat4 projectionMatrix = pCamera->GetProjectionMatrix();

	model = model * g_parkedPlaneFix;

	ourShader.setMat4("model", model);
	ourShader.setMat4("view", viewMatrix);
	ourShader.setMat4("projection", projectionMatrix);

	ourModel.Draw(ourShader);
}

void renderTerrain(Shader& terrainShader, Model& terrainModel, const glm::vec3& position, const glm::vec3& scale, int terrainTexture) {
	glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
	model = glm::scale(model, scale);
	terrainShader.use();
	terrainShader.setMat4("model", model);
	terrainShader.setMat4("view", pCamera->GetViewMatrix());
	terrainShader.setMat4("projection", pCamera->GetProjectionMatrix());
	glBindTexture(GL_TEXTURE_2D, terrainTexture);

	int gridWidth = 10;
	int gridDepth = 10;

	for (int x = 0; x < gridWidth; x++) {
		for (int z = 0; z < gridDepth; z++) {
			glm::vec3 pos = position + glm::vec3(x * scale.x * 2.0f, 0.0f, z * scale.z * 2.0f); 
			glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
			model = glm::scale(model, scale);
			terrainShader.setMat4("model", model);
			terrainModel.Draw(terrainShader);
		}
	}
}