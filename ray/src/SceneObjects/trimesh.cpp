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
    double tmin = 0.0;
    double tmax = 0.0;
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
    glm::dvec3 a_coords = parent->vertices[0];
    glm::dvec3 b_coords = parent->vertices[1];
    glm::dvec3 c_coords = parent->vertices[2];
    glm::dvec3 vab = (b_coords - a_coords);
    glm::dvec3 vac = (c_coords - a_coords);
    glm::dvec3 vcb = (b_coords - c_coords);

    // cout<<"\n\nValue of a: "<<a_coords[0]<<" "<<a_coords[1]<<" "<<a_coords[2];
    // cout<<"\n\nValue of b: "<<b_coords[0]<<" "<<b_coords[1]<<" "<<b_coords[2];
    // cout<<"\n\nValue of c: "<<c_coords[0]<<" "<<c_coords[1]<<" "<<c_coords[2];

    glm::dvec3 normal = glm::cross(vab, vac);
    // cout<<"\n\nValue of normal: "<<normal[0]<<" "<<normal[1]<<" "<<normal[2];
    glm::normalize(normal);
    // cout<<"\n\nValue of normalize: "<<normal[0]<<" "<<normal[1]<<" "<<normal[2];
    glm::dvec3 d = r.getDirection();
    glm::dvec3 p = r.getPosition();

    double t = (glm::dot(normal, p+d))/glm::dot(normal, d);

    if(t < RAY_EPSILON)
    {
        cout<<"\n\nBEHIND!!";
        return false;
    }
    // cout<<"\n\nValue of p: "<<p[0]<<" "<<p[1]<<" "<<p[2];
    // cout<<"\nValue of t: "<<t;
    glm::dvec3 isection = p + (t * d);
    double viu = glm::dot(isection, vab);
    double viv = glm::dot(isection, vac);

    //Get barycentrics
    double area = glm::dot(glm::dot(vab,vac),glm::dot(vab, vac)) - glm::dot(glm::dot(vab,vab), glm::dot(vac,vac));
    double beta = (glm::dot(glm::dot(vab,vac),viv) - glm::dot(glm::dot(vac,vac),viu))/area;
    double gamma = (glm::dot(glm::dot(vab,vac),viu) - glm::dot(glm::dot(vab,vab),viv))/area;
    double alpha = 1.0 - (beta + gamma);

    if(alpha < 0.0 || beta < 0.0 || gamma < 0.0)
    {
        return false;
    }
        i.setT(t);
	i.
 
    // cout<<"\n\nt's x, y, z: "<<t[0]<<" "<<t[1]<<" "<<t[2];
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

