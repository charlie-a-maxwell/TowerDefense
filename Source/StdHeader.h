#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <crtdbg.h>
#include <assert.h>
#include <string>
#include <list>
#include <queue>
#include <map>
#include <tchar.h>

#if defined (_MSC_VER) && (_MSC_VER < 1300)
	#include "types.h"
#else
	#ifdef _DEBUG
		#undef _DEBUG
		#include <atltypes.h>			
		#define _DEBUG
	#else
		#include <atltypes.h>
	#endif
#endif

// used for memory leak detection
#if defined(_DEBUG)
#	define SAFE_NEW new(_NORMAL_BLOCK,__FILE__, __LINE__)
#else
#	define SAFE_NEW new
#endif

#include <dxstdafx.h>
#include <d3dx9tex.h>

#include <boost\config.hpp>
#include <boost\shared_ptr.hpp>

using boost::shared_ptr;

struct AppMsg
{
	HWND m_hWnd;
	UINT m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;
};

typedef D3DXCOLOR Color;

#pragma warning( disable : 4244 ) 
#pragma warning( disable : 4996 )

#include "geometry.h"
#include "Interfaces.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MEGABYTE (1024 * 1024)
#define SIXTY_HERTZ (16.66f)

#define fOPAQUE 1.0

#if !defined(SAFE_DELETE)
	#define SAFE_DELETE(x) if(x) delete x; x=NULL;
#endif

#if !defined(SAFE_DELETE_ARRAY)
	#define SAFE_DELETE_ARRAY(x) if (x) delete [] x; x=NULL; 
#endif

#if !defined(SAFE_RELEASE)
	#define SAFE_RELEASE(x) if(x) x->Release(); x=NULL;
#endif

#ifdef UNICODE
	#define _tcssprintf wsprintf
	#define tcsplitpath _wsplitpath
#else
	#define _tcssprintf sprintf
	#define tcsplitpath _splitpath
#endif


extern Vec3 g_Up;
extern Vec3 g_Right;
extern Vec3 g_Forward;

extern Color g_White;
extern Color g_Red;
extern Color g_Blue;
extern Color g_Black;

extern Vec4 g_Up4;
extern Vec4 g_Right4;
extern Vec4 g_Forward4;