#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {

	// YOUR CODE HERE
	// FIXME: Implement Cube Map here
	double min = 0;
	int index = -1;
	glm::dvec3 coord_vals[6] = {glm::dvec3(1,0,0), glm::dvec3(-1,0,0), 
		glm::dvec3(0,1,0), glm::dvec3(0,-1,0), 
		glm::dvec3(0,0,1), glm::dvec3(0,0,-1)};
	for(int i = 0;i < 6; i++)
	{
		double t = 1.0/(glm::dot(coord_vals[i], r.d));
		if(t > 0 && (t < min || index == -1))
		{
			index = i;
			min = t;
		}
	}

	glm::dvec3 isection = r.d * min;

	glm::dvec2 proj(0,0);
	glm::dvec3 p = isection - (glm::dot(isection, coord_vals[index] * coord_vals[index]));

	if(index == 0 || index == 1)
	{
		proj = glm::dvec2(p.z,p.y);
	}
	if(index == 2 || index == 3)
	{
		proj = glm::dvec2(p.x,p.z);
	}
	if(index == 4 || index == 5)
	{
		proj = glm::dvec2(p.x,p.y);
	}
	glm::dvec2 uv = (proj + glm::dvec2(1, 1))/2.0;
	return tMap[index]->getMappedValue(uv);
}
