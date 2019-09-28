#version 330 core

// input data (different for all executions of this shader)
in vec3 aPosition;
in vec3 aNormal;

// uniform input data
uniform mat4 uModelViewProjectionMatrix;
uniform mat4 uModelViewMatrix;

// output data (will be interpolated for each fragment)
out vec3 vPosition;
out vec3 vNormal;

void main()
{
	// set vertex position
    gl_Position = uModelViewProjectionMatrix * vec4(aPosition, 1.0);

	// eye/camera space
	vPosition = (uModelViewMatrix * vec4(aPosition, 1.0)).xyz;
	vNormal = (uModelViewMatrix * vec4(aNormal, 0.0)).xyz;
}

