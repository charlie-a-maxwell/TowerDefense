/*
The events are what drive everything in the game structure. It works like the event system in Windows programming.
It is based off the event system in the book Game Coding Complete with minor changes.
The event manager is a global object that manages itself. It is called through helper functions.
*/


#include "Event.h"


char * const Evt_New_Actor::gkName = "create_actor_event";
char * const Evt_Remove_Actor::gkName = "remove_actor_event";
char * const Evt_New_Runner::gkName = "new_runner_event";
char * const Evt_New_Tower::gkName = "new_tower_event";
char * const Evt_Move_Actor::gkName = "move_actor_event";
char * const Evt_Move_Camera::gkName = "move_camera_event";
char * const Evt_Change_GameState::gkName = "change_status_event";
char * const Evt_Set_Path::gkName = "set_path_event";
char * const Evt_Find_Closest_Tar::gkName = "find_closest_tar_event";
char * const Evt_Shoot_Tar::gkName = "shoot_tar_event";
char * const Evt_Sell_Tower::gkName = "sell_tower_event";
char * const Evt_Shot::gkName = "shot_event";
char * const Evt_Remove_Effect::gkName = "remove_effect";
char * const Evt_Remove_Effect_By_Id::gkName = "remove_effect_by_id";
char * const Evt_Spawn_Wave::gkName = "spawn_wave_effect";
char * const Evt_Device_Created::gkName = "device_created_event";
char * const Evt_Change_Tower_Type::gkName = "change_tower_type";
char * const Evt_New_Tower_Type::gkName = "new_tower_type_event";
char * const Evt_RebuildUI::gkName = "rebuild_ui";
char * const Evt_Damage_Actor::gkName = "damage_actor";
char * const Evt_Apply_Buff::gkName = "apply_buff";
char * const Evt_Create_Missile::gkName = "create_missile";
char * const Evt_Left_Click::gkName = "right_click_event";
char * const Evt_Select_Tower::gkName = "select_tower";
char * const Evt_Sell_Selected_Tower::gkName = "sell_selected_tower";
char * const Evt_Upgrade_Selected_Tower::gkName = "upgrade_selected_tower";
char * const Evt_Mouse_Move::gkName = "mouse_move";



// List of helper functions to access the eventmanager.
IEventManager::IEventManager()
{
	g_EventManager = this;
}

IEventManager::~IEventManager()
{
	if (g_EventManager == this)
		g_EventManager = NULL;
}

IEventManager * IEventManager::Get()
{
	return g_EventManager;
}

bool safeAddListener(EventListenerPtr const & listener, EventType const & type)
{
	assert(IEventManager::Get() && "No Event Manager!");
	return IEventManager::Get()->addListener(listener, type);
}

bool safeTriggerEvent(Event const & event)
{
	assert(IEventManager::Get() && "No Event Manager!");
	return IEventManager::Get()->triggerEvent(event);
}

bool safeQueueEvent(EventPtr const & event)
{
	assert(IEventManager::Get() && "No Event Manager!");
	return IEventManager::Get()->queueEvent(event);
}

bool safeTick(unsigned int maxMS)
{
	assert(IEventManager::Get() && "No Event Manager!");
	return IEventManager::Get()->tick(maxMS);
}

bool safeValidateType(EventType const & type)
{
	assert(IEventManager::Get() && "No Event Manager!");
	return IEventManager::Get()->validateType(type);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////GameApp////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Adds an event listener to the list for that event type. Returns false if listener is not added, true if added
bool EventManager::addListener(EventListenerPtr const & listener, EventType const & type)
{
	if (!validateType(type))
		return false;

	EventTypeSet::iterator it = m_eventTypes.find(type);

	// Looks to see if the event is a type seen before and adds the event type if not.
	if (it == m_eventTypes.end())
	{
		EventTypeSetRet itEvent = m_eventTypes.insert(type);

		if (itEvent.second == false)
			return false;

		if (itEvent.first == m_eventTypes.end())
			return false;

		it = itEvent.first;
	}

	EventListenerMap::iterator itMap = m_listenerMap.find(type.getId());

	// Finds the list of listeners for the event type and creates one if it doesn't exist.
	if (itMap == m_listenerMap.end())
	{
		EventListenerMapRet itListMap = m_listenerMap.insert(EventListenerMapEntry(type.getId(), EventListenerList()));
		
		if (itListMap.second == false)
			return false;

		if (itListMap.first == m_listenerMap.end())
			return false;

		itMap = itListMap.first;
	}

	EventListenerList & theList= (*itMap).second;

	// Checks to see if the listener is already in the map.
	for (EventListenerList::iterator i = theList.begin(); i != theList.end(); i++)
	{
		if ((*i) == listener)
			return false;
	}

	theList.push_back(listener);

	return true;
}

// Instantly triggers an event. Returns true if event is processed
bool EventManager::triggerEvent(Event const & event)
{
	EventType type = event.getType();

	if (!validateType(type))
		return false;

	EventListenerMap::iterator itMap = m_listenerMap.find(type.getId());

	if (itMap == m_listenerMap.end())
		return false;

	EventListenerList theList = (*itMap).second;

	bool processed = false;

	for (EventListenerList::iterator i = theList.begin(); i != theList.end(); i++)
	{
		if ((*i)->HandleEvent(event))
			processed = true;
	}

	return processed;
}

// Adds event to event queue. Returns true if added.
bool EventManager::queueEvent(EventPtr const & event)
{
	EventType type = event->getType();

	if (!validateType(type))
		return false;

	EventListenerMap::iterator itMap = m_listenerMap.find(type.getId());
	if (itMap == m_listenerMap.end())
		return false;

	m_eventQueue.push_back(event);
	return true;
}

// Goes through the event queue and processes events for the given time.
bool EventManager::tick(unsigned int maxMS)
{
	unsigned int curTick = GetTickCount();
	unsigned int maxTime = maxMS + curTick;

	bool processed = false;

	while (m_eventQueue.size() > 0)
	{
		EventPtr event = m_eventQueue.front();
		m_eventQueue.pop_front();

		EventType type = event->getType();
		EventListenerMap::iterator itMap = m_listenerMap.find(type.getId());

		if (itMap == m_listenerMap.end())
			continue;

		EventListenerList theList = (*itMap).second;

		for (EventListenerList::iterator i = theList.begin(); i != theList.end(); i++)
		{
			if ((*i)->HandleEvent(*event))
				break;
		}

		curTick = GetTickCount();
		if (curTick >= maxTime)
			break;
	}
	return processed;
}

// Checks if type is valid. Does this by finding the hash value of the name
// and then checking if the names match if that hash number is already taken.
bool EventManager::validateType(EventType const & type)
{
	if (type.getName() == NULL)
		return false;

	if (type.getId() == 0)
		return false;

	EventTypeSet::iterator it = m_eventTypes.find(type);

	if (it != m_eventTypes.end())
	{
		EventType known = (*it);
		char * const oldName = known.getName();
		char * const newName = type.getName();

		int check = strcmp(oldName, newName);

		assert(check==0 && "Event types hashed wrong!");

		if (check!=0)
			return false;
	}

	return true;
}