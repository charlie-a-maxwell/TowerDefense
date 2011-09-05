// The idea for this and most of the code is from Game Coding Complete.
// It is a very basic cooperative multitasker.


#pragma once

#include "StdHeader.h"
#include <boost\config.hpp>
#include <boost\shared_ptr.hpp>
#include <list>
#include "Event.h"

static const int PROCESS_FLAG_ATTACHED		= 0x00000001;

class Process
{
	friend class ProcessManager;
private:
	int m_flags;

protected:
	bool m_bKill;
	bool m_bPaused;
	bool m_bActive;
	bool m_bInitialUpdate;
	int m_type;
	ActorId m_id;
	
	shared_ptr<Process> m_nextProcess;
public:	
	
	virtual void Kill() {m_bKill = true;}
	virtual bool IsDead() {return m_bKill;}
	virtual bool IsPause() {return m_bPaused;}
	virtual void TogglePause() {m_bPaused = !m_bPaused;}
	virtual bool IsActive() {return m_bActive;}
	virtual void SetActive(bool active) {m_bActive = active;}
	virtual shared_ptr<Process> const GetNext() {return (m_nextProcess);}
	virtual void SetNext(shared_ptr<Process> next);
	virtual void SetAttached(bool attached);
	virtual bool IsAttached() {return (m_flags & PROCESS_FLAG_ATTACHED) ? true : false;}
	virtual int GetType() {return m_type;}
	virtual ActorId GetId() {return m_id;}
	virtual void SetActorId(ActorId id) {m_id = id;}

	Process(int type, ActorId id = -1);
	Process(const Process& in);
	virtual ~Process();

	virtual void OnUpdate(int deltaMS);
	virtual void OnInitialize() {}
};


typedef std::list<shared_ptr<Process> > ProcessList;

class ProcessManager
{
private:
	EventListenerPtr m_eventListener;
	void Detach(shared_ptr<Process> process);
protected:
	ProcessList m_processList;
public:
	ProcessManager();
	void UpdateProcesses(int deltaMS);
	void DeleteProcessList();
	bool IsProcessActive(int type);
	void Attach(shared_ptr<Process> process);
	bool HasProcesses();
	void RemoveActor(ActorId id);
};

class ProcessManagerListener: public IEventListener
{
	ProcessManager * m_manager;
public:
	ProcessManagerListener(ProcessManager * manager):m_manager(manager){};
	virtual bool HandleEvent(Event const & e);
};
