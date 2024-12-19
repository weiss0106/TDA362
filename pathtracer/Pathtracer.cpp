#include "Pathtracer.h"
#include <memory>
#include <iostream>
#include <map>
#include <algorithm>
#include "material.h"
#include "embree.h"
#include "sampling.h"
#include "labhelper.h"
#include <random>

using namespace std;
using namespace glm;
using namespace labhelper;

namespace pathtracer
{
///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////
Settings settings;
Environment environment;
Image rendered_image;
PointLight point_light;
std::vector<DiscLight> disc_lights;

///////////////////////////////////////////////////////////////////////////
// Restart rendering of image
///////////////////////////////////////////////////////////////////////////
void restart()
{
	// No need to clear image,
	rendered_image.number_of_samples = 0;
}

int getSampleCount()
{
	return std::max(rendered_image.number_of_samples - 1, 0);
}

///////////////////////////////////////////////////////////////////////////
// On window resize, window size is passed in, actual size of pathtraced
// image may be smaller (if we're subsampling for speed)
///////////////////////////////////////////////////////////////////////////
void resize(int w, int h)
{
	rendered_image.width = w / settings.subsampling;
	rendered_image.height = h / settings.subsampling;
	rendered_image.data.resize(rendered_image.width * rendered_image.height);
	restart();
}

///////////////////////////////////////////////////////////////////////////
/// Return the radiance from a certain direction wi from the environment
/// map.
///////////////////////////////////////////////////////////////////////////
vec3 Lenvironment(const vec3& wi)
{
	const float theta = acos(std::max(-1.0f, std::min(1.0f, wi.y)));
	float phi = atan(wi.z, wi.x);
	if(phi < 0.0f)
		phi = phi + 2.0f * M_PI;
	vec2 lookup = vec2(phi / (2.0 * M_PI), 1 - theta / M_PI);
	return environment.multiplier * environment.map.sample(lookup.x, lookup.y);
}

///////////////////////////////////////////////////////////////////////////
/// Calculate the radiance going from one point (r.hitPosition()) in one
/// direction (-r.d), through path tracing.
///////////////////////////////////////////////////////////////////////////
vec3 Li(Ray& primary_ray)
{
	vec3 L = vec3(0.0f);
	vec3 path_throughput = vec3(1.0);
	Ray current_ray = primary_ray;

	///////////////////////////////////////////////////////////////////
	// Get the intersection information from the ray
	///////////////////////////////////////////////////////////////////
	int bounces = 0;
	for (bounces = 0;bounces < settings.max_bounces;bounces++){
		//Get the intersection information from the ray
		Intersection hit = getIntersection(current_ray);
		//Create a Material tree
		//Diffuse diffuse(hit.material->m_color);
		//BTDF& mat = diffuse;
		/*MicrofacetBRDF microfacet(hit.material->m_shininess);
		DielectricBSDF dielectric(&microfacet, &diffuse, hit.material->m_fresnel);
		MetalBSDF metal(&microfacet, hit.material->m_color, hit.material->m_fresnel);
		BSDFLinearBlend metal_blend(hit.material->m_metalness, &metal, &dielectric);
		BSDF& mat = metal_blend;*/

		//Glass refrection
		
		GlassBTDF glass(hit.material->m_ior);
		Diffuse diffuse(hit.material->m_color);
		BTDFLinearBlend glassblend(hit.material->m_transparency, &glass, &diffuse);
		BTDF& mat = glassblend;
		
		//Calculate direct illumination
		Ray hit2lightray;
		hit2lightray.o = hit.position + EPSILON * hit.shading_normal;
		hit2lightray.d = normalize(point_light.position - hit.position);
		if (!occluded(hit2lightray)) {
			const float distance_to_light = length(point_light.position - hit.position);
			const float falloff_factor = 1.0f / (distance_to_light * distance_to_light);
			vec3 Li = point_light.intensity_multiplier * point_light.color * falloff_factor;
			vec3 wi = normalize(point_light.position - hit.position);
			L += path_throughput * mat.f(wi, hit.wo, hit.shading_normal) * Li * std::max(0.0f, dot(wi, hit.shading_normal));
		}
		L += path_throughput * hit.material->m_emission;

		//sample an incoming direction
		WiSample r = mat.sample_wi(hit.wo, hit.shading_normal);
		//if the pdf is too close to zero,the current path is unlikely to exist
		//avoid numerical instability
		if (r.pdf<EPSILON) {
			return L;
		}
		float cosineterm = abs(dot(r.wi, hit.shading_normal));
		path_throughput = path_throughput * (r.f * cosineterm) / r.pdf;
		if (path_throughput == vec3(0.0f, 0.0f, 0.0f)) {
			return L;
		}
		// Create next ray on path
		Ray currentray;
		current_ray = currentray;
		current_ray.o = hit.position;
		current_ray.d = r.wi;
		// Bias the ray slightly to avoid self-intersection 
		if (dot(r.wi, hit.geometry_normal) < 0)
			current_ray.o -= EPSILON * hit.geometry_normal;
		else
			current_ray.o += EPSILON * hit.geometry_normal;
		//if there no intersection add environment contribution and finish
		if (!intersect(current_ray))
			return L + path_throughput * Lenvironment(current_ray.d);

	}
	return L;
	//Intersection hit = getIntersection(current_ray);
	/////////////////////////////////////////////////////////////////////
	//// Create a Material tree for evaluating brdfs and calculating
	//// sample directions.
	/////////////////////////////////////////////////////////////////////
	//
	//Diffuse diffuse(hit.material->m_color);
	//MicrofacetBRDF microfacet(hit.material->m_shininess);
	//DielectricBSDF dielectric(&microfacet, &diffuse, hit.material->m_fresnel);
	//MetalBSDF metal(&microfacet, hit.material->m_color, hit.material->m_fresnel);
	//BSDFLinearBlend metal_blend(hit.material->m_metalness, &metal, &dielectric);
	////BTDF& mat = diffuse;
	////BSDF& mat = dielectric;
	//BSDF& mat = metal_blend;
	/////////////////////////////////////////////////////////////////////
	//// Calculate Direct Illumination from light.
	/////////////////////////////////////////////////////////////////////
	//{
	//	const float distance_to_light = length(point_light.position - hit.position);
	//	const float falloff_factor = 1.0f / (distance_to_light * distance_to_light);
	//	//vec3 Li = point_light.intensity_multiplier * point_light.color * falloff_factor;
	//	vec3 wi = normalize(point_light.position - hit.position);
	//	//Create a shadow ray from the hit point
	//	Ray shadow_ray;
	//	shadow_ray.o = hit.position + hit.shading_normal * EPSILON;
	//	shadow_ray.d = wi;
	//	//Check if the path to the light is occluded
	//	if (!occluded(shadow_ray)) {
	//		//If not occluded,calculate the light contribution
	//		float falloff_factor = 1.0f / (distance_to_light * distance_to_light);
	//		vec3 Li = point_light.intensity_multiplier * point_light.color * falloff_factor;		
	//		L = mat.f(wi, hit.wo, hit.shading_normal) * Li * std::max(0.0f, dot(wi, hit.shading_normal));
	//	}
	//}
	// Return the final outgoing radiance for the primary ray
	//return L;
}

///////////////////////////////////////////////////////////////////////////
/// Used to homogenize points transformed with projection matrices
///////////////////////////////////////////////////////////////////////////
inline static glm::vec3 homogenize(const glm::vec4& p)
{
	return glm::vec3(p * (1.f / p.w));
}

///////////////////////////////////////////////////////////////////////////
/// Trace one path per pixel and accumulate the result in an image
///////////////////////////////////////////////////////////////////////////
void tracePaths(const glm::mat4& V, const glm::mat4& P)
{
	// Stop here if we have as many samples as we want
	if((int(rendered_image.number_of_samples) > settings.max_paths_per_pixel)
	   && (settings.max_paths_per_pixel != 0))
	{
		return;
	}
	vec3 camera_pos = vec3(glm::inverse(V) * vec4(0.0f, 0.0f, 0.0f, 1.0f));
	// Trace one path per pixel (the omp parallel stuf magically distributes the
	// pathtracing on all cores of your CPU).
	int num_rays = 0;
	vector<vec4> local_image(rendered_image.width * rendered_image.height, vec4(0.0f));

#pragma omp parallel for
	for(int y = 0; y < rendered_image.height; y++)
	{
		for(int x = 0; x < rendered_image.width; x++)
		{
			vec3 color;
			Ray primaryRay;
			primaryRay.o = camera_pos;
			// Create a ray that starts in the camera position and points toward
			// the current pixel on a virtual screen.
			//Task1: Random offset within the pixel
			// Jittered screen coordinates
			vec2 screenCoord = vec2(
				(float(x) + randf()) / float(rendered_image.width),
				(float(y) + randf()) / float(rendered_image.height)
			);

			//vec2 screenCoord = vec2(float(x) / float(rendered_image.width),
			//                        float(y) / float(rendered_image.height));
			// Calculate direction
			vec4 viewCoord = vec4(screenCoord.x * 2.0f - 1.0f, screenCoord.y * 2.0f - 1.0f, 1.0f, 1.0f);
			vec3 p = homogenize(inverse(P * V) * viewCoord);
			primaryRay.d = normalize(p - camera_pos);
			// Intersect ray with scene
			if(intersect(primaryRay))
			{
				// If it hit something, evaluate the radiance from that point
				color = Li(primaryRay);
			}
			else
			{
				// Otherwise evaluate environment
				color = Lenvironment(primaryRay.d);
			}
			// Accumulate the obtained radiance to the pixels color
			float n = float(rendered_image.number_of_samples);
			rendered_image.data[y * rendered_image.width + x] =
			    rendered_image.data[y * rendered_image.width + x] * (n / (n + 1.0f))
			    + (1.0f / (n + 1.0f)) * color;
		}
	}
	rendered_image.number_of_samples += 1;
}
}; // namespace pathtracer
