#pragma once
#ifndef Tr2SwapChainALStub_H
#define Tr2SwapChainALStub_H

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2TextureAL.h"


class Tr2RenderContextAL;


#if TRINITY_PLATFORM==TRINITY_STUB

class Tr2SwapChainAL: 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	int GetWidth() const;
	int GetHeight() const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

	Tr2TextureAL m_backBuffer;
private:
	Tr2WindowHandle m_windowHandle;
};

#endif // TRINITY_PLATFORM==TRINITY_STUB

#endif // Tr2SwapChainALStub_H