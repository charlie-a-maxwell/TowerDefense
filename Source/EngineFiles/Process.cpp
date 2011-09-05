#include "Process.h"


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Process////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Adds the next process to start after this one ends
void Process::SetNext(shared_ptr<Process> next)
{
	m_nextProcess = next;
}

// Sets if the process is attached to a process manager
void Process::SetAttached(bool attached)
{
	if (attached)
	{
		m_flags |= PROCESS_FLAG_ATTACHED;
	}
	else
	{
		m_flags &= ~PROCESS_FLAG_ATTACHED;
	}
}

Process::Process(int type, ActorId id):m_bKill(false),
	m_bPaused(false),
	m_bActive(true),
	m_bInitialUpdate(true),	
	m_type(type),
	m_flags(0),
	m_id(id)
{
}

Process::~Process()
{
}

void Process::OnUpdate(int deltaMS)
{
	if (m_bInitialUpdate)
	{
		OnInitialize();
		m_bInitialUpdate = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////ProcessManager/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void ListenForProcessEvents(EventListenerPtr listener)
{
	safeAddListener( listener, EventType(Evt_Remove_Actor::gkName) );
}


ProcessManager::ProcessManager()
{
	EventListenerPtr listener (SAFE_NEW ProcessManagerListener( this) );
	ListenForProcessEvents(listener);
	m_eventListener = listener;
}

// Itereates through the processes updating all of them and removing dead ones.
void ProcessManager::UpdateProcesses(int deltaMS)
{
	ProcessList::iterator i = m_processList.begin();

	shared_ptr<Process> nextProcess;

	while (i != m_processList.end())
	{
		shared_ptr<Process> p = (*i);
		i++;
		if (p->IsDead())
		{
			nextProcess = p->GetNext();
			if (nextProcess)
			{
				nextProcess->SetAttached(true);
				Attach(nextProcess);
			}
			Detach(p);
		}
		else if (p->IsActive() && !p->IsPause())
		{
			p->OnUpdate(deltaMS);
		}
	}
}

// Clears the process list
void ProcessManager::DeleteProcessList()
{
	for (ProcessList::iterator i = m_processList.begin(); i != m_processList.end();)
	{
		Detach(* (i++));
	}
}

// Checks if any processes of the given type are attached.
bool ProcessManager::IsProcessActive(int type)
{
	for (ProcessList::iterator i = m_processList.begin(); i != m_processList.end(); i++)
	{
		if ( (*i)->GetType() == type && ( (*i)->IsDead() == false || (*i)->GetNext()))
			return true;
	}
	return false;
}

// Attaches a process to the process list
void ProcessManager::Attach(shared_ptr<Process> process)
{
	m_processList.push_back(process);
	process->SetAttached(true);
}

// Removes a process from the process list
void ProcessManager::Detach(shared_ptr<Process> process)
{
	m_processList.remove(process);
	process->SetAttached(false);
}

// Checks if there are processes waiting to be updated
bool ProcessManager::HasProcesses()
{
	return !m_processList.empty();
}

// Removes all processes associated with the actor
void ProcessManager::RemoveActor(ActorId id)
{
	for (ProcessList::iterator i = m_processList.begin(); i != m_processList.end(); i++)
	{
		if ( (*i)->GetId() == id)
			Detach(* (i++));
	}
}

// Handles events for the process manager
bool ProcessManagerListener::HandleEvent(Event const & e)
{
	if (strcmp(e.getType().getName(), Evt_Remove_Actor::gkName)==0)
	{
		EvtData_Remove_Actor *data = e.getData<EvtData_Remove_Actor>();
		m_manager->RemoveActor(data->m_id);
	}

	return false;
}