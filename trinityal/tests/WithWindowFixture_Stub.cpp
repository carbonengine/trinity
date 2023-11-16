#include "StdAfx.h"
#if !defined(_WIN32) && !defined(TRINITY_AL_MOBILE) && (TRINITY_PLATFORM == TRINITY_STUB)

#include "WithWindowFixture.h"
#include "RenderWindow.h"


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
    return true;
}

Tr2WindowHandle WithWindow::GetWindowHandle()
{
	return Tr2WindowHandle( 0 );
}

RenderWindow* WithWindow::GetWindow()
{
	return s_wnd;
}



#endif
