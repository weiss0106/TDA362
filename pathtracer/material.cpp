#include "material.h"
#include "sampling.h"
#include "labhelper.h"

using namespace labhelper;

namespace pathtracer
{
WiSample sampleHemisphereCosine(const vec3& wo, const vec3& n)
{
	mat3 tbn = tangentSpace(n);
	vec3 sample = cosineSampleHemisphere();
	WiSample r;
	r.wi = tbn * sample;
	if(dot(r.wi, n) > 0.0f)
		r.pdf = max(0.0f, dot(r.wi, n)) / M_PI;
	return r;
}

///////////////////////////////////////////////////////////////////////////
// A Lambertian (diffuse) material
///////////////////////////////////////////////////////////////////////////
vec3 Diffuse::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	if(dot(wi, n) <= 0.0f)
		return vec3(0.0f);
	if(!sameHemisphere(wi, wo, n))
		return vec3(0.0f);
	return (1.0f / M_PI) * color;
}

WiSample Diffuse::sample_wi(const vec3& wo, const vec3& n) const
{
	WiSample r = sampleHemisphereCosine(wo, n);
	r.f = f(r.wi, wo, n);
	return r;
}

vec3 MicrofacetBRDF::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	vec3 wh = normalize(wi + wo);
	float wodotwh = max(dot(wo, wh), 0.001f);//ensure not division 0
	//Calculate the microfacet distribution function
	float ndotwh = max(dot(n, wh), 0.001f);
	float ndotwi = max(dot(n, wi), 0.001f);	
	float ndotwo = max(dot(n, wo), 0.001f);
	float D = ((shininess + 2.0f) / (2.0f * M_PI)) * pow(ndotwh, shininess);
	//Calculate the shadowing function

	float G = min(1.0f, min(2.0f * ndotwh * ndotwo / wodotwh, 2.0f * ndotwh * ndotwi / wodotwh));
	//Calculate brdf
	float denominator = 4.0 * clamp(ndotwo * ndotwi, 0.001f, 1.0f);
	float brdf = D * G / denominator;
	return brdf*vec3(1.0,1.0,1.0);
}

WiSample MicrofacetBRDF::sample_wi(const vec3& wo, const vec3& n) const
{
	//WiSample r = sampleHemisphereCosine(wo, n);
	//r.f = f(r.wi, wo, n);
	WiSample r;
	vec3 tangent = normalize(perpendicular(n));
	vec3 bitangent = normalize(cross(tangent, n));
	float phi = 2.0f * M_PI * randf();
	float cos_theta = pow(randf(), 1.0f / (shininess + 1));
	float sin_theta = sqrt(max(0.0f, 1.0f - cos_theta * cos_theta));
	vec3 wh = normalize(sin_theta * cos(phi) * tangent + sin_theta * sin(phi) * bitangent + cos_theta * n);

	float ndotwh = abs(dot(n, wh));
	float wodotwh = abs(dot(wo, wh));
	float pwh = (shininess + 1.0f) * pow(ndotwh, shininess) / (2 * M_PI);
	float pwi = pwh / (4 * wodotwh);
	r.pdf = pwi;
	r.wi = normalize(2 * dot(wh, wo) * wh - wo);
	r.f = f(r.wi, wo, n);
	return r;
}


float BSDF::fresnel(const vec3& wi, const vec3& wo) const
{
	vec3 wh = normalize(wi + wo);
	float whdotwi = max(0.001f, dot(wh, wi));
	float F = R0 + (1 - R0) * pow((1 - whdotwi), 5.0);
	return F;
}


vec3 DielectricBSDF::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	float f = BSDF::fresnel(wi, wo);
	vec3 brdf = reflective_material->f(wi, wo, n);
	vec3 btdf = transmissive_material->f(wi, wo, n);
	vec3 bsdf = f * brdf + (1 - f) * btdf;
	return bsdf;
}

WiSample DielectricBSDF::sample_wi(const vec3& wo, const vec3& n) const
{
	WiSample r;
	if (randf() < 0.5) {
		//Sample the BRDF
		r = reflective_material->sample_wi(wo, n);
		r.pdf *= 0.5;
		float F = BSDF::fresnel(r.wi, wo);
		r.f = r.f * F;
	}
	else {
		//Sample the BSDF
		r = transmissive_material->sample_wi(wo, n);
		r.pdf *= 0.5;
		float F = BSDF::fresnel(r.wi, wo);
		r.f = r.f * (1 - F);
	}
	//r = sampleHemisphereCosine(wo, n);
	//r.f = f(r.wi, wo, n);

	return r;
}

vec3 MetalBSDF::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	float f = BSDF::fresnel(wi, wo);
	vec3 brdf = reflective_material->f(wi, wo, n);
	vec3 bsdf = f * brdf * color;
	return bsdf;
}

WiSample MetalBSDF::sample_wi(const vec3& wo, const vec3& n) const
{
	WiSample r;
	//r = sampleHemisphereCosine(wo, n);
	//r.f = f(r.wi, wo, n);
	r = reflective_material->sample_wi(wo,n);
	float F = BSDF::fresnel(r.wi, n);
	r.f = r.f * F * color;
	return r;
}


vec3 BSDFLinearBlend::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	vec3 metal = bsdf0->f(wi, wo, n);
	vec3 di = bsdf1->f(wi, wo, n);
	vec3 linearmix = w * metal + (1 - w) * di;
	return linearmix;
}

WiSample BSDFLinearBlend::sample_wi(const vec3& wo, const vec3& n) const
{
	WiSample r;
	if (randf() < w) {
		r = bsdf0->sample_wi(wo, n);
		float F = BSDF::fresnel(r.wi, wo);
	}
	else {
		r = bsdf1->sample_wi(wo, n);
		float F = BSDF::fresnel(r.wi, wo);
	}
	return r;
}


#if SOLUTION_PROJECT == PROJECT_REFRACTIONS
///////////////////////////////////////////////////////////////////////////
// A perfect specular refraction.
///////////////////////////////////////////////////////////////////////////
vec3 GlassBTDF::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	//Determine whether the ray is entering or exiting the medium
	bool entering = dot(wo, n) > 0.0f;
	float etaI = entering ? 1.0f : ior;  // Refractive index of the incoming medium
	float etaT = entering ? ior : 1.0f;  // Refractive index of the outgoing medium
	vec3 normal = entering ? n : -n;    // Adjust the normal direction based on ray direction

	// consine the angle of incidence
	float cosThetaI = std::abs(dot(normal, wo));
	float F0 = pow((etaI - etaT) / (etaI + etaT), 2);

	//calculate fresnel
	float fresnel = F0 + (1.0f - F0) * pow(1.0f - cosThetaI, 5);

	//If wi and wo are in different hemispheres
	if (!sameHemisphere(wi, wo, n))
	{
		return vec3(fresnel); // Return the reflection coefficient
	}
	return vec3(0.0f); //Refrection direction is a delta distribution
	
}

WiSample GlassBTDF::sample_wi(const vec3& wo, const vec3& n) const
{
	WiSample r;

	float eta;
	glm::vec3 N;
	if(dot(wo, n) > 0.0f)
	{
		N = n;
		eta = 1.0f / ior;
	}
	else
	{
		N = -n;
		eta = ior;
	}

	// Alternatively:
	// d = dot(wo, N)
	// k = d * d (1 - eta*eta)
	// wi = normalize(-eta * wo + (d * eta - sqrt(k)) * N)

	// or

	// d = dot(n, wo)
	// k = 1 - eta*eta * (1 - d * d)
	// wi = - eta * wo + ( eta * d - sqrt(k) ) * N

	float w = dot(wo, N) * eta;
	float k = 1.0f + (w - eta) * (w + eta);
	if(k < 0.0f)
	{
		// Total internal reflection
		r.wi = reflect(-wo, n);
	}
	else
	{
		k = sqrt(k);
		r.wi = normalize(-eta * wo + (w - k) * N);
	}
	r.pdf = abs(dot(r.wi, n));
	r.f = vec3(1.0f, 1.0f, 1.0f);

	return r;
}

vec3 BTDFLinearBlend::f(const vec3& wi, const vec3& wo, const vec3& n) const
{
	return w * btdf0->f(wi, wo, n) + (1.0f - w) * btdf1->f(wi, wo, n);
}

WiSample BTDFLinearBlend::sample_wi(const vec3& wo, const vec3& n) const
{
	if(randf() < w)
	{
		WiSample r = btdf0->sample_wi(wo, n);
		return r;
	}
	else
	{
		WiSample r = btdf1->sample_wi(wo, n);
		return r;
	}
}

#endif
} // namespace pathtracer
