#include "LuaReader.h"
#include "Event.h"
#include "ResourceCache\ResCache2.h"
#include "EngineFiles\Game.h"

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////LuaReader//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Base constructor
LuaReader::LuaReader() 
{
	L = NULL;
	m_file = "";
}

LuaReader::~LuaReader()
{
	if (L)
		lua_close(L);
}

// Sets up the reader with the specified file
HRESULT LuaReader::Init(std::string file)
{
	m_file = file;

	if (m_file.length() < 3)
		return S_FALSE;

	// Gets the resource from the resource file
	Resource resource(file.c_str());
	int size = g_App->m_ResCache->Create(resource);

	// Fills the character buffer with the characters from the file and puts a null at the end.
	assert(size);
	char *textureBuffer = (char *)g_App->m_ResCache->Get(resource);
	std::string s (textureBuffer);
	s[size] = '\0';

	// Starts lua for this class and registers the common functions.
	L = luaL_newstate();
	luaL_openlibs(L);
	RegisterFunctions();

	// Loads the script into lua.
	int tmp;
	tmp = luaL_loadstring(L, s.c_str());

	if (tmp)
	{
		return S_FALSE;
	}
	
	return S_OK;
}

// Runs the lua script
HRESULT LuaReader::Run()
{
	int tmp = lua_pcall(L, 0, 0, 0);

	if (tmp)
	{
		return S_FALSE;
	}

	return S_OK;
}

// Registers the functions in lua so they can be called
int LuaReader::RegisterFunctions()
{
	lua_pushcfunction(L, lua_shoot_tower);
	lua_setglobal(L, "shoot_tower");
	lua_pushcfunction(L, lua_damage_target);
	lua_setglobal(L, "damage_target");
	lua_pushcfunction(L, lua_slow_target);
	lua_setglobal(L, "slow_target");
	lua_pushcfunction(L, lua_fire_missile);
	lua_setglobal(L, "fire_missile");
	return 1;
}

// Called when a tower is shooting a target
int LuaReader::lua_shoot_tower(lua_State *l)
{
	int id = (int)luaL_checknumber(l, 1);
	int damage = (int)luaL_checknumber(l, 2);

	safeTriggerEvent(Evt_Shoot_Tar(id, damage));
	return 0;
}

// Damages a target for a certain amount
int LuaReader::lua_damage_target(lua_State *l)
{
	int id = (int) luaL_checknumber(l, 1);
	int damage = (int) luaL_checknumber(l, 2);

	safeTriggerEvent(Evt_Damage_Actor(id, damage));
	return 0;
}

// Applys the slow debuf on the given actor.
int LuaReader::lua_slow_target(lua_State *l)
{
	int id = (int) luaL_checknumber(l, 1);

	shared_ptr<IBuff> buff (SAFE_NEW Slow(id));
	safeTriggerEvent(Evt_Apply_Buff(buff));
	return 0;
}

// Fires a missle from the tower
int LuaReader::lua_fire_missile(lua_State *l)
{
	int id = (int) luaL_checknumber(l, 1);
//	int tar = (int) luaL_checknumber(l, 2);

	safeQueueEvent(EventPtr (SAFE_NEW Evt_Create_Missile(id)));
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////LuaTowerReader/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Reads in the different aspects of the tower type
void LuaTowerReader::ReadTowerType()
{
	TowerType p;
	
	lua_getglobal(L,"damage");
	if (lua_isnumber(L,-1))
		p.m_damage = (int) lua_tonumber(L,-1);

	lua_getglobal(L,"reload");
	if (lua_isnumber(L,-1))
		p.m_reloadTime = (int) lua_tonumber(L,-1);

	lua_getglobal(L,"range");
	if (lua_isnumber(L,-1))
		p.m_range = (int) lua_tonumber(L,-1);

	lua_getglobal(L,"cost");
	if (lua_isnumber(L,-1))
		p.m_cost = (int) lua_tonumber(L,-1);

	lua_getglobal(L,"shottexture");
	if (lua_isstring(L, -1))
		p.m_shottexture = lua_tostring(L, -1);

	lua_getglobal(L,"chartexture");
	if (lua_isstring(L, -1))
		p.m_chartexture = lua_tostring(L, -1);

	p.m_script = m_file;

	safeTriggerEvent(Evt_New_Tower_Type(p)); 
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////LuaMainGame////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


// Reads in a new tower type from the given file
void LuaMainGame::NewTowerType(std::string file)
{
	LuaTowerReader reader;
	reader.Init(file);
	reader.Run();
	reader.ReadTowerType();
}

// Loops through the scripts and loads default tower information
int LuaMainGame::ReadTowerTypes()
{
	lua_getglobal(L, "towers");

	int tmp;
	tmp = lua_istable(L,-1);
	if(!tmp)
    {
        return 0;
    }

    lua_pushnil(L);  /* first key */
	int count = 0;
    while (lua_next(L, -2) != 0) 
	{
        /* uses 'key' (at index -2) and 'value' (at index -1) */ 
        //add your own code to get the key and value
		if (lua_isstring(L, -1))
		{
			std::string file = lua_tostring(L, -1);
			NewTowerType(file);
			count++;
		}

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }

	lua_pop(L, 1);

	return count;
}

// Reads in the information for the waves
int LuaMainGame::ReadWave(int numWave)
{
	lua_getglobal(L, "waves");

	int tmp = lua_istable(L, -1);
	if (!tmp)
		return 0;

	//lua_pushnil(L);
	lua_pushnumber(L,numWave);
    /* Have Lua functions look up the keyed value */
    lua_gettable(L, -2 );
    /* Extract the result which is on the stack */
	int num=0;
	if (lua_isnumber(L,-1))
		num = (int) lua_tonumber(L,-1);
    /* Tidy up the stack - get rid of the extra we added*/
    lua_pop(L,1);
	
	if (num)
		return num;
	else
		return -1;
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////LuaTower///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Updates the script on the current time
void LuaTower::OnUpdate(int deltaMS)
{
	if (m_bInitialUpdate)
	{
		OnInitialize();
		m_bInitialUpdate = false;
	}
	lua_getglobal(L,"OnUpdate");
	lua_pushnumber(L, deltaMS);
	int tmp = lua_pcall(L, 1, 0, 0);
}

// Initializes the information in the script before running
void LuaTower::OnInitialize()
{
	Init(m_file);
	Run();
	lua_getglobal(L,"OnInitialize");
	lua_pushnumber(L, m_id);
	int tmp = lua_pcall(L, 1, 0, 0);
}

// Calls the fire function in the script
void LuaTower::Fire(ActorId id)
{
	lua_getglobal(L,"Fire");
	lua_pushnumber(L, id);
	int tmp = lua_pcall(L, 1, 0, 0);
}

// Calls the set target function in the script
void LuaTower::SetTarget(ActorId id)
{
	lua_getglobal(L, "SetTarget");
	lua_pushnumber(L, id);
	int tmp = lua_pcall(L, 1, 0, 0);
}

// Calls the upgrade function in the script
void LuaTower::UpgradeTower(Upgrade u)
{
	lua_getglobal(L, "Upgrade");
	lua_pushnumber(L, u.m_damage);
	lua_pushnumber(L, u.m_range);
	lua_pushnumber(L, u.m_reload);
	int tmp = lua_pcall(L, 3, 0, 0);
}

LuaTower::~LuaTower()
{
}