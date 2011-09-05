#pragma once
//========================================================================
// ResCache.h : Defines a simple resource cache.
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

// Note: this was renamed from struct Resource in the book.
class Resource
{
public:
	std::string m_name;
	unsigned int m_size;

	Resource(std::string name) { m_name=name; m_size=0; }
};


class CZipFile;

class ResourceZipFile : public IResourceFile
{
	CZipFile *m_pZipFile;
	std::wstring m_resFileName;

public:
	ResourceZipFile(const _TCHAR *resFileName) { m_pZipFile = NULL; m_resFileName=resFileName; }
	virtual ~ResourceZipFile();

	virtual bool VOpen();
	virtual int VGetResourceSize(const Resource &r);
	virtual int VGetResource(const Resource &r, char *buffer);
};


class ResHandle
{
	friend class ResCache;

protected:
	Resource m_resource;
	const char *m_buffer;						

public:
	ResHandle(const Resource & resource, const char *buffer);
	virtual ~ResHandle();
};


typedef std::list<ResHandle *> ResHandleList;			// lru list
typedef std::map<std::string, ResHandle *> ResHandleMap;		// maps indentifiers to resource data

class ResCache
{
	ResHandleList m_lru;								// lru list
	ResHandleMap m_resources;
	IResourceFile *m_file;

	unsigned int			m_cacheSize;			// total memory size
	unsigned int			m_allocated;			// total memory allocated

protected:

	bool MakeRoom(unsigned int size);
	char *Allocate(unsigned int size);
	void Free(ResHandle *gonner);

	const void *Load(const Resource & r);
	ResHandle *Find(const Resource & r);
	const void *Update(ResHandle *handle);

	void FreeOneResource();

public:
	ResCache(const unsigned int sizeInMb, IResourceFile *file);
	virtual ~ResCache();

	bool Init() { return m_file->VOpen(); }
	int Create(Resource & r);
	const void *Get(const Resource & r);

	void Flush(void);

};



