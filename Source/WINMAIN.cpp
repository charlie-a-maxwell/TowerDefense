#include "StdHeader.h"
#include "EngineFiles\Game.h"

GameApp g_Game;

INT WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{

	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	// set this flag to keep memory blocks around
	//tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;				// this flag will cause intermittent pauses in your game!

	// perform memory check for each alloc/dealloc
	//tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;				// remember this is VERY VERY SLOW!

	// always perform a leak check just before app exits.
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;					

	_CrtSetDbgFlag(tmpDbgFlag);


	DXUTSetCallbackDeviceCreated( GameApp::OnCreateDevice );
	DXUTSetCallbackDeviceDestroyed( GameApp::OnDestroyDevice );
	DXUTSetCallbackMsgProc( GameApp::MsgProc );
    DXUTSetCallbackDeviceReset( GameApp::OnResetDevice );
    DXUTSetCallbackDeviceLost( GameApp::OnLostDevice );
	DXUTSetCallbackFrameRender( GameApp::OnRender );
	DXUTSetCallbackFrameMove( GameApp::OnUpdateGame );


	DXUTSetCursorSettings( true, true);

	if (!g_App->InitInstance(hInstance, lpCmdLine) )
		return FALSE;

	DXUTMainLoop();

	DXUTSimpleShutdown();
	SAFE_DELETE(g_App->m_pGame);

	return g_App->GetExitCode();
}