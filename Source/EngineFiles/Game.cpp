/*
The basic game engine code. This is based on the book Game Coding Complete. 
The general structure is:
a base class that directly interfaces with the OS.
a game class that holds the logic for the game.
a view class that interacts with the player (video, audio, input)

Then the even system is used to tie it all together.
*/






#include "StdHeader.h"
#include "Game.h"
#include "..\ResourceCache\ResCache2.h"
#include <direct.h>
#include "SceneNode.h"
#include "Event.h"
#include <time.h>
#include "LuaReader.h"
#include "Sound.h"


GameApp *g_App;


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////GameApp////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Basic Constructor
GameApp::GameApp():m_eventManager()
{
	g_App = this;
	m_pGame = NULL;
	m_ResCache = NULL;
}

// Called before the object is destroyed to clean up variables.
int GameApp::OnClose()
{
	SAFE_DELETE(m_pGame);

	DestroyWindow(GetHwnd());

	SAFE_DELETE(m_ResCache);
	return 0;
}


/// checks the free space on a hard drive
// from Game Code Complete
bool GameApp::CheckHardDisk(int diskSpace)
{
	int const drive = _getdrive();

	struct _diskfree_t diskfree;

	_getdiskfree(drive, &diskfree);

	unsigned int const neededClusters = 
		diskSpace / (diskfree.sectors_per_cluster * diskfree.bytes_per_sector);

	if (diskfree.avail_clusters < neededClusters)
		return false;

	return true;
}


// checks the memory on the computer
// from Game Code Complete
bool GameApp::CheckMemory(const DWORD physicalRAM, const DWORD virtualRAM)
{
	MEMORYSTATUS status;

	GlobalMemoryStatus(&status);

	if (status.dwTotalPhys < physicalRAM)
		return false;

	if (status.dwAvailVirtual < virtualRAM)
		return false;

	char *buff = SAFE_NEW char[virtualRAM];

	if (buff)
	{
		delete[] buff;
		return true;
	}
	else
		return false;

	return true;
}

// Used to set up everything needed for the game before running.
bool GameApp::InitInstance(HINSTANCE hInstance, LPTSTR lpCommandLine)
{
	// this is from Game Code Complete
	bool resourceCheck = true;

	// Checks for enough ram and harddrive space.
	while (!resourceCheck)
	{
		const DWORD physicalRAM = 32 * MEGABYTE;
		const DWORD virtualRAM = 64 * MEGABYTE;
		if (!CheckMemory(physicalRAM, virtualRAM))
			return false;

		const int diskspace = 10 * MEGABYTE;
		if (!CheckHardDisk(diskspace))
			return false;

		const int minCPUSpeed = 266;
		extern int GetCPUSpeed();
		int thisCPU = GetCPUSpeed();

		if (thisCPU < minCPUSpeed)
			return false;

		resourceCheck = true;

	}
	
	// Opens the resource needed for the game.
	m_ResCache = SAFE_NEW ResCache(5, SAFE_NEW ResourceZipFile(_T("TowerGame.zip")));
	if (!m_ResCache->Init())
	{
		return false;
	}

	// Basic DXUT initialization.
	DXUTInit(true, true, true);

	DXUTCreateWindow(GetGameTitle(), hInstance);

	if (!GetHwnd())
		return false;

	SetWindowText( GetHwnd(), GetGameTitle() );

	// Creates the game class and the human view (only view used in this game).
	m_pGame = CreateGameAndView();
	if (!m_pGame)
		return false;

	DXUTCreateDevice( D3DADAPTER_DEFAULT, true, SCREEN_WIDTH, SCREEN_HEIGHT, IsDeviceAcceptable, ModifyDeviceSettings);

	srand(time(NULL));
	return true;
}

// Callback for interacting with the OS. Standard Windows messages.
// Most are just passed on to the appropriate classes.
LRESULT CALLBACK GameApp::MsgProc (HWND hWnd, UINT uMsg, WPARAM wParam, 
			LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
	LRESULT result;

	switch (uMsg)
	{
		case WM_DEVICECHANGE:
		{
			int event = (int)wParam;
			result = g_App->OnDeviceChange(event);
			break;
		}

		case WM_DISPLAYCHANGE:
		{
			int colorDepth = (int)wParam;
			int width = (int)(short) LOWORD(lParam);
			int height = (int)(short) HIWORD(lParam);

			result = g_App->OnDisplayChange(colorDepth, width, height);
			break;
		}

		case WM_KEYDOWN:
        case WM_KEYUP:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEWHEEL:
		{
			// Input is passed to the game views for processesing. 
			if (g_App->m_pGame)
			{
				TowerGame *pGame = g_App->m_pGame;
				AppMsg msg;
				msg.m_hWnd = hWnd;
				msg.m_uMsg = uMsg;
				msg.m_wParam = wParam;
				msg.m_lParam = lParam;
				for(GameViewList::reverse_iterator i=pGame->m_viewList.rbegin(); i!=pGame->m_viewList.rend(); ++i)
				{
					if ( (*i)->VOnMsgProc( msg ) )
					{
						result = true;
						break;
					}
				}
			}
			break;
		}

		case WM_CLOSE:
		{
			result = g_App->OnClose();
			break;
		}

		case WM_SYSCOMMAND: 
		{
			result = g_App->OnSysCommand(wParam, lParam);
			if (result)
			{
				*pbNoFurtherProcessing = true;
			}
			break;
		}

		default:
			break;
	}

	return 0;
}

// Used for system command processing.
LRESULT GameApp::OnSysCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case SC_CLOSE :
		{

			if (m_Quitting)
				return true;
			// used to close the game
			m_Quitting = true;

			return true;
		}
		return 0;

		default:
			return DefWindowProc(GetHwnd(), WM_SYSCOMMAND, wParam, lParam);
	}

	return 0;
}

// Used to restore the device
HRESULT CALLBACK GameApp::OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void *pUserContext  )
{
	if (g_App->m_pGame)
	{
		GameViewList::iterator it;
		TowerGame* game = g_App->m_pGame;
		for (it = game->m_viewList.begin(); it != game->m_viewList.end(); it++)
		{
			(*it)->VOnRestore();
		}
	}
	return 0;
}

// Called when the device is lost
void CALLBACK GameApp::OnLostDevice(void *pUserContext)
{
	HRESULT hr = D3DERR_DEVICELOST ;
	if (g_App->m_pGame)
	{
		GameViewList::iterator it;
		TowerGame* game = g_App->m_pGame;
		for (it = game->m_viewList.begin(); it != game->m_viewList.end(); it++)
		{
			(*it)->VOnLostDevice();
		}
	}
}


// Checks if the display is useable or not.
bool CALLBACK GameApp::IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	IDirect3D9* pD3D = DXUTGetD3DObject(); 
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}

// Called to update the game AKA the main loop.
void CALLBACK GameApp::OnUpdateGame( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void *pUserContext )
{
	static DWORD lastTime = 0;
	DWORD elapsedTime = 0;
	DWORD now = timeGetTime();
	if (lastTime == 0)
	{
		lastTime = now;
	}

	// Finds the elapsed time from the last call of the main loop.
	elapsedTime = now - lastTime;
	lastTime = now;

	// If it is quiting, post the close message to clean up.
	if (g_App->IsQuitting())
	{
		PostMessage( g_App->GetHwnd(), WM_CLOSE, 0, 0);
		return;
	}

	// Update the game.
	if (g_App->m_pGame)
	{
		safeTick( 20 );
		g_App->m_pGame->OnUpdate(elapsedTime);
	}
}


// Called every time the game needs to be rendered.
void CALLBACK GameApp::OnRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void *pUserContext )
{
	if (g_App->m_pGame)
	{
		GameViewList::iterator it;
		TowerGame* game = g_App->m_pGame;
		for (it = game->m_viewList.begin(); it != game->m_viewList.end(); it++)
		{
			(*it)->VRender(fTime, fElapsedTime);
		}
	}
}

// Changes device settings to what is needed.
bool CALLBACK GameApp::ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext )
{
	static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( pDeviceSettings->DeviceType == D3DDEVTYPE_REF )
            DXUTDisplaySwitchingToREFWarning();
    }

    return true;
}

// Used to store the size of the display.
LRESULT GameApp::OnDisplayChange(int colorDepth, int width, int height)
{
	m_rcDesktop.left = 0;
	m_rcDesktop.top = 0; 
	m_rcDesktop.right = width;
	m_rcDesktop.bottom = height;
	m_iColorDepth = colorDepth;

	return 0;
}

// Creates a base game and a human view.
TowerGame* GameApp::CreateGameAndView()
{
	TowerGame* game = SAFE_NEW TowerGame();
	if (game)
	{

		shared_ptr<IGameView> view (SAFE_NEW HumanView());
		game->VAddView(view);
	}
	return game;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////TowerGame//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Listener for Game events.
void ListenForGameEvents(EventListenerPtr listener)
{
	safeAddListener( listener, EventType(Evt_New_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Remove_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_New_Runner::gkName) );
	safeAddListener( listener, EventType(Evt_New_Tower::gkName) );
	safeAddListener( listener, EventType(Evt_Move_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Change_GameState::gkName) );
	safeAddListener( listener, EventType(Evt_Set_Path::gkName) );
	safeAddListener( listener, EventType(Evt_Find_Closest_Tar::gkName) );
	safeAddListener( listener, EventType(Evt_Shoot_Tar::gkName) );
	safeAddListener( listener, EventType(Evt_Sell_Tower::gkName) );
	safeAddListener( listener, EventType(Evt_Spawn_Wave::gkName) );
	safeAddListener( listener, EventType(Evt_Change_Tower_Type::gkName) );
	safeAddListener( listener, EventType(Evt_New_Tower_Type::gkName) );
	safeAddListener( listener, EventType(Evt_Damage_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Apply_Buff::gkName) );
	safeAddListener( listener, EventType(Evt_Create_Missile::gkName) );
	safeAddListener( listener, EventType(Evt_Left_Click::gkName) );
	safeAddListener( listener, EventType(Evt_Select_Tower::gkName) );
	safeAddListener( listener, EventType(Evt_Upgrade_Selected_Tower::gkName) );
	safeAddListener( listener, EventType(Evt_Sell_Selected_Tower::gkName) );
}

// Base constructor, adds listener.
TowerGame::TowerGame():m_gameMap()
{
	m_data.m_timeLeftUntilWave = 0;
	m_data.m_waveTimeLimit = 10000;
	m_data.m_curWave = 1;
	m_data.m_curMoney = 6;
	m_data.m_curLife = 10;
	m_LastActorId = 0;
	m_status = Game_Initializing;
	m_curTowerType = -1;
	m_selectedTower = 0;

	EventListenerPtr gameLogicListener (SAFE_NEW GameLogicListener( this) );
	ListenForGameEvents(gameLogicListener);
	m_eventListener = gameLogicListener;
}

// Clears out all actors and flushes process list.
TowerGame::~TowerGame()
{
	while(!m_pActorMap.empty())
	{
		ActorMap::iterator it = m_pActorMap.begin();
		ActorId id = (*it).first;
		m_pActorMap.erase(it);
	}

	m_processManager.DeleteProcessList();
}

// Main game loop.
void TowerGame::OnUpdate(int deltaMS)
{
	// Updates all the views
	for(GameViewList::iterator i=m_viewList.begin(); i!=m_viewList.end(); ++i)
	{
		(*i)->VOnUpdate( deltaMS );
	}

	switch (m_status)
	{
		// Main game running status, updates processes/actors, checks for win/lose condition, spawns waves
		case Game_Running:
			m_processManager.UpdateProcesses(deltaMS);
			for(ActorMap::iterator it=m_pActorMap.begin(); it != m_pActorMap.end(); it++)
			{
				shared_ptr<IActor> actor = it->second;
				actor->VOnUpdate( deltaMS );
			}
			m_data.m_timeLeftUntilWave -= deltaMS;
			if (m_data.m_timeLeftUntilWave <=0)
			{
				safeTriggerEvent(Evt_Spawn_Wave());
				m_data.m_timeLeftUntilWave = m_data.m_waveTimeLimit;
			}
			if (m_data.m_curLife <= 0)
			{
				safeTriggerEvent(Evt_Change_GameState(Game_Pause));
				MessageBox(NULL, (LPCWSTR)L"You have lost! MUAHAHAHAHHAHA!", (LPCWSTR)L"TOO MANY SKELETONS!", MB_OK);
				g_App->AbortGame();
			}
			break;
		
		// Starting a new game.
		case Game_Initializing:
			BuildInitialScene();
			safeTriggerEvent(Evt_Change_GameState(Game_Running));
			m_data.m_timeLeftUntilWave = m_data.m_waveTimeLimit;
			break;

		case Game_Pause:
			break;
	}	
}

// Creates the basic scene for the game and sets up the tower types.
void TowerGame::BuildInitialScene()
{
	if (m_luaReader.Init("test.lua") || m_luaReader.Run())
		return; 
	m_luaReader.ReadTowerTypes();
	safeTriggerEvent(Evt_RebuildUI());
	m_gameMap.CreateMap();
}

// Adds an actor to the actor list, sends event to add actors elsewhere.
void TowerGame::VAddActor(shared_ptr<IActor> actor)
{
	m_pActorMap[m_LastActorId] = actor;
	actor->VSetId(m_LastActorId);
	m_LastActorId++;
	m_gameMap.AddActor(actor);
	safeQueueEvent(EventPtr (SAFE_NEW Evt_New_Actor(actor)));

	if (actor->VGet()->m_Type == AT_TOWER)
	{
		FindNewPaths();
	}
}

// Changes the game state.
void TowerGame::VGameStatusChange(GameStatus status)
{
	m_status = status;
}

// Removes an actor from the actor list.
void TowerGame::VRemoveActor(ActorId id)
{
	ActorMap::iterator it = m_pActorMap.find(id);
	if (it == m_pActorMap.end())
		return;

	shared_ptr<IActor> actor = (*it).second;
	bool atEnd = m_gameMap.TestRunnerAtEnd(actor);

	// If the actor is a tower, find new paths for the runners and get money
	if (actor->VGet()->m_Type == AT_TOWER)
	{
		FindNewPaths();
		m_data.m_curMoney += actor->VGet()->m_cost/2;
	}
	// If actor is not at the ending location, add the money from it.
	else if (!atEnd)
		m_data.m_curMoney += actor->VGet()->m_cost/2;
	// If actor is at end, remove life from the total.
	else
		m_data.m_curLife--; 

	m_gameMap.RemoveActor(id);

	m_pActorMap.erase(id);
}

// Moves an actor to the new location.
void TowerGame::VMoveActor(ActorId id, const Mat4x4 &m)
{
	if (id < m_LastActorId)
	{
		shared_ptr<IActor> actor = m_pActorMap[id];
		if (actor)
			actor->VSetMat(m);
	}
}

// Adds a view to the view list.
void TowerGame::VAddView(shared_ptr<IGameView> view)
{
	m_viewList.push_back(view);
}

// Creates a basic square grid for the base of the map.
void TowerGame::CreateGrid()
{
	shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
	p->m_Color = g_White;
	p->m_Texture = "background2.bmp";
	p->m_Mat = Mat4x4::g_Identity;
	p->m_Squares = MAP_SIZE;
	p->m_Frame = 0;
	p->m_Width = 1;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 1.0f;
	p->m_ActualWidth = 1.0f;
	p->m_Direction = 1;
	p->m_NumFrames = 1;
	p->m_NumDirections = 1;
	p->m_hasTextureAlpha = false;
	p->m_Type = AT_GROUND;
	p->m_life = 2;
	p->m_cost = 2;
	p->m_speed = 3;
	shared_ptr<IActor> actor (SAFE_NEW Actor(p));
	VAddActor(actor);
}

// Creates a runner and sets its location to the beginning. 
void TowerGame::CreateRunner()
{
	shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
	p->m_Color = g_White;
	p->m_Texture = "skeleton.dds";
	p->m_Mat = m_gameMap.GetGridLocation(-1);
	p->m_Squares = 1;
	p->m_Frame = 0;
	p->m_Width = 1;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 1.0f;
	p->m_ActualWidth = 1.0f;
	p->m_Direction = 1;
	p->m_NumFrames = 3;
	p->m_NumDirections = 4;
	p->m_hasTextureAlpha = true;
	p->m_Type = AT_RUNNER;
	p->m_MSPerFrame = 1000 / 3;
	p->m_LoopingAnim = false;
	p->m_life = 5;//*(m_data.m_curWave/10);
	p->m_cost = 2;
	p->m_speed = 2;
	shared_ptr<IActor> actor (SAFE_NEW Actor(p));
	VAddActor(actor);
}

// Creates a new tower at the given location.
void TowerGame::CreateTower(Vec3 loc)
{
	TowerTypeMap::iterator it = m_towerMap.find(m_curTowerType);
	if (it == m_towerMap.end())
		return;
	int cost = m_towerMap[m_curTowerType].m_cost;

	// Checks if there is enough money for this tower type and if it will block the path to the goal.
	if ((m_data.m_curMoney-cost >= 0) && m_gameMap.IsLocationOccupied(loc, 2, 2))
	{
		shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
		p->m_Color = g_White;
		p->m_Texture = m_towerMap[m_curTowerType].m_chartexture;
		p->m_Squares = 1;
		p->m_Frame = 0;
		p->m_Width = 1;
		p->m_Height = 0.4f;
		p->m_ActualHeight = 2;
		p->m_ActualWidth = 2;	
		p->m_Mat = m_gameMap.GetGridLocation(loc, p->m_ActualHeight, p->m_ActualWidth);
		p->m_Direction = 1;
		p->m_NumFrames = 1;
		p->m_NumDirections = 1;
		p->m_hasTextureAlpha = true;
		p->m_Type = AT_TOWER;
		p->m_MSPerFrame = 1000 / 3;
		p->m_LoopingAnim = false;
		p->m_life = 2;
		p->m_cost = cost;
		p->m_speed = 3;
		shared_ptr<IActor> actor;
		
		actor.reset(SAFE_NEW TowerActor(m_towerMap[m_curTowerType].GetParams(), p));
		VAddActor(actor);
	
		m_data.m_curMoney -= p->m_cost;
		
	}
}

// Creates a missle to fire at the tower's target.
void TowerGame::CreateMissile(ActorId id)
{
	shared_ptr<IActor> t = GetActor(id);
	if (t->VGet()->m_Type != AT_TOWER)
		return;

	shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor>(t);
	SetTowerTarget(id);
	ActorId tar = tower->GetTarget();
	shared_ptr<IActor> target = GetActor(tar);

	// Make sure the target is a runner.
	if (target->VGet()->m_Type != AT_RUNNER)
		return;
	
	shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
	p->m_Color = g_White;
	p->m_Texture = "red.bmp";
	p->m_Mat = tower->VGetMat();
	p->m_Squares = 1;
	p->m_Frame = 0;
	p->m_Width = 0.5;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 0.3f;
	p->m_ActualWidth = 0.3f;
	p->m_Direction = 1;
	p->m_NumFrames = 3;
	p->m_NumDirections = 4;
	p->m_hasTextureAlpha = true;
	p->m_Type = AT_MISSILE;
	p->m_MSPerFrame = 1000 / 3;
	p->m_LoopingAnim = false;
	p->m_life = 5;
	p->m_cost = 0;
	p->m_speed = 8;
	shared_ptr<IActor> actor (SAFE_NEW MissileActor(p, id, tar));
	VAddActor(actor);
}

// Removes a tower
void TowerGame::SellTower(Vec3 loc)
{
	ActorId id = m_gameMap.GetActorAtLoc(loc);
	
	if (id == 0)
		return;

	ActorMap::iterator i = m_pActorMap.find(id);

	if (i == m_pActorMap.end())
		return;

	safeTriggerEvent(Evt_Remove_Actor(id));
}

// Sets the path to get to the goal
void TowerGame::SetActorPath(ActorId id)
{
	ActorMap::iterator i = m_pActorMap.find(id);

	if (i == m_pActorMap.end())
		return;

	// Checks first that the actor is not at the goal, and remove if it is.
	shared_ptr<IActor> actor = (*i).second;
	if (m_gameMap.TestRunnerAtEnd(actor))
	{
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(actor->VGet()->m_Id)));
	}
	else
	{
		actor->VClearQueue();
		m_gameMap.SetActorPath(actor);
	}
}

// Updates all the paths for the runners.
void TowerGame::FindNewPaths()
{
	for(ActorMap::iterator it=m_pActorMap.begin(); it != m_pActorMap.end(); it++)
	{
		shared_ptr<IActor> actor = it->second;
		if (actor->VGet()->m_Type == AT_RUNNER)
		{
			SetActorPath(actor->VGet()->m_Id);
		}
	}
}

// Changes the tower to target the closest runner.
void TowerGame::SetTowerTarget(ActorId id)
{
	ActorMap::iterator i = m_pActorMap.find(id);

	if (i == m_pActorMap.end())
		return;

	if ((*i).second->VGet()->m_Type == AT_TOWER)
	{
		shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor> ((*i).second);

		float disSq=9999.9f;
		float range=tower->GetRange();
		ActorId closestId=0;
		Vec3 loc = tower->VGetMat().GetPosition();

		// Iterate through the list to find the closest runner.
		for(ActorMap::iterator it=m_pActorMap.begin(); it != m_pActorMap.end(); it++)
		{
			shared_ptr<IActor> actor = it->second;
			if (actor->VGet()->m_Type == AT_RUNNER)
			{
				Vec3 tmpLoc = actor->VGetMat().GetPosition();
				float tmpDis = loc.SqDistance(tmpLoc);
				if (tmpDis < disSq )
				{
					disSq = tmpDis;
					closestId = actor->VGet()->m_Id;
				}
			}
		}

		tower->SetTarget(closestId);
	}
}


// Fires a shot at the closest runner.
void TowerGame::ShootTar(ActorId shooter, int damage)
{
	ActorMap::iterator i = m_pActorMap.find(shooter);

	if (i == m_pActorMap.end())
		return;

	if ((*i).second->VGet()->m_Type != AT_TOWER)
		return;

	shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor> ((*i).second);

	float disSq=9999.9f;
	float range=tower->GetRange();
	ActorId closestId=0;
	Vec3 loc = tower->VGetMat().GetPosition(), dir;

	// First checks to see if target is the closest
	for(ActorMap::iterator it=m_pActorMap.begin(); it != m_pActorMap.end(); it++)
	{
		shared_ptr<IActor> actor = it->second;
		if (actor->VGet()->m_Type == AT_RUNNER)
		{
			Vec3 tmpLoc = actor->VGetMat().GetPosition();
			float tmpDis = sqrt((loc.x - tmpLoc.x) * (loc.x - tmpLoc.x) + (loc.z - tmpLoc.z) * (loc.z - tmpLoc.z));
			if (tmpDis < disSq && tmpDis <= range  )
			{
				disSq = tmpDis;
				closestId = actor->VGet()->m_Id;
			}
		}
	}
	if  ( closestId > 0)
	{
		tower->OnFire(closestId);
	}	
}

// Creates a new wave of runners
void TowerGame::CreateWave()
{
	m_data.m_curWave++;
	int spawns = WaveSpawns(m_data.m_curWave);
	if (spawns <= 0)
	{
		float num = m_data.m_curWave / 5.0f;
		spawns = ceil(num) * 3;
	}

	for (; spawns > 0; spawns--)
	{
		CreateRunner();
	}
}

// Adds a new tower type to the list.
void TowerGame::NewTowerType(TowerType p)
{
	p.m_type = m_towerMap.size();
	m_towerMap[m_towerMap.size()] = p;
}

// Gets the pointer to the actor with this id.
shared_ptr<IActor> TowerGame::GetActor(ActorId id)
{
	ActorMap::iterator it = m_pActorMap.find(id);
	shared_ptr<IActor> actor;

	//actor = m_pActorMap[id];

	if (it != m_pActorMap.end())
		actor = it->second;

	return actor;
}

// Deals damage to the actor of this id.
void TowerGame::DamageActor(ActorId id, int damage)
{
	if (id > 2)
		m_pActorMap[id]->VTakeDamage(damage);
}

// Applys a buff to the actor.
void TowerGame::ApplyBuffToActor(ActorId id, shared_ptr<IBuff> buff)
{
	m_pActorMap[id]->VApplyBuff(buff);
}

// Used when the mouse if right clicked.
void TowerGame::RightClick(Vec3 l)
{
	// Check if the location is in the grid.
	if (m_gameMap.HashLocation(l) < 0)
	{
		m_selectedTower = 0;
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Change_Tower_Type(-1)));
		return;
	}

	// Get the actor and select that tower at the location.
	if (m_gameMap.GetActorAtLoc(l))
	{
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Select_Tower(m_gameMap.GetActorAtLoc(l))));
		return;
	}

	// Create a new tower at the given location.
	m_selectedTower = 0;
	safeQueueEvent(EventPtr (SAFE_NEW Evt_New_Tower(l)));
}

/// Sells the selected tower, if a tower is selected.
void TowerGame::SellTower()
{
	if (m_selectedTower>0)
	{
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(m_selectedTower)));
	}
}

// Upgrades the currently sellected tower with it's upgrade line.
void TowerGame::UpgradeTower()
{
	ActorMap::iterator i = m_pActorMap.find(m_selectedTower);

	if (i == m_pActorMap.end())
		return;

	if ((*i).second->VGet()->m_Type != AT_TOWER)
		return;

	shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor> ((*i).second);

	TowerParams t = tower->GetTowerParams();

	// Checks the tower type, it is a correct type, it isn't already at max upgrade, and the player has enough money for it.
	if (t.m_type >= 0 && t.m_type < m_towerMap.size() && t.m_nextUpgrade<t.m_maxUpgrade &&
		(m_data.m_curMoney-m_towerMap[t.m_type].m_upgrades[t.m_nextUpgrade].m_cost >= 0) ) 
	{
		tower->UpgradeTower(m_towerMap[t.m_type].m_upgrades[t.m_nextUpgrade]);
		m_data.m_curMoney -= m_towerMap[t.m_type].m_upgrades[t.m_nextUpgrade].m_cost;
		safeTriggerEvent(Evt_Select_Tower(m_selectedTower));
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////HumanView//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Listener for human view events.
void ListenForViewEvents(EventListenerPtr listener)
{
	safeAddListener( listener, EventType(Evt_New_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Move_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Move_Camera::gkName) );
	safeAddListener( listener, EventType(Evt_Remove_Actor::gkName) );
	safeAddListener( listener, EventType(Evt_Change_GameState::gkName) );
	safeAddListener( listener, EventType(Evt_Shot::gkName ) );
	safeAddListener( listener, EventType(Evt_Remove_Effect::gkName ) );
	safeAddListener( listener, EventType(Evt_Device_Created::gkName) );
	safeAddListener( listener, EventType(Evt_Change_Tower_Type::gkName) );
	safeAddListener( listener, EventType(Evt_RebuildUI::gkName) );
	safeAddListener( listener, EventType(Evt_Remove_Effect_By_Id::gkName) );
	safeAddListener( listener, EventType(Evt_Select_Tower::gkName) );
	safeAddListener( listener, EventType(Evt_Mouse_Move::gkName) );
}

// Constructor
HumanView::HumanView():m_controller(),m_lastShot(0)
{
	m_id=0;

	// Starts the audio engine.
	InitAudio();
	
	m_processManager = SAFE_NEW ProcessManager;
	m_pScene.reset( SAFE_NEW HumanPlayerScene());
	m_humanUI.reset( SAFE_NEW HumanUI());

	// Frustum used for culling
	Frustum frustum;
	frustum.Init(D3DX_PI/4.0f, 1.0f, 1.0f, 100.0f);
	m_pCamera.reset(SAFE_NEW CameraNode(&Mat4x4::g_Identity, frustum));
	assert(m_pScene && m_pCamera && _T("Out of memory"));

	m_pScene->VAddChild(-1, m_pCamera);
	m_pScene->SetCamera(m_pCamera);
	m_status = Game_Initializing;

	EventListenerPtr viewListener (SAFE_NEW GameViewListener( this) );
	ListenForViewEvents(viewListener);
	m_eventListener = viewListener;
	m_pFont = NULL;
	m_pTextSprite = NULL;
}

// Destructor
HumanView::~HumanView()
{
	// Remove each of the screen elements to call it's destructor
	while (!m_screenElementList.empty())	
	{
		ScreenElementList::iterator it = m_screenElementList.begin();
		m_screenElementList.pop_front();
	}

	SAFE_RELEASE( m_pFont );
    SAFE_RELEASE( m_pTextSprite );

	m_music.reset();
	m_processManager->DeleteProcessList();
	SAFE_DELETE(m_processManager);

	// Need to shutdown the audio specially incase of errors
	if (g_Audio)
		g_Audio->VShutdown();

	SAFE_DELETE(g_Audio);
}

// Creates the audio device and gets it read for sounds.
bool HumanView::InitAudio()
{
	SAFE_DELETE(g_Audio);
	g_Audio = SAFE_NEW CDirectSoundAudio();

	if (!g_Audio)
		return false;

	if (!g_Audio->VInitialize(g_App->GetHwnd()))
		return false;

	return true;
}

// Updates processes and the screen elements.
void HumanView::VOnUpdate(int deltaMS)
{
	m_processManager->UpdateProcesses(deltaMS);

	switch (m_status)
	{
		case Game_Initializing:
			BuildInitialScene();
			break;
	}

	for (ScreenElementList::iterator it = m_screenElementList.begin(); it != m_screenElementList.end(); it++)
	{
		if (*it)
			(*it)->VOnUpdate(deltaMS);
	}

	m_controller.OnUpdate(deltaMS);
}

// Sets view id.
void HumanView::VOnAttach(GameViewId vid)
{
	m_id = vid;
}

// Renders the scene from the scene node graph.
void HumanView::VRender(double fTime, float fElapsedTime)
{
	m_currTick = timeGetTime();

	if (m_currTick == m_lastDraw)
		return;

	HRESULT hr;

	// Runs as fast as possible
	if (1)//( (m_currTick - m_lastDraw) > SCREEN_REFRESH_RATE ) )
	{
		// clear the render target
		V( DXUTGetD3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
			D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0));

		// render the scenes
		if ( SUCCEEDED(DXUTGetD3DDevice()->BeginScene() ) )
		{
			CDXUTTextHelper txtHelper( m_pFont, m_pTextSprite, 15 );			
			VRenderText(txtHelper);

			m_screenElementList.sort();

			
			for (ScreenElementList::iterator it = m_screenElementList.begin();
				it != m_screenElementList.end(); it++)
			{
				if ((*it)->VIsVisible())
					(*it)->VRender(fTime, fElapsedTime);
			}
			
			m_lastDraw = m_currTick;
		}

		V( DXUTGetD3DDevice()->EndScene());
	}
}

// Renders text to the screen, mostly for debugging purposes
void HumanView::VRenderText(CDXUTTextHelper &txtHelper)
{
	txtHelper.Begin();
	txtHelper.SetInsertionPos( 5, 5 );
	txtHelper.SetForegroundColor( D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f ) );
	std::wstring scoreStr;
//	TCHAR buffer[256];
//	wsprintf( buffer, _T("G = %d\nL = %d\nW = %d\n"), g_App->m_pGame->GetData().m_curMoney, g_App->m_pGame->GetData().m_curLife,g_App->m_pGame->GetData().m_curWave );
//	scoreStr.append(buffer);
//	txtHelper.DrawTextLine( scoreStr.c_str());
	scoreStr.append(DXUTGetFrameStats());
	scoreStr.append(_T("     /     "));
	scoreStr.append(DXUTGetDeviceStats());
	txtHelper.DrawTextLine( scoreStr.c_str() );
	txtHelper.End();
}

// Message handler for Windows messages
LRESULT CALLBACK HumanView::VOnMsgProc( AppMsg msg )
{
	// First checks if any of the screen elements will consume the message
	for(ScreenElementList::iterator it = m_screenElementList.begin();
		it != m_screenElementList.end(); it++)
	{
		if ( (*it)->VIsVisible() )
		{
			if ((*it)->VOnMsgProc( msg) )
			{
				return 1;
			}
		}
	}

	// Passes it on to the input controllers.
	switch (msg.m_uMsg)
	{
		case WM_KEYDOWN:
			m_controller.OnKeyDown(msg.m_wParam);
			break;
        case WM_KEYUP:
			m_controller.OnKeyUp(msg.m_wParam);
			break;
		case WM_LBUTTONDOWN:
			SetCapture(msg.m_hWnd);
			m_controller.OnLButtonDown(CPoint(LOWORD(msg.m_lParam), HIWORD(msg.m_lParam)));
			break;
		case WM_MOUSEMOVE:
			m_controller.OnMouseMove(CPoint(LOWORD(msg.m_lParam), HIWORD(msg.m_lParam)));
			break;
		
		case WM_RBUTTONDOWN:
			SetCapture(msg.m_hWnd);
			m_controller.OnRButtonDown(CPoint(LOWORD(msg.m_lParam), HIWORD(msg.m_lParam)));
			break;

		
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			{
				SetCapture(NULL);
			}
			break;

		case WM_MOUSEWHEEL:
			m_controller.OnMouseScroll((short)HIWORD(msg.m_wParam));
			break;
	}
	return 0;	
}

// Called when the display device is created.
HRESULT HumanView::DeviceCreated(IDirect3DDevice9* device)
{ 
	HRESULT hr;
	m_humanUI->OnDeviceCreate(device); 
	V_RETURN( D3DXCreateFont( device, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                 OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                 L"Arial", &m_pFont ) );

	return S_OK;
}

// Called when the display device is restored.
HRESULT HumanView::VOnRestore()
{
	HRESULT hr;

	if( !m_pFont )
	{
	    // Initialize the font
	    V_RETURN( D3DXCreateFont( DXUTGetD3DDevice(), 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         L"Arial", &m_pFont ) );
	}
	else
	{
       V_RETURN( m_pFont->OnResetDevice());
	}

	if (!m_pTextSprite)
	{
		// Create a sprite to help batch calls when drawing many lines of text
		V_RETURN( D3DXCreateSprite( DXUTGetD3DDevice(), &m_pTextSprite ) );
	}
	else
	{
        V_RETURN( m_pTextSprite->OnResetDevice() );		
	}

	for (ScreenElementList::iterator it = m_screenElementList.begin(); it != m_screenElementList.end(); it++)
	{
		V_RETURN( (*it)->VOnRestore());
	}

	return hr;
}

// Handles losing the display device.
void HumanView::VOnLostDevice()
{
	m_humanUI->OnDeviceLost();
	if( m_pFont )
        m_pFont->OnLostDevice();
    SAFE_RELEASE( m_pTextSprite );
}

// Adds an actor to the scene node graph
void HumanView::VAddActor(shared_ptr<IActor> actor)
{
	shared_ptr<ISceneNode> object (SAFE_NEW PlaneNode(actor->VGet()));
	// If the actor is one of the runners, add a life bar above it.
	if (actor && actor->VGet()->m_Type == AT_RUNNER)
	{
		shared_ptr<ISceneNode> lifebar (SAFE_NEW LifeBarNode(actor->VGet()->m_Id, actor->VGet()->m_life));
		object->VAddChild(lifebar);
	}

	// If the actor is an effct, add a range effect to show it's radius.
	if (actor && actor->VGet()->m_Type == AT_EFFECT)
	{
		shared_ptr<ISceneNode> rangebar (SAFE_NEW RangeNode(actor->VGetRadius()));
		object->VAddChild(rangebar);
	}
	m_pScene->AddChild(actor->VGet()->m_Id, object);
	object->VOnRestore(&*m_pScene);
}

// Removes an actor from the scene node.
void HumanView::VRemoveActor(ActorId id)
{
	m_pScene->RemoveChild(id);
}

// Recreates the UI to the correct sizes
void HumanView::RebuildUI()
{
	int numButtons = g_App->m_pGame->GetNumTowerTypes();
	m_humanUI->BuildInitialDialogs(numButtons);
}

// Sets up the beginning scene and gets the scene node graph ready.
void HumanView::BuildInitialScene()
{
	//m_music.reset(SAFE_NEW CSoundProcess("music2.ogg",2, 20, true));
	//Attach(m_music);
	VPushScreen(m_pScene);
	//RebuildUI();
	VPushScreen(m_humanUI);
	VOnRestore();
}

// Adds a screen to the element list.
void HumanView::VPushScreen(shared_ptr<IScreenElement> e)
{
	m_screenElementList.push_back(e);
}

// Removes a screen from the element list.
void HumanView::VPopScreen()
{
	m_screenElementList.pop_back();
}

// Moves the scene node to the correct location
void HumanView::VMoveActor(ActorId id, Mat4x4 const &mat)
{
    shared_ptr<ISceneNode> node = m_pScene->FindActor(id);
	if (node)
	{
		node->VSetTransform(&mat);
	}
}

// Moves the camera to the changed location.
void HumanView::MoveCamera(Mat4x4 const &change)
{
 //   shared_ptr<CameraNode> node = m_pScene->GetCamera();
	//if (node)
	//{
	//	Mat4x4 mat, tmp;
	//	tmp = node->VGet()->ToWorld() * change;
	//	node->VSetTransform(&change);
	//}
}

// Changes the "direction" of the scene node, meaning the image's texture locations
void HumanView::ChangeActorDirection(ActorId id, int dir)
{
    shared_ptr<ISceneNode> node = m_pScene->FindActor(id);
	if (node)
	{
		const SceneNodeProperties * const p = node->VGet();
		if (strcmp(p->Name(), "Character")==0)
		{
			shared_ptr<PlaneNode> c = boost::dynamic_pointer_cast<PlaneNode>(node);
			c->ChangeDirection(dir);
		}
	}
}

// Sets if the animation is looking or not.
void HumanView::LoopAnimation(ActorId id, bool loop)
{
    shared_ptr<ISceneNode> node = m_pScene->FindActor(id);
	if (node)
	{
		const SceneNodeProperties * const p = node->VGet();
		if (strcmp(p->Name(), "Character")==0)
		{
			shared_ptr<PlaneNode> c = boost::dynamic_pointer_cast<PlaneNode>(node);
			c->ChangeAnimationLoop(loop);
		}
	}
}

// Moves the camera to the position
void HumanView::MoveCamera(Vec3 pos)
{
    shared_ptr<CameraNode> node = m_pScene->GetCamera();
	if (node)
	{
		Mat4x4 mat, tmp;
		mat.BuildTranslation(pos);
		tmp = node->VGet()->ToWorld() * mat;
		node->VSetTransform(&tmp);
	}
}

// Adds a new "shot" effect to the scene node graph.
void HumanView::AddShot(ActorId id, int time, Vec3 start, Vec3 end, std::string texture)
{
	Mat4x4 s,e;
	s.BuildTranslation(start);
	e.BuildTranslation(end);
	shared_ptr<ISceneNode> object (SAFE_NEW ShotNode(id, time, m_lastShot, texture, s, e));
	++m_lastShot;
	m_pScene->AddChild(-1, object);
	object->VOnRestore(&*m_pScene);
}

// Changes the selected tower type to the given type
void HumanView::TowerChange(int type)
{
	m_humanUI->TowerSwitch(type);

	if (m_mouseOver)
	{
		ActorId id = m_mouseOver->VGet()->m_Id;
		m_mouseOver.reset();
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(id)));
	}

	if (type >= 0)
	{
		shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
		p->m_Color = g_White;
		p->m_Texture = "square.dds";
		p->m_Mat = Mat4x4::g_Identity;
		p->m_Squares = 1;
		p->m_Frame = 0;
		p->m_Width = 1;
		p->m_Height = 0.4f;
		p->m_ActualHeight = 2;
		p->m_ActualWidth = 2;
		p->m_Direction = 1;
		p->m_NumFrames = 1;
		p->m_NumDirections = 1;
		p->m_hasTextureAlpha = true;
		p->m_Type = AT_EFFECT;
		p->m_life = 0;
		p->m_cost = 0;
		p->m_speed = 0;
		shared_ptr<IActor> actor (SAFE_NEW EffectActor(p, g_App->m_pGame->GetTowerData(type).m_range));
		g_App->m_pGame->VAddActor(actor);
		m_mouseOver = actor;
	}
}

// Selects the tower with the given id.
void HumanView::SelectTower(ActorId id)
{
	m_humanUI->SelectTower(id);
	// clears the old mouse over data
	if (m_mouseOver)
	{
		ActorId mid = m_mouseOver->VGet()->m_Id;
		m_mouseOver.reset();
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(mid)));
	}
}

// Used for the mouse over the towers.
void HumanView::MouseMove(Vec3 pos)
{
	if (m_mouseOver)
	{
		Mat4x4 m (g_App->m_pGame->m_gameMap.GetGridLocation(pos, m_mouseOver->VGet()->m_ActualHeight, m_mouseOver->VGet()->m_ActualWidth));
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Move_Actor(m_mouseOver->VGet()->m_Id, m) ) );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Actor//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Default constructor.
Actor::Actor()
{
	shared_ptr<ActorParams> p;
	m_params = p;
	m_elapsedTime = 0;
	m_timeToStart = rand() % 3000;
}

// Builds the actor out from the actor params.
Actor::Actor(shared_ptr<ActorParams> p)
{
	m_params = p;
	m_elapsedTime = 0;
	m_timeToStart = rand() % 3000;
}

// Updates the buffs on the actor and then checks if there is place set to move the actor to.
void Actor::VOnUpdate(int elapsedTime)
{
	// move the character if there is a matrix
	const int frameUpdate = 10;

	for (BuffList::iterator it = m_buffs.begin(); it != m_buffs.end();)
	{
		if ((*it)->VOnUpdate(elapsedTime))
		{
			(*it)->VRemove();
			it = m_buffs.erase(it);
		}
		else
			it++;
	}

	if (m_timeToStart > 0)
	{
		m_timeToStart -= elapsedTime;
		return;
	}
	
	// Don't need to go further if there is no location to move to.
	if (!m_moveQueue.empty())
	{
		m_elapsedTime += elapsedTime;
		if (m_elapsedTime > frameUpdate)
		{
			Vec3 A = m_params->m_Mat.GetPosition();
			Vec3 B = m_moveQueue.back().GetPosition();

			// Finds the distance from the current location to the next location
			float k = A.Distance(B);
			// If the actor is far from the destination, move the actor
			if (k > 0.1f)
			{
				m_params->m_LoopingAnim=true;
				if (k > 0.9f)
					VSetDirection(B);

				// Finds how much to move based on how fast the actor moves and how long it has been.
				float distanceToMove = m_elapsedTime / frameUpdate;
				m_elapsedTime -= distanceToMove * frameUpdate;
				float speed = m_params->m_speed * 0.003;
				float d = (speed * distanceToMove) / k;

				// Make sure it doesn't go over the distance needed.
				if (d > k)
					d = k;

				// Find the actor's new location
				Vec3 C = A + (B - A)*d;

				//C.z = B.z;
				Mat4x4 moveTo = Mat4x4::g_Identity;
				moveTo.SetPosition(C);

				safeTriggerEvent(Evt_Move_Actor(m_params->m_Id,moveTo));
			}
			// If the actor is close to the destination, remove that destination from the queue
			else
			{
				m_params->m_LoopingAnim=false;
				m_moveQueue.pop_back();
				m_elapsedTime = 0;
			}
		}
	}
	else
	{
		// If the actor is a runner and it doesn't have a path already, find one.
		m_elapsedTime = 0;
		if (m_params->m_Type == AT_RUNNER)
			safeQueueEvent(EventPtr (SAFE_NEW Evt_Set_Path(m_params->m_Id)));
		
	}
}

// Will face the actor in the direction given from its current location
void Actor::VSetDirection(Vec3 B)
{
	Vec3 A = m_params->m_Mat.GetPosition();
	float direction = atan2(B.z - A.z, B.x - A.x);
	if (direction < 0)
		direction =  2*3.1415 + direction;

	if (direction > 0 && direction < 0.785375)
	{
		m_params->m_Direction = 1;
	}
	else
	if (direction > 0.785375 && direction < 2.356125)
	{
		m_params->m_Direction = 0;
	}
	else
	if (direction > 2.356125 && direction < 3.926875)
	{
		m_params->m_Direction = 3;
	}
	else
	if (direction > 3.926875 && direction < 5.497625)
	{
		m_params->m_Direction = 2;
	}
	else
	if (direction > 5.497625)
	{
		m_params->m_Direction = 1;
	}
}

// Gives damage to the actor and sends an event if it dies.
bool Actor::VTakeDamage(int damage)
{
	m_params->m_life -= damage;
	if (m_params->m_life < 0)
	{
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(m_params->m_Id)));
		return 0;
	}

	return 1;
}

// Adds a buff on the actor. Can only have 1 buff of each type going at a time.
void Actor::VApplyBuff(shared_ptr<IBuff> buff)
{
	for (BuffList::iterator it = m_buffs.begin(); it != m_buffs.end(); it++)
		if ( (*it)->VGetType() == buff->VGetType())
			return;

	m_buffs.push_back(buff);
	buff->VApply();
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////TowerActor/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Updates the tower by running the lua script for it.
void TowerActor::VOnUpdate(int deltaMS)
{	
	m_luaScript.OnUpdate(deltaMS);
}

// Sets the target the tower is pointing at
void TowerActor::SetTarget(ActorId id)
{
	m_curTarget = id;
	shared_ptr<IActor> runner = g_App->m_pGame->GetActor(id);
	VSetDirection(runner->VGetMat().GetPosition());
	m_luaScript.SetTarget(id);
}

// Fires a shot at the current target
void TowerActor::OnFire(ActorId id)
{
	shared_ptr<IActor> runner = g_App->m_pGame->GetActor(id);
	if (runner && runner->VGet()->m_Type == AT_RUNNER)
	{
		m_curTarget = id;
		VSetDirection(runner->VGetMat().GetPosition());
		m_luaScript.Fire(m_curTarget);
		//safeTriggerEvent(Evt_Damage_Actor(id, m_towerParams.m_damage));
		safeTriggerEvent(Evt_Shot(VGet()->m_Id, 100, VGetMat().GetPosition(), runner->VGetMat().GetPosition(), m_towerParams.m_shottexture));
	}
}

// Upgrades the tower with the upgrade sent
void TowerActor::UpgradeTower(Upgrade u)
{
	m_towerParams.m_damage += u.m_damage;
	m_towerParams.m_cost += u.m_cost;
	m_towerParams.m_reloadTime += u.m_reload;
	m_towerParams.m_range += u.m_reload;
	m_towerParams.m_nextUpgrade++;
	m_params->m_cost += u.m_cost;
	m_luaScript.UpgradeTower(u);
}

// Rotates the tower to face the direction given.
void TowerActor::VSetDirection(Vec3 B)
{
	Vec3 A = m_params->m_Mat.GetPosition();
	float direction = atan2(B.z - A.z, B.x - A.x);
	if (direction < 0)
		direction =  2*3.1415 + direction;
	Mat4x4 m;
	m = Mat4x4::g_Identity;
	m.BuildRotationY(-direction);
	m_params->m_Mat.BuildTranslation(A);
	m_params->m_Mat = m  * m_params->m_Mat;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////MissileActor///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Moves the missile towards its target and does damage if it gets close.
// Similar to the normal actor movement.
void MissileActor::VOnUpdate(int deltaMS)
{
	const int frameUpdate = 10;

	shared_ptr<IActor> tar = g_App->m_pGame->GetActor(m_target);

	if (tar)
		m_location = tar->VGetMat();

	m_elapsedTime += deltaMS;
	if (m_elapsedTime > frameUpdate)
	{
		Vec3 A = m_params->m_Mat.GetPosition();
		Vec3 B = m_location.GetPosition();

		// Gets the distance from the actor and its destination.
		float k = A.Distance(B);
		if (k > 0.1f)
		{
			m_params->m_LoopingAnim=true;
			if (k > 0.9f)
				VSetDirection(B);

			// Finds the distance to move based on how much time has passed.
			float distanceToMove = m_elapsedTime / frameUpdate;
			m_elapsedTime -= distanceToMove * frameUpdate;
			float speed = m_params->m_speed * 0.003;
			float d = (speed * distanceToMove) / k;

			if (d > k)
				d = k;

			Vec3 C = A + (B - A)*d;

			//C.z = B.z;
			Mat4x4 moveTo = Mat4x4::g_Identity;
			moveTo.SetPosition(C);

			safeTriggerEvent(Evt_Move_Actor(m_params->m_Id,moveTo));
		}
		else
		{
			// If the missile is close to its destination, then it will damage the target and remove itself.
			m_params->m_LoopingAnim=false;
			m_elapsedTime = 0;
			shared_ptr<IActor> t = g_App->m_pGame->GetActor(m_tower);
			if (t && t->VGet()->m_Type == AT_TOWER)
			{
				shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor> (t);
				tower->OnFire(m_target);
			}
			safeQueueEvent(EventPtr (SAFE_NEW Evt_Remove_Actor(m_params->m_Id)));
		}
	}
}

// Sets the target for the missile
void MissileActor::SetTarget(ActorId id)
{
	m_curTarget = id;
	shared_ptr<IActor> runner = g_App->m_pGame->GetActor(id);
	VSetDirection(runner->VGetMat().GetPosition());
}

// Missiles on fire function. Used to conform to TowerActor, but without the luascript
void MissileActor::OnFire(ActorId id)
{
	shared_ptr<IActor> runner = g_App->m_pGame->GetActor(id);
	m_curTarget = id;
	VSetDirection(runner->VGetMat().GetPosition());
	safeTriggerEvent(Evt_Shot(VGet()->m_Id, 100, VGetMat().GetPosition(), runner->VGetMat().GetPosition(), m_towerParams.m_shottexture));
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////HumanInterfaceController///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define RADIANS_TO_DEGREES(x) ((x) * 180.0f / D3DX_PI)
#define DEGREES_TO_RADIANS(x) ((x) * D3DX_PI / 180.0f)

// Default constructor
HumanInterfaceController::HumanInterfaceController():m_scrolled(13)
{
	m_fTargetYaw = m_fYaw = RADIANS_TO_DEGREES(0);
	m_fTargetPitch = m_fPitch = RADIANS_TO_DEGREES(0);

	m_maxSpeed = 3.0f;			// 30 meters per second
	m_currentSpeed = 0.0f;

	m_matToWorld = Mat4x4::g_Identity;
	Vec3 pos = m_matToWorld.GetPosition();

	m_matPosition.BuildTranslation(pos);

	//m_bLeftMouseDown = false;
    POINT ptCursor;
    GetCursorPos( &ptCursor );
	m_lastMousePos = ptCursor;

	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_scrollLines, 0);
	memset(m_bKey,0,sizeof(m_bKey));
}

// Checks if any keys are down and does the appropriate action for that key.
void HumanInterfaceController::OnUpdate(int deltaMS)
{
	bool bTranslating = false;
	Vec4 atWorld(0,0,0,0);
	Vec4 rightWorld(0,0,0,0);
	Vec4 upWorld(0,0,0,0);

	if (m_bKey['W'] || m_bKey['S'])
	{
		// In D3D, the "look at" default is always
		// the positive Z axis.
		Vec4 at = g_Forward4; 
		if (m_bKey['S'])
			at *= -1;

		// This will give us the "look at" vector 
		// in world space - we'll use that to move
		// the camera.
		atWorld = m_matToWorld.Xform(at);
		bTranslating = true;
	}

	if (m_bKey['A'] || m_bKey['D'])
	{
		// In D3D, the "right" default is always
		// the positive X axis.
		Vec4 right = g_Right4; 
		if (m_bKey['A'])
			right *= -1;

		// This will give us the "right" vector 
		// in world space - we'll use that to move
		// the camera
		rightWorld = m_matToWorld.Xform(right);
		bTranslating = true;
	}

	if (m_bKey[' '] || m_bKey['C'] || m_bKey['X'])
	{
		// In D3D, the "up" default is always
		// the positive Y axis.
		Vec4 up = g_Right4; 
		if (!m_bKey[' '])
			up *= -1;

		//Unlike strafing, Up is always up no matter
		//which way you are looking
		upWorld = up;
		bTranslating = true;
	}

	//Handling rotation as a result of mouse position

		// The secret formula!!! Don't give it away!
		//If you are seeing this now, then you must be some kind of elite hacker!
		Mat4x4 matRot;
		/*m_fYaw += (m_fTargetYaw - m_fYaw) * ( .35f );
		m_fTargetPitch = MAX(-90, MIN(90, m_fTargetPitch));
		m_fPitch += (m_fTargetPitch - m_fPitch) * ( .35f );*/

		m_fYaw = m_fTargetYaw;
		m_fPitch = 0;//MAX(-90, MIN(90, m_fTargetPitch));

		// Calculate the new rotation matrix from the camera
		// yaw and pitch.
		
		matRot.BuildYawPitchRoll(DEGREES_TO_RADIANS(-m_fYaw), DEGREES_TO_RADIANS(m_fPitch), 0);

		// Create the new object-to-world matrix, and the
		// new world-to-object matrix. 

		m_matToWorld = matRot;// * m_matPosition;
		bTranslating = true;
	

	if (bTranslating)
	{
		float elapsedTime = (float)deltaMS / 1000.0f;

		Vec3 direction = atWorld + rightWorld + upWorld;
		direction.Normalize(); 

		// Ramp the acceleration by the elapsed time.
		float numberOfSeconds = 500.f;
		m_currentSpeed += m_maxSpeed * ( (elapsedTime*elapsedTime) / numberOfSeconds);
		if (m_currentSpeed > m_maxSpeed)
			m_currentSpeed = m_maxSpeed;

		direction *= m_currentSpeed;

		Vec3 pos = m_matPosition.GetPosition() + direction;
		m_matPosition.SetPosition(pos);
		m_matToWorld.SetPosition(pos);

		m_matPosition = matRot;// * m_matPosition;
		m_matFromWorld = m_matToWorld.Inverse();
	//	safeTriggerEvent(Evt_Move_Camera(m_matPosition));
	}
	else
	{
		m_currentSpeed = 0.0f;
	}
}

// Zooms the camera in or out
void HumanInterfaceController::OnMouseScroll(int lines)
{
	int scrollMove = lines / WHEEL_DELTA;
	m_scrolled -= scrollMove;
//	if (m_scrolled <= 26 && m_scrolled >= 0)
//		safeTriggerEvent(Evt_Move_Camera(Vec3(0, -scrollMove, 0)));
//	else
//		m_scrolled += scrollMove;
}

// Finds the location on the map and sends the left click event.
void HumanInterfaceController::OnLButtonDown(const CPoint &mousePos)
{
	D3DVIEWPORT9		p_viewPort;
	DXUTGetD3DDevice()->GetViewport(&p_viewPort);

	Vec3 v(mousePos.x, mousePos.y, 0.97f);

	Mat4x4 projection;
	DXUTGetD3DDevice()->GetTransform(D3DTS_PROJECTION, &projection);

	Mat4x4 view;
	DXUTGetD3DDevice()->GetTransform(D3DTS_VIEW, &view);

	Mat4x4 world;
	DXUTGetD3DDevice()->GetTransform(D3DTS_WORLD, &world);

	v = v.UnProject(p_viewPort, projection, view, Mat4x4::g_Identity);

	safeTriggerEvent(Evt_Left_Click(v));
}

// Sends the event to sell a tower at the clicked location
void HumanInterfaceController::OnRButtonDown(const CPoint &mousePos)
{
	Vec3 v1(mousePos.x,mousePos.y,0);

	//safeTriggerEvent(Evt_Right_Click(v1));
}

// Sends the mouse move event when the mouse is moving
void HumanInterfaceController::OnMouseMove(const CPoint &mousePos)
{
	//if(m_lastMousePos!=mousePos)
	//{
	//	m_fTargetYaw = m_fTargetYaw + (m_lastMousePos.x - mousePos.x);
	//	m_fTargetPitch = m_fTargetPitch + (mousePos.y - m_lastMousePos.y);
	//	m_lastMousePos = mousePos;

	//	Mat4x4 matRot;
	//	/*m_fYaw += (m_fTargetYaw - m_fYaw) * ( .35f );
	//	m_fTargetPitch = MAX(-90, MIN(90, m_fTargetPitch));
	//	m_fPitch += (m_fTargetPitch - m_fPitch) * ( .35f );*/

	//	m_fYaw = m_fTargetYaw;
	//	m_fPitch =  m_fTargetPitch;

	//	// Calculate the new rotation matrix from the camera
	//	// yaw and pitch.

	//	Mat4x4 mat = Mat4x4::g_Identity;
	//	Vec4 at (g_Up.x, g_Up.y, g_Up.z, 0.0f);
	//	//at = at * 26.0f;
	//	//at.x += 4.0f;
	//	Vec4 atWorld = mat.Xform(at);

	//	Vec3 pos = mat.GetPosition() + Vec3(atWorld);
	//	mat.BuildRotationX(D3DX_PI / 2.0);
	//	mat.SetPosition(pos);
	//	
	//	matRot.BuildYawPitchRoll(DEGREES_TO_RADIANS(-m_fYaw), DEGREES_TO_RADIANS(m_fPitch), 0);
	//	//matRot.SetPosition(pos);


	//	safeTriggerEvent(Evt_Move_Camera(mat * matRot));
	//}

	D3DVIEWPORT9		p_viewPort;
	DXUTGetD3DDevice()->GetViewport(&p_viewPort);

	Vec3 v(mousePos.x, mousePos.y, 0.97f);

	Mat4x4 projection;
	DXUTGetD3DDevice()->GetTransform(D3DTS_PROJECTION, &projection);

	Mat4x4 view;
	DXUTGetD3DDevice()->GetTransform(D3DTS_VIEW, &view);

	Mat4x4 world;
	DXUTGetD3DDevice()->GetTransform(D3DTS_WORLD, &world);

	v = v.UnProject(p_viewPort, projection, view, Mat4x4::g_Identity);

	safeTriggerEvent(Evt_Mouse_Move(v));
	return;
}




/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Map////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Default constructor 
Map::Map():m_start(MAP_SIZE*HALF_MAP_SIZE), m_end(MAP_SIZE*HALF_MAP_SIZE+(MAP_SIZE-1))
{
	memset(&m_grid,0,sizeof(m_grid));
}

// Checks if the actor is in a valid spot and fills those in on the map grid.
bool Map::AddActor(shared_ptr<IActor> actor)
{
	if (actor->VGet()->m_Type == AT_TOWER)
	{
		ActorId id = actor->VGet()->m_Id;
		Mat4x4 mat = actor->VGetMat();
		Vec3 loc = mat.GetPosition();
		int	mapPlace = HashLocation(loc);
		if (mapPlace >= 0)
		{
			int p=0;
			for (int i = actor->VGet()->m_ActualHeight-1; i >= 0; i--)
			{
				for (int j = actor->VGet()->m_ActualWidth-1; j >= 0; j--)
				{
					p = mapPlace-MAP_SIZE*i-j;
					m_grid[p] = id;
				}
			}
			m_grid[mapPlace] = id;
			return true;
		}
	}
	
	return false;
}


// Finds the map grid index based on the 3d location
int Map::HashLocation(Vec3 location)
{
	bool no=true;
	int x = floor(location.x);
	x+=HALF_MAP_SIZE;
	if (x<0)
		no=false;
	if (x>=MAP_SIZE)
		no=false;

	int y = floor(location.z);		
	y+=HALF_MAP_SIZE;

	if (y<0)
		no=false;
	if (y>=MAP_SIZE)
		no=false;
	if (no)
		return x + (y*MAP_SIZE);
	else
		return -1;
}

// Finds the location for the center of the square for the vector given.
Mat4x4 Map::GetGridLocation(Vec3 v)
{
	int x = floor(v.x);
	int y = floor(v.z);	
	Mat4x4 m;
	m.BuildTranslation(x+0.5f, 0, y+0.5f);

	return m;
}

// Gets the location for the center of the square with height and width for the vector
Mat4x4 Map::GetGridLocation(Vec3 v, int height, int width)
{
	int x = floor(v.x);
	int y = floor(v.z);	
	Mat4x4 m;
	m.BuildTranslation(x+width/2.0f, 0, y+height/2.0f);

	return m;
}

// Gets the location for the map grid index
Mat4x4 Map::GetGridLocation(int i)
{
	bool s = false;
	Mat4x4 m = Mat4x4::g_Identity;

	if (i < 0)
	{
		i = m_start;
		s = true;
	}

	if (i < MAP_SIZE * MAP_SIZE)
	{
		int x = i % MAP_SIZE;
		int y = i / MAP_SIZE;
		x -= HALF_MAP_SIZE;
		y -= HALF_MAP_SIZE;
		
		if (s)
			x--;

		m.BuildTranslation(x+0.5f, 0, y+0.5f);
	}

	return m;
}

// Tests if the locatin will block movement to the end location from the beginning
bool Map::CheckLocation(Vec3 loc)
{
	bool b = false;
	int t = HashLocation(loc);
	shared_ptr<IActor> p;
	if (t >= 0)
	{
		m_grid[t] = -1;
		b = TestLocation(m_end, m_start, p);
		m_grid[t] = 0;
	}
	return b;
}

// Checks if the location is filled and if it blocks
bool Map::IsLocationOccupied(Vec3 loc)
{
	int t = HashLocation(loc);
	
	if (m_grid[t] == 0)
	{
		return CheckLocation(loc);
	}

	return false;
}

// Checks if the location is filled and if it blocks for a square
bool Map::IsLocationOccupied(Vec3 loc, int height, int width)
{
	int t = HashLocation(loc);
	t += MAP_SIZE + 1;
	bool test = true;
	int	p=0;

	for (int i = height-1; i >= 0; i--)
	{
		for (int j = width-1; j >= 0; j--)
		{
			p = t-MAP_SIZE*i-j;
			int id = m_grid[p];
			if (m_grid[p] != 0)
				test = false;
			else
				m_grid[p] = -1;
		}
	}
	
	if (test)
		test = CheckLocation(loc);

	for (int i = height-1; i >= 0; i--)
	{
		for (int j = width-1; j >= 0; j--)
		{
			p = t-MAP_SIZE*i-j;
			if (m_grid[p] == -1)
				m_grid[p] = 0;
		}
	}

	return test;
}

// Uses the A* algorithm to find a path from the start node to the end node.
// If the actor is given, it will give that actor the path for the start to the end.
bool Map::TestLocation(int endNode, int startNode, shared_ptr<IActor> actor)
{
	bool end = false;
	SearchNodeMap openList, closedList;
	SearchNode cur;
	cur.loc = startNode;
	// finds the temp numbers for the Manhattan distance.
	int endX = endNode % MAP_SIZE, endY = endNode / MAP_SIZE;
	int dis = 9999;
	openList[startNode] = cur;
	SearchNodeMap::iterator it, itClosed;

	// While there are nodes left to check and we are not at the end.
	while (!openList.empty() && !end)
	{
		// Finds the shortest distance node.
		dis = 9999;
		for (it = openList.begin(); it != openList.end(); it++)
		{
			if ((*it).second.F <= dis)
			{
				cur = (*it).second;
				dis = (*it).second.F;
			}
		}

		// Checks if the current square is the end square
		if (cur.loc == endX + endY * MAP_SIZE)
			end = true;

		// Removes the current node from the open list and adds to the closed
		openList.erase(cur.loc);
		closedList[cur.loc] = cur;

		// Checks the 4 squares around it, up, down, left and right.
		for (int i = 0; i < 4; i++)
		{
			int sign = (i >= 2 ? -1 : 1);
			int test = cur.loc;
			int x = test % MAP_SIZE;
			int y = test / MAP_SIZE;

			if (i % 2 == 0)
			{
				test += sign;
				test = min(test, ((y+1)*MAP_SIZE));
				test = max(test, (y * MAP_SIZE));
			}
			else
			{
				test += sign*MAP_SIZE;
			}

			// Make sure the test square is on the map and not occupied.
			if ((test >= 0) && (test < MAP_SIZE*MAP_SIZE) && m_grid[test] == 0)
			{
				// Looks for that suqare in the closed set.
				itClosed = closedList.find(test);
				if (itClosed == closedList.end())
				{
					// Looks for the node in the open set
					it = openList.find(test);
					if (it == openList.end())
					{
						// If not in the open set, create a new node
						SearchNode tmp;
						tmp.G = cur.G + 1;
						tmp.H = abs(x - endX) + abs(y - endY);
						tmp.F = tmp.G + tmp.H;
						tmp.parent = cur.loc;
						tmp.loc = test;

						// Adds the node to the open list.
						openList[test] = tmp;
					}
					else
					{
						// If already in the open set, update the G, H, and F numbers.
						if ((*it).second.G > cur.G + 10)
						{
							(*it).second.G = cur.G + 10;
							(*it).second.F = (*it).second.G + (*it).second.H;
							(*it).second.parent = cur.loc;
						}
					}
				}
			}
		}

	}

	// If we are at the end and we have an actor, set up the path for it to follow
	if (end && actor && cur.loc == endNode)
	{
		while (cur.parent >= 0)
		{
			actor->VQueuePosition(GetGridLocation(cur.loc));
			cur = closedList[cur.parent];
		}
	}

	// Returns true if at the end.
	return end;
}


// Sets the actor's path from its current location to the end.
void Map::SetActorPath(shared_ptr<IActor> actor)
{
	Vec3 v = actor->VGet()->m_Mat.GetPosition();
	int start = HashLocation(v);
	if (start >= 0)
		TestLocation(m_end, start, actor);
	else
	{
		TestLocation(m_end, m_start, actor);
		actor->VQueuePosition(GetGridLocation(m_start));
	}
}

// Tests if the runner is at the end.
bool Map::TestRunnerAtEnd(shared_ptr<IActor> actor)
{
	Vec3 v = actor->VGet()->m_Mat.GetPosition();
	int test = HashLocation(v);
	if (test == m_end)
		return true;
	
	return false;
}

// Finds the actor at the given location
ActorId Map::GetActorAtLoc(Vec3 v)
{
	int test = HashLocation(v);
	return m_grid[test];
}

// Removes the actor from the map
bool Map::RemoveActor(ActorId id)
{
	int i = 0;
	bool end=false;

	while (i < MAP_SIZE*MAP_SIZE)
	{
		if (m_grid[i] == id)
		{
			m_grid[i] = 0;
			end = (i == m_end ? false : true);
		}
		i++;
	}

	return end;
}

// Creates the scene node elements for the map
void Map::CreateMap()
{
	// Base background for the map
	shared_ptr<ActorParams> p (SAFE_NEW ActorParams());
	p->m_Color = g_White;
	p->m_Texture = "background2.bmp";
	p->m_Mat = Mat4x4::g_Identity;
	p->m_Squares = MAP_SIZE;
	p->m_Frame = 0;
	p->m_Width = 1;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 1.0;
	p->m_ActualWidth = 1.0f;
	p->m_Direction = 1;
	p->m_NumFrames = 1;
	p->m_NumDirections = 1;
	p->m_hasTextureAlpha = false;
	p->m_Type = AT_GROUND;
	p->m_life = 2;
	p->m_cost = 2;
	p->m_LoopingAnim = true;
	shared_ptr<IActor> actor (SAFE_NEW Actor(p));

	g_App->m_pGame->VAddActor(actor);

	// A red square for the starting location.
	p.reset(SAFE_NEW ActorParams());
	p->m_Color = g_Red;
	p->m_Texture = "red.bmp";
	p->m_Mat = GetGridLocation(m_start);
	p->m_Squares = 1;
	p->m_Frame = 0;
	p->m_Width = 1;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 1.0f;
	p->m_ActualWidth = 1.0f;
	p->m_Direction = 1;
	p->m_NumFrames = 1;
	p->m_NumDirections = 1;
	p->m_hasTextureAlpha = false;
	p->m_Type = AT_GROUND;
	p->m_life = 2;
	p->m_cost = 2;
	actor.reset(SAFE_NEW Actor(p));
	g_App->m_pGame->VAddActor(actor);

	// A blue square for the end location.
	p.reset(SAFE_NEW ActorParams());
	p->m_Color = g_Blue;
	p->m_Texture = "blue.bmp";
	p->m_Mat = GetGridLocation(m_end);
	p->m_Squares = 1;
	p->m_Frame = 0;
	p->m_Width = 1;
	p->m_Height = 0.4f;
	p->m_ActualHeight = 1.0f;
	p->m_ActualWidth = 1.0f;
	p->m_Direction = 1;
	p->m_NumFrames = 1;
	p->m_NumDirections = 1;
	p->m_hasTextureAlpha = false;
	p->m_Type = AT_GROUND;
	p->m_life = 2;
	p->m_cost = 2;
	actor.reset(SAFE_NEW Actor(p));
	g_App->m_pGame->VAddActor(actor);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////GameLogicListener//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Event listener for the game logic, mostly just calls the game's functions.
bool GameLogicListener::HandleEvent(Event const & e)
{
	if (strcmp(e.getType().getName(), Evt_Remove_Actor::gkName)==0)
	{
		EvtData_Remove_Actor *data = e.getData<EvtData_Remove_Actor>();
		m_game->VRemoveActor(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_New_Runner::gkName)==0)
	{
		m_game->CreateRunner();
	}
	else
	if (strcmp(e.getType().getName(), Evt_New_Tower::gkName)==0)
	{
		EvtData_New_Tower *data = e.getData<EvtData_New_Tower>();
		m_game->CreateTower(data->m_pos);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Move_Actor::gkName)==0)
	{
		EvtData_Move_Actor *data = e.getData<EvtData_Move_Actor>();
		m_game->VMoveActor(data->m_id, data->m_Mat);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Change_GameState::gkName)==0)
	{
		EvtData_Change_GameState *data = e.getData<EvtData_Change_GameState>();
		m_game->VGameStatusChange(data->m_state);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Set_Path::gkName)==0)
	{
		EvtData_Set_Path *data = e.getData<EvtData_Set_Path>();
		m_game->SetActorPath(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Find_Closest_Tar::gkName) == 0)
	{
		EvtData_Closest_Tar *data = e.getData<EvtData_Closest_Tar>();
		m_game->SetTowerTarget(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Shoot_Tar::gkName) == 0)
	{
		EvtData_Shoot_Tar *data = e.getData<EvtData_Shoot_Tar>();
		m_game->ShootTar(data->m_shooterId, data->m_damage);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Sell_Tower::gkName) == 0 )
	{
		EvtData_Sell_Tower *data = e.getData<EvtData_Sell_Tower>();
		m_game->SellTower(data->m_pos);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Spawn_Wave::gkName) == 0 )
	{
		m_game->CreateWave();
	}
	else
	if (strcmp(e.getType().getName(), Evt_Change_Tower_Type::gkName) == 0 )
	{
		EvtData_Change_Tower_Type *data = e.getData<EvtData_Change_Tower_Type>();
		m_game->ChangeTowerType(data->m_type);
	}
	else
	if (strcmp(e.getType().getName(), Evt_New_Tower_Type::gkName) == 0 )
	{
		EvtData_New_Tower_Type *data = e.getData<EvtData_New_Tower_Type>();
		m_game->NewTowerType(data->m_params);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Damage_Actor::gkName) == 0 )
	{
		EvtData_Damage_Actor *data = e.getData<EvtData_Damage_Actor>();
		m_game->DamageActor(data->m_id, data->m_damage);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Apply_Buff::gkName) == 0 )
	{
		EvtData_Apply_Buff *data = e.getData<EvtData_Apply_Buff>();
		m_game->ApplyBuffToActor(data->m_buff->VGetActorId(), data->m_buff);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Create_Missile::gkName) == 0 )
	{
		EvtData_Create_Missile *data = e.getData<EvtData_Create_Missile>();
		m_game->CreateMissile(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Left_Click::gkName) == 0 )
	{
		EvtData_Right_Click *data = e.getData<EvtData_Right_Click>();
		m_game->RightClick(data->m_loc);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Select_Tower::gkName) == 0 )
	{
		EvtData_Select_Tower *data = e.getData<EvtData_Select_Tower>();
		m_game->SelectTower(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Sell_Selected_Tower::gkName) == 0 )
	{
		m_game->SellTower();
	}
	else
	if (strcmp(e.getType().getName(), Evt_Upgrade_Selected_Tower::gkName) == 0 )
	{
		m_game->UpgradeTower();
	}

	return false;
}

// Event listener for the game view, mostly just calls the view's functions.
bool GameViewListener::HandleEvent(Event const & e)
{
	if (strcmp(e.getType().getName(), Evt_New_Actor::gkName)==0)
	{
		EvtData_New_Actor *data = e.getData<EvtData_New_Actor>();
		m_view->VAddActor(data->m_Actor);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Remove_Actor::gkName)==0)
	{
		EvtData_Remove_Actor *data = e.getData<EvtData_Remove_Actor>();
		m_view->VRemoveActor(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Move_Actor::gkName)==0)
	{
		EvtData_Move_Actor *data = e.getData<EvtData_Move_Actor>();
		m_view->VMoveActor(data->m_id, data->m_Mat);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Move_Camera::gkName)==0)
	{
		EvtData_Move_Camera *data = e.getData<EvtData_Move_Camera>();
	
		m_view->MoveCamera(data->m_pos);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Change_GameState::gkName)==0)
	{
		EvtData_Change_GameState *data = e.getData<EvtData_Change_GameState>();
		m_view->VGameStatusChange(data->m_state);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Shot::gkName)==0)
	{
		EvtData_Shot *data = e.getData<EvtData_Shot>();
		m_view->AddShot(data->m_id, data->m_time, data->m_start, data->m_end, data->m_texture);
		shared_ptr<CSoundProcess> sfx (SAFE_NEW CSoundProcess("tada.wav"));
		m_view->Attach(sfx);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Remove_Effect::gkName)==0)
	{
		EvtData_Remove_Effect *data = e.getData<EvtData_Remove_Effect>();
		m_view->RemoveEffect(data->m_eventNum);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Remove_Effect_By_Id::gkName)==0)
	{
		EvtData_Remove_Effect_By_Id *data = e.getData<EvtData_Remove_Effect_By_Id>();
		m_view->RemoveEffectById(data->m_Id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Device_Created::gkName)==0)
	{
		EvtData_Device_Created *data = e.getData<EvtData_Device_Created>();
		m_view->DeviceCreated(data->m_device);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Change_Tower_Type::gkName) == 0 )
	{
		EvtData_Change_Tower_Type *data = e.getData<EvtData_Change_Tower_Type>();
		m_view->TowerChange(data->m_type);
	}
	else
	if (strcmp(e.getType().getName(), Evt_RebuildUI::gkName) == 0 )
	{
		m_view->RebuildUI();
	}
	else
	if (strcmp(e.getType().getName(), Evt_Select_Tower::gkName) == 0 )
	{
		EvtData_Select_Tower *data = e.getData<EvtData_Select_Tower>();
		m_view->SelectTower(data->m_id);
	}
	else
	if (strcmp(e.getType().getName(), Evt_Mouse_Move::gkName) == 0 )
	{
		EvtData_Mouse_Move *data = e.getData<EvtData_Mouse_Move>();
		m_view->MouseMove(data->m_pos);
	}


	return false;
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////HumanUI////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Constructor for the UI
HumanUI::HumanUI():m_buttonsPerRow(4),m_visible(true),m_towerSelected(false)
{
	// Changes the default text for the manager
	AddFontResourceEx(L"data/Amiga.ttf",0,0);
	// Default ui for tower switching.
	m_UI.Init(&m_DialogResourceManager);
	m_UI.SetTexture(0, L"data/tmpUI.bmp");
	m_UI.SetCallback( OnGUIEvent);
	m_UI.SetLocation(0,0);
	m_UI.SetFont(0, L"Amiga Forever", 15, 5);

	// UI for the selected tower
	m_SelectedTower.Init(&m_DialogResourceManager);
	m_SelectedTower.SetTexture(0, L"data/tmpUI.bmp");
	m_SelectedTower.SetCallback( OnGUIEvent);
	m_SelectedTower.SetLocation(0,0);
	m_SelectedTower.SetFont(0, L"Amiga Forever", 20, 5);

	// UI for the tower type.
	m_TowerType.Init(&m_DialogResourceManager);
	m_TowerType.SetTexture(0, L"data/tmpUI.bmp");
	m_TowerType.SetCallback( OnGUIEvent);
	m_TowerType.SetLocation(0,0);
	m_TowerType.SetFont(0, L"Amiga Forever", 20, 5);

	m_Width = 200;
	m_Height = 30;
	m_PosX = (DXUTGetBackBufferSurfaceDesc()->Width);// - m_Width;
	m_PosY = (DXUTGetBackBufferSurfaceDesc()->Height);// - m_Height;
}

// Destructor for the ui.
HumanUI::~HumanUI()
{
	m_DialogResourceManager.OnLostDevice();
	m_DialogResourceManager.OnDestroyDevice();
}

// Restores the UI after device lost
HRESULT HumanUI::VOnRestore()
{
	m_DialogResourceManager.OnResetDevice();
	// Finds the new location based on the size
	m_PosX = (DXUTGetBackBufferSurfaceDesc()->Width) - (m_Width + 5);
	if (100 > DXUTGetBackBufferSurfaceDesc()->Height)
		m_PosY = (DXUTGetBackBufferSurfaceDesc()->Height)- m_Height;
	else
		m_PosY = 100;
	// Sets the location and size for the ui.
	m_UI.SetLocation(m_PosX,m_PosY);
	m_UI.SetSize(m_Width, m_Height);

	m_SelectedTower.SetLocation(m_PosX, m_PosY + 70);
	m_SelectedTower.SetSize(m_Width, 230);
	m_TowerType.SetLocation(m_PosX, m_PosY + 70);
	m_TowerType.SetSize(m_Width, 230);
	return S_OK;
}

// Renders the UI every frame.
HRESULT HumanUI::VRender(double fTime, float fElapsedTime)
{
	TCHAR buffer[256];
	wsprintf(buffer, _T("%d"), g_App->m_pGame->GetData().m_curMoney);
	m_UI.GetStatic(22)->SetText(buffer);
	wsprintf(buffer, _T("%d"), g_App->m_pGame->GetData().m_curLife);
	m_UI.GetStatic(33)->SetText(buffer);
	m_UI.OnRender(fElapsedTime);
	m_SelectedTower.OnRender(fElapsedTime);
	m_TowerType.OnRender(fElapsedTime);
	return S_OK;
}

// Handles messages from the ui.
LRESULT HumanUI::VOnMsgProc(AppMsg msg)
{
	LRESULT r,t;
	r = m_UI.MsgProc(msg.m_hWnd, msg.m_uMsg, msg.m_wParam, msg.m_lParam);
	t = m_SelectedTower.MsgProc(msg.m_hWnd, msg.m_uMsg, msg.m_wParam, msg.m_lParam);
	return r || t;
}

// Called when the UI triggers an event (such as button being pressed)
void CALLBACK HumanUI::OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void* pUserContext )
{
	switch (nControlID)
	{
	case IDOK:
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Spawn_Wave()));
		break;
	case 10:
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Sell_Selected_Tower()));
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Change_Tower_Type(-1)));
		break;
	case 20:
		//upgrade
		safeQueueEvent(EventPtr (SAFE_NEW Evt_Upgrade_Selected_Tower()));
		break;
	default:
		if (nControlID >= 100)
		{
			int i = nControlID % 100;
			safeTriggerEvent(Evt_Change_Tower_Type(i));
		}
		break;

	}
	//PostMessage(g_App->GetHwnd(), 1, 0, nControlID);
}

// Called when the device is created
void HumanUI::OnDeviceCreate(IDirect3DDevice9 * device)
{
	m_DialogResourceManager.OnCreateDevice(device);
}

// Called when the device is lost
void HumanUI::OnDeviceLost()
{
	m_DialogResourceManager.OnLostDevice();
}

// Switches the tower to the new type
void HumanUI::TowerSwitch(int type)
{
	// If it is a valid type, put the base stats for it
	if (type >= 0)
	{
		m_SelectedTower.SetVisible(false);
		m_TowerType.SetVisible(true);
		TowerParams t = g_App->m_pGame->GetTowerData(type);
		TCHAR buffer[256];
		wsprintf(buffer, _T("Basic Tower %d"), type);
		m_TowerType.GetStatic(1)->SetText(buffer);
		wsprintf(buffer, _T("RoF: %ds"), t.m_reloadTime/1000);
		m_TowerType.GetStatic(2)->SetText(buffer);
		wsprintf(buffer, _T("Damage: %d"), t.m_damage);
		m_TowerType.GetStatic(3)->SetText(buffer);
		wsprintf(buffer, _T("Range: %d"), t.m_range);
		m_TowerType.GetStatic(4)->SetText(buffer);
		wsprintf(buffer, _T("Cost: %d"), t.m_cost);
		m_TowerType.GetStatic(5)->SetText(buffer);
		m_towerSelected = true;
	}
	// Otherwise hide the selection
	else
	{
		m_TowerType.SetVisible(false);
		m_SelectedTower.SetVisible(false);
	}
}

// Selects the tower with the ActorId
void HumanUI::SelectTower(ActorId id)
{
	shared_ptr<IActor> actor = g_App->m_pGame->GetActor(id);

	// Checks if actor exists and is a tower
	if (actor && actor->VGet()->m_Type == AT_TOWER)
	{
		shared_ptr<TowerActor> tower = boost::dynamic_pointer_cast<TowerActor>(actor);
		TowerParams t = tower->GetTowerParams();
		m_TowerType.SetVisible(false);
		m_SelectedTower.SetVisible(true);

		// Fills the selection with the correct information.
		TCHAR buffer[256];
		wsprintf(buffer, _T("Basic Tower %d"), t.m_type);
		m_SelectedTower.GetStatic(1)->SetText(buffer);
		wsprintf(buffer, _T("RoF: %ds"), t.m_reloadTime/1000);
		m_SelectedTower.GetStatic(2)->SetText(buffer);
		wsprintf(buffer, _T("Damage: %d"), t.m_damage);
		m_SelectedTower.GetStatic(3)->SetText(buffer);
		wsprintf(buffer, _T("Range: %d"), t.m_range);
		m_SelectedTower.GetStatic(4)->SetText(buffer);
		wsprintf(buffer, _T("Cost: %d"), t.m_cost);
		m_SelectedTower.GetStatic(5)->SetText(buffer);

		if (t.m_nextUpgrade > t.m_maxUpgrade)
			m_SelectedTower.GetButton(20)->SetEnabled(false);
		else
			m_SelectedTower.GetButton(20)->SetEnabled(true);
	}
	else
		m_SelectedTower.SetVisible(false);
}

// Sets up the location for the dialogs in their initial positions.
void HumanUI::BuildInitialDialogs(int numButtons)
{
	int numRows = numButtons / m_buttonsPerRow;
	int w = (m_Width- 3*(m_buttonsPerRow-2)) / m_buttonsPerRow;
	m_UI.RemoveAllControls();
	m_UI.AddButton(IDOK, L"Send Next Wave", 0, 0, m_Width, m_Height);
	CDXUTElement *el = m_UI.GetButton(IDOK)->GetElement(1);
	RECT r;
	SetRect(&r, (w+3)*(numButtons), 0, (w+3)*(numButtons+1), 32);
	el->SetTexture(0, &r);
	el->FontColor.States[0] = g_Black;
	el->FontColor.States[5] = g_Black;
	m_UI.GetButton(IDOK)->SetElement(1, el);
	m_UI.GetButton(IDOK)->SetElement(0, el);

	m_UI.AddButton(2, L"", 0, -40, 30, m_Height);
	el = m_UI.GetButton(2)->GetElement(1);
	SetRect(&r, 0, 32, 50, 64);
	el->SetTexture(0, &r);
	m_UI.GetButton(2)->SetElement(1, el);
	m_UI.GetButton(2)->SetElement(0, el);
	m_UI.GetButton(2)->SetEnabled(false);

	m_UI.AddStatic(22, L"", 40, -40, 40, m_Height);

	m_UI.AddButton(3, L"", 0, -80, 30, m_Height);
	el = m_UI.GetButton(3)->GetElement(1);
	SetRect(&r, 50, 32, 100, 64);
	el->SetTexture(0, &r);
	m_UI.GetButton(3)->SetElement(1, el);
	m_UI.GetButton(3)->SetElement(0, el);
	m_UI.GetButton(3)->SetEnabled(false);

	m_UI.AddStatic(33, L"", 40, -80, 40, m_Height);
	

	
	for (int i = 0; i < numButtons; i++)
	{
		m_UI.AddButton(100+i, L"", (w + 3) * (i%m_buttonsPerRow), (m_Height + 3) * (i/m_buttonsPerRow +1), w, m_Height);
		el = m_UI.GetButton(100+i)->GetElement(1);
		SetRect(&r, (w+3)*i, 0, (w+3)*(i+1), 32);
		el->SetTexture(0, &r);
		m_UI.GetButton(100+i)->SetElement(1, el);
		m_UI.GetButton(100+i)->SetElement(0, el);
	}

	m_SelectedTower.RemoveAllControls();
	m_SelectedTower.AddButton(10, L"Sell", 5, 5, 90, 30);
	m_SelectedTower.AddButton(20, L"Upgrade", 100, 5, 95, 30);
	m_SelectedTower.GetButton(10)->GetElement(1);
	SetRect(&r, (w+3)*(numButtons), 0, (w+3)*(numButtons+1), 32);
	el->SetTexture(0, &r);
	el->FontColor.States[0] = g_Black;
	el->FontColor.States[5] = g_Black;
	m_SelectedTower.GetButton(10)->SetElement(1, el);
	m_SelectedTower.GetButton(10)->SetElement(0, el);
	m_SelectedTower.GetButton(20)->SetElement(1, el);
	m_SelectedTower.GetButton(20)->SetElement(0, el);
	m_SelectedTower.SetBackgroundColors(D3DCOLOR_ARGB(255,255,255,255));
	el->dwTextFormat = DT_LEFT;
	m_SelectedTower.AddStatic(1, L"", 20, (25)+40, m_Width, m_Height);
	m_SelectedTower.GetStatic(1)->SetElement(0, el);
	m_SelectedTower.AddStatic(2, L"", 20, (25) * (2)+40, m_Width, m_Height);
	m_SelectedTower.GetStatic(2)->SetElement(0, el);
	m_SelectedTower.AddStatic(3, L"", 20, (25) * (3)+40, m_Width, m_Height);
	m_SelectedTower.GetStatic(3)->SetElement(0, el);
	m_SelectedTower.AddStatic(4, L"", 20, (25) * (4)+40, m_Width, m_Height);
	m_SelectedTower.GetStatic(4)->SetElement(0, el);
	m_SelectedTower.AddStatic(5, L"", 20, (25) * (5)+40, m_Width, m_Height);
	m_SelectedTower.GetStatic(5)->SetElement(0, el);
	m_SelectedTower.SetVisible(false);

	m_TowerType.RemoveAllControls();
	m_TowerType.SetBackgroundColors(D3DCOLOR_ARGB(255,255,255,255));
	m_TowerType.AddStatic(1, L"", 20, (25)+40, m_Width, m_Height);
	m_TowerType.GetStatic(1)->SetElement(0, el);
	m_TowerType.AddStatic(2, L"", 20, (25) * (2)+40, m_Width, m_Height);
	m_TowerType.GetStatic(2)->SetElement(0, el);
	m_TowerType.AddStatic(3, L"", 20, (25) * (3)+40, m_Width, m_Height);
	m_TowerType.GetStatic(3)->SetElement(0, el);
	m_TowerType.AddStatic(4, L"", 20, (25) * (4)+40, m_Width, m_Height);
	m_TowerType.GetStatic(4)->SetElement(0, el);
	m_TowerType.AddStatic(5, L"", 20, (25) * (5)+40, m_Width, m_Height);
	m_TowerType.GetStatic(5)->SetElement(0, el);
	m_TowerType.SetVisible(false);
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Slow///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Changes the actor's speed when applied.
void Slow::VApply()
{
	shared_ptr<IActor> actor = g_App->m_pGame->GetActor(m_id);

	if (actor)
	{
		int speed = actor->VGet()->m_speed;
		speed = speed/2;
		actor->VGet()->m_speed = speed;
	}
}

// Resets the actor's speed when removed.
void Slow::VRemove()
{
	shared_ptr<IActor> actor = g_App->m_pGame->GetActor(m_id);

	if (actor)
	{
		int speed = actor->VGet()->m_speed;
		speed = speed * 2;
		actor->VGet()->m_speed = speed;
	}
}

// Slowly ticks down until it is removed.
bool Slow::VOnUpdate(int deltaMS)
{
	m_time -= deltaMS;
	if (m_time <= 0)
		return true;

	return false;
}