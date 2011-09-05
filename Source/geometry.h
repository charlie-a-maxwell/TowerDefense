#pragma once
//========================================================================
// Geometry.h : Collection of code for 3D math and 3D shapes
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
//  class Vec3				- Chapter 14, page 478
//  class Vec4				- Chapter 14, page 478
//  class Quaternion		- Chapter 14, page 479
//  class Mat4x4			- Chapter 14, page 481
//  class Plane				- Chapter 14, page 483
//  class Frustum			- Chapter 14, page 486
//


////////////////////////////////////////////////////
//
// Utility classes for vectors and matrices 
//
////////////////////////////////////////////////////

////////////////////////////////////////////////////
//
// Vec3 Description
//
//
////////////////////////////////////////////////////
class Mat4x4;


class Vec3 : public D3DXVECTOR3 
{
public:
	inline float Length() { return D3DXVec3Length(this); }
	inline Vec3 *Normalize() { return static_cast<Vec3 *>(D3DXVec3Normalize(this, this)); }
	inline float Dot(const Vec3 &b) { return D3DXVec3Dot(this, &b); }
	inline Vec3 Cross(const Vec3 &b) const;
	inline Vec3 UnProject(const D3DVIEWPORT9 &viewport, const Mat4x4 &projection, 
							const Mat4x4 &view, const Mat4x4 &world) const;
	float Distance(const Vec3 &b);
	float SqDistance(const Vec3 &b);
	
	Vec3(D3DXVECTOR3 &v3) { x = v3.x; y = v3.y; z = v3.z; }
	Vec3() : D3DXVECTOR3() { }
	Vec3(const float _x, const float _y, const float _z) { x=_x; y=_y; z=_z; }
	
	inline Vec3(const class Vec4 &v4);
};

inline Vec3 Vec3::Cross(const Vec3 &b) const
{
	Vec3 out;
	D3DXVec3Cross(&out, this, &b);
	return out;
}

////////////////////////////////////////////////////
//
// Vec4 Description
//
//
////////////////////////////////////////////////////

class Vec4 : public D3DXVECTOR4
{
public:
	inline float Length() { return D3DXVec4Length(this); }
	inline Vec4 *Normalize() { return static_cast<Vec4 *>(D3DXVec4Normalize(this, this)); }
	// If you want the cross product, use Vec3::Cross
	inline float Dot(const Vec4 &b) { return D3DXVec4Dot(this, &b); }

	Vec4(D3DXVECTOR4 &v4)  { x = v4.x; y = v4.y; z = v4.z; w = v4.w; }
	Vec4() : D3DXVECTOR4() { }
	Vec4(const float _x, const float _y, const float _z, const float _w) { x=_x; y=_y; z=_z; w=_w; }
	Vec4(const Vec3 &v3) { x = v3.x; y = v3.y; z = v3.z; w = 1.0f; }

};

inline Vec3::Vec3(const Vec4 &v4) { x = v4.x; y = v4.y; z = v4.z; }


////////////////////////////////////////////////////
//
// Vec3List Description
// Vec4List Description
//
//   An STL list of Vectors
//
////////////////////////////////////////////////////

typedef std::list<Vec3> Vec3List;
typedef std::list<Vec4> Vec4List;


////////////////////////////////////////////////////
//
// Quaternion Description
//
//
////////////////////////////////////////////////////

class Quaternion : public D3DXQUATERNION
{
public:

	// Modifiers
	void Normalize() { D3DXQuaternionNormalize(this, this); };
	void Slerp(const Quaternion &begin, const Quaternion &end, float cooef)
	{
		// performs spherical linear interpolation between begin & end 
		// NOTE: set cooef between 0.0f-1.0f
		D3DXQuaternionSlerp(this, &begin, &end, cooef);
	}

	// Accessors
	void GetAxisAngle(Vec3 &axis, float &angle) const
	{
		D3DXQuaternionToAxisAngle(this, &axis, &angle); 
	}

	// Initializers
	void Build(const class Mat4x4 &mat); 

	void BuildRotYawPitchRoll(
			const float yawRadians, 
			const float pitchRadians, 
			const float rollRadians)
	{
		D3DXQuaternionRotationYawPitchRoll(this, yawRadians, pitchRadians, rollRadians);
	}

	void BuildAxisAngle(const Vec3 &axis, const float radians)
	{
		D3DXQuaternionRotationAxis(this, &axis, radians);
	}
	
	Quaternion(D3DXQUATERNION &m);
	Quaternion() : D3DXQUATERNION() { }

	static Quaternion g_Identity;
};

inline Quaternion operator * (const Quaternion &a, const Quaternion &b) 
{
	// for rotations, this is exactly like concatenating
	// matrices - the new quat represents rot A followed by rot B.
	Quaternion out;
	D3DXQuaternionMultiply(&out, &a, &b);
	return out;
}



////////////////////////////////////////////////////
//
// Mat4x4 Description
//
//
////////////////////////////////////////////////////


class Mat4x4 : public D3DXMATRIX
{
public:
	// Modifiers
	inline void SetPosition(Vec3 const &pos);
	inline void SetPosition(Vec4 const &pos);

	// Accessors and Calculation Methods
	inline Vec3 GetPosition() const;
	inline Vec4 Xform(Vec4 &v) const;
	inline Vec3 Xform(Vec3 &v) const;
	inline Mat4x4 Inverse() const;

	Mat4x4(D3DXMATRIX &m);
	Mat4x4() : D3DXMATRIX() { }

	static Mat4x4 g_Identity;

	// Initialization methods
	inline void BuildTranslation(const Vec3 &pos);
	inline void BuildTranslation(const float x, const float y, const float z );
	inline void BuildRotationX(const float radians) { D3DXMatrixRotationX(this, radians); }
	inline void BuildRotationY(const float radians) { D3DXMatrixRotationY(this, radians); }
	inline void BuildRotationZ(const float radians) { D3DXMatrixRotationZ(this, radians); }
	inline void BuildYawPitchRoll(const float yawRadians, const float pitchRadians, const float rollRadians)
		{ D3DXMatrixRotationYawPitchRoll(this, yawRadians, pitchRadians, rollRadians); }
	inline void BuildRotationQuat(const Quaternion &q) { D3DXMatrixRotationQuaternion(this, &q); }
};


inline void Mat4x4::SetPosition(Vec3 const &pos)
{
	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = 1.0f;
}

inline void Mat4x4::SetPosition(Vec4 const &pos)
{
	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = pos.w;
}

inline Vec3 Mat4x4::GetPosition() const
{
	Vec3 pos;
	pos.x = m[3][0];
	pos.y = m[3][1];
	pos.z = m[3][2];
	return pos;
}

inline Vec4 Mat4x4::Xform(Vec4 &v) const
{
	Vec4 temp;
	D3DXVec4Transform(&temp, &v, this);
	return temp;
}

inline Vec3 Mat4x4::Xform(Vec3 &v) const
{
	Vec4 temp(v);
	Vec4 out;
	D3DXVec4Transform(&out, &temp, this);
	return Vec3(out.x, out.y, out.z);
}

inline Mat4x4 Mat4x4::Inverse() const
{
	Mat4x4 out;
	D3DXMatrixInverse(&out, NULL, this);
	return out;
}

inline void Mat4x4::BuildTranslation(const Vec3 &pos)
{
	*this = Mat4x4::g_Identity;
	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
}

inline void Mat4x4::BuildTranslation(const float x, const float y, const float z )
{
	*this = Mat4x4::g_Identity;
	m[3][0] = x;
	m[3][1] = y;
	m[3][2] = z;
}


inline Mat4x4 operator * (const Mat4x4 &a, const Mat4x4 &b) 
{
	Mat4x4 out;
	D3DXMatrixMultiply(&out, &a, &b);
	return out;
}

inline void Quaternion::Build(const Mat4x4 &mat) 
{ 
	D3DXQuaternionRotationMatrix(this, &mat); 
}

inline Vec3 Vec3::UnProject( const D3DVIEWPORT9 &viewport, const Mat4x4 &projection, 
								const Mat4x4 &view, const Mat4x4 &world) const
{
	Vec3 out;

	D3DXVec3Unproject(&out, this, &viewport, &projection, &view, &world);

	return out;
}

////////////////////////////////////////////////////
//
// Vertex Type Definitions
//
//  TRANSFORMED_VERTEX Description
//  UNTRANSFORMED_VERTEX Description
//  UNTRANSFORMED_LIT_VERTEX Description
//  UNTRANSFORMED_UNLIT_VERTEX Description
//  COLORED_TEXTURED_VERTEX Description
//  COLORED_VERTEX Description
//
//  Note: There's been a slight change from the book in this code.
//        Instead of #define D3DFVF_BlahBlah they are static constants;
//        find them at the top of Geometry.cpp
//
// See Chapter 13, page 459, 463, 464
////////////////////////////////////////////////////

struct TRANSFORMED_VERTEX
{
    D3DXVECTOR3 position;	// The screen x, y, z
	float rhw;				// always 1.0, the reciprocal of homogeneous w)

	static DWORD FVF;
};


struct UNTRANSFORMED_VERTEX
{
    D3DXVECTOR3 position; // The position in 3D space

	static DWORD FVF;
};


struct UNTRANSFORMED_LIT_VERTEX
{
    D3DXVECTOR3 position;	// The position in 3D space
    D3DCOLOR    diffuse;    // The diffuse color
    D3DCOLOR    specular;   // The specular color

	static DWORD FVF;
};

struct UNTRANSFORMED_UNLIT_VERTEX
{
    D3DXVECTOR3 position;	// The position in 3D space
    D3DXVECTOR3 normal;		// The normal vector (must be 1.0 units in length)
    D3DCOLOR    diffuse;    // The diffuse color
    D3DCOLOR    specular;   // The specular color

	static DWORD FVF;
};



// A structure for our custom vertex type. We added texture coordinates
struct COLORED_TEXTURED_VERTEX
{
    D3DXVECTOR3 position; // The position
    D3DCOLOR    color;    // The color
    FLOAT       tu, tv;   // The texture coordinates

	static DWORD FVF;
};


// A structure for our custom vertex type. We added texture coordinates
struct COLORED_VERTEX
{
    D3DXVECTOR3 position; // The position
    D3DCOLOR    color;    // The color

	static DWORD FVF;
};




////////////////////////////////////////////////////
//
// TriangleIterator Definition - added post press
//
//    Allows a variety of different vertex buffers to be iterated as
//    a series of triangles.
//
////////////////////////////////////////////////////

class TriangleIterator
{
protected:
	Vec3 *m_Triangles;
	unsigned int m_Size;

public:
	TriangleIterator() { m_Triangles=0; m_Size=0; }
	virtual ~TriangleIterator() { SAFE_DELETE_ARRAY(m_Triangles); }

	bool InitializeStrippedMesh(LPDIRECT3DVERTEXBUFFER9 pVerts, int stride, int strips, int *triCountList );

	virtual unsigned int VGetSize() { return m_Size; }
	virtual void *VGet(unsigned int i);
};


////////////////////////////////////////////////////
//
// Plane Definition
//
////////////////////////////////////////////////////

class Plane : public D3DXPLANE
{
public:
	inline void Normalize();

	// normal faces away from you if you send in verts in counter clockwise order....
	inline void Init(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2);
	bool Inside(const Vec3 &point, const float radius) const;
	bool Inside(const Vec3 &point) const;
};

inline void Plane::Normalize()
{
	float mag;
	mag = sqrt(a * a + b * b + c * c);
	a = a / mag;
	b = b / mag;
	c = c / mag;
	d = d / mag;
}

inline void Plane::Init(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
{
	D3DXPlaneFromPoints(this, &p0, &p1, &p2);
	Normalize();
}

////////////////////////////////////////////////////
//
// Frustum Definition
//
////////////////////////////////////////////////////

class Frustum
{
public:
	enum Side { Near, Far, Top, Right, Bottom, Left, NumPlanes };

	Plane m_Planes[NumPlanes];	// planes of the frusum in camera space
	Vec3 m_NearClip[4];			// verts of the near clip plane in camera space
	Vec3 m_FarClip[4];			// verts of the far clip plane in camera space

	float m_Fov;				// field of view in radians
	float m_Aspect;				// aspect ratio - width divided by height
	float m_Near;				// near clipping distance
	float m_Far;				// far clipping distance

public:
	Frustum();

	bool Inside(const Vec3 &point) const;
	bool Inside(const Vec3 &point, const float radius) const;
	const Plane &Get(Side side) { return m_Planes[side]; }
	void SetFOV(float fov) { m_Fov=fov; Init(m_Fov, m_Aspect, m_Near, m_Far); }
	void SetAspect(float aspect) { m_Aspect=aspect; Init(m_Fov, m_Aspect, m_Near, m_Far); }
	void SetNear(float nearClip) { m_Near=nearClip; Init(m_Fov, m_Aspect, m_Near, m_Far); }
	void SetFar(float farClip) { m_Far=farClip; Init(m_Fov, m_Aspect, m_Near, m_Far); }
	void Init(const float fov, const float aspect, const float near, const float far);

	void Render();
};


inline Vec3 CalcVelocity(Vec3 const &pos0, Vec3 const &pos1, float time)
{
	// CalcVelocity - Chapter 15, page TODO
	return (pos1 - pos0) / time;
}

inline Vec3 CalcAcceleration(Vec3 const &vel0, Vec3 const &vel1, float time)
{
	// CalcVelocity - Chapter 15, page TODO
	return (vel1 - vel0) / time;
}

inline Vec3 HandleAccel(Vec3 &pos, Vec3 &vel, Vec3 &accel, float time)
{
	// CalcVelocity - Chapter 15, page TODO
	vel += accel * time;
	pos += vel * time;
}

