#pragma once
#ifndef RenderWindow_H
#define RenderWindow_H

class RenderWindow
{
public:
	RenderWindow( uint32_t width = 128, uint32_t height = 64 );
	~RenderWindow();

	Tr2WindowHandle GetHandle() const
	{
		return m_handle;
	}

	operator Tr2WindowHandle () const
	{
		return GetHandle();
	}

	uint32_t GetClientWidth() const;
	uint32_t GetClientHeight() const;

	bool Resize( uint32_t width, uint32_t height );
private:
	Tr2WindowHandle m_handle;
};

#endif