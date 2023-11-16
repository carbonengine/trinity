#include "StdAfx.h"
#if defined(__APPLE__) && TRINITY_PLATFORM != TRINITY_STUB

#include "WithWindowFixture.h"
#include "RenderWindow.h"
#import <Cocoa/Cocoa.h>


extern bool s_keyPressed;

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
    s_keyPressed = false;
}

bool WithWindow::DoLoopProcessing()
{
    while( true )
    {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event == nil)
        {
            break;
        }
        [NSApp sendEvent:event];
    }
    return !s_keyPressed;
}

Tr2WindowHandle WithWindow::GetWindowHandle()
{
    return s_wnd ? s_wnd->GetHandle() : Tr2WindowHandle( 0 );
}

RenderWindow* WithWindow::GetWindow()
{
    return s_wnd;
}

#endif
