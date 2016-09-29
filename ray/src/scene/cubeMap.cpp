#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {

	// YOUR CODE HERE
	// FIXME: Implement Cube Map here
	double absX = fabs(r.d[0]);
	double absY = fabs(r.d[1]);
	double absZ = fabs(r.d[2]);
	double max = glm::max(absX, absY);
	max = glm::max(max, absZ);
	//Pos X and max
	if(r.d[0] > 0 && max == absX)
	{
		double x = r.d[1]/max;
		double y = r.d[2]/max;
		glm::dvec2 proj = (glm::dvec2(y, x) + glm::dvec2(1,1)) / 2.0;
		return tMap[0]->getMappedValue(proj);
	}
	//Neg X and max
	if(r.d[0] < 0 && max == absX)
	{
		double x = r.d[1]/max;
		double y = r.d[2]/max;
		glm::dvec2 proj = (glm::dvec2(-1.0*y, x) + glm::dvec2(1,1)) / 2.0;
		return tMap[1]->getMappedValue(proj);
	}
	//Pos Y and max
	if(r.d[1] > 0 && max == absY)
	{
		double x = r.d[0]/max;
		double y = r.d[2]/max;
		glm::dvec2 proj = (glm::dvec2(x ,y) + glm::dvec2(1,1)) / 2.0;
		return tMap[2]->getMappedValue(proj);
	}
	//Neg Y and max
	if(r.d[1] < 0 && max == absY)
	{
		double x = r.d[0]/max;
		double y = r.d[2]/max;
		glm::dvec2 proj = (glm::dvec2(x ,-1.0*y) + glm::dvec2(1,1)) / 2.0;
		return tMap[3]->getMappedValue(proj);
	}
	//Pos Z and max
	if(r.d[2] > 0 && max == absZ)
	{
		double x = r.d[0]/max;
		double y = r.d[1]/max;
		glm::dvec2 proj = (glm::dvec2(-1.0*x ,y) + glm::dvec2(1,1)) / 2.0;
		return tMap[5]->getMappedValue(proj);
	}
	//Neg Z and max
	if(r.d[2] < 0 && max == absZ)
	{
		double x = r.d[0]/max;
		double y = r.d[1]/max;
		glm::dvec2 proj = (glm::dvec2(x ,y) + glm::dvec2(1,1)) / 2.0;
		return tMap[4]->getMappedValue(proj);
	}
}
