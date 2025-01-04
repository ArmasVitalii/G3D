#include <GL/glew.h>
#include <GLM.hpp>



//We'll need them for shader reading
#include <stdlib.h> 
#include <stdio.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

struct BoundingSphere
{
	glm::vec3 center;
	float radius;

};


// SETTINGS (As a upgrade we could store them in a file and set up before app launch)

// Time of day ( between [0.0, 24.0] )
float timeOfDay = 6.0f;

// Duration of a day (in seconds)
const float dayDuration = 60.0f;

//Screen settings (We'll be implemented soon)
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



struct Cloud {
	glm::vec3 position;
	glm::vec3 scale;
	float rotation;
	float speed;
};

class Camera
{
private:
	//default camera values
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;
	const float MAX_YAW = -60.0f;
	const float MIN_YAW = -120.0f;
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
		// If center of plane is camera position, that’s enough:
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
		//speed -= acceleration * deltaTime;
		speed -= 0.05f;
		//if (speed < minSpeed) speed = minSpeed;
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

		// define the viewport transformation
		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::mat4 GetViewMatrix() const
	{
		// Returns the View Matrix
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

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
	{
		float rotationSpeed = 45.0f * deltaTime;  // Rotation speed factor
		float movementSpeed = 10.0f * deltaTime;  //

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
			if (yaw > MAX_YAW)
				yaw = MAX_YAW;
			if (yaw < MIN_YAW)
				yaw = MIN_YAW;
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

			// Modifică pitch-ul pentru a "ridica botul"
			pitch += pitchIncrement * deltaTime;
			if (pitch > 20.0f) pitch = 20.0f; // Limităm pitch-ul la 20 de grade pentru decolare
			glm::vec3 newPosition = position + forward * GetSpeed() * deltaTime * 10.0f;

			// Verifică dacă noua poziție este sub înălțimea minimă permisă
			if (newPosition.y < -20.0f) {
				newPosition.y = 0.0F; // Asigură-te că camera nu coboară sub plan
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
		//speed = 0.0f;
	}

	//float GetSpeed() const { return speed; }
	bool IsFlying() const { return isFlying; }

	private:
		void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
		{
			yaw += xOffset;
			pitch += yOffset;

			// Avem grijã sã nu ne dãm peste cap
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

			// Se modificã vectorii camerei pe baza unghiurilor Euler
			UpdateCameraVectors();
		}

		void UpdateCameraVectors() {
			// Calculate the new forward vector
			this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			this->forward.y = sin(glm::radians(pitch));
			this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			this->forward = glm::normalize(this->forward);

			// Calculate new Right and Up vectors
			this->right = glm::normalize(glm::cross(forward, worldUp));  // Adjust right vector
			this->up = glm::normalize(glm::cross(right, forward));  // Recompute up vector

			// Apply roll rotation around the forward axis if needed
			glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(roll), forward);
			this->right = glm::normalize(glm::vec3(rollMatrix * glm::vec4(this->right, 0.0f)));
			this->up = glm::normalize(glm::cross(right, forward));
		}

		protected:
			const float cameraSpeedFactor = 2.5f;
			//const float cameraSpeedFactor = 1.5f;
			const float mouseSensitivity = 0.1f;

			// Perspective properties
			float zNear;
			float zFar;
			float FoVy;
			int width;
			int height;
			bool isPerspective;
			float roll = 0.0f;  // Roll angle in degrees
			float speed = 0.01f;       // Viteza curentă a avionului
			float acceleration = 0.1f; // Accelerarea avionului
			float maxSpeed = 10.0f;     // Viteza maximă
			float minSpeed = 0.0f;     // Viteza minimă, poate fi zero pentru a opri complet
			bool isFlying = false;
			bool grounded = true;// Dacă avionul este în aer sau nu
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


int main() {


	return 0;
}