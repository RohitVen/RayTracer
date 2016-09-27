#include <cmath>

#include "light.h"
#include <glm/glm.hpp>


using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


glm::dvec3 DirectionalLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
	// FINISHED!!!
	// You should implement shadow-handling code here.
	ray shadow(p, getDirection(p), ray::SHADOW);
	isect i;
	if(scene->intersect(shadow, i))
	{
		return glm::dvec3(0.0,0.0,0.0);
	}
	else
		return glm::dvec3(1.0,1.0,1.0);
}

glm::dvec3 DirectionalLight::getColor() const
{
	return color;
}

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const
{
	return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3& P) const
{

	// FINISHED!!

	// You'll need to modify this method to attenuate the intensity 
	// of the light based on the distance between the source and the 
	// point P.  For now, we assume no attenuation and just return 1.0

	glm::dvec3 dist = P - position;
	double d = glm::length(dist);
	double atten = 1.0 / (constantTerm + (linearTerm * d) + (quadraticTerm * d * d));
	double min = glm::min(1.0, atten);
	return min;
}

glm::dvec3 PointLight::getColor() const
{
	return color;
}

glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const
{
	return glm::normalize(position - P);
}


glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
	// FINISHED!!!
	// You should implement shadow-handling code here.

	isect i;
	ray shadow(p, getDirection(p), ray::SHADOW);

	//Intersection and before light source
	if(scene->intersect(shadow, i) && (i.t < glm::length(position - p)))
	{
		return glm::dvec3(0.0,0.0,0.0);
	}
	else
		return glm::dvec3(1.0,1.0,1.0);
}

