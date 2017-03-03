////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALGLES2_H
#define Tr2SwapChainALGLES2_H

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../Tr2AutoResetObjectAL.h"

class Tr2RenderContextAL;

class Tr2SwapChainAL: 
	public Tr2AutoResetObjectAL, 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL();
	~Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	int GetWidth() const;
	int GetHeight() const;

	bool operator==( const Tr2SwapChainAL& other ) const { return this == &other; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

	Tr2RenderTargetAL m_backBuffer;
private:
	friend class Tr2RenderContextAL;

	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	ALResult CreateFramebuffer( Tr2RenderContextAL& renderContext );

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	Tr2WindowHandle m_hWnd;
#ifdef _WIN32
	HDC m_hDC;
#endif
	uint32_t m_width;
	uint32_t m_height;
};

#endif // TRINITY_PLATFORM==TRINITY_OPENGLES2

#endif // Tr2SwapChainALGLES2_H