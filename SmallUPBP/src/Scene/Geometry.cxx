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

#include "Geometry.hxx"

Isect ** AbstractGeometry::sIntersections = nullptr;
bool  AbstractGeometry::sUseShadingNormal = true;

void AcceleratedGeometryList::GrowBBox(
		Pos &aoBBoxMin,
		Pos &aoBBoxMax)
{
	int trianglesCount = 0;
	for (int i = 0; i < (int)mGeometry.size(); i++)
	{
		mGeometry[i]->GrowBBox(aoBBoxMin, aoBBoxMax);
		mGeometry[i]->setId(i);
		if (mGeometry[i]->getType() == GEOM_TRIANGLE)
			++trianglesCount;
	}
	// Prepare embree structure for triangles
	mMesh = embree::rtcNewTriangleMesh(trianglesCount, trianglesCount * 3, "bvh4.triangle4");
	UPBP_ASSERT(mMesh != nullptr);
	embree::RTCVertex * vertices = embree::rtcMapPositionBuffer(mMesh);
	UPBP_ASSERT(vertices != nullptr);
	embree::RTCTriangle* triangles = embree::rtcMapTriangleBuffer(mMesh);
	UPBP_ASSERT(triangles != nullptr);

	// Prepare embree structure for other geometry
	mOtherGeometry = embree::rtcNewVirtualGeometry(mGeometry.size() - trianglesCount, "default");
	mAnyNonTriangles = mGeometry.size() != trianglesCount;
	UPBP_ASSERT(mOtherGeometry != nullptr);

	// Copy geometry to embree buffers
	for (int i = 0, idtr = 0, idver = 0, idother = 0; i < (int)mGeometry.size(); i++)
	{
		if (mGeometry[i]->getType() == GEOM_TRIANGLE)
		{
			const Triangle * tr = dynamic_cast<const Triangle *>(mGeometry[i]);
			// Point 1
			vertices[idver].x = tr->p[0].x();
			vertices[idver].y = tr->p[0].y();
			vertices[idver].z = tr->p[0].z();
			++idver;
			// Point 2
			vertices[idver].x = tr->p[1].x();
			vertices[idver].y = tr->p[1].y();
			vertices[idver].z = tr->p[1].z();
			++idver;
			// Point 3
			vertices[idver].x = tr->p[2].x();
			vertices[idver].y = tr->p[2].y();
			vertices[idver].z = tr->p[2].z();
			++idver;
			// Triangle
			triangles[idtr].id0 = i;
			triangles[idtr].id1 = i;
			triangles[idtr].v0 = idver - 3;
			triangles[idtr].v1 = idver - 2;
			triangles[idtr].v2 = idver - 1;
			++idtr;
		}
		else
		{
			Pos bboxMin(1e36f);
			Pos bboxMax(-1e36f);
			mGeometry[i]->GrowBBox(bboxMin, bboxMax);
			embree::rtcSetVirtualGeometryUserData(mOtherGeometry, idother, i, i);
			embree::rtcSetVirtualGeometryBounds(mOtherGeometry, idother, &bboxMin.x(), &bboxMax.x());
			embree::rtcSetVirtualGeometryIntersector1(mOtherGeometry, idother, mGeometry[i]);
			mOtherGeometryList.push_back(mGeometry[i]);
			++idother;
		}
	}
	embree::rtcUnmapPositionBuffer(mMesh);
	embree::rtcUnmapTriangleBuffer(mMesh);

	// Builds both structure
	embree::rtcBuildAccel(mMesh, "spatialsplit");
	embree::rtcCleanupGeometry(mMesh);
	embree::rtcBuildAccel(mOtherGeometry, "default");
	embree::rtcCleanupGeometry(mOtherGeometry);

	// Prepare mesh intersector
	mMeshIntersector = embree::rtcQueryIntersector1(mMesh, "default");
	UPBP_ASSERT(mMeshIntersector != nullptr);

	// Prepare other geometry intersector
	mOtherIntersector = embree::rtcQueryIntersector1(mOtherGeometry, "default");
	UPBP_ASSERT(mOtherIntersector != nullptr);

	// For intersections reporting
	AbstractGeometry::allocStaticResources();
}