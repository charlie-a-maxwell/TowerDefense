#pragma once

#include "StdHeader.h"

class SceneNodeProperties
{
	friend class SceneNode;

	ActorId			m_ActorId;
	std::string		m_Name;
	Mat4x4			m_toWorld, m_fromWorld;
	float			m_Radius;
	bool			m_hasAlpha;
	RenderPass		m_renderPass;

public:
	ActorId const &ActorId() const {return m_ActorId;}

	Mat4x4 const &ToWorld() const {return m_toWorld;}
	Mat4x4 const &FromWorld() const {return m_fromWorld;}
	void Transform(Mat4x4 *toWorld, Mat4x4 *fromWorld) const;

	const char * Name() const {return m_Name.c_str(); }

	float Radius() const { return m_Radius;}
	bool HasAlpha() const {return m_hasAlpha;}
	bool SetHasAlpha(bool alpha) { m_hasAlpha = alpha; return true;}
	RenderPass GetRenderPass() const {return m_renderPass;}
	void SetRenderPass(RenderPass t) {m_renderPass = t;}
};


class SceneNode: public ISceneNode
{
protected:
	SceneNodeList		m_children;
	SceneNode			*m_parent;
	SceneNodeProperties m_props;

public:
	SceneNode();
	SceneNode(ActorId id, std::string name, SceneNode *parent, RenderPass render, const Mat4x4 *to, const Mat4x4 *from=NULL);
	virtual ~SceneNode();
	virtual const SceneNodeProperties * const VGet() const {return &m_props;}
	virtual void VSetTransform(const Mat4x4 *toWorld, const Mat4x4 *fromWorld=NULL);
	virtual HRESULT VOnUpdate(Scene *, DWORD const elapsed);
	virtual HRESULT VOnRestore(Scene *pScene);

	virtual HRESULT VPreRender(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VRenderChildren(Scene *pScene);
	virtual HRESULT VPostRender(Scene *pScene);

	virtual bool VIsVisible(Scene *pScene);

	virtual bool VAddChild(shared_ptr<ISceneNode> kid);
	virtual bool VRemoveChild(ActorId id);	
	void SetRadius (const float radius) { m_props.m_Radius = radius;}
};

class CameraNode : public SceneNode
{
	Frustum		m_frustum;
	Mat4x4		m_Projection;
public:
	CameraNode(Mat4x4 const *t, Frustum const &frustum): 
	  SceneNode(-1, "Camera", NULL, RenderPass_Static, t),
	  m_frustum(frustum)
	  {
		Mat4x4 mat = Mat4x4::g_Identity;
		Vec4 at (g_Up.x, g_Up.y, g_Up.z, 0.0f);
		at = at * 26.0f;
		at.x += 4.0f;
		Vec4 atWorld = mat.Xform(at);

		Vec3 pos = mat.GetPosition() + Vec3(atWorld);
		mat.BuildRotationX(D3DX_PI / 2.0);
		mat.SetPosition(pos);
		VSetTransform(&mat);
	  }
	HRESULT SetView(Scene *pScene);
	virtual HRESULT VOnRestore(Scene *pScene);
	Frustum GetFrustum() {return m_frustum;}
};

class RootNode: public SceneNode
{
public:
	RootNode();
	virtual bool VAddChild(shared_ptr<ISceneNode> kid);
	virtual bool RemoveEffect(unsigned int num);
	virtual bool RemoveEffectById(ActorId id);
	virtual HRESULT VRenderChildren(Scene *pScene);
	virtual bool VIsVisible(Scene *pScene) const {return true;}
	virtual bool VRemoveChild(ActorId id);
};


class Scene
{
protected:
	shared_ptr<RootNode>			m_Root;
	shared_ptr<CameraNode>			m_Camera;
	ID3DXMatrixStack				*m_MatrixStack;
	AlphaSceneNodes					m_AlphaSceneNodes;
	SceneActorMap					m_ActorMap;

	void RenderAlphaPass();

public:
	Scene();
	virtual ~Scene();

	HRESULT OnRender();
	HRESULT OnRestore();
	HRESULT OnUpdate(const int deltaMilliseconds);

	shared_ptr<ISceneNode> FindActor(ActorId id);
	bool AddChild(ActorId id, shared_ptr<ISceneNode> kid)
	{
		if (id >= 0)
		{
			m_ActorMap[id] = kid;
		}
		return m_Root->VAddChild(kid);
	}
	bool RemoveChild (ActorId id)
	{ 
		m_ActorMap.erase(id);
		return m_Root->VRemoveChild(id); 
	}
	bool RemoveEffect (unsigned int num)
	{
		return m_Root->RemoveEffect(num);
	}
	bool RemoveEffectById (ActorId id)
	{
		return m_Root->RemoveEffectById(id);
	}

	void SetCamera (shared_ptr<CameraNode> camera) { m_Camera = camera;}
	const shared_ptr<CameraNode> GetCamera() const { return m_Camera;}

	void PushAndSetMatrix(const Mat4x4 &toWorld);
	void PopMatrix();
	const Mat4x4 *GetTopMatrix();

	void AddAlphaSceneNode(AlphaSceneNode asn) {m_AlphaSceneNodes.push_back(asn);}
};

class PlaneNode : public SceneNode
{
	LPDIRECT3DTEXTURE9				m_pTexture;
	LPDIRECT3DVERTEXBUFFER9			m_pVerts;
	LPDIRECT3DINDEXBUFFER9			m_pIndices;
	DWORD							m_numVerts;
	DWORD							m_numPolys;
	shared_ptr<ActorParams>			m_params;

public:
	bool							m_bTextureHasAlpha;

	PlaneNode();
	PlaneNode(shared_ptr<ActorParams> p);
	~PlaneNode();

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VPreRender(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);

	void ChangeAnimationLoop (bool loop) {m_params->m_IsPaused = loop;}
	void ChangeDirection(int dir) {m_params->m_Direction=dir; }
	virtual HRESULT VOnUpdate(Scene *pScene, const DWORD elapsedMS);
};

class LifeBarNode : public SceneNode
{
	LPDIRECT3DTEXTURE9				m_pTexture;
	LPDIRECT3DVERTEXBUFFER9			m_pVerts;
	LPDIRECT3DINDEXBUFFER9			m_pIndices;
	DWORD							m_numVerts;
	DWORD							m_numPolys;
	ActorId							m_id;
	std::string						m_textureFile;
	Mat4x4							m_TextureMat;
	float							m_maxLife;	
	DWORD							m_lastTime;
	DWORD							m_fadeOutTime;
	int								m_curLife;
	DWORD							m_alpha;
public:
	bool							m_bTextureHasAlpha;

	LifeBarNode();
	LifeBarNode(ActorId id, float startLife);
	~LifeBarNode();

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VOnUpdate(Scene *pScene, const DWORD elapsedMS);
};

class RangeNode : public SceneNode
{
	LPDIRECT3DTEXTURE9				m_pTexture;
	LPDIRECT3DVERTEXBUFFER9			m_pVerts;
	LPDIRECT3DINDEXBUFFER9			m_pIndices;
	DWORD							m_numVerts;
	DWORD							m_numPolys;
	std::string						m_textureFile;
	Mat4x4							m_TextureMat;
	float							m_range;
public:

	RangeNode();
	RangeNode(float range);
	~RangeNode();

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
};

class ShotNode : public SceneNode
{
	LPDIRECT3DTEXTURE9				m_pTexture;
	LPDIRECT3DVERTEXBUFFER9			m_pVerts;
	LPDIRECT3DINDEXBUFFER9			m_pIndices;
	DWORD							m_numVerts;
	DWORD							m_numPolys;
	float							m_distance;
	ActorId							m_id;
	std::string						m_textureFile;
	int								m_elapsedTime;
	Mat4x4							m_TextureMat;
public:
	bool							m_bTextureHasAlpha;
	unsigned int					m_shotNum;
	int								m_timeLeft;

	ShotNode();
	ShotNode(ActorId id, int time, unsigned int num,std::string texture, Mat4x4 start, Mat4x4 end);
	~ShotNode();

	virtual HRESULT VOnRestore(Scene *pScene);
	virtual HRESULT VRender(Scene *pScene);
	virtual HRESULT VOnUpdate(Scene *pScene, const DWORD elapsedMS);
};


class HumanPlayerScene : public IScreenElement, public Scene
{
public:
	HumanPlayerScene(): Scene() {}

	// IScreenElements stuff
	virtual void VOnUpdate(int deltaMS)					{OnUpdate(deltaMS);}
	virtual HRESULT VOnRestore()						{OnRestore(); return S_OK;}
	virtual HRESULT VRender(double fTime, float fElapsedTIme) {OnRender(); return S_OK;}
	virtual int VGetZOrder() const						{return 1;}
	virtual void VSetZOrder(int const zOrder)			{}

	virtual LRESULT CALLBACK VOnMsgProc(AppMsg msgP)	{return 0;}

	virtual bool VIsVisible() const						{return true;}
	virtual void VSetVisible(bool visible)				{ }
	virtual bool VAddChild(ActorId id, shared_ptr<ISceneNode> kid) {return Scene::AddChild(id, kid);}
};