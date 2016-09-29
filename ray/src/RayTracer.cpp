// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <string.h> // for memset

#include <iostream>
#include <fstream>

using namespace std;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = true;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y, unsigned char *pixel, unsigned int ctr)
{

    // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) scene->intersectCache.clear();
    ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), pixel, ctr, glm::dvec3(1,1,1), ray::VISIBILITY);
    scene->getCamera().rayThrough(x,y,r);
    double dummy;

    glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth() , dummy);
    ret = glm::clamp(ret, 0.0, 1.0);

    return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j, unsigned int ctr)
{
	glm::dvec3 col(0,0,0);

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	col = trace(x, y, pixel, ctr);

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t )
{
	isect i;
	glm::dvec3 colorC;
	
	if(scene->intersect(r, i)) {
		// FINISHED!!!

		// An intersection occurred!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		const Material& m = i.getMaterial();
		colorC = m.shade(scene, r, i);

		if(!depth)
			return colorC;

		glm::dvec3 ray_pos = r.at(i.t);
		glm::dvec3 ray_dir = r.d;
		glm::dvec3 norm = i.N;

		//Why reflect when we shouldn't?
		if(glm::length(m.kr(i)) != 0)
		{
			glm::dvec3 R = glm::reflect(ray_dir, i.N);
			R = glm::normalize(R);
			ray reflect(ray_pos, R, ray::REFLECTION);
			glm::dvec3 col = traceRay(reflect, thresh, depth-1, t);
			colorC += col * m.kr(i);
		}
		//Why refract when we shouldn't?
		if(glm::length(m.kt(i)) != 0.0)
		{
			glm::dvec3 cos_i = -1.0 * ray_dir;
			double enter = glm::dot(cos_i, norm);
			double n_i;
			double n_t;
			double index;
			if(enter > 0.0)
			{
				n_i = 1.0002772;
				n_t = m.index(i);
				norm = i.N;
			}
			else
			{
				n_i = m.index(i);
				n_t = 1.0002772;
				norm = -1.0 * i.N;
			}
			index = n_i / n_t;
			if(enter == 0.0)
			{
				index = 0;
				norm = {0.0,0.0,0.0};
			}
			
			double tir = (1.0 - pow(index, 2) * (1.0 - (pow(enter, 2))));
			if(tir > 0.0)
			{
				glm::dvec3 T = glm::refract(ray_dir, norm, index);
				T = glm::normalize(T);
				ray refract(ray_pos, T, ray::REFRACTION);
				glm::dvec3 col = traceRay(refract, thresh, depth-1, t);
				colorC += col * m.kt(i);
			}
		}
		return colorC;
	} 
	else {
		// cout<<"\n\n No Intersection";
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		// 
		// FIXME: Add CubeMap support here.

		if(haveCubeMap())
		{
			// cout<<"\n\nWe have a cubemap!";
			return getCubeMap()->getColor(r);
		}

		return glm::dvec3(0.0, 0.0, 0.0);
	}
}

RayTracer::RayTracer()
	: scene(0), buffer(0), thresh(0), buffer_width(256), buffer_height(256), m_bBufferReady(false), cubemap (0)
{
}

RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
	if(cubemap)
		delete cubemap;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn ) {
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos ) path = ".";
	else path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
	Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if( !sceneLoaded() ) return false;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		delete[] buffer;
		buffer = new unsigned char[bufferSize];
	}
	memset(buffer, 0, w*h*3);
	m_bBufferReady = true;
}

void RayTracer::traceImage(int w, int h, int bs, double thresh)
{
	traceSetup(w, h);
	int i=0;
	int j=0;
	for(i;i<w;++i)
	{
		for(j;j<h;++j)
		{
			tracePixel(i,j,0);  //TRASH CTR VALUE
		}
		j = 0;
	}
}

int RayTracer::aaImage(int samples, double aaThresh)
{
	// FINISHED!!
	// FIXME: Implement Anti-aliasing here
	if(samples > 0)
	{
		double x_inc = (1.0/((double)buffer_width * (double)samples));
		double y_inc = (1.0/((double)buffer_height * (double)samples));
		double sample_x;
		double sample_y;
		int sample = 0;
		glm::dvec3 col = {0.0,0.0,0.0};
		glm::dvec3 neighbors[9];
		for(int i=1;i<buffer_width;i++)                                                                      
		{
			for(int j=1;j<buffer_height;j++)
			{
				col = getPixel(i, j);
				neighbors[0] = getPixel(i-1, j-1);
				neighbors[1] = getPixel(i-1, j);
				neighbors[2] = getPixel(i-1, j+1);
				neighbors[3] = getPixel(i, j-1);
				neighbors[4] = getPixel(i, j);
				neighbors[5] = getPixel(i, j+1);
				neighbors[6] = getPixel(i+1, j-1);
				neighbors[7] = getPixel(i+1, j);
				neighbors[8] = getPixel(i+1, j+1);
				for(int k=0;k<9;k++)
				{
					glm::dvec3 check;
					check = neighbors[k] - col;
					if(check[0] > aaThresh || check[1] > aaThresh || check[2] > aaThresh)
					{
						sample++;
					}
				}
				//We have samples to take!!
				if(sample > 0)
				{
					col = {0.0,0.0,0.0};
					double x = double(i - 0.5)/double(buffer_width);
					double y = double(j - 0.5)/double(buffer_height);
					unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
					for(int u=0; u<samples; u++)
					{
						sample_x = ((double) u) * ((double) x_inc) + x;
						for(int v=0; v<samples; v++)
						{
							sample_y = ((double) v) * ((double) y_inc) + y;
							col += trace(sample_x, sample_y, pixel, 0);
						}
					}
					col /= pow(samples, 2);
				}
				setPixel(i, j, col);
			}
		}
	}
	return 0;
}

bool RayTracer::checkRender()
{
	// YOUR CODE HERE
	// FIXME: Return true if tracing is done.
	//NO FIXING REQUIRED
	return true;
}

glm::dvec3 RayTracer::getPixel(int i, int j)
{
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
	return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * color[0]);
	pixel[1] = (int)( 255.0 * color[1]);
	pixel[2] = (int)( 255.0 * color[2]);
}

