#include <cmath>
#include <float.h>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include "trimesh.h"
#include "../ui/TraceUI.h"
#include <iostream>
extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh()
{
	for( Materials::iterator i = materials.begin(); i != materials.end(); ++i )
		delete *i;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex( const glm::dvec3 &v )
{
    vertices.push_back( v );
}

void Trimesh::addMaterial( Material *m )
{
    materials.push_back( m );
}

void Trimesh::addNormal( const glm::dvec3 &n )
{
    normals.push_back( n );
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace( int a, int b, int c )
{
    int vcnt = vertices.size();

    if( a >= vcnt || b >= vcnt || c >= vcnt ) return false;

    TrimeshFace *newFace = new TrimeshFace( scene, new Material(*this->material), this, a, b, c );
    newFace->setTransform(this->transform);
    if (!newFace->degen) faces.push_back( newFace );


    // Don't add faces to the scene's object list so we can cull by bounding box
    // scene->add(newFace);
    return true;
}

const char* Trimesh::doubleCheck()
// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
{
    if( !materials.empty() && materials.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of materials.";
    if( !normals.empty() && normals.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of normals.";

    return 0;
}

bool Trimesh::intersectLocal(ray& r, isect& i) const
{
	typedef Faces::const_iterator iter;
	bool have_one = false;
	for( iter j = faces.begin(); j != faces.end(); ++j )
	{
		isect cur;
		if( (*j)->intersectLocal( r, cur ) )
		{
			if( !have_one || (cur.t < i.t) )
			{
				i = cur;
				have_one = true;
			}
		}
	}
	if( !have_one ) i.setT(1000.0);
	return have_one;
} 

bool TrimeshFace::intersect(ray& r, isect& i) const {
  return intersectLocal(r, i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
    //FINISHED!!!
    glm::dvec3 a_coords = parent->vertices[ids[0]];
    glm::dvec3 b_coords = parent->vertices[ids[1]];
    glm::dvec3 c_coords = parent->vertices[ids[2]];

    glm::dvec3 d = r.getDirection();
    glm::dvec3 p = r.getPosition();

    if(glm::dot(normal, d) == 0) //Ray is parallel to plane
    {
        return false;
    }

    double t = (dist - glm::dot(normal, p))/glm::dot(normal, d);

    if(t < RAY_EPSILON)
    {
        return false;
    }

    glm::dvec3 q = r.at(t);   //Here's our intersection!
    
    glm::dvec3 b_a_q = glm::cross(b_coords - a_coords, q - a_coords);
    glm::dvec3 c_b_q = glm::cross(c_coords - b_coords, q - b_coords);
    glm::dvec3 a_c_q = glm::cross(a_coords - c_coords, q - c_coords);
    glm::dvec3 a_b_c = glm::cross(b_coords - a_coords, c_coords - a_coords);
    b_a_q = glm::normalize(b_a_q);
    c_b_q = glm::normalize(c_b_q);
    a_c_q = glm::normalize(a_c_q);
    a_b_c = glm::normalize(a_b_c);
    double AreaQBC = glm::dot(c_b_q, normal);
    double AreaAQC = glm::dot(a_c_q, normal);
    double AreaABQ = glm::dot(b_a_q, normal);
    double AreaABC = glm::dot(a_b_c, normal);

    if(AreaQBC < 0 || AreaAQC < 0 || AreaABQ < 0)
    {
        return false;
    }

    double alpha = (AreaQBC)/(AreaABC);
    double beta = (AreaAQC)/(AreaABC);
    double gamma = (AreaABQ)/(AreaABC);

    if(alpha < RAY_EPSILON || beta < RAY_EPSILON || gamma < RAY_EPSILON)
    {
        return false;
    }
    i.setT(t);
    i.setBary(alpha, beta, gamma);
    i.setUVCoordinates({alpha, beta});
    i.setObject(this);

    if(parent->vertNorms) //Interpolating intersection Normal if parent has vertNorms
    {
        glm::dvec3 norma = parent->normals[ids[0]];
        glm::dvec3 normb = parent->normals[ids[1]];
        glm::dvec3 normc = parent->normals[ids[2]];
        i.N = (norma * alpha) + (normb * beta) + (normc * gamma);
        i.N = glm::normalize(i.N);
    }
    else
    {
        i.N = normal;
        i.N = glm::normalize(i.N);
    }

    if(parent->materials.empty()) //Interpolating materials as necessary
    {
            i.setMaterial(getMaterial());
    }
    else
    {
        Material mata = (*parent->materials[ids[0]]);
        Material matb = (*parent->materials[ids[1]]);
        Material matc = (*parent->materials[ids[2]]);

        Material mat = (alpha * mata);
        mat += (beta * matb);
        mat += (gamma * matc);

        i.setMaterial(mat);
    }

    return true;
}

void Trimesh::generateNormals()
// Once you've loaded all the verts and faces, we can generate per
// vertex normals by averaging the normals of the neighboring faces.
{
    int cnt = vertices.size();
    normals.resize( cnt );
    int *numFaces = new int[ cnt ]; // the number of faces assoc. with each vertex
    memset( numFaces, 0, sizeof(int)*cnt );
    
    for( Faces::iterator fi = faces.begin(); fi != faces.end(); ++fi )
    {
		glm::dvec3 faceNormal = (**fi).getNormal();
        
        for( int i = 0; i < 3; ++i )
        {
            normals[(**fi)[i]] += faceNormal;
            ++numFaces[(**fi)[i]];
        }
    }

    for( int i = 0; i < cnt; ++i )
    {
        if( numFaces[i] )
            normals[i]  /= numFaces[i];
    }

    delete [] numFaces;
    vertNorms = true;
}

