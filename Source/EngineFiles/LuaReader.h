#pragma once

#include "StdHeader.h"
#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>
#include "Process.h"

// Default lua class
class LuaReader
{
protected:
	lua_State * L;
	std::string m_file;

	int RegisterFunctions();
	static int lua_shoot_tower(lua_State *l);
	static int lua_damage_target(lua_State *l);
	static int lua_slow_target(lua_State *l);
	static int lua_fire_missile(lua_State *l);
public:
	LuaReader();
	~LuaReader();
	HRESULT Init(std::string file);
	HRESULT Run();
};

// Lua reader to get information for the main game
class LuaMainGame: public LuaReader
{
private:
	void NewTowerType(std::string file);

public:
	int ReadTowerTypes();
	int ReadWave(int waveNum);
};

// Lua reader to get information for the different tower types.
class LuaTowerReader: public LuaReader
{
public:
	void ReadTowerType();
};

// Lua reader for the tower scripts.
class LuaTower: public LuaReader
{
	ActorId m_id;
	bool	m_bInitialUpdate;
public:
	LuaTower():LuaReader(),m_bInitialUpdate(true){}
	LuaTower(ActorId id, std::string s):LuaReader(),m_bInitialUpdate(true){ m_file = s;}
	~LuaTower();
	void SetId(ActorId id) {m_id = id;}

	virtual void OnUpdate(int deltaMS);
	virtual void OnInitialize();
	virtual void Fire(ActorId target);
	virtual void SetTarget(ActorId target);
	virtual void UpgradeTower(Upgrade u);
};