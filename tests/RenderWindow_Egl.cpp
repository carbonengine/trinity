#include "StdAfx.h"
#if defined(TRINITY_AL_MOBILE)
#include "RenderWindow.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
	:m_handle( 0 )
{
}

RenderWindow::~RenderWindow()
{
}

uint32_t RenderWindow::GetClientWidth() const
{
	return 0;
}

uint32_t RenderWindow::GetClientHeight() const
{
	return 0;
}

bool RenderWindow::Resize( uint32_t width, uint32_t height )
{
	return false;
}
#endif