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

int main() {


	return 0;
}