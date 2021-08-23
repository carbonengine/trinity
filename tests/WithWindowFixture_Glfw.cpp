#include "StdAfx.h"
#if !defined(_WIN32) && !defined(TRINITY_AL_MOBILE) && (TRINITY_PLATFORM == TRINITY_OPENGLES2)

#include "WithWindowFixture.h"
#include "RenderWindow.h"
#include "GLFW/glfw3.h"


namespace
{
	RenderWindow* s_wnd = nullptr;
    bool s_keyPressed = false;
    void OnKeyDown( GLFWwindow*, int key, int, int action )
    {
        if( GLFW_PRESS == action )
        {
            s_keyPressed = true;
        }
    }
}

void WithWindow::SetUpTestCase()
{
	CCP_DELETE s_wnd;
	s_wnd = CCP_NEW( "WithWindowFixture/s_wnd" ) RenderWindow( 640, 480 );

	glfwSetKeyCallback( reinterpret_cast<GLFWwindow*>( s_wnd->GetHandle() ), &OnKeyDown );
    glfwMakeContextCurrent( reinterpret_cast<GLFWwindow*>( s_wnd->GetHandle() ) );
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
    if( glfwWindowShouldClose( reinterpret_cast<GLFWwindow*>( s_wnd->GetHandle() ) ) )
    {
        return false;
    }
    glfwPollEvents();
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
