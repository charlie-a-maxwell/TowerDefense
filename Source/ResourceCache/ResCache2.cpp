//========================================================================
// ResCache.cpp : Defines a simple resource cache
//
// Part of the GameCode2 Application
//
// GameCode2 is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 2nd Edition" by Mike McShaffry, published by
// Paraglyph Press. ISBN: 1-932111-91-3
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the author a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: http://www.amazon.com/exec/obidos/ASIN/1932111913/gamecodecompl-20/
//
// There's also a companion web site at http://www.mcshaffry.com/GameCode/portal.php
//
// (c) Copyright 2005 Michael L. McShaffry
//
// This work is licensed under the Creative Commons Attribution-ShareAlike License. 
// To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/1.0/ 
// or send a letter to:
//      Creative Commons
//      559 Nathan Abbott Way
//      Stanford, California 94305, USA.
//
//========================================================================

//========================================================================
//  Content References in Game Coding Complete 2nd Edition
// 
//  class Resource			- Chapter 7, page 213
//  class ResourceZipFile	- Chapter 7, page 214
//  class ResHandle			- Chapter 7, page 216
//  class ResCache			- Chapter 7, page 217
//
//========================================================================

#include "StdHeader.h"
#include <assert.h>
#include <list>
#include <map>

#include "ResCache2.h"
#include "ZipFile.h"

#pragma comment(lib, "zlib.lib")

ResourceZipFile::~ResourceZipFile() 
{ 
	SAFE_DELETE(m_pZipFile); 
}

bool ResourceZipFile::VOpen()
{
	m_pZipFile = SAFE_NEW CZipFile;
    if (m_pZipFile)
    {
		return m_pZipFile->Init(m_resFileName.c_str());
	}
	return false;	
}

int ResourceZipFile::VGetResourceSize(const Resource &r)
{
	int size = 0;
	int resourceNum = m_pZipFile->Find(r.m_name.c_str());
	if (resourceNum>=0)
	{
		size = m_pZipFile->GetFileLen(resourceNum);
	}
	return size;	
}

int ResourceZipFile::VGetResource(const Resource &r, char *buffer)
{
	int size = 0;
	int resourceNum = m_pZipFile->Find(r.m_name.c_str());
	if (resourceNum>=0)
	{
		size = m_pZipFile->GetFileLen(resourceNum);
		m_pZipFile->ReadFile(resourceNum, buffer);
	}
	return 0;	
}



ResHandle::ResHandle(const Resource & resource, const char *buffer)
: m_resource(resource)
{
	m_buffer = buffer;
}

ResHandle::~ResHandle()
{
	if (m_buffer) delete [] m_buffer;
}

ResCache::ResCache(const unsigned int sizeInMb, IResourceFile *resFile )
{
	m_cacheSize = sizeInMb * 1024 * 1024;				// total memory size
	m_allocated = 0;									// total memory allocated
	m_file = resFile;
}

ResCache::~ResCache()
{
	while (!m_lru.empty())
	{
		FreeOneResource();
	}
	SAFE_DELETE(m_file);
}


int ResCache::Create(Resource &r)
{
	r.m_size = m_file->VGetResourceSize(r);
	return (r.m_size);
}


const void *ResCache::Get(const Resource & r )
{
	ResHandle *handle = Find(r);
	return (handle!=NULL) ? Update(handle) : Load(r);
}


const void * ResCache::Load(const Resource & r)
{
	// Note: Change Post-printing
	//
	// Graphic resources are loaded differently, since the tool that created them (Ipac)
	// added some data to grab any sequence of frames - not just the whole thing.
	// 
	// Added an optional<int> &len in case we wanted to return the
	// length of the resource in bytes.

	int size = m_file->VGetResourceSize(r);
	char *buffer = Allocate(size);
	if (buffer==NULL)
	{
		return NULL;		// ResCache is out of memory!
	}

	memset(buffer,0,sizeof(buffer));
	// Create a new resource and add it to the lru list and map
	ResHandle *handle = SAFE_NEW ResHandle(r, buffer);
	m_lru.push_front(handle);
	m_resources[r.m_name] = handle;

	m_file->VGetResource(r, buffer);

	return buffer;
}


ResHandle *ResCache::Find(const Resource & r)
{
	ResHandleMap::iterator i = m_resources.find(r.m_name);
	if (i==m_resources.end())
		return NULL;

	return (*i).second;
}

const void *ResCache::Update(ResHandle *handle)
{
	m_lru.remove(handle);
	m_lru.push_front(handle);

	return handle->m_buffer;
}




char *ResCache::Allocate(unsigned int size)
{
	if (!MakeRoom(size))
		return NULL;

	char *mem = SAFE_NEW char[size];
	if (mem)
	{
		m_allocated += size;
	}

	return mem;
}


void ResCache::FreeOneResource()
{
	ResHandleList::iterator gonner = m_lru.end();
	gonner--;

	ResHandle *handle = *gonner;

	m_lru.pop_back();							
	m_resources.erase(handle->m_resource.m_name);
	m_allocated -= handle->m_resource.m_size ;
	delete handle;
}




void ResCache::Flush()
{
	while (!m_lru.empty())
	{
		ResHandle *handle = *(m_lru.begin());
		Free(handle);
		m_lru.pop_front();
	}
}



bool ResCache::MakeRoom(unsigned int size)
{
	if (size > m_cacheSize)
	{
		return false;
	}

	// return null if there's no possible way to allocate the memory
	while (size > (m_cacheSize - m_allocated))
	{
		// The cache is empty, and there's still not enough room.
		if (m_lru.empty())
			return false;

		FreeOneResource();
	}

	return true;
}



void ResCache::Free(ResHandle *gonner)
{
	m_lru.remove(gonner);
	m_resources.erase(gonner->m_resource.m_name);
	m_allocated -= gonner->m_resource.m_size;
	delete gonner;
}


