// diffuse component of light by Lambertian reflectance
float lambertTerm(vec3 normal, vec3 lightVec) {
	return max( dot( normal, lightVec ), 0.0f);
}

// specular component of light by Blinn-Phong BRDF
float blinnTerm(vec3 normal, vec3 halfVector, float specularMap, float reflectionValue)
{
	float blinnTerm = dot(normal, halfVector);
	float specularReflection = reflectionValue * (1.0f - specularMap * specularMap);
	return clamp(pow(blinnTerm, specularReflection), 0.0f, 1.0f);
}
