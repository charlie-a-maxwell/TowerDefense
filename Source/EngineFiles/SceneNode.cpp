#include "StdHeader.h"
#include "SceneNode.h"
#include "ResourceCache\ResCache2.h"
#include "EngineFiles\Game.h"


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////SceneNodeProperties////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Sets the Scene nodes properties matices.
void SceneNodeProperties::Transform(Mat4x4 *toWorld, Mat4x4 *fromWorld) const
{
	if (toWorld)
		*toWorld = m_toWorld;

	if (fromWorld)
		*fromWorld = m_fromWorld;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////SceneNode//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


SceneNode::SceneNode()
{
	m_props.m_ActorId = -1;
	m_props.m_Name = "";
	m_props.m_toWorld = m_props.m_fromWorld = Mat4x4::g_Identity;
	m_props.m_Radius = 0;
	m_props.m_renderPass = RenderPass_Static;
}

SceneNode::SceneNode(ActorId id, std::string name, SceneNode *parent, RenderPass render, const Mat4x4 *to, const Mat4x4 *from)
{
	m_parent = parent;
	m_props.m_ActorId = id;
	m_props.m_Name = name;
	VSetTransform(to, from);
	m_props.m_Radius = 0;
	m_props.m_renderPass = render;
}

SceneNode::~SceneNode()
{
}

// Sets the matrices for the scene node, does an inverse if there isn't a from world.
void SceneNode::VSetTransform(const Mat4x4 *toWorld, const Mat4x4 *fromWorld)
{
	if (toWorld)
	{
		m_props.m_toWorld = *toWorld;
		if (fromWorld)
			m_props.m_fromWorld = *fromWorld;
		else
			m_props.m_fromWorld = m_props.m_toWorld.Inverse();
	}
}

// Called to update the scene nodes
HRESULT SceneNode::VOnUpdate(Scene *pScene, DWORD const elapsed)
{
	HRESULT hr = S_OK;

	for (SceneNodeList::iterator it = m_children.begin(); it != m_children.end(); it++)
	{
		if (*it)
			hr = (*it)->VOnUpdate(pScene, elapsed);
	}

	return hr;
}

// Called when the display device is restored
HRESULT SceneNode::VOnRestore(Scene *pScene)
{
	HRESULT hr = S_OK;

	for (SceneNodeList::iterator it = m_children.begin(); it != m_children.end(); it++)
	{
		if (*it)
			hr = (*it)->VOnRestore(pScene);
	}

	return hr;
}

// Called before the scene node is rendered
HRESULT SceneNode::VPreRender(Scene *pScene)
{
	pScene->PushAndSetMatrix(m_props.m_toWorld);
	return S_OK;
}

// Renders the node.
HRESULT SceneNode::VRender(Scene *pScene)
{
	return S_OK;
}

// Iterates through children, rendering them.
HRESULT SceneNode::VRenderChildren(Scene *pScene)
{
	HRESULT hr = S_OK;

	for (SceneNodeList::iterator it = m_children.begin(); it != m_children.end(); it++)
	{
		if ((*it)->VPreRender(pScene) == S_OK)
		{
			if ((*it)->VIsVisible(pScene))
			{
				// If the node has any alpha value, it adds it to the alpha scene nodes to be rendered later
				if (!(*it)->VGet()->HasAlpha())
				{
					hr = (*it)->VRender(pScene);
				}
				else
				{
					AlphaSceneNode asn;
					asn.m_pNode = (*it);
					asn.m_concat = *pScene->GetTopMatrix();
					
					Vec4 worldPos(asn.m_concat.GetPosition());
					Mat4x4 fromWorld = pScene->GetCamera()->VGet()->FromWorld();
					Vec4 screenPos = fromWorld.Xform(worldPos);
					asn.m_ScreenZ = screenPos.y;

					pScene->AddAlphaSceneNode(asn);
				}
			}
			(*it)->VRenderChildren(pScene);
		}
		(*it)->VPostRender(pScene);
	}

	return hr;
}

// Any clean up that needs to be done for the scene node
HRESULT SceneNode::VPostRender(Scene *pScene)
{
	pScene->PopMatrix();
	return S_OK;
}

// Checks if the scene node can be culled safely.
bool SceneNode::VIsVisible(Scene *pScene)
{
	Mat4x4 toWorld, fromWorld;

	pScene->GetCamera()->VGet()->Transform(&toWorld, &fromWorld);

	Vec3 pos = VGet()->ToWorld().GetPosition();

	pos = fromWorld.Xform(pos);

	Frustum const &frustum = pScene->GetCamera()->GetFrustum();
	return frustum.Inside(pos, VGet()->Radius());
}

// Adds a child node to the scene node.
bool SceneNode::VAddChild(shared_ptr<ISceneNode> kid)
{
	m_children.push_back(kid);
	Vec3 kidPos = kid->VGet()->ToWorld().GetPosition();
	Vec3 dir = kidPos - m_props.ToWorld().GetPosition();
	float newRadius = dir.Length() + kid->VGet()->Radius();
	// updates the radius of the node
	if (newRadius > m_props.m_Radius)
		m_props.m_Radius = newRadius;
	
	return true;
}

// Removes a child with the given id.
bool SceneNode::VRemoveChild(ActorId id)
{
	for (SceneNodeList::iterator it = m_children.begin(); it != m_children.end(); it++)
	{
		if ((*it) && ((*it)->VGet()->ActorId() == id))
		{
			it = m_children.erase(it);
			return true;
		}
		(*it)->VRemoveChild(id);

	}
	return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Scene//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

Scene::Scene()
{
	m_Root.reset(SAFE_NEW RootNode());
	D3DXCreateMatrixStack(0, &m_MatrixStack);
}

Scene::~Scene()
{
	SAFE_RELEASE(m_MatrixStack);
}

// Calls the root scene node to render.
HRESULT Scene::OnRender()
{
	if (m_Root && m_Camera)
	{
		m_Camera->SetView(this);

		if (m_Root->VPreRender(this)==S_OK)
		{
			m_Root->VRender(this);
			m_Root->VRenderChildren(this);
			m_Root->VPostRender(this);
		}
	}

	RenderAlphaPass();

	return S_OK;
}

// Sends the restore method along to the scene node.
HRESULT Scene::OnRestore()
{
	if (!m_Root)
		return S_OK;

	return m_Root->VOnRestore(this);
}

// Sends the update message along to the root scene node.
HRESULT Scene::OnUpdate(const int deltaMilliseconds)
{
	if (!m_Root)
		return S_OK;

	return m_Root->VOnUpdate(this, deltaMilliseconds);
}

// Finds the scene node for the actor
shared_ptr<ISceneNode> Scene::FindActor(ActorId id)
{
	SceneActorMap::iterator it = m_ActorMap.find(id);

	if (it == m_ActorMap.end())
	{
		shared_ptr<ISceneNode> node;
		return node;
	}
	return (*it).second;
}

// Pushes the matrix onto the matrix stack for D3D
void Scene::PushAndSetMatrix(const Mat4x4 &toWorld)
{
	m_MatrixStack->Push();
	m_MatrixStack->MultMatrixLocal(&toWorld);
	DXUTGetD3DDevice()->SetTransform(D3DTS_WORLD, m_MatrixStack->GetTop());
}

// Pops the top of the matrix stack
void Scene::PopMatrix()
{
	m_MatrixStack->Pop();
	DXUTGetD3DDevice()->SetTransform(D3DTS_WORLD, m_MatrixStack->GetTop());
}

// Gets the top matrix on the matrix stack
const Mat4x4* Scene::GetTopMatrix()
{
	return static_cast<const Mat4x4 *>(m_MatrixStack->GetTop());
}

// Renders the alpha scene nodes.
void Scene::RenderAlphaPass()
{
	Mat4x4 oldWorld;

	DXUTGetD3DDevice()->GetTransform(D3DTS_WORLD, &oldWorld);
	
	// Sets up the alpha blending
	DWORD oldZWriteEnable;
	DXUTGetD3DDevice()->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, false);

	DXUTGetD3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	m_AlphaSceneNodes.sort();

	// Iterates through the list backward, drawing closer items first.
	while (!m_AlphaSceneNodes.empty())
	{
		AlphaSceneNodes::reverse_iterator i = m_AlphaSceneNodes.rbegin();
		Mat4x4 cat = ((*i).m_concat);
		DXUTGetD3DDevice()->SetTransform(D3DTS_WORLD, &cat);
		(*i).m_pNode->VRender(this);
		m_AlphaSceneNodes.pop_back();
	}
	
	DXUTGetD3DDevice()->SetRenderState(D3DRS_COLORVERTEX, false);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
	DXUTGetD3DDevice()->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);

	DXUTGetD3DDevice()->SetTransform(D3DTS_WORLD, &oldWorld);
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CameraNode/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Sets the view matrix for the camera
HRESULT CameraNode::SetView(Scene *pScene)
{
	Mat4x4 fromWorld = VGet()->FromWorld();

	DXUTGetD3DDevice()->SetTransform( D3DTS_VIEW, &fromWorld );
	return S_OK;
}

// Rebuilds the frustum on the display device being restored.
HRESULT CameraNode::VOnRestore(Scene *pScene)
{
	m_frustum.SetAspect(
		DXUTGetBackBufferSurfaceDesc()->Width / (FLOAT)DXUTGetBackBufferSurfaceDesc()->Height);

	D3DXMatrixPerspectiveFovLH( &m_Projection, m_frustum.m_Fov, 
		m_frustum.m_Aspect, m_frustum.m_Near, m_frustum.m_Far);

	DXUTGetD3DDevice()->SetTransform( D3DTS_PROJECTION, &m_Projection);
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////RootNode///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


// Starts all the different types of scene nodes.
RootNode::RootNode():SceneNode(-1, "Root", NULL, RenderPass_Static, &Mat4x4::g_Identity)
{
	m_children.reserve(RenderPass_Last);

	shared_ptr<SceneNode> staticGroup( new SceneNode(-1, "StaticGroup", NULL, RenderPass_Static, &Mat4x4::g_Identity));
	m_children.push_back(staticGroup);

	shared_ptr<SceneNode> staticActor( new SceneNode(-1, "ActorGroup", NULL, RenderPass_Actor, &Mat4x4::g_Identity));
	m_children.push_back(staticActor);

	shared_ptr<SceneNode> staticEffect( new SceneNode(-1, "EffectGroup", NULL, RenderPass_Effect, &Mat4x4::g_Identity));
	m_children.push_back(staticEffect);
}

// Adds a child to the correct render pass
bool RootNode::VAddChild(shared_ptr<ISceneNode> kid)
{
	if (!m_children[kid->VGet()->GetRenderPass()])
	{
		assert( 0 && _T("There is no such render pass"));
		return false;
	}

	return m_children[kid->VGet()->GetRenderPass()]->VAddChild(kid);
}

// Renders the nodes in the pass order
HRESULT RootNode::VRenderChildren(Scene *pScene)
{
	for (int pass = RenderPass_0; pass < RenderPass_Last; ++pass)
	{
		switch (pass)
		{
		case RenderPass_Static:
		case RenderPass_Actor:
		case RenderPass_Effect:
			m_children[pass]->VRenderChildren(pScene);
			break;

		case RenderPass_Sky:
			break;
		}
	}

	return S_OK;
}

// Removes an effect from the node list.
bool RootNode::RemoveEffect(unsigned int num)
{	
	m_children[RenderPass_Effect]->VRemoveChild(num);
	return false;
}

// Removes an effect from the node list
bool RootNode::RemoveEffectById(ActorId id)
{	
	m_children[RenderPass_Effect]->VRemoveChild(id);
	return false;
}

// Removes an actor from the node list
bool RootNode::VRemoveChild(ActorId id)
{
	for (int pass = RenderPass_0; pass < RenderPass_Last; ++pass)
	{
		switch (pass)
		{
		case RenderPass_Static:
		case RenderPass_Actor:
			if (m_children[pass]->VRemoveChild(id))
				return true;
			break;

		case RenderPass_Effect:
		case RenderPass_Sky:
			break;
		}
	}
	return false;
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////PlaneNode//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


PlaneNode::PlaneNode():SceneNode()
{
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
}

PlaneNode::PlaneNode(shared_ptr<ActorParams> p):SceneNode(p->m_Id, "PlaneNode", NULL, RenderPass_Static, &p->m_Mat)
{
	m_params = p;
	m_props.SetHasAlpha(p->m_hasTextureAlpha);
	m_bTextureHasAlpha = p->m_hasTextureAlpha;
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
	if (p->m_Type != AT_GROUND)
	{
		m_props.SetRenderPass(RenderPass_Actor);
	}

	m_numVerts = m_numPolys = 0;
}

PlaneNode::~PlaneNode()
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);
}

HRESULT PlaneNode::VPreRender(Scene *pScene)
{
	pScene->PushAndSetMatrix(m_params->m_Mat);
	return S_OK;
}

// Sets up the resources for the plane node 
HRESULT PlaneNode::VOnRestore(Scene *pScene)
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);

	SceneNode::VOnRestore(pScene);

	// Get the texture from the resource file
	Resource resource(m_params->m_Texture);
	int size = g_App->m_ResCache->Create(resource);

	assert(size);
	char *textureBuffer = (char *)g_App->m_ResCache->Get(resource);

	if (FAILED( D3DXCreateTextureFromFileInMemory( DXUTGetD3DDevice(),
				textureBuffer, resource.m_size, &m_pTexture) ) )
		return E_FAIL;

	SetRadius( sqrt(2.0f * m_params->m_Squares) );


	m_numVerts = (m_params->m_Squares+1)*(m_params->m_Squares+1);

	if ( FAILED( DXUTGetD3DDevice ()->CreateVertexBuffer(m_numVerts*sizeof(COLORED_TEXTURED_VERTEX),
		D3DUSAGE_WRITEONLY, COLORED_TEXTURED_VERTEX::FVF, D3DPOOL_MANAGED, &m_pVerts, NULL) ) )
		return E_FAIL;

	COLORED_TEXTURED_VERTEX* pVertices;

	if ( FAILED (m_pVerts->Lock(0, 0, (void**)&pVertices, 0 )))
		return E_FAIL;

	// Goes through the number of squares and makes that many planes.
	for (DWORD j=0; j<(m_params->m_Squares+1); j++)
	{
		for (DWORD i=0; i<(m_params->m_Squares+1); i++)
		{
			int index = i + (j * (m_params->m_Squares+1));
			COLORED_TEXTURED_VERTEX *vert = &pVertices[index];

			float x = (float)i - (m_params->m_Squares/2.0f);
			float y = (float)j - (m_params->m_Squares/2.0f);

			vert->position = (x * Vec3(1,0,0) )* m_params->m_ActualWidth + (y * Vec3(0,0,1))*m_params->m_ActualHeight;
			vert->color = m_params->m_Color;

			if (m_params->m_Type == AT_GROUND)
			{
				vert->tu = x;
				vert->tv = y;
			}
			else
			{
				vert->tu = (float)((i)  / (float)m_params->m_NumFrames);
				vert->tv = (float)((1-j)  / (float)m_params->m_NumDirections);
			}
		}
	}

	m_pVerts->Unlock();

	m_numPolys = m_params->m_Squares*m_params->m_Squares*2;

	if ( FAILED( DXUTGetD3DDevice()->CreateIndexBuffer(sizeof(WORD) * m_numPolys * 3,
				D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndices, NULL) ) )
		return E_FAIL;
	
	WORD *pIndices;
	if ( FAILED( m_pIndices->Lock(0, 0, (void**) &pIndices, 0 ) ) )
		return E_FAIL;

	// A - B
	// | / |
	// C - D
	
	// Sets the indices
	for (DWORD j=0; j<m_params->m_Squares; j++)
	{
		for (DWORD i=0; i<m_params->m_Squares; i++)
		{
			// Triangle #1 ACB
			*(pIndices)   = WORD(i     + ( j   *(m_params->m_Squares+1)));
			*(pIndices+1) = WORD(i     + ((j+1)*(m_params->m_Squares+1)));
			*(pIndices+2) = WORD((i+1) + ( j   *(m_params->m_Squares+1)));

			// Triangle #2 BCD
			*(pIndices+3) = WORD((i+1) + (j    *(m_params->m_Squares+1)));
			*(pIndices+4) = WORD(i     + ((j+1)*(m_params->m_Squares+1)));
			*(pIndices+5) = WORD((i+1) + ((j+1)*(m_params->m_Squares+1)));
			pIndices+=6;
		}
	}

	m_pIndices->Unlock();

	return S_OK;
}

// Plane node rendering code
HRESULT PlaneNode::VRender(Scene *pScene)
{
	// Sets the state in D3D for scene node.
	DWORD oldLightMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE);
	
	DWORD oldCullMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_CULLMODE, &oldCullMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
	DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );

	DXUTGetD3DDevice()->SetTexture( 0, m_pTexture);

	if (m_bTextureHasAlpha)
	{
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);  
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	}

	// Transforms the texture to where it needs to be.
	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(m_params->m_TextureMat) ); 
	DXUTGetD3DDevice()->SetStreamSource(0, m_pVerts, 0, sizeof(COLORED_TEXTURED_VERTEX) );
	DXUTGetD3DDevice()->SetIndices(m_pIndices);
	DXUTGetD3DDevice()->SetFVF( COLORED_TEXTURED_VERTEX::FVF );
	DXUTGetD3DDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVerts, 0, m_numPolys);

	// Reset the state
	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(Mat4x4::g_Identity) ); 
	DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	DXUTGetD3DDevice()->SetTexture(0, NULL);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, oldLightMode );
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, oldCullMode );

	return S_OK;
}

// Update for plane nodes
HRESULT PlaneNode::VOnUpdate(Scene *pScene, const DWORD elapsedMS)
{
	SceneNode::VOnUpdate(pScene, elapsedMS);
	m_params->m_ElapsedTime += elapsedMS;

	// Checks if the animation should move to the next frame.
	if (m_params->m_ElapsedTime >= m_params->m_MSPerFrame)
	{
		DWORD const numFramesToAdvance = (m_params->m_ElapsedTime / m_params->m_MSPerFrame);

		m_params->m_ElapsedTime -= (numFramesToAdvance * m_params->m_MSPerFrame);

		int desiredFrame = m_params->m_Frame+ numFramesToAdvance;

		if ( (false == m_params->m_LoopingAnim) && (desiredFrame >= m_params->m_NumFrames) )
		{
			desiredFrame = m_params->m_NumFrames - 1;
		}

		if (desiredFrame % m_params->m_NumFrames == 0)
			desiredFrame = 1; 

		if (!m_params->m_IsPaused)
			desiredFrame = 0;

		m_params->m_Frame = desiredFrame % m_params->m_NumFrames;
	}

	Mat4x4 trans = Mat4x4::g_Identity; // this is kind of nasty, probably shouldn't be changing the matrix like this
	trans.m[2][0] = m_params->m_Frame / (float)m_params->m_NumFrames;
	trans.m[2][1] = m_params->m_Direction / (float)m_params->m_NumDirections;

	m_params->m_TextureMat = trans;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////ShotNode///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


ShotNode::ShotNode():SceneNode(), m_elapsedTime(0)
{
}

ShotNode::ShotNode(ActorId id, int time, unsigned int num, std::string texture, Mat4x4 start, Mat4x4 end):SceneNode(num, "ShotNode", NULL, RenderPass_Effect, &start),
					m_shotNum(num),m_timeLeft(time),m_id(id),m_textureFile(texture),m_elapsedTime(0)
{
	Vec3 s = start.GetPosition();
	Vec3 e = end.GetPosition();
	float direction = atan2(e.z - s.z, e.x - s.x);

	Mat4x4 turn;
	turn.BuildRotationY(-direction);
	turn.SetPosition(s);
	VSetTransform(&turn);
	m_distance = s.Distance(e);
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
}

ShotNode::~ShotNode()
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);
}

// Called when the display device is restored
HRESULT ShotNode::VOnRestore(Scene *pScene)
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);

	SceneNode::VOnRestore(pScene);

	// Gets the texture from the recource file.
	Resource resource(m_textureFile);
	int size = g_App->m_ResCache->Create(resource);

	assert(size);
	char *textureBuffer = (char *)g_App->m_ResCache->Get(resource);

	if (FAILED( D3DXCreateTextureFromFileInMemory( DXUTGetD3DDevice(),
				textureBuffer, resource.m_size, &m_pTexture) ) )
		return E_FAIL;

	SetRadius(4);
	m_numVerts = 4;

	if ( FAILED( DXUTGetD3DDevice ()->CreateVertexBuffer(m_numVerts*sizeof(COLORED_TEXTURED_VERTEX),
		D3DUSAGE_WRITEONLY, COLORED_TEXTURED_VERTEX::FVF, D3DPOOL_MANAGED, &m_pVerts, NULL) ) )
		return E_FAIL;

	COLORED_TEXTURED_VERTEX* pVertices;

	if ( FAILED (m_pVerts->Lock(0, 0, (void**)&pVertices, 0 )))
		return E_FAIL;

	// Only has 1 square, so 4 vertices
	for (DWORD j=0; j<2; j++)
	{
		for (DWORD i=0; i<2; i++)
		{
			int index = i + (j * 2);
			COLORED_TEXTURED_VERTEX *vert = &pVertices[index];

			float x = (float)i * (m_distance);
			float y = (float)j * 0.1;

			vert->position = (x * Vec3(1,0,0) ) + (y * Vec3(0,0,1));
			vert->color = g_White;
			vert->tu = (float)(i);
			vert->tv = (float)(1-j);
		}
	}

	m_pVerts->Unlock();

	m_numPolys = 2;

	if ( FAILED( DXUTGetD3DDevice()->CreateIndexBuffer(sizeof(WORD) * m_numPolys * 3,
				D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndices, NULL) ) )
		return E_FAIL;
	
	WORD *pIndices;
	if ( FAILED( m_pIndices->Lock(0, 0, (void**) &pIndices, 0 ) ) )
		return E_FAIL;

	// A - B
	// | / |
	// C - D
	
	
	// Triangle #1 ACB
	*(pIndices)   = 0;
	*(pIndices+1) = 2;
	*(pIndices+2) = 1;

	// Triangle #2 BCD
	*(pIndices+3) = 1;
	*(pIndices+4) = 2;
	*(pIndices+5) = 3;


	m_pIndices->Unlock();

	return S_OK;
}

// Renders a shot node
HRESULT ShotNode::VRender(Scene *pScene)
{
	// Sets the state needed for the shot node
	DWORD oldLightMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE);
	
	DWORD oldCullMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_CULLMODE, &oldCullMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
	DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );

	DXUTGetD3DDevice()->SetTexture( 0, m_pTexture);

	if (m_bTextureHasAlpha)
	{
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);  
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	}

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(m_TextureMat) ); 
	DXUTGetD3DDevice()->SetStreamSource(0, m_pVerts, 0, sizeof(COLORED_TEXTURED_VERTEX) );
	DXUTGetD3DDevice()->SetIndices(m_pIndices);
	DXUTGetD3DDevice()->SetFVF( COLORED_TEXTURED_VERTEX::FVF );
	DXUTGetD3DDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVerts, 0, m_numPolys);

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(Mat4x4::g_Identity) ); 
	DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	DXUTGetD3DDevice()->SetTexture(0, NULL);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, oldLightMode );
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, oldCullMode );

	return S_OK;
}

// Shot node update function
HRESULT ShotNode::VOnUpdate(Scene *pScene, const DWORD elapsedMS)
{
	// Checks how much longer it should stay there
	m_timeLeft -= elapsedMS;
	if (m_timeLeft <= 0)	
	{
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Effect(m_shotNum)));
	}


	m_elapsedTime += elapsedMS;
	DWORD const numFramesToAdvance = (m_elapsedTime / 100);

	Mat4x4 trans = Mat4x4::g_Identity; // this is kind of nasty, probably shouldn't be changing the matrix like this
	trans.m[2][0] = -(float)(m_elapsedTime / 100.0f);

	m_elapsedTime -= (numFramesToAdvance * 100.0f);
	m_TextureMat = trans;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////LifeBarNode////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


LifeBarNode::LifeBarNode():SceneNode()
{
	Mat4x4 m;
	m.BuildTranslation(g_Forward*0.5);
	VSetTransform(&m);
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
}
LifeBarNode::LifeBarNode(ActorId id, float startlife):SceneNode(-1, "ShotNode", NULL, RenderPass_Effect, &Mat4x4::g_Identity),
					m_id(id),m_textureFile("lifebar.bmp"),m_TextureMat(),m_maxLife(startlife),m_fadeOutTime(1000),m_lastTime(0), m_curLife(startlife),m_alpha(0)
{
	Mat4x4 m;
	m.BuildTranslation(g_Forward*0.5);
	VSetTransform(&m);
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
}

LifeBarNode::~LifeBarNode()
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);
}

// Function called when display device is restored
HRESULT LifeBarNode::VOnRestore(Scene *pScene)
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);

	SceneNode::VOnRestore(pScene);

	Resource resource(m_textureFile);
	int size = g_App->m_ResCache->Create(resource);

	assert(size);
	char *textureBuffer = (char *)g_App->m_ResCache->Get(resource);

	// Creates the texture from the resource cache
	if (FAILED( D3DXCreateTextureFromFileInMemory( DXUTGetD3DDevice(),
				textureBuffer, resource.m_size, &m_pTexture) ) )
		return E_FAIL;

	SetRadius(4);

	m_numVerts = 4;

	if ( FAILED( DXUTGetD3DDevice ()->CreateVertexBuffer(m_numVerts*sizeof(COLORED_TEXTURED_VERTEX),
		D3DUSAGE_WRITEONLY, COLORED_TEXTURED_VERTEX::FVF, D3DPOOL_MANAGED, &m_pVerts, NULL) ) )
		return E_FAIL;

	COLORED_TEXTURED_VERTEX* pVertices;

	if ( FAILED (m_pVerts->Lock(0, 0, (void**)&pVertices, 0 )))
		return E_FAIL;

	for (DWORD j=0; j<2; j++)
	{
		for (DWORD i=0; i<2; i++)
		{
			int index = i + (j * 2);
			COLORED_TEXTURED_VERTEX *vert = &pVertices[index];

			float x = (float)(i *0.7f) - 0.3f;
			float y = (float)j * 0.1;

			vert->position = (x * Vec3(1,0,0) ) + (y * Vec3(0,0,1));
			vert->color = g_White;
			vert->tu = (float)(i)*0.5;
			vert->tv = (float)(1-j);
		}
	}

	m_pVerts->Unlock();

	m_numPolys = 2;

	if ( FAILED( DXUTGetD3DDevice()->CreateIndexBuffer(sizeof(WORD) * m_numPolys * 3,
				D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndices, NULL) ) )
		return E_FAIL;
	
	WORD *pIndices;
	if ( FAILED( m_pIndices->Lock(0, 0, (void**) &pIndices, 0 ) ) )
		return E_FAIL;

	// A - B
	// | / |
	// C - D
	
	// Triangle #1 ACB
	*(pIndices)   = 0;
	*(pIndices+1) = 2;
	*(pIndices+2) = 1;

	// Triangle #2 BCD
	*(pIndices+3) = 1;
	*(pIndices+4) = 2;
	*(pIndices+5) = 3;


	m_pIndices->Unlock();

	return S_OK;
}

//Render function for the life bar node
HRESULT LifeBarNode::VRender(Scene *pScene)
{
	DWORD oldLightMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE);
	
	DWORD oldCullMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_CULLMODE, &oldCullMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
	DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );

	DXUTGetD3DDevice()->SetTexture( 0, m_pTexture);

	if (m_bTextureHasAlpha)
	{
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);  
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_CONSTANT, m_alpha);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CONSTANT);

		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	}

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(m_TextureMat) ); 
	DXUTGetD3DDevice()->SetStreamSource(0, m_pVerts, 0, sizeof(COLORED_TEXTURED_VERTEX) );
	DXUTGetD3DDevice()->SetIndices(m_pIndices);
	DXUTGetD3DDevice()->SetFVF( COLORED_TEXTURED_VERTEX::FVF );
	DXUTGetD3DDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVerts, 0, m_numPolys);

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(Mat4x4::g_Identity) ); 
	DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	DXUTGetD3DDevice()->SetTexture(0, NULL);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, oldLightMode );
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, oldCullMode );

	return S_OK;
}

// Sets how far the life bar should be for the actor it is over
HRESULT LifeBarNode::VOnUpdate(Scene *pScene, const DWORD elapsedMS)
{
	shared_ptr<IActor> actor = g_App->m_pGame->GetActor(m_id);
	if (actor)
	{
		float curLife = actor->VGet()->m_life;

		m_lastTime += elapsedMS;
		if (curLife != m_curLife)
		{
			m_curLife = curLife;
			m_lastTime = 0;
		}
		int time = m_fadeOutTime - m_lastTime;
		if (time > 0)
		{
			float alphaChange = (float)((float)time/(float)200);
			if (alphaChange >1)
				alphaChange = 1;
			alphaChange = alphaChange*255;
			m_alpha =  D3DCOLOR_ARGB((int)(alphaChange),255,255,255);
		}
		Mat4x4 trans = Mat4x4::g_Identity; // this is kind of nasty, probably shouldn't be changing the matrix like this
		trans.m[2][0] = -(float)(curLife / m_maxLife) * 0.5;

		m_TextureMat = trans;
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////RangeNode//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


RangeNode::RangeNode():SceneNode()
{
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
}

RangeNode::RangeNode(float range):SceneNode(-1, "ShotNode", NULL, RenderPass_Effect, &Mat4x4::g_Identity),
						m_textureFile("circle.dds"),m_TextureMat( Mat4x4::g_Identity),m_range(range)
{
	m_pTexture = NULL;
	m_pVerts = NULL;
	m_pIndices = NULL;
	m_props.SetHasAlpha(true);
}

RangeNode::~RangeNode()
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);
}

HRESULT RangeNode::VOnRestore(Scene *pScene)
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pVerts);
	SAFE_RELEASE(m_pIndices);

	SceneNode::VOnRestore(pScene);

	Resource resource(m_textureFile);
	int size = g_App->m_ResCache->Create(resource);

	assert(size);
	char *textureBuffer = (char *)g_App->m_ResCache->Get(resource);

	if (FAILED( D3DXCreateTextureFromFileInMemory( DXUTGetD3DDevice(),
				textureBuffer, resource.m_size, &m_pTexture) ) )
		return E_FAIL;

	SetRadius(4);


	m_numVerts = 4;

	if ( FAILED( DXUTGetD3DDevice ()->CreateVertexBuffer(m_numVerts*sizeof(COLORED_TEXTURED_VERTEX),
		D3DUSAGE_WRITEONLY, COLORED_TEXTURED_VERTEX::FVF, D3DPOOL_MANAGED, &m_pVerts, NULL) ) )
		return E_FAIL;

	COLORED_TEXTURED_VERTEX* pVertices;

	if ( FAILED (m_pVerts->Lock(0, 0, (void**)&pVertices, 0 )))
		return E_FAIL;

	float diameter = m_range * 2;
	for (DWORD j=0; j<2; j++)
	{
		for (DWORD i=0; i<2; i++)
		{
			int index = i + (j * 2);
			COLORED_TEXTURED_VERTEX *vert = &pVertices[index];

			float x = (float)i - 0.5f;
			float y = (float)j - 0.5f;

			vert->position = (x * Vec3(1,0,0)) * diameter + (y * Vec3(0,0,1)) * diameter;
			vert->color = g_White;
			vert->tu = (float)(i);
			vert->tv = (float)(j);
		}
	}

	m_pVerts->Unlock();

	m_numPolys = 2;

	if ( FAILED( DXUTGetD3DDevice()->CreateIndexBuffer(sizeof(WORD) * m_numPolys * 3,
				D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndices, NULL) ) )
		return E_FAIL;
	
	WORD *pIndices;
	if ( FAILED( m_pIndices->Lock(0, 0, (void**) &pIndices, 0 ) ) )
		return E_FAIL;

	// A - B
	// | / |
	// C - D
	
	
	// Triangle #1 ACB
	*(pIndices)   = 0;
	*(pIndices+1) = 2;
	*(pIndices+2) = 1;

	// Triangle #2 BCD
	*(pIndices+3) = 1;
	*(pIndices+4) = 2;
	*(pIndices+5) = 3;


	m_pIndices->Unlock();

	return S_OK;
}

// this is pulled from Game Coding Complete
HRESULT RangeNode::VRender(Scene *pScene)
{
	DWORD oldLightMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE);
	
	DWORD oldCullMode;
	DXUTGetD3DDevice()->GetRenderState( D3DRS_CULLMODE, &oldCullMode);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
	DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );

	DXUTGetD3DDevice()->SetTexture( 0, m_pTexture);

	if (true)
	{
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);  
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

		DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		DXUTGetD3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	}

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(m_TextureMat) ); 
	DXUTGetD3DDevice()->SetStreamSource(0, m_pVerts, 0, sizeof(COLORED_TEXTURED_VERTEX) );
	DXUTGetD3DDevice()->SetIndices(m_pIndices);
	DXUTGetD3DDevice()->SetFVF( COLORED_TEXTURED_VERTEX::FVF );
	DXUTGetD3DDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVerts, 0, m_numPolys);

	DXUTGetD3DDevice()->SetTransform( D3DTS_TEXTURE0, &(Mat4x4::g_Identity) ); 
	DXUTGetD3DDevice()->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	DXUTGetD3DDevice()->SetTexture(0, NULL);
	DXUTGetD3DDevice()->SetRenderState( D3DRS_LIGHTING, oldLightMode );
	DXUTGetD3DDevice()->SetRenderState( D3DRS_CULLMODE, oldCullMode );

	return S_OK;
}