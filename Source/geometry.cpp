//========================================================================
// Geometry.cpp : Collection of code for 3D math and 3D shapes
//
// Part of the GameCode2 Application
//
// GameCode2 is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 2nd Edition" by Mike McShaffry, published by
// Paraglyph Press. ISBN: 1-932111-91-3
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the author a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: http://www.amazon.com/exec/obidos/ASIN/1932111913/gamecodecompl-20/
//
// There's also a companion web site at http://www.mcshaffry.com/GameCode/portal.php
//
// (c) Copyright 2005 Michael L. McShaffry
//
// This work is licensed under the Creative Commons Attribution-ShareAlike License. 
// To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/1.0/ 
// or send a letter to:
//      Creative Commons
//      559 Nathan Abbott Way
//      Stanford, California 94305, USA.
//
//========================================================================

//========================================================================
//  Content References in Game Coding Complete 2nd Edition
// 
#include "StdHeader.h"
#include "Geometry.h"
#include "math.h"

//
// Our custom FVF, which describes our custom vertex structure
// These were #define'd in the book - now they are static constants.
//
DWORD TRANSFORMED_VERTEX::FVF = D3DFVF_XYZRHW;
DWORD UNTRANSFORMED_VERTEX::FVF = D3DFVF_XYZ;
DWORD UNTRANSFORMED_LIT_VERTEX::FVF = (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_SPECULAR);
DWORD UNTRANSFORMED_UNLIT_VERTEX::FVF =
	(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_SPECULAR);		
DWORD COLORED_TEXTURED_VERTEX::FVF = (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);
DWORD COLORED_VERTEX::FVF = (D3DFVF_XYZ|D3DFVF_DIFFUSE);

Mat4x4 Mat4x4::g_Identity(D3DXMATRIX(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1));
Quaternion Quaternion::g_Identity(D3DXQUATERNION(0,0,0,1));

Mat4x4::Mat4x4(D3DXMATRIX &mat)
{
	memcpy(&m, &mat.m, sizeof(mat.m));
}

Quaternion::Quaternion(D3DXQUATERNION &quat)
{
	x = quat.x;
	y = quat.y;
	z = quat.z;
	w = quat.w;
}

bool Plane::Inside(const Vec3 &point) const
{
	// Inside the plane is defined as the direction the normal is facing
	float result = D3DXPlaneDotCoord(this, &point);
	return (result >= 0.0f);
}

bool Plane::Inside(const Vec3 &point, const float radius) const
{
	float fDistance;	// calculate our distances to each of the planes

	// find the distance to this plane
	fDistance = D3DXPlaneDotCoord(this, &point);		

	// if this distance is < -radius, we are outside
	return (fDistance >= -radius);
}

//
// Frustum::Frustum					- Chapter 14, page 487
//
Frustum::Frustum()
{
	m_Fov = D3DX_PI/4.0f;		// default field of view is 90 degrees
	m_Aspect = 1.0f;			// default aspect ratio is 1:1
	m_Near = 1.0f;				// default near clip plane is 1m away from the camera
	m_Far = 1000.0f;				// default near clip plane is 100m away from the camera
}


//
// Frustum::Inside					- Chapter 14, page 487
//
bool Frustum::Inside(const Vec3 &point) const
{
	//for (int i=0; i<NumPlanes; ++i)
	for (int i=0; i<=Far; ++i)
	{
		if (!m_Planes[i].Inside(point))
			return false;
	}

	return true;
}


//
// Frustum::Frustum					- Chapter 14, page 488
//
bool Frustum::Inside(const Vec3 &point, const float radius) const
{
	for(int i = 0; i < NumPlanes; ++i) 
	{	
		if (!m_Planes[i].Inside(point, radius))
			return false;
	}	
	
	// otherwise we are fully in view
	return(true);
}

//
// Frustum::Frustum					- Chapter 14, page 488
//
void Frustum::Init(const float fov, const float aspect, const float nearClip, const float farClip)
{
	m_Fov = fov;
	m_Aspect = aspect;
	m_Near = nearClip;
	m_Far = farClip;

	double tanFovOver2 = tan(m_Fov/2.0f);
	Vec3 nearRight = (m_Near * tanFovOver2) * m_Aspect * g_Right;
	Vec3 farRight = (m_Far * tanFovOver2) * m_Aspect * g_Right;
	Vec3 nearUp = (m_Near * tanFovOver2 ) * g_Up;
	Vec3 farUp = (m_Far * tanFovOver2)  * g_Up;

	// points start in the upper right and go around clockwise
	m_NearClip[0] = (m_Near * g_Forward) - nearRight + nearUp;
	m_NearClip[1] = (m_Near * g_Forward) + nearRight + nearUp;
	m_NearClip[2] = (m_Near * g_Forward) + nearRight - nearUp;
	m_NearClip[3] = (m_Near * g_Forward) - nearRight - nearUp;

	m_FarClip[0] = (m_Far * g_Forward) - farRight + farUp;
	m_FarClip[1] = (m_Far * g_Forward) + farRight + farUp;
	m_FarClip[2] = (m_Far * g_Forward) + farRight - farUp;
	m_FarClip[3] = (m_Far * g_Forward) - farRight - farUp;

	// now we have all eight points. Time to construct 6 planes.
	// the normals point away from you if you use counter clockwise verts.

	Vec3 origin(0.0f, 0.0f, 0.0f);
	m_Planes[Near].Init(m_NearClip[2], m_NearClip[1], m_NearClip[0]);
	m_Planes[Far].Init(m_FarClip[0], m_FarClip[1], m_FarClip[2]);
	m_Planes[Right].Init(m_FarClip[2], m_FarClip[1], origin);
	m_Planes[Top].Init(m_FarClip[1], m_FarClip[0], origin);
	m_Planes[Left].Init(m_FarClip[0], m_FarClip[3], origin);
	m_Planes[Bottom].Init(m_FarClip[3], m_FarClip[2], origin);
}

/*
void Frustum::Init(const Mat4x4 projection)
{
	// This code was adapted from the original authored code.
	// The original code was authored by Gill Gribb and Klaus Hartman 
	// in the article "Fast Extraction of Viewing Frustum Planes from the 
	// World-View-Projection Matrix" and published on 
	// http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf

	// Left clipping plane
	p_planes[Left].a = projection._14 + projection._11;
	p_planes[Left].b = projection._24 + projection._21;
	p_planes[Left].c = projection._34 + projection._31;
	p_planes[Left].d = projection._44 + projection._41;
	// Right clipping plane
	p_planes[Right].a = projection._14 - projection._11;
	p_planes[Right].b = projection._24 - projection._21;
	p_planes[Right].c = projection._34 - projection._31;
	p_planes[Right].d = projection._44 - projection._41;
	// Top clipping plane
	p_planes[Top].a = projection._14 - projection._12;
	p_planes[Top].b = projection._24 - projection._22;
	p_planes[Top].c = projection._34 - projection._32;
	p_planes[Top].d = projection._44 - projection._42;
	// Bottom clipping plane
	p_planes[Bottom].a = projection._14 + projection._12;
	p_planes[Bottom].b = projection._24 + projection._22;
	p_planes[Bottom].c = projection._34 + projection._32;
	p_planes[Bottom].d = projection._44 + projection._42;
	// Near clipping plane
	p_planes[Near].a = projection._13;
	p_planes[Near].b = projection._23;
	p_planes[Near].c = projection._33;
	p_planes[Near].d = projection._43;
	// Far clipping plane
	p_planes[Far].a = projection._14 - projection._13;
	p_planes[Far].b = projection._24 - projection._23;
	p_planes[Far].c = projection._34 - projection._33;
	p_planes[Far].d = projection._44 - projection._43;
	// Normalize the plane equations, if requested

	if (normalize == true)
	{
		NormalizePlane(p_planes[0]);
		NormalizePlane(p_planes[1]);
		NormalizePlane(p_planes[2]);
		NormalizePlane(p_planes[3]);
		NormalizePlane(p_planes[4]);
		NormalizePlane(p_planes[5]);
	}
}
*/



//
// Frustum::Render					- Chapter 14, page 489
//
void Frustum::Render()
{
	COLORED_VERTEX verts[24];
	for (int i=0; i<8; ++i)
	{
		verts[i].color = g_White;
	}

	for (int i=0; i<8; ++i)
	{
		verts[i+8].color = g_Red;
	}

	for (int i=0; i<8; ++i)
	{
		verts[i+16].color = g_Blue;
	}


	// Draw the near clip plane
	verts[0].position = m_NearClip[0];	verts[1].position = m_NearClip[1];
	verts[2].position = m_NearClip[1];	verts[3].position = m_NearClip[2];
	verts[4].position = m_NearClip[2];	verts[5].position = m_NearClip[3];
	verts[6].position = m_NearClip[3];	verts[7].position = m_NearClip[0];

	// Draw the far clip plane
	verts[8].position = m_FarClip[0];	verts[9].position = m_FarClip[1];
	verts[10].position = m_FarClip[1];	verts[11].position = m_FarClip[2];
	verts[12].position = m_FarClip[2];	verts[13].position = m_FarClip[3];
	verts[14].position = m_FarClip[3];	verts[15].position = m_FarClip[0];

	// Draw the edges between the near and far clip plane
	verts[16].position = m_NearClip[0];	verts[17].position = m_FarClip[0];
	verts[18].position = m_NearClip[1];	verts[19].position = m_FarClip[1];
	verts[20].position = m_NearClip[2];	verts[21].position = m_FarClip[2];
	verts[22].position = m_NearClip[3];	verts[23].position = m_FarClip[3];

	DWORD oldLightMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_LIGHTING, &oldLightMode );
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE );

    DXUTGetD3DDevice()->SetFVF( COLORED_VERTEX::FVF );
	DXUTGetD3DDevice()->DrawPrimitiveUP( D3DPT_LINELIST, 12, verts, sizeof(COLORED_VERTEX) );

	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, oldLightMode );
}

bool TriangleIterator::InitializeStrippedMesh(LPDIRECT3DVERTEXBUFFER9 pVerts, int stride, int strips, int *triCountList )
{
	char *pVertices = NULL;
	if( FAILED( pVerts->Lock( 0, 0, (void**)&pVertices, 0 ) ) )
        return false;

	for (int i=0; i<strips; ++i)
	{
		m_Size += triCountList[i];
	}

	m_Triangles = SAFE_NEW Vec3[m_Size * 3];
	int src = 0;
	int dest = 0;

	for (int strip=0; strip<strips; ++strip )
	{
		int vertsInStrip = triCountList[strip]+2;
		assert(vertsInStrip);

		m_Triangles[dest] = *((Vec3*)&pVertices[stride * src]);
		m_Triangles[dest+1] = *((Vec3*)&pVertices[stride * (src+1)]);
		m_Triangles[dest+2] = *((Vec3*)&pVertices[stride * (src+2)]);
		dest+=3;
		src+=3;
		for (int tri=1; tri<triCountList[strip]; ++tri)
		{
			// for every extra vertex in the triangle strip, you have to grab
			// the two previous verts in the dest list, reverse them, and copy them
			// forward. This will give you a triangle with the same winding
			m_Triangles[dest] = m_Triangles[dest-1];
			m_Triangles[dest+1] = m_Triangles[dest-2];
			m_Triangles[dest+2] = *((Vec3*)&pVertices[stride * (++src)]);
			dest+=3;
		}
	}
	assert(dest==m_Size*3);
	pVerts->Unlock();

	return true;
}

void *TriangleIterator::VGet(unsigned int i) 
{	
	assert(i<m_Size);
	return &m_Triangles[i*3]; 
}



float Vec3::Distance(const Vec3 &b)
{
	float k = (x - b.x) * (x - b.x) + (y - b.y) * (y - b.y) + (z - b.z) * (z - b.z);

	return sqrt(k);
}

float Vec3::SqDistance(const Vec3 &b)
{
	float k = (x - b.x) * (x - b.x) + (y - b.y) * (y - b.y) + (z - b.z) * (z - b.z);

	return k;
}



