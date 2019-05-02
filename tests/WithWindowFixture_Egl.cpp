#include "StdAfx.h"
#if defined(TRINITY_AL_MOBILE)

#include "WithWindowFixture.h"

namespace
{
    Tr2WindowHandle s_hwnd = 0;
    uint32_t framesPassed = 0;
}

extern ANativeWindow* g_androidWindow;

void WithWindow::SetUpTestCase()
{
    s_hwnd = reinterpret_cast<Tr2WindowHandle>( g_androidWindow );
}

void WithWindow::TearDownTestCase()
{
    s_hwnd = 0;
}

void WithWindow::BeginLoopProcessing()
{
    framesPassed = 0;
}

bool WithWindow::DoLoopProcessing()
{
    return framesPassed++ < 200;
}

Tr2WindowHandle WithWindow::GetWindowHandle()
{
	return s_hwnd;
}

RenderWindow* WithWindow::GetWindow()
{
	return nullptr;
}


#endif
