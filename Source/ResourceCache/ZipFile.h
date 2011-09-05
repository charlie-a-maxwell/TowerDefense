#pragma once
//========================================================================
// ZipFile.h : API to use Zip files
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

// --------------------------------------------------------------------------
// File:        ZipFile.h
//
// Purpose:     The declaration of a quick'n dirty ZIP file reader class.
//              Original code from Javier Arevalo.
//              Get zlib from http://www.cdrom.com/pub/infozip/zlib/
//
// class CZipFile - Chapter 7, page 202
// --------------------------------------------------------------------------


#include <stdio.h>

typedef std::map<std::string, int> ZipContentsMap;		// maps path to a zip content id

class CZipFile
{
  public:
    CZipFile() { m_nEntries=0; m_pFile=NULL; m_pDirData=NULL; }
    virtual ~CZipFile() { End(); fclose(m_pFile); }

    bool Init(const _TCHAR *resFileName);
    void End();

    int GetNumFiles()const { return m_nEntries; }
    void GetFilename(int i, char *pszDest) const;
    int GetFileLen(int i) const;
    bool ReadFile(int i, char *pBuf);
	int Find(const char *path) const;

	ZipContentsMap m_ZipContentsMap;

  private:
    struct TZipDirHeader;
    struct TZipDirFileHeader;
    struct TZipLocalHeader;

    FILE *m_pFile;		// Zip file
    char *m_pDirData;	// Raw data buffer.
    int  m_nEntries;	// Number of entries.

    // Pointers to the dir entries in pDirData.
    const TZipDirFileHeader **m_papDir;   
};


