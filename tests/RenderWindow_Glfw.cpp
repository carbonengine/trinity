#include "StdAfx.h"
#if !defined(_WIN32) && !defined(TRINITY_AL_MOBILE) && (TRINITY_PLATFORM == TRINITY_OPENGLES2)
#include "RenderWindow.h"
#include "WithWindowFixture.h"
#include "GLFW/glfw3.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
    static bool glfwInitialized = false;
    if( !glfwInitialized )
    {
        glfwInitialized = true;
        glfwInit();
    }
#if TRINITY_PLATFORM != TRINITY_OPENGLES2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    m_handle = (Tr2WindowHandle) glfwCreateWindow( width, height, g_moduleName, nullptr, reinterpret_cast<GLFWwindow*>( WithWindow::GetWindowHandle() ) );
}

RenderWindow::~RenderWindow()
{
    glfwDestroyWindow( reinterpret_cast<GLFWwindow*>( m_handle ) );
}

uint32_t RenderWindow::GetClientWidth() const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( m_handle ), &width, &height );
	return uint32_t( width );
}

uint32_t RenderWindow::GetClientHeight() const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( m_handle ), &width, &height );
	return uint32_t( height );
}

bool RenderWindow::Resize( uint32_t width, uint32_t height )
{
	return false;
}

#endif
