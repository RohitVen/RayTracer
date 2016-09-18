#include "material.h"
#include "ray.h"
#include "light.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

#include "../fileio/bitmap.h"
#include "../fileio/pngimage.h"

using namespace std;
extern bool debugMode;

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.
	//	if( debugMode )
	//		std::cout << "Debugging Phong code..." << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:

	glm::dvec3 color = {0,0,0};
	glm::dvec3 isect_pos = r.at(i.t);

	color += ke(i); //Emissivity
	color += (ka(i)*scene->ambient()); //Ambient

	for ( vector<Light*>::const_iterator litr = scene->beginLights(); 
			litr != scene->endLights(); 
			++litr )
	{
			Light *pLight = *litr; //Got a light?
			glm::dvec3 light_dir = pLight->getDirection(isect_pos);
			glm::dvec3 norm = i.N; //Here's the normal of the Intersection

			//Calculating diffusal
			double dot_prod = glm::dot(light_dir,norm);
			double to_add = glm::max(dot_prod,0.0); 
			glm::dvec3 diffusal = to_add * kd(i) 
			* pLight->getColor(); //Final diffusal

			//Calculating specular
			glm::dvec3 neg_light_dir = light_dir * -1.0; //Since the light is coming from the other direction
			glm::dvec3 R = neg_light_dir - (2.0 * (norm * neg_light_dir) * norm); // Equation to find a ray about a normal
			glm::normalize(R);

			glm::dvec3 V = (scene->getCamera().getEye() - isect_pos);
			glm::normalize(V);
			dot_prod = glm::dot(V, R);
			to_add = glm::max(dot_prod, 0.0);
			glm::dvec3 specular = ks(i) * pow(to_add, shininess(i)) 
			* pLight->getColor(); //Final specular

			//Calculating f atten
			glm::dvec3 f = 1.0/(0.25 + (0.25 * light_dir) + (0.25 * glm::dot(light_dir, light_dir)));
			glm::dvec3 min = glm::min({1.0,1.0,1.0}, f);

			//Add it all up!!
			color += (diffusal + specular) * min;
	}
	return color;
}

TextureMap::TextureMap( string filename ) {

	int start = (int) filename.find_last_of('.');
	int end = (int) filename.size() - 1;
	if (start >= 0 && start < end) {
		string ext = filename.substr(start, end);
		if (!ext.compare(".png")) {
			png_cleanup(1);
			if (!png_init(filename.c_str(), width, height)) {
				double gamma = 2.2;
				int channels, rowBytes;
				unsigned char* indata = png_get_image(gamma, channels, rowBytes);
				int bufsize = rowBytes * height;
				data = new unsigned char[bufsize];
				for (int j = 0; j < height; j++)
					for (int i = 0; i < rowBytes; i += channels)
						for (int k = 0; k < channels; k++)
							*(data + k + i + j * rowBytes) = *(indata + k + i + (height - j - 1) * rowBytes);
				png_cleanup(1);
			}
		}
		else
			if (!ext.compare(".bmp")) data = readBMP(filename.c_str(), width, height);
			else data = NULL;
	} else data = NULL;
	if (data == NULL) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue( const glm::dvec2& coord ) const
{
	// YOUR CODE HERE
	// 
	// In order to add texture mapping support to the 
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.

	if (0 == data)
      return glm::dvec3(1.0, 1.0, 1.0); //Taken from getPixelAt

	int x = (int)(coord[0] * width);
	int y = (int)(coord[1] * height);

	if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;   //Taken from getPixelAt

   	int pos = (y * width + x) * 3;
   	glm::dvec3 color(double(data[pos])/255.0, double(data[pos+1])/255.0, double(data[pos+2])/255.0);

	return color;
}


glm::dvec3 TextureMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return glm::dvec3(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return glm::dvec3(double(data[pos]) / 255.0, 
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0);
}

glm::dvec3 MaterialParameter::value( const isect& is ) const
{
    if( 0 != _textureMap )
        return _textureMap->getMappedValue( is.uvCoordinates );
    else
        return _value;
}

double MaterialParameter::intensityValue( const isect& is ) const
{
    if( 0 != _textureMap )
    {
        glm::dvec3 value( _textureMap->getMappedValue( is.uvCoordinates ) );
        return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
    }
    else
        return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}

