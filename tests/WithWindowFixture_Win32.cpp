#include "StdAfx.h"
#include "WithWindowFixture.h"
#include "RenderWindow.h"

#ifdef _WIN32

namespace
{
RenderWindow* s_wnd = nullptr;

}

void WithWindow::SetUpTestCase() 
{
	CCP_DELETE s_wnd;
	s_wnd = CCP_NEW( "WithWindowFixture/s_wnd" ) RenderWindow( 640, 480 );
}

void WithWindow::TearDownTestCase() 
{
	CCP_DELETE s_wnd;
	s_wnd = nullptr;
}

void WithWindow::BeginLoopProcessing()
{
}

bool WithWindow::DoLoopProcessing()
{
	bool running = true;
	MSG msg;
	while( PeekMessage( &msg, 0, 0, 0, 1 ) )
	{
		if( msg.message == WM_QUIT || msg.message == WM_CHAR )
		{
			running = false;
			break;
		}
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	return running;
}

Tr2WindowHandle WithWindow::GetWindowHandle()
{
	return *s_wnd;
}

RenderWindow* WithWindow::GetWindow()
{
	return s_wnd;
}

#endif
