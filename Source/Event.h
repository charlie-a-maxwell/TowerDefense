/*
Based on the event system in Game Coding Complete.
I made some minor changes to things I didn't think I needed or to make the simplier.
*/


#pragma once

#include "StdHeader.h"
#include <set>


// Class that holds information on the event. Will be unique for each type of event, but the same for all events of the same type.
class EventType
{
	char * const m_name;
	unsigned int m_id;
public:
	EventType(char * const name): m_id(hashName(name)), m_name(name) {}
	static unsigned int hashName(char * const name)
	{
		// Jenkins one at a time hash
		unsigned int hash = 0;
		size_t i, key_len = strlen(name);
	 
		for (i = 0; i < key_len; i++) {
			hash += name[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return hash;
	}
	unsigned int getId() const {return m_id;}
	char * const getName() const {return m_name;}

	bool operator< (EventType const &o) const
	{
		bool t = (getId() < o.getId());
		return t;
	}

	bool operator= (EventType const &o) const
	{
		bool t = (getId() == o.getId());
		return t;
	}
};

// Base class for the events.
class Event
{
	EventType m_type;
	int m_timeIn;
	EventDataPtr m_data;
public:
	Event (char * const name, int timeIn, EventDataPtr data = EventDataPtr((IEventData *)NULL)):
	  m_type(name), m_timeIn(timeIn), m_data(data) {};
	
	  ~Event() {}
	  unsigned int getId() {return m_type.getId();}

	  // Used to get the data from the event pointer. The data is dependant on the type of event.
	  template<typename _T>
	  _T * getData() const {return reinterpret_cast<_T *>(m_data.get());}

	  char* const getName() {return m_type.getName();}
	  EventType getType() const {return m_type;}
};


// Common definitions used for the events
typedef std::set<EventType> EventTypeSet;
typedef std::pair<EventTypeSet::iterator, bool> EventTypeSetRet;
typedef std::list<EventPtr> EventQueue;
typedef std::map<unsigned int, EventListenerList> EventListenerMap;
typedef std::pair<EventListenerMap::iterator, bool> EventListenerMapRet;
typedef std::pair<unsigned int, EventListenerList> EventListenerMapEntry;


// Class used to manage the events. This is a global class that manages itself. 
class EventManager : public IEventManager
{
	EventTypeSet m_eventTypes;
	EventListenerMap m_listenerMap;
	EventQueue m_eventQueue;
	
public:
	EventManager(){};
	virtual bool addListener(EventListenerPtr const & listener, EventType const & type);
	virtual bool triggerEvent(Event const & event);
	virtual bool queueEvent(EventPtr const & event);
	virtual bool tick(unsigned int maxMS);
	virtual bool validateType(EventType const & type);
};

// Event for adding new actor based on shared pointer of the actor interface class.
class EvtData_New_Actor : public IEventData
{
public:
	shared_ptr<IActor> m_Actor;

	EvtData_New_Actor(shared_ptr<IActor> p): m_Actor(p){}
};

class Evt_New_Actor :public Event
{
public:
	static char * const gkName;
	Evt_New_Actor(shared_ptr<IActor> p):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_New_Actor(p))){}
};




// Event for removing the actor of the given id.
class EvtData_Remove_Actor : public IEventData
{
public:
	ActorId m_id;

	EvtData_Remove_Actor(ActorId id):m_id(id) {}
};

class Evt_Remove_Actor :public Event
{
public:
	static char * const gkName;
	Evt_Remove_Actor(ActorId id):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Remove_Actor(id))){}
};





// Event for moving an actor. The matrix is the location to move the actor to.
class EvtData_Move_Actor : public IEventData
{
public:
	ActorId m_id;
	Mat4x4 m_Mat;

	EvtData_Move_Actor(ActorId id, Mat4x4 mat):m_id(id),m_Mat(mat) {}
};

class Evt_Move_Actor :public Event
{
public:
	static char * const gkName;
	Evt_Move_Actor(ActorId id, Mat4x4 mat):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Move_Actor(id, mat))){}
};





// Event to create a new tower at the location given.
class EvtData_New_Tower : public IEventData
{
public:
	Vec3 m_pos;

	EvtData_New_Tower(Vec3 pos):m_pos(pos) {}
};

class Evt_New_Tower :public Event
{
public:
	static char * const gkName;
	Evt_New_Tower(Vec3 pos):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_New_Tower(pos))){}
};





// Event to sell tower at the location given.
class EvtData_Sell_Tower : public IEventData
{
public:
	Vec3 m_pos;

	EvtData_Sell_Tower(Vec3 pos):m_pos(pos) {}
};

class Evt_Sell_Tower :public Event
{
public:
	static char * const gkName;
	Evt_Sell_Tower(Vec3 pos):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Sell_Tower(pos))){}
};





// Event to create a new runner at the starting location.
class Evt_New_Runner :public Event
{
public:
	static char * const gkName;
	Evt_New_Runner():Event(gkName, 0){}
};




// Event to move the camera to the given location.
class EvtData_Move_Camera : public IEventData
{
public:
	Mat4x4 m_pos;

	EvtData_Move_Camera(Mat4x4 pos):m_pos(pos) {}
};

class Evt_Move_Camera :public Event
{
public:
	static char * const gkName;
	Evt_Move_Camera(Mat4x4 pos):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Move_Camera(pos))){}
};





// Event to change the state of the game (paused, running, etc)
class EvtData_Change_GameState : public IEventData
{
public:
	GameStatus m_state;

	EvtData_Change_GameState(GameStatus state):m_state(state) {}
};

class Evt_Change_GameState :public Event
{
public:
	static char * const gkName;
	Evt_Change_GameState(GameStatus state): Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Change_GameState(state))){} 
};
 




// Event to find the path for the runners to get from their location to the end.
class EvtData_Set_Path : public IEventData
{
public:
	ActorId m_id;

	EvtData_Set_Path(ActorId id):m_id(id) {}
};

class Evt_Set_Path :public Event
{
public:
	static char * const gkName;
	Evt_Set_Path(ActorId id):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Set_Path(id))){}
};






// Event to find the closest runner to the actor given.
class EvtData_Closest_Tar : public IEventData
{
public:
	ActorId m_id;

	EvtData_Closest_Tar(ActorId id):m_id(id) {}
};

class Evt_Find_Closest_Tar :public Event
{
public:
	static char * const gkName;
	Evt_Find_Closest_Tar(ActorId id):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Closest_Tar(id))) {}
};






// Event to shoot a target with the given damage.
class EvtData_Shoot_Tar : public IEventData
{
public:
	ActorId m_shooterId;
	int m_damage;


	EvtData_Shoot_Tar(ActorId shooter, int damage): m_shooterId(shooter),m_damage(damage){}
};

class Evt_Shoot_Tar :public Event
{
public:
	static char * const gkName;
	Evt_Shoot_Tar(ActorId shooter, int damage):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Shoot_Tar(shooter, damage))) {}
};






// Event used to create a visual effect for the shot from a tower.
class EvtData_Shot : public IEventData
{
public:
	Vec3 m_start;
	Vec3 m_end;
	ActorId m_id;
	int m_time;
	std::string m_texture;

	EvtData_Shot(ActorId id, int time, Vec3 start, Vec3 end, std::string texture):m_start(start), m_end(end), m_id(id), m_time(time), m_texture(texture){}
};

class Evt_Shot :public Event
{
public:
	static char * const gkName;
	Evt_Shot(ActorId id, int time, Vec3 start, Vec3 end, std::string texture):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Shot(id, time, start, end, texture))) {}
};






// Event used to remove a visual effect.
class EvtData_Remove_Effect: public IEventData
{
public:
	unsigned int m_eventNum;
	EvtData_Remove_Effect(unsigned int num):m_eventNum(num) {}
};

class Evt_Remove_Effect: public Event
{
public:
	static char * const gkName;
	Evt_Remove_Effect(unsigned int num):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Remove_Effect(num))) {}
};







// Event used to remove an effect by its id.
class EvtData_Remove_Effect_By_Id: public IEventData
{
public:
	ActorId m_Id;
	EvtData_Remove_Effect_By_Id(ActorId id):m_Id(id) {}
};

class Evt_Remove_Effect_By_Id: public Event
{
public:
	static char * const gkName;
	Evt_Remove_Effect_By_Id(ActorId id):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Remove_Effect_By_Id(id))) {}
};






// Event to spawn a new wave of runners.
class Evt_Spawn_Wave: public Event
{
public:
	static char * const gkName;
	Evt_Spawn_Wave():Event(gkName, 0) {}
};






// Event used when the display device is created.
class EvtData_Device_Created: public IEventData
{
public:
	IDirect3DDevice9 * m_device;
	EvtData_Device_Created(IDirect3DDevice9 * device):m_device(device) {}
};
class Evt_Device_Created: public Event
{
public:
	static char * const gkName;
	Evt_Device_Created(IDirect3DDevice9 * device):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Device_Created(device))) {}
};



// Event that changes the currently selected tower type.
class EvtData_Change_Tower_Type: public IEventData
{
public:
	unsigned int m_type;
	EvtData_Change_Tower_Type(unsigned int type):m_type(type) {}
};

class Evt_Change_Tower_Type: public Event
{
public:
	static char * const gkName;
	Evt_Change_Tower_Type(unsigned int type):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Change_Tower_Type(type))) {}
};





// Event creates a new type of tower.
class EvtData_New_Tower_Type: public IEventData
{
public:
	TowerType m_params;
	EvtData_New_Tower_Type(TowerType n):m_params(n) {}
};

class Evt_New_Tower_Type: public Event
{
public:
	static char * const gkName;
	Evt_New_Tower_Type(TowerType p):
				Event(gkName, 0, EventDataPtr( SAFE_NEW EvtData_New_Tower_Type(p))) {}
};




// Event sent to rebuild the ui. Used when the display changes or resets.
class Evt_RebuildUI: public Event
{
public:
	static char * const gkName;
	Evt_RebuildUI():Event(gkName, 0){}
};




// Event used to deal damage to an actor.
class EvtData_Damage_Actor : public IEventData
{
public:
	ActorId m_id;
	int m_damage;
	EvtData_Damage_Actor(ActorId id, int damage): m_id(id),m_damage(damage){}
};

class Evt_Damage_Actor : public Event
{
public:
	static char * const gkName;
	Evt_Damage_Actor(ActorId id, int damage): Event(gkName, 0, EventDataPtr( SAFE_NEW EvtData_Damage_Actor(id, damage))) {}
};



// Event used to apply a buff (or modifier) to a target.
class EvtData_Apply_Buff : public IEventData
{
public:
	shared_ptr<IBuff> m_buff;
	EvtData_Apply_Buff(shared_ptr<IBuff> buff): m_buff(buff) {}
};

class Evt_Apply_Buff : public Event
{
public:
	static char * const gkName;
	Evt_Apply_Buff(shared_ptr<IBuff> buff): Event(gkName, 0, EventDataPtr( SAFE_NEW EvtData_Apply_Buff(buff) ) ) {}
};





// Event to create a missle type actor to target the given id.
class EvtData_Create_Missile : public IEventData
{
public:
	ActorId m_id;
	EvtData_Create_Missile(ActorId id):m_id(id){}
};

class Evt_Create_Missile : public Event
{
public:
	static char * const gkName;
	Evt_Create_Missile(ActorId id):Event(gkName, 0 , EventDataPtr( SAFE_NEW EvtData_Create_Missile(id))) {}
};




// Event used when the right mouse button has been clicked.
class EvtData_Right_Click : public IEventData
{
public:
	Vec3 m_loc;
	EvtData_Right_Click(Vec3 l):m_loc(l) {}
};

class Evt_Left_Click : public Event
{
public:
	static char * const gkName;
	Evt_Left_Click(Vec3 l):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Right_Click(l))) {}
};




// Event used to make a tower the currently selected tower. 
// Removes the currently selected tower type.
class EvtData_Select_Tower : public IEventData
{
public:
	ActorId m_id;
	EvtData_Select_Tower(ActorId id) : m_id(id){}
};

class Evt_Select_Tower : public Event
{
public:
	static char * const gkName;
	Evt_Select_Tower(ActorId id):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Select_Tower(id))) {}
};



// Event used to sell the currently selected tower.
// Does nothing if selected tower is a tower type.
class Evt_Sell_Selected_Tower : public Event
{
public:
	static char * const gkName;
	Evt_Sell_Selected_Tower():Event(gkName, 0) {}
};




// Event used to upgrade the currently selected tower.
// Does nothing if selected tower is a tower type.
class Evt_Upgrade_Selected_Tower : public Event
{
public: 
	static char * const gkName;
	Evt_Upgrade_Selected_Tower():Event(gkName,0) {}
};


// Event for when the mouse has moved.
class EvtData_Mouse_Move : public IEventData
{
public:
	Vec3 m_pos;

	EvtData_Mouse_Move(Vec3 pos):m_pos(pos) {}
};

class Evt_Mouse_Move :public Event
{
public:
	static char * const gkName;
	Evt_Mouse_Move(Vec3 pos):Event(gkName, 0, EventDataPtr(SAFE_NEW EvtData_Mouse_Move(pos))){}
};