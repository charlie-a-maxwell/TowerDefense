#pragma once

#include "StdHeader.h"
#include "SceneNode.h"
#include "Event.h"
#include "LuaReader.h"
#include "Process.h"

const double SCREEN_REFRESH_RATE(1000.0f/60.0f);
const int	MAP_SIZE = 20;
const int	HALF_MAP_SIZE = 10;

class HumanView;

// Mouse and Keyboard controller
class HumanInterfaceController
{
	unsigned int	m_scrollLines;
	int				m_scrolled;

	Mat4x4  m_matFromWorld;
	Mat4x4	m_matToWorld;
    Mat4x4  m_matPosition;

	CPoint					m_lastMousePos;
	BYTE					m_bKey[256];			// Which keys are up and down

	// Orientation Controls
	float		m_fTargetYaw;
	float		m_fTargetPitch;
	float		m_fYaw;
	float		m_fPitch;
	float		m_fPitchOnDown;
	float		m_fYawOnDown;
	float		m_maxSpeed;
	float		m_currentSpeed;
	
public:
	HumanInterfaceController();
	void OnKeyDown(const BYTE c) {m_bKey[c] = true; }
	void OnKeyUp(const BYTE c) {m_bKey[c] = false; }
	void OnUpdate(int deltaMS);
	void OnMouseScroll(int lines);
	void OnLButtonDown(const CPoint &mousePoint);
	void OnRButtonDown(const CPoint &mousePoint);
	void OnMouseMove(const CPoint &mousePoint);
};

// UI for the game
class HumanUI: public IScreenElement
{
	CDXUTDialogResourceManager m_DialogResourceManager;
	CDXUTDialog m_UI;
	CDXUTDialog m_SelectedTower;
	CDXUTDialog m_TowerType;
	int m_Zorder;
	bool m_visible;
	float	m_PosX, m_PosY, m_Width, m_Height;
	int m_buttonsPerRow;
	int m_index;
	bool m_towerSelected;
	
public:
	HumanUI();
	~HumanUI();
	virtual void VOnUpdate(int deltaMS){}
	virtual HRESULT VRender(double fTime, float fElapsedTime);
	virtual HRESULT VOnRestore();
	virtual LRESULT CALLBACK VOnMsgProc( AppMsg msg );

	virtual int VGetZOrder() const {return m_Zorder;}
	virtual void VSetZOrder(int const zOrder) {m_Zorder = zOrder;}
	virtual bool VIsVisible() const {return m_visible;}
	virtual void VSetVisible(bool visible) {m_visible = visible;}
	void OnDeviceCreate(IDirect3DDevice9* device);
	static void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void* pUserContext );
	void OnDeviceLost();
	void TowerSwitch(int type);
	void BuildInitialDialogs(int numButtons);
	void SelectTower(ActorId id);
};

class CSoundProcess;

// Human view, includes 3D renderer
class HumanView: public IGameView
{
	GameViewId						m_id;
	ScreenElementList				m_screenElementList;
	DWORD							m_currTick;
	DWORD							m_lastDraw;
	GameStatus						m_status;
	HumanInterfaceController		m_controller;

	shared_ptr<HumanUI>				m_humanUI;
	shared_ptr<HumanPlayerScene>	m_pScene;
	shared_ptr<CameraNode>			m_pCamera;
	shared_ptr<CSoundProcess>		m_music;
	EventListenerPtr				m_eventListener;

	ID3DXFont*						m_pFont;
	ID3DXSprite*					m_pTextSprite;
	unsigned int					m_lastShot;
	ProcessManager					*m_processManager;

	shared_ptr<IActor>				m_mouseOver;
public:
	HumanView();
	~HumanView();

	virtual void VOnUpdate(int deltaMS);
	virtual GameViewId VGetId() const {return m_id;}
	virtual void VOnAttach(GameViewId vid);
	virtual void VRender(double fTime, float fElapsedTime);
	virtual LRESULT CALLBACK VOnMsgProc( AppMsg msg );
	virtual HRESULT VOnRestore();
	virtual void VOnLostDevice();
	virtual void VPushScreen(shared_ptr<IScreenElement> e);
	virtual void VPopScreen();
	virtual void VRenderText(CDXUTTextHelper &txtHelper);

	virtual void VAddActor(shared_ptr<IActor> actor);
	virtual void VRemoveActor(ActorId id);
	void AddShot(ActorId id, int time, Vec3 start, Vec3 end, std::string texture);

	void VMoveActor(ActorId id, Mat4x4 const &mat);
	void MoveCamera(Mat4x4 const &change);
	void ChangeActorDirection(ActorId id, int dir);
	void LoopAnimation(ActorId id, bool loop);
	void RemoveEffect(unsigned int num) { m_pScene->RemoveEffect(num); }
	void RemoveEffectById(ActorId id) { m_pScene->RemoveEffectById(id); }
	HRESULT DeviceCreated(IDirect3DDevice9* device);
	void Attach(shared_ptr<Process> process) {m_processManager->Attach(process);}
	void BuildInitialScene();
	void RebuildUI();
	virtual void VGameStatusChange(GameStatus status) {m_status = status;}
	void MoveCamera(Vec3 pos);
	void TowerChange(int type);
	void SelectTower(ActorId id);
	void MouseMove(Vec3 pos);

	bool InitAudio();
};

// Node used in the A* algorithm
struct SearchNode
{
	int F,G,H;
	int parent, loc;
	SearchNode():F(0),G(0),H(0),parent(-1),loc(0) {}
	bool const operator < (SearchNode const &other){ return F < other.F;}
};
typedef std::map<int, SearchNode> SearchNodeMap;

// Used to hold information about the playing area.
class Map
{
	friend class TowerGame;
	const int m_start;
	const int m_end;
	ActorId		m_grid[MAP_SIZE * MAP_SIZE];
	bool TestLocation(int end, int start, shared_ptr<IActor> actor);
	
public:
	Map();
	void CreateMap();
	int HashLocation(Vec3 loc);
	bool AddActor(shared_ptr<IActor> actor);
	bool RemoveActor(ActorId id);
	Mat4x4 GetGridLocation(Vec3 v);
	Mat4x4 GetGridLocation(Vec3 v, int height, int width);
	Mat4x4 GetGridLocation(int i);
	bool CheckLocation(Vec3 v);
	void SetActorPath(shared_ptr<IActor> actor);
	bool TestRunnerAtEnd(shared_ptr<IActor> actor);
	ActorId GetActorAtLoc(Vec3 v);
	bool IsLocationOccupied(Vec3 v);
	bool IsLocationOccupied(Vec3 v, int height, int width);
};

// State data about the game
struct GameData
{
public: 
	int				m_timeLeftUntilWave;
	int				m_waveTimeLimit;
	int				m_curWave;
	int				m_curMoney;
	int				m_curLife;
};

// Logic class for the game.
class TowerGame: public IGame
{
	friend class GameApp;
	GameViewList		m_viewList;
	ActorMap			m_pActorMap;
	ActorId				m_LastActorId;
	GameStatus			m_status;
	
	EventListenerPtr	m_eventListener;
	GameData			m_data;
	TowerTypeMap		m_towerMap;
	int					m_curTowerType;
	ProcessManager		m_processManager;
	LuaMainGame			m_luaReader;
	ActorId				m_selectedTower;
	
	void CreateGrid();
	void FindNewPaths();
	
public:
	Map					m_gameMap;

	TowerGame();
	~TowerGame();
	void CreateWave();
	void NewTowerType(TowerType p);
	void CreateRunner();
	void CreateMissile(ActorId id);
	virtual void OnUpdate(int deltaMS);
	virtual void VAddActor(shared_ptr<IActor> actor);
	virtual void VRemoveActor(ActorId id);
	virtual void VMoveActor(ActorId id, const Mat4x4 &m);
	virtual void VAddView(shared_ptr<IGameView> view);
	void BuildInitialScene();
	virtual void VGameStatusChange(GameStatus status);
	void CreateTower(Vec3 loc);
	void SetActorPath(ActorId id);
	void SetTowerTarget(ActorId id);
	void ShootTar(ActorId shooter, int damage);
	void SellTower(Vec3 loc);
	void SellTower();
	void UpgradeTower();
	GameData GetData() {return m_data;}
	void ChangeTowerType(int type) {m_curTowerType = type;}
	TowerParams GetTowerData(int type) { if (type >= 0 && type < m_towerMap.size()) return m_towerMap[type].GetParams(); else return TowerParams();}
	int	GetNumTowerTypes() {return m_towerMap.size();}
	int WaveSpawns(int curWave) {return m_luaReader.ReadWave(curWave); }
	shared_ptr<IActor> GetActor(ActorId id);
	void DamageActor(ActorId id, int damage);
	void ApplyBuffToActor(ActorId id, shared_ptr<IBuff> buff);
	void RightClick(Vec3 l);
	void SelectTower(ActorId id) {m_selectedTower = id; m_curTowerType = -1;}
};

// Base class that interacts with the underlying OS
class GameApp
{
	CRect m_rcWindow, m_rcDesktop;
	int m_iColorDepth;
	bool CheckMemory(const DWORD physicalRAM, const DWORD virtualRAM);
	bool CheckHardDisk(const int diskSpace);
	EventManager m_eventManager;
	bool	m_Quitting;
public:
	GameApp();
	HWND GetHwnd() {return DXUTGetHWND();}
	TCHAR *GetGameTitle() { return _T("TowerDefense"); }
	bool InitInstance(HINSTANCE hInstance, LPTSTR lpCommandLine);

	static LRESULT CALLBACK MsgProc (HWND hWnd, UINT uMsg, WPARAM wParam, 
			LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);

	static HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void *pUserContext  );
	static void    CALLBACK OnLostDevice(void *pUserContext);
	static bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
	static void CALLBACK OnUpdateGame( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void *pUserContext );
	static void CALLBACK OnRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void *pUserContext );
	static HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext) { safeTriggerEvent(Evt_Device_Created(pd3dDevice)); return S_OK; }
	static void CALLBACK OnDestroyDevice( void* pUserContext ) {};
	static bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext );
	int GetExitCode() { return DXUTGetExitCode(); }

	LRESULT OnDisplayChange(int colorDepth, int width, int height);
	LRESULT OnDeviceChange(int eventType) {return TRUE;}
	int OnClose();
	LRESULT OnSysCommand(WPARAM wParam, LPARAM lParam);

	TowerGame* CreateGameAndView();
	TowerGame* m_pGame;
	class ResCache *m_ResCache;

	bool IsQuitting() {return m_Quitting;}
	void AbortGame() {m_Quitting = true;}
};

// Base class for all the game components
class Actor: public IActor
{
protected:
	shared_ptr<ActorParams>		m_params;
	std::list<Mat4x4>			m_moveQueue;
	int							m_elapsedTime;
	int							m_timeToStart;
	BuffList					m_buffs;
public:
	Actor();
	Actor(shared_ptr<ActorParams> p);

	virtual Mat4x4 const &VGetMat() {return m_params->m_Mat;}
	virtual void VSetMat(const Mat4x4 &m) {m_params->m_Mat = m;}
	virtual void VOnUpdate(int deltaMS);
	virtual float VGetRadius() {return m_params->m_radius;}
	virtual shared_ptr<ActorParams> VGet() {return m_params;}
	virtual void VSetId(ActorId id) {m_params->m_Id = id;}
	virtual void VSetParams(shared_ptr<ActorParams> p) {m_params = p;}
	virtual void VQueuePosition(const Mat4x4 &m) {m_moveQueue.push_back(m);}
	virtual void VClearQueue() {m_moveQueue.clear();}
	virtual bool VTakeDamage(int damage);
	virtual void VSetDirection(Vec3 b);
	virtual void VApplyBuff(shared_ptr<IBuff> buff);
};

// Tower actor default class
class TowerActor: public Actor
{
protected:
	ActorId		m_curTarget;
	TowerParams m_towerParams;
	int			m_timeUntilNextShot;
	LuaTower	m_luaScript;
public:
	TowerActor():Actor(),m_towerParams(),m_luaScript(),m_curTarget(-1) {}
	TowerActor(shared_ptr<ActorParams> p): Actor(p),m_towerParams(),m_luaScript(),m_curTarget(-1) {}
	TowerActor(TowerParams t, shared_ptr<ActorParams> p): Actor(p),m_towerParams(t),m_luaScript(p->m_Id, t.m_script),m_curTarget(-1){}
	virtual void VOnUpdate(int deltaMS);
	virtual void SetTarget(ActorId id); 
	ActorId GetTarget() {return m_curTarget;}
	float GetRange() {return m_towerParams.m_range;}
	std::string GetScript() {return m_towerParams.m_script;}
	virtual void VSetId(ActorId id) {m_params->m_Id = id; m_luaScript.SetId(id);}
	virtual void OnFire(ActorId id);
	TowerParams GetTowerParams() {return m_towerParams;}
	void UpgradeTower(Upgrade u);
	virtual void VSetDirection(Vec3 b);
};

// Used to do visual effects
class EffectActor : public Actor
{
protected:
	float			m_size;
public:
	EffectActor(): Actor() {}
	EffectActor(shared_ptr<ActorParams> p, float size) : Actor(p), m_size(size) {}
	virtual float VGetRadius() {return m_size;}
};

// A missle that will track it's target until it gets close enough.
class MissileActor : public TowerActor
{
	ActorId m_target;
	ActorId m_tower;
	Mat4x4 m_location;
public:
	MissileActor(shared_ptr<ActorParams> p, ActorId id, ActorId tar): TowerActor(p), m_tower(id),m_target(tar),m_location(Mat4x4::g_Identity) {}
	virtual void VOnUpdate(int deltaMS);
	virtual void SetTarget(ActorId id);
	virtual void OnFire(ActorId id);
};

// Modifies some part of the actor
class Buff: public IBuff
{
protected:
	ActorId m_id;
	BuffType m_type;
	int		m_time;

public:
	Buff(ActorId id, BuffType type, int time): m_id(id),m_type(type),m_time(time) {}
	virtual void VApply() {}
	virtual void VRemove() {}
	virtual bool VOnUpdate(int deltaMS) { return false;}
	virtual BuffType VGetType() {return m_type;}
	virtual ActorId VGetActorId() {return m_id;}
};

// A buff that slows the target
class Slow: public Buff
{
public:
	Slow(ActorId id):Buff(id, BT_ICE, 1100) {}
	virtual void VApply();
	virtual void VRemove();
	virtual bool VOnUpdate(int deltaMS);
};

// Event listener for the game logic
class GameLogicListener: public IEventListener
{
	TowerGame * m_game;
public:
	GameLogicListener(TowerGame * game):m_game(game){};
	virtual bool HandleEvent(Event const & e);
};

// Event listener for the game view
class GameViewListener: public IEventListener
{
	HumanView * m_view;
public:
	GameViewListener(HumanView * view):m_view(view){};
	virtual bool HandleEvent(Event const & e);
};

extern GameApp *g_App;
