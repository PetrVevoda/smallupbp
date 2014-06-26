/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __OBJREADER_HXX__
#define __OBJREADER_HXX__

#include <string>
#include <vector>

#include "Defs.hxx"

namespace ObjReader
{

typedef std::vector< unsigned int > Indices;
typedef std::vector< float > floats;

/// Material info
struct Material
{
	std::string name;           /* name of material */
	float diffuse[3];           /* diffuse component */
	float ambient[3];           /* ambient component */
	float specular[3];          /* specular component */
	float emmissive[3];         /* emissive component */
	float shininess;            /* specular exponent 0 - 1000 */
	// auxiliary
	int mediumId;				/* medium index (-1 for no medium)*/
	int enclosingMatId;         /* index of material enclosing emissive material of area light (-1 for light in global medium) */
	GeometryType geometryType;	/* Geometry type of triangles with this material */
	bool isEmissive;		    /* true for non-zero emission */
	float IOR;					/* index of refraction (-1 for non-transmissive)*/
	float mirror[3];			/* mirror component */
	int priority;				/* material priority (default is -1)*/
	//this is for textures
	unsigned int IDTexture;		// texture id
};

typedef std::vector<Material > Materials;

/// Medium info
struct Medium
{
	std::string name;           /* name of medium */
	float absorptionCoef[3];    /* absorption coefficient */
	float emissionCoef[3];      /* emission coefficient */
	float scatteringCoef[3];    /* scattering coefficient */
	float continuationProbability;/*cont. probability, default is -1 -> must be changed in scene.hxx for albedo*/
	float meanCosine;			/* g - mean cosine */
};

typedef std::vector<Medium > Media;

/// Triangle structure
struct Triangle {
	unsigned int vindices[3];           /* array of triangle vertex indices */
	unsigned int nindices[3];           /* array of triangle normal indices */
	unsigned int tindices[3];           /* array of triangle texture coordinate indices*/
	unsigned int findex;                /* index of triangle facet normal */
};

typedef std::vector<Triangle > Triangles;

/// Texture info
struct Texture {
	std::string name;					/* name of this texture */
	unsigned int id;                    /* texture ID */
};

typedef std::vector<Texture > Textures;

/// Group of vertices
struct Group {
	std::string name;					/* name of this group */
	Indices triangles;					/* array of triangle indices */
	unsigned int material;				/* index to material for group */
};

typedef std::vector<Group > Groups;

/// Camera info
struct Camera {
	float origin[3];					/* Camera origin */
	float target[3];					/* Camera target */
	float roll[3];						/* Camera roll direction */
	float horizontalFOV;				/* Horizontal field of view */
	float focalDistance;				/* Focal distance */
	int resolutionX;					/* Camera resolution X*/
	int resolutionY;					/* Camera resolution Y*/
	int material;   			     	/* Index to material enclosing medium the camera is located in, -1 for camera in global medium */
};

enum LightType
{
	DIRECTIONAL = 0,
	POINT = 1,
	BACKGROUND = 2
};

/// Additional light info
struct AdditionalLight {
	float position[3];					/* stores either position for point light or direction for directional light*/
	float emission[3];					/* light emission */
	LightType lightType;				/* Type of light */
	std::string envMap;					/* environment map filename for background light */
	float envMapScale;					/* scaling of env map */
	float envMapRotate;					/* rotation of env map around vertical axis (0-1, 1 = 360 degrees)*/
};

typedef std::vector< AdditionalLight > Lights;

/// Obj file
class ObjFile {
public:
	// Loads obj file
	ObjFile(const char * filename);

	~ObjFile() {}

	// Getter functions
	const floats & vertices() const { return m_vertices; }

	const floats & normals() const { return m_normals; }

	const floats & texcoords() const { return m_texcoords; }

	const Triangles & triangles() const { return m_triangles; }

	const Materials & materials() const { return m_materials; }

	const Media & media() const { return m_media; }

	const Groups & groups() const { return m_groups; }

	const Textures & textures() const { return m_textures; }

	const Lights & lights() const { return m_lights; }
	
	int globalMediumId() const { return m_globalMediumId; }

	const Camera & camera() const { return m_camera; }
private:

	/// Reads obj from a selected file
	void readObj(FILE * file);

	/// Reads material file
	void readMtl(const std::string & filename);

	/// Reads aux file
	void readAux(const std::string & filename);

	/// Returns id of a selected material
	unsigned int findMaterial(const char* name) const;

	/// Returns id of a selected group, if it does not exist, new is created
	Group * findGroup(const char* name);

	/// Returns id of a selected medium, if it does not exist, new is created
	Medium * findMedium(const char* name);

	/// Returns id of a selected texture, if it does not exist, new is created
	unsigned int findTexture(const char* name);

	/// Fills camera info from matrix
	void fillCameraInfo(float * row0, float * row1, float * row2, float * row3 );

	std::string   m_pathname;            /* path to this model */
	std::string   m_mtllibname;          /* name of the material library */

	floats m_vertices;            /* array of vertices  */

	floats m_normals;  /* array of normals */

	floats m_texcoords;           /* array of texture coordinates */

	Triangles m_triangles;       /* array of triangles */

	Materials m_materials;       /* array of materials */

	Media m_media;			/* array of media */

	Groups m_groups;          /* array of groups */

	Textures m_textures; /* array of textures */

	Lights m_lights;	/* array of lights */

	int m_globalMediumId; /* global medium id */

	Camera m_camera; /* scene camera */
};

}

#endif //__OBJREADER_HXX__