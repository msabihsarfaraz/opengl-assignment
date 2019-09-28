#version 330 core

// interpolated values from the vertex shaders
in vec3 vPosition;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vTexCoord;

// light and material structs
struct Light
{
	vec3 position;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	int type;
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
uniform bool isReflective = false;
uniform sampler2D uTextureSampler;
uniform sampler2D uNormalSampler;
uniform samplerCube uEnvironmentMap;
uniform float uAlpha = 1.0f;

// output data
out vec4 fColor;

void main()
{
	if(isReflective){
		vec3 normal = normalize(vNormal);
		vec3 tangent = normalize(vTangent);
		vec3 biTangent = normalize(cross(tangent, normal));
		vec3 normalMap = 2.0f * texture(uNormalSampler, vTexCoord).xyz - 1.0f;

		normal = normalize(mat3(tangent, biTangent, normal) * normalMap);

		vec3 L;

		// determine whether the light is a point light source or directional light
		if(uLight.type == 0)
			L = normalize((uViewMatrix * vec4(uLight.position, 1.0f)).xyz - vPosition);
		else
			L = normalize((uViewMatrix * vec4(-uLight.direction, 0.0f)).xyz);

		vec3 E = normalize(-vPosition);
		vec3 H = normalize(L + E);

		// calculate the ambient, diffuse and specular components
		vec3 ambient  = uLight.ambient * uMaterial.ambient;
		vec3 diffuse  = uLight.diffuse * uMaterial.diffuse * max(dot(L, normal), 0.0);
		vec3 specular = vec3(0.0f, 0.0f, 0.0f);

		if(dot(L, normal) > 0.0f)
			specular = uLight.specular * uMaterial.specular * pow(max(dot(normal, H), 0.0), uMaterial.shininess);

		vec3 reflectEnvMap = reflect(-E, normal);
		// set output color
		vec3 sColor = texture(uEnvironmentMap, reflectEnvMap).rgb;
		//sColor *= diffuse + specular + ambient;
		fColor = vec4(sColor,uAlpha);
		//fColor *= diffuse + specular + ambient;
	}else{
		vec3 normal = normalize(vNormal);
		vec3 tangent = normalize(vTangent);
		vec3 biTangent = normalize(cross(tangent, normal));
		vec3 normalMap = 2.0f * texture(uNormalSampler, vTexCoord).xyz - 1.0f;

		normal = normalize(mat3(tangent, biTangent, normal) * normalMap);

		vec3 L;

		// determine whether the light is a point light source or directional light
		if(uLight.type == 0)
			L = normalize((uViewMatrix * vec4(uLight.position, 1.0f)).xyz - vPosition);
		else
			L = normalize((uViewMatrix * vec4(-uLight.direction, 0.0f)).xyz);

		vec3 E = normalize(-vPosition);
		vec3 H = normalize(L + E);

		// calculate the ambient, diffuse and specular components
		vec3 ambient  = uLight.ambient * uMaterial.ambient;
		vec3 diffuse  = uLight.diffuse * uMaterial.diffuse * max(dot(L, normal), 0.0);
		vec3 specular = vec3(0.0f, 0.0f, 0.0f);

		if(dot(L, normal) > 0.0f)
			specular = uLight.specular * uMaterial.specular * pow(max(dot(normal, H), 0.0), uMaterial.shininess);

		// set output color
		vec3 sColor = diffuse + specular + ambient;
		sColor *= texture(uTextureSampler, vTexCoord).rgb;
		fColor = vec4(sColor,uAlpha);
	}
    
}