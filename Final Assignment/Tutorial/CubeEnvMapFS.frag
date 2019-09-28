#version 330 core

// interpolated values from the vertex shaders
in vec3 vPosition;
in vec3 vNormal;

// light and material structs
struct Light
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

// uniform input data
uniform mat4 uViewMatrix;
uniform Light uLight;
uniform Material uMaterial;
uniform samplerCube uEnvironmentMap;

// output data
out vec3 fColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize((uViewMatrix * vec4(uLight.position, 1.0f)).xyz - vPosition);;
    vec3 E = normalize(-vPosition);
    vec3 H = normalize(L + E);

	// calculate the ambient, diffuse and specular components
	vec3 ambient  = uLight.ambient * uMaterial.ambient;
    vec3 diffuse  = uLight.diffuse * uMaterial.diffuse * max(dot(L, N), 0.0);
	vec3 specular = vec3(0.0f, 0.0f, 0.0f);

	if(dot(L, N) > 0.0f)
	    specular = uLight.specular * uMaterial.specular * pow(max(dot(N, H), 0.0), uMaterial.shininess);

	vec3 reflectEnvMap = reflect(-E, N);

	// set output color
	fColor = texture(uEnvironmentMap, reflectEnvMap).rgb;
	fColor *= (diffuse + specular + ambient);
}