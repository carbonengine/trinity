#include "StdAfx.h"
#if !defined(__ORBIS__) && !defined(_WIN32) && !defined(TRINITY_AL_MOBILE) && (TRINITY_PLATFORM == TRINITY_STUB)
#include "RenderWindow.h"
#include "WithWindowFixture.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
    m_handle = ( width & 0xffff ) | ( ( height & 0xffff ) << 16 );
}

RenderWindow::~RenderWindow()
{
}

uint32_t RenderWindow::GetClientWidth() const
{
    return m_handle & 0xffff;
}

uint32_t RenderWindow::GetClientHeight() const
{
    return ( m_handle >> 16 ) & 0xffff;
}

bool RenderWindow::Resize( uint32_t width, uint32_t height )
{
	return false;
}

#endif