#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform vec3 material_emission;

uniform int has_color_texture;
layout(binding = 0) uniform sampler2D colorMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;


vec3 calculateDirectIllumiunation(vec3 wo, vec3 n, vec3 base_color)
{
	vec3 direct_illum = base_color;
	///////////////////////////////////////////////////////////////////////////
	// Task 1.2 - Calculate the radiance Li from the light, and the direction
	//            to the light. If the light is backfacing the triangle,
	//            return vec3(0);
	///////////////////////////////////////////////////////////////////////////
	//direction from fragment to light
	vec3 wi = normalize(viewSpaceLightPosition-viewSpacePosition);
	//distance from fragment to light
	float d = length(viewSpaceLightPosition-viewSpacePosition);
	//Inverse-square falloff for attenuation
	float attenuation = 1.0/(d*d);
	//calculate radiance li
	vec3 li = point_light_intensity_multiplier*point_light_color*attenuation;
	///////////////////////////////////////////////////////////////////////////
	// Task 1.3 - Calculate the diffuse term and return that as the result
	///////////////////////////////////////////////////////////////////////////
	// vec3 diffuse_term = ...
	float ndotwi = max(dot(n,wi),0.001);//clamp to 0.0 when light is behind
	vec3 diffuse_term = base_color * (1.0/PI) *ndotwi *li;
	direct_illum=diffuse_term;
	///////////////////////////////////////////////////////////////////////////
	// Task 2 - Calculate the Torrance Sparrow BRDF and return the light
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////
	vec3 wh = normalize(wi+wo);
	float whdotwi = max(dot(wh,wi),0.001);
	float wodotwh = max(dot(wo,wh),0.001);//ensure not division 0
	//Calculate fresnel term
	float F = material_fresnel+(1-material_fresnel)*pow((1-whdotwi),5.0);
	//Calculate the microfacet distribution function
	float ndotwh = max(dot(n,wh),0.001);
	float D = ((material_shininess+2)/(2.0*PI))*pow(ndotwh,material_shininess);
	//Calculate the shadowing function
	float ndotwo =max(dot(n,wo),0.001);
	//float wodotwh = max(dot(wo,wh),0.001);//ensure not division 0
	float G = min(1,min(2*ndotwh*ndotwo/wodotwh,2*ndotwh*ndotwi/wodotwh));
	//Calculate brdf
	float denominator = 4*clamp(ndotwo*ndotwi,0.001,1.0);
	float brdf = F*D*G/denominator;
	//return vec3(D);
	//return brdf*ndotwi*li;
	//return brdf*ndotwi*li*base_color;//reason for no color
	///////////////////////////////////////////////////////////////////////////
	// Task 3 - Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////
	vec3 dielectric_term = brdf * ndotwi * li +(1-F)*diffuse_term;
	vec3 metal_term = brdf * base_color * ndotwi*li;
	direct_illum = material_metalness * metal_term+(1-material_metalness)*dielectric_term;
	return direct_illum;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n, vec3 base_color)
{
	vec3 indirect_illum = vec3(0.f);
	///////////////////////////////////////////////////////////////////////////
	// Task 5 - Lookup the irradiance from the irradiance map and calculate
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////
    // Convert view-space normal to world-space
	vec3 worldSpaceNormal =vec3((viewInverse * vec4(viewSpaceNormal, 0.0)));
	// Calculate the spherical coordinates of the direction
	float theta = acos(max(-1.0f, min(1.0f, worldSpaceNormal.y)));//polar angle of the world space normal
	float phi = atan(worldSpaceNormal.z, worldSpaceNormal.x);//azimuthal angle
	if(phi < 0.0f)
	{
		phi = phi + 2.0f * PI;
	}//If phi is negative, it¡¯s shifted into the positive range by adding 2PI

	// Convert spherical coordinates to texture coordinates
	vec2 lookup = vec2(phi / (2.0 * PI), 1 - theta / PI);
	 // Fetch irradiance from the irradiance map
    vec3 irradiance = texture(irradianceMap, lookup).rgb;
	vec3 diffuse_term = base_color * (1.0/PI) * environment_multiplier*irradiance;
	indirect_illum = diffuse_term;
	///////////////////////////////////////////////////////////////////////////
	// Task 6 - Look up in the reflection map from the perfect specular
	//          direction and calculate the dielectric and metal terms.
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = normalize(reflect(viewSpaceLightPosition,viewSpaceNormal));
	
	float roughness = sqrt(sqrt(2/(material_shininess+2)));
	vec3 li = environment_multiplier * textureLod(reflectionMap,lookup,roughness*7.0).rgb;
	vec3 wh = normalize(wi+wo);
	float whdotwi = max(dot(wh,wi),0.001);
	float F = material_fresnel+(1-material_fresnel)*pow((1-whdotwi),5.0);
	vec3 dielectric_term = F*li + (1-F)*diffuse_term ;
	vec3 metal_term = F * base_color *li;
	indirect_illum = material_metalness * metal_term+(1-material_metalness)*dielectric_term;
	return indirect_illum;
}


void main()
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.1 - Fill in the outgoing direction, wo, and the normal, n. Both
	//            shall be normalized vectors in view-space.
	///////////////////////////////////////////////////////////////////////////
	//vec3 wo = vec3(0.0);
	//vec3 n = vec3(0.0);
	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);
	vec3 base_color = material_color;
	if(has_color_texture == 1)
	{
		base_color *= texture(colorMap, texCoord).rgb;
	}

	vec3 direct_illumination_term = vec3(0.0);
	{ // Direct illumination
		direct_illumination_term = calculateDirectIllumiunation(wo, n, base_color);
	}

	vec3 indirect_illumination_term = vec3(0.0);
	{ // Indirect illumination
		indirect_illumination_term = calculateIndirectIllumination(wo, n, base_color);
	}

	///////////////////////////////////////////////////////////////////////////
	// Task 1.4 - Make glowy things glow!
	///////////////////////////////////////////////////////////////////////////
	//vec3 emission_term = vec3(0.0);
	vec3 emission_term = material_emission;
	vec3 final_color = direct_illumination_term + indirect_illumination_term + emission_term;

	// Check if we got invalid results in the operations
	if(any(isnan(final_color)))
	{
		final_color.rgb = vec3(1.f, 0.f, 1.f);
	}

	fragmentColor.rgb = final_color;
}
