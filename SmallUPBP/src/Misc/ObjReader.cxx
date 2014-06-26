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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>

#include "ObjReader.hxx"

namespace ObjReader
{

	static const unsigned int bufferSize = 4096;

	/// Returns directory name from path
	static std::string getDirName(const std::string & path)
	{
		std::string::size_type i = path.find_last_of('\\');
		if (i != std::string::npos)
			return path.substr(0, i + 1);
		else
			return "";
	}

	unsigned int ObjFile::findMaterial(const char* name) const
	{
		unsigned int i;

		for (i = 0; i < m_materials.size(); i++) {
			if (m_materials[i].name == name)
				return i;
		}

		std::cerr << "Error: material not found: " << name << std::endl;
		exit(2);

		return -1;
	}

	Group * ObjFile::findGroup(const char* name)
	{
		unsigned int i;

		for (i = 0; i < m_groups.size(); i++) {
			if (m_groups[i].name == name)
				return &m_groups[i];
		}
		Group group;
		group.material = 0;
		group.name = name;
		m_groups.push_back(group);

		return &*m_groups.rbegin();
	}

	Medium * ObjFile::findMedium(const char* name)
	{
		unsigned int i;

		for (i = 0; i < m_media.size(); i++) {
			if (m_media[i].name == name)
				return &m_media[i];
		}
		Medium medium;
		medium.name = name;
		medium.absorptionCoef[0] = 0.0f;
		medium.absorptionCoef[1] = 0.0f;
		medium.absorptionCoef[2] = 0.0f;
		medium.emissionCoef[0] = 0.0f;
		medium.emissionCoef[1] = 0.0f;
		medium.emissionCoef[2] = 0.0f;
		medium.scatteringCoef[0] = 0.5f;
		medium.scatteringCoef[1] = 0.5f;
		medium.scatteringCoef[2] = 0.5f;
		medium.meanCosine = 0.0f;
		medium.continuationProbability = -1.0f;
		m_media.push_back(medium);
		return &*m_media.rbegin();
	}

	unsigned int ObjFile::findTexture(const char* name)
	{
		unsigned int i;

		for (i = 0; i < m_textures.size(); i++) {
			if (m_textures[i].name == name)
				return i;
		}

		Texture texture;
		texture.id = (unsigned int)m_textures.size();
		texture.name = name;
		m_textures.push_back(texture);
		return texture.id;
	}

	void ObjFile::fillCameraInfo(float * row0, float * row1, float * row2, float * row3)
	{
		// First perform invert  -> just compute third column, plus xyz -> zxy (3dmax rules :( )
		float inverse[3][4] = { 
			{ row0[0], row1[0], row2[0], row3[0] }, //row0[0] * row3[1] + row1[0] * row3[2] + row2[0] * row3[0] },
			{ row0[1], row1[1], row2[1], row3[1] }, //row0[1] * row3[1] + row1[1] * row3[2] + row2[2] * row3[0] },
			{ row0[2], row1[2], row2[2], row3[2] }//row0[2] * row3[1] + row1[2] * row3[2] + row2[1] * row3[0]
	
		};
		/* Now we will transform these points 
		a: 0, 0, 0, 1
		b: 0, 0, -1, 1
		c: 0, 1, 0, 1
		Camera origin is then A, camera target is B, and camera roll is (C-A) normalized
		*/
		m_camera.origin[0] = inverse[0][3];
		m_camera.origin[1] = inverse[1][3];
		m_camera.origin[2] = inverse[2][3];
		m_camera.target[0] = inverse[0][3] - inverse[0][2];
		m_camera.target[1] = inverse[1][3] - inverse[1][2];
		m_camera.target[2] = inverse[2][3] - inverse[2][2];
		m_camera.roll[0] = inverse[0][1];
		m_camera.roll[1] = inverse[1][1];
		m_camera.roll[2] = inverse[2][1];
		// Normalize roll
		float size = 1.0f / sqrtf(m_camera.roll[0] * m_camera.roll[0] + m_camera.roll[1] * m_camera.roll[1] + m_camera.roll[2] * m_camera.roll[2]);
		m_camera.roll[0] *= size;
		m_camera.roll[1] *= size;
		m_camera.roll[2] *= size;
	}

	void createDefaultMat(Material & material, const char * name)
	{
		material.name = name;
		material.shininess = 0.0f;
		material.diffuse[0] = 0.0f;
		material.diffuse[1] = 0.0f;
		material.diffuse[2] = 0.0f;
		material.ambient[0] = 0.0f;
		material.ambient[1] = 0.0f;
		material.ambient[2] = 0.0f;
		material.specular[0] = 0.0f;
		material.specular[1] = 0.0f;
		material.specular[2] = 0.0f;
		material.isEmissive = false;
		material.IDTexture = -1;
		material.priority = -1;
		material.IOR = -1.0f;
		material.mirror[0] = 0.0f;
		material.mirror[1] = 0.0f;
		material.mirror[2] = 0.0f;
		material.mediumId = -1;
		material.enclosingMatId = -1;
		material.geometryType = REAL;
	}
	void ObjFile::readMtl(const std::string & filename)
	{
		FILE* file;
		char texture[FILENAME_MAX];
		char    buf[bufferSize];

		errno_t err = fopen_s(&file, filename.c_str(), "r");
		if (err != 0) {
			std::cerr << "Error: could not open ``" << filename << "''" << std::endl;
			exit(2);
		}

		Materials::iterator material = m_materials.end() - 1;

		while (fscanf_s(file, "%s", buf, bufferSize) != EOF) {
			switch (buf[0]) {
			case '#':               /* comment */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			case 'n':               /* newmtl */
				fgets(buf, sizeof(buf), file);
				sscanf_s(buf, "%s", buf, bufferSize);
				m_materials.push_back(Material());
				material = m_materials.end() - 1;
				createDefaultMat(*material, buf);
				break;
			case 'N':
				if (buf[1] != 's') break;
				fscanf_s(file, "%f", &material->shininess); /* 0 - 1000 */
				break;
			case 'K':
				switch (buf[1]) {
				case 'd':
					fscanf_s(file, "%f %f %f",
						&material->diffuse[0],
						&material->diffuse[1],
						&material->diffuse[2]);
					break;
				case 's':
					fscanf_s(file, "%f %f %f",
						&material->specular[0],
						&material->specular[1],
						&material->specular[2]);
					break;
				case 'a':
					fscanf_s(file, "%f %f %f",
						&material->ambient[0],
						&material->ambient[1],
						&material->ambient[2]);
					break;
				case 'e':
					fscanf_s(file, "%f %f %f",
						&material->emmissive[0],
						&material->emmissive[1],
						&material->emmissive[2]);
					material->isEmissive = material->emmissive[0] > 0.0f || material->emmissive[1] > 0.0f || material->emmissive[2] > 0.0f;
					break;
				default:
					/* eat up rest of line */
					fgets(buf, sizeof(buf), file);
					break;
				}
				break;
			case 'm':
				// Texture
				fgets(texture, FILENAME_MAX, file);
				if (strncmp(buf, "map_Kd", 6) == 0)
				{
					material->IDTexture = findTexture(texture);
				}
				break;
			default:
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			}
		}
		fclose(file);
	}

	void ObjFile::readAux(const std::string & filename)
	{
		FILE* file;
		char    buf[bufferSize];

		errno_t err = fopen_s(&file, filename.c_str(), "r");
		if (err != 0) {
			std::cerr << "Error: could not open ``" << filename << "''" << std::endl;
			exit(2);
		}

		Material * material = NULL;
		Medium * medium = NULL;
		// camera matrix
		float tm[4][3];
		m_camera.resolutionX = m_camera.resolutionY = 256; // Default
		m_camera.material = -1;
		m_globalMediumId = -1;
		AdditionalLight al;

		while (fscanf_s(file, "%s", buf, bufferSize) != EOF) {
			switch (buf[0]) {
			case '#':               /* comment */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			case 'T': /* transform matrix camera*/
				fscanf_s(file, "%f %f %f",
					&tm[buf[6] - '0'][0],
					&tm[buf[6] - '0'][1],
					&tm[buf[6] - '0'][2]);
				break;
			case 'C': /* camera options*/
				switch (buf[7])
				{
				case 'F': /*camera_FOV*/
					fscanf_s(file, "%f",
						&m_camera.horizontalFOV);
					break;
				case 'T': /*camera_TDist*/
					fscanf_s(file, "%f",
						&m_camera.focalDistance);
					break;
				case 'M':
					fgets(buf, sizeof(buf), file);
					sscanf_s(buf, "%s", buf, bufferSize);
					m_camera.material = findMaterial(buf);
					break;
				case 'R': /*camera_resolution_*/
					switch (buf[18])
					{
					case 'X':
						fscanf_s(file, "%i",
							&m_camera.resolutionX);
						if (m_camera.resolutionX < 1)
							m_camera.resolutionX = 256;
						break;
					case 'Y':
						fscanf_s(file, "%i",
							&m_camera.resolutionY);
						if (m_camera.resolutionY < 1)
							m_camera.resolutionY = 256;
						break;
					default:
						std::cerr << "Error: unknown camera option: " << buf << std::endl;
						exit(2);
					}
					break;
				default:
					std::cerr << "Error: unknown camera option: " << buf << std::endl;
					exit(2);
				}
				break;
			case 'm':               /* material or medium? */
				switch (buf[1])
				{
				case 'a': /* material */
					fgets(buf, sizeof(buf), file);
					sscanf_s(buf, "%s", buf, bufferSize);
					material = &m_materials[0] + findMaterial(buf);
					break;
				case 'e': /* medium */
					switch (buf[6])
					{
					case 'I': /* mediumId*/
						if (material == NULL)
						{
							std::cerr << "Error: using mediumId option without material selection" << std::endl;
							exit(2);
						}
						fgets(buf, sizeof(buf), file);
						sscanf_s(buf, "%s", buf, bufferSize);
						material->mediumId = (int)( findMedium(buf) - &m_media[0]);
						break;
					case '\0': /* medium */
						fgets(buf, sizeof(buf), file);
						sscanf_s(buf, "%s", buf, bufferSize);
						medium = findMedium(buf);
						break;
					default:
						/* eat up rest of line */
						fgets(buf, sizeof(buf), file);
						break;
					}
					break;
				case 'i': /* mirror*/
					if (material == NULL)
					{
						std::cerr << "Error: using mirror option without material selection" << std::endl;
						exit(2);
					}
					fscanf_s(file, "%f %f %f",
						&material->mirror[0],
						&material->mirror[1],
						&material->mirror[2]);
					break;
				default:
					/* eat up rest of line */
					fgets(buf, sizeof(buf), file);
					break;
				}

				break;
			case 'i': /* ior */
				if (material == NULL)
				{
					std::cerr << "Error: using IOR option without material selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%f",&material->IOR);
				break;
			case 'p': /* priority */
				if (material == NULL)
				{
					std::cerr << "Error: using priority option without material selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%i", &material->priority);
				break;
			case 'g': /* g */
				switch (buf[1])
				{
				case '\0': /* g */
					if (medium == NULL)
					{
						std::cerr << "Error: using g option without medium selection" << std::endl;
						exit(2);
					}
					fscanf_s(file, "%f", &medium->meanCosine);
					break;
				case 'e': /* geometryType */
					if (material == NULL)
					{
						std::cerr << "Error: using geometryType option without material selection" << std::endl;
						exit(2);
					}
					fgets(buf, sizeof(buf), file);
					sscanf_s(buf, "%s", buf, bufferSize);
					if (buf[0] == 'r')
						material->geometryType = REAL;
					else if (buf[0] == 'i')
						material->geometryType = IMAGINARY;
					else
					{
						std::cerr << "Error: unknown geometry type" << std::endl;
						exit(2);
					}
					break;
				case 'l': /* globalMediumId */
					fgets(buf, sizeof(buf), file);
					sscanf_s(buf, "%s", buf, bufferSize);
					m_globalMediumId = (int)(findMedium(buf) - &m_media[0]);
					break;
				default:
					/* eat up rest of line */
					fgets(buf, sizeof(buf), file);
					break;
				}
				break;
			case 'K': /* Ke */
				if (material == NULL)
				{
					std::cerr << "Error: using Ke option without material selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%f %f %f",
					&material->emmissive[0],
					&material->emmissive[1],
					&material->emmissive[2]);
				material->isEmissive = material->emmissive[0] > 0.0f || material->emmissive[1] > 0.0f || material->emmissive[2] > 0.0f;
				break;
			case 'a': /* absorption */
				if (medium == NULL)
				{
					std::cerr << "Error: using absorption option without medium selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%f %f %f",
					&medium->absorptionCoef[0],
					&medium->absorptionCoef[1],
					&medium->absorptionCoef[2]);
				break;
			case 'c': /* continuation_probability */
				if (medium == NULL)
				{
					std::cerr << "Error: using continuation_probability option without medium selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%f",
					&medium->continuationProbability);
				break;
			case 'e': 
				switch (buf[1])
				{
				case 'm': /* emission */
					if (medium == NULL)
					{
						std::cerr << "Error: using emission option without medium selection" << std::endl;
						exit(2);
					}
					fscanf_s(file, "%f %f %f",
						&medium->emissionCoef[0],
						&medium->emissionCoef[1],
						&medium->emissionCoef[2]);
					break;
				case 'n': /* enclosingMatId */
					if (material == NULL)
					{
						std::cerr << "Error: using enclosingMatId option without material selection" << std::endl;
						exit(2);
					}
					fgets(buf, sizeof(buf), file);
					sscanf_s(buf, "%s", buf, bufferSize);
					material->enclosingMatId = findMaterial(buf);
					break;
				}
				break;
			case 's': /* scattering */
				if (medium == NULL)
				{
					std::cerr << "Error: using scattering option without medium selection" << std::endl;
					exit(2);
				}
				fscanf_s(file, "%f %f %f",
					&medium->scatteringCoef[0],
					&medium->scatteringCoef[1],
					&medium->scatteringCoef[2]);
				break;
			case 'l': /* light */
				switch (buf[6])
				{
				case 'p': /* light_point */
					al.lightType = POINT;
					fscanf_s(file, "%f %f %f %f %f %f",
						&al.position[0],
						&al.position[1],
						&al.position[2],
						&al.emission[0],
						&al.emission[1],
						&al.emission[2]);
					m_lights.push_back(al);
					break;
				case 'd': /* light_directional */
					al.lightType = DIRECTIONAL;
					fscanf_s(file, "%f %f %f %f %f %f",
						&al.position[0],
						&al.position[1],
						&al.position[2],
						&al.emission[0],
						&al.emission[1],
						&al.emission[2]);
					m_lights.push_back(al);
					break;
				case 'b': /* light_background... */
					al.lightType = BACKGROUND;
					switch (buf[17])
					{
					case 'c': /* light_background_constant */
						fscanf_s(file, "%f %f %f",
						&al.emission[0],
						&al.emission[1],
						&al.emission[2]);
						break;
					case 'e': /* light_background_em */
						fscanf_s(file, "%f %f",
							&al.envMapScale,
							&al.envMapRotate);
						fgets(buf, sizeof(buf), file);
						sscanf_s(buf, "%s", buf, bufferSize);
						al.envMap = std::string(buf);
						if (al.envMap[1] != ':') al.envMap = getDirName(filename) + al.envMap; // get absolute
						break;
					}					
					m_lights.push_back(al);
					break;
				default:
					/* eat up rest of line */
					fgets(buf, sizeof(buf), file);
					break;
				}
				break;
			default:
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			}
		}
		fclose(file);
		fillCameraInfo(tm[0],tm[1],tm[2],tm[3]);
	}

	void ObjFile::readObj(FILE * file)
	{
		Group * group = findGroup("   ???   ");		 /* current group */
		group->material = 0;
		unsigned int material = 0;           /* current material */
		m_materials.push_back(Material());
		createDefaultMat(m_materials.back(), "   ???   ");
		unsigned int  v, n, t;
		char buf[bufferSize];
		while (fscanf_s(file, "%s", buf, bufferSize) != EOF)
		{
			float x, y, z;
			switch (buf[0])
			{
			case '#':               /* comment */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			case 'v':               /* v, vn, vt */
				switch (buf[1])
				{
				case '\0':          /* vertex */
					fscanf_s(file, "%f %f %f", &x, &y, &z);
					m_vertices.push_back(x);
					m_vertices.push_back(y);
					m_vertices.push_back(z);
					break;
				case 'n':           /* normal */
					fscanf_s(file, "%f %f %f", &x, &y, &z);
					m_normals.push_back(x);
					m_normals.push_back(y);
					m_normals.push_back(z);
					break;
				case 't':           /* texcoord */
					fscanf_s(file, "%f %f", &x, &y);
					m_texcoords.push_back(x);
					m_texcoords.push_back(y);
					break;
				}
				break;
			case 'm': //mtllib
				fgets(buf, sizeof(buf), file);
				sscanf_s(buf, "%s", buf, bufferSize);
				m_mtllibname = buf;
				readMtl(getDirName(m_pathname) + m_mtllibname);
				break;
			case 'u':
				fgets(buf, sizeof(buf), file);
				sscanf_s(buf, "%s", buf, bufferSize);
				group = findGroup(buf);
				group->material = material = findMaterial(buf);
				break;
//			case 'g':               /* group */
//				/* eat up rest of line */
//				fgets(buf, sizeof(buf), file);
//#if SINGLE_STRING_GROUP_NAMES
//				sscanf(buf, "%s", buf);
//#else
//				buf[strlen(buf) - 1] = '\0';  /* nuke '\n' */
//#endif
//				group = findGroup(buf);
//				group->material = material;
//				break;
			case 'f':  /* face */
			{
						   m_triangles.push_back(Triangle());
						   Triangles::iterator triangle = m_triangles.end() - 1;
						   v = n = t = 0;
						   fscanf_s(file, "%s", buf, bufferSize);
						   /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
						   if (strstr(buf, "//")) {
							   /* v//n */
							   sscanf_s(buf, "%d//%d", &v, &n);
							   triangle->vindices[0] = v-1;
							   triangle->nindices[0] = n - 1;
							   fscanf_s(file, "%d//%d", &v, &n);
							   triangle->vindices[1] = v - 1;
							   triangle->nindices[1] = n - 1;
							   fscanf_s(file, "%d//%d", &v, &n);
							   triangle->vindices[2] = v - 1;
							   triangle->nindices[2] = n - 1;
							   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   while (fscanf_s(file, "%d//%d", &v, &n) > 0)
							   {
								   m_triangles.push_back(Triangle());
								   triangle = m_triangles.end() - 1;
								   Triangles::iterator prev = triangle - 1;
								   triangle->vindices[0] = prev->vindices[0];
								   triangle->nindices[0] = prev->nindices[0];
								   triangle->vindices[1] = prev->vindices[2];
								   triangle->nindices[1] = prev->nindices[2];
								   triangle->vindices[2] = v - 1;
								   triangle->nindices[2] = n - 1;
								   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   }
						   }
						   else if (sscanf_s(buf, "%d/%d/%d", &v, &t, &n) == 3)
						   {
							   /* v/t/n */
							   triangle->vindices[0] = v - 1;
							   triangle->tindices[0] = t - 1;
							   triangle->nindices[0] = n - 1;
							   fscanf_s(file, "%d/%d/%d", &v, &t, &n);
							   triangle->vindices[1] = v - 1;
							   triangle->tindices[1] = t - 1;
							   triangle->nindices[1] = n - 1;
							   fscanf_s(file, "%d/%d/%d", &v, &t, &n);
							   triangle->vindices[2] = v - 1;
							   triangle->tindices[2] = t - 1;
							   triangle->nindices[2] = n - 1;
							   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   while (fscanf_s(file, "%d/%d/%d", &v, &t, &n) > 0)
							   {
								   m_triangles.push_back(Triangle());
								   triangle = m_triangles.end() - 1;
								   Triangles::iterator prev = triangle - 1;
								   triangle->vindices[0] = prev->vindices[0];
								   triangle->tindices[0] = prev->tindices[0];
								   triangle->nindices[0] = prev->nindices[0];
								   triangle->vindices[1] = prev->vindices[2];
								   triangle->tindices[1] = prev->tindices[2];
								   triangle->nindices[1] = prev->nindices[2];
								   triangle->vindices[2] = v - 1;
								   triangle->tindices[2] = t - 1;
								   triangle->nindices[2] = n - 1;
								   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   }
						   }
						   else if (sscanf_s(buf, "%d/%d", &v, &t) == 2) {
							   /* v/t */
							   triangle->vindices[0] = v - 1;
							   triangle->tindices[0] = t - 1;
							   fscanf_s(file, "%d/%d", &v, &t);
							   triangle->vindices[1] = v - 1;
							   triangle->tindices[1] = t - 1;
							   fscanf_s(file, "%d/%d", &v, &t);
							   triangle->vindices[2] = v - 1;
							   triangle->tindices[2] = t - 1;
							   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   while (fscanf_s(file, "%d/%d", &v, &t) > 0)
							   {
								   m_triangles.push_back(Triangle());
								   triangle = m_triangles.end() - 1;
								   Triangles::iterator prev = triangle - 1;
								   triangle->vindices[0] = prev->vindices[0];
								   triangle->tindices[0] = prev->tindices[0];
								   triangle->vindices[1] = prev->vindices[2];
								   triangle->tindices[1] = prev->tindices[2];
								   triangle->vindices[2] = v - 1;
								   triangle->tindices[2] = t - 1;
								   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   }
						   }
						   else {
							   /* v */
							   sscanf_s(buf, "%d", &v);
							   triangle->vindices[0] = v - 1;
							   fscanf_s(file, "%d", &v);
							   triangle->vindices[1] = v - 1;
							   fscanf_s(file, "%d", &v);
							   triangle->vindices[2] = v - 1;
							   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   while (fscanf_s(file, "%d", &v) > 0)
							   {
								   m_triangles.push_back(Triangle());
								   triangle = m_triangles.end() - 1;
								   Triangles::iterator prev = triangle - 1;
								   triangle->vindices[0] = prev->vindices[0];
								   triangle->vindices[1] = prev->vindices[2];
								   triangle->vindices[2] = v - 1;
								   group->triangles.push_back((unsigned int)m_triangles.size() - 1);
							   }
						   }
			}
				break;

			default:
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			}
		}

	}


	ObjFile::ObjFile(const char* filename)
	{
		FILE*  file;
		/* open the file */
		errno_t err = fopen_s(&file, filename, "r");
		if (err != 0) {
			std::cerr << "Error: could not open ``" << filename << "''" << std::endl;
			exit(2);
		}

		m_pathname = filename;

		readObj(file);

		fclose(file);

		readAux(std::string(filename) + ".aux");
	}

}