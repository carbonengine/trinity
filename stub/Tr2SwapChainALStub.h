#pragma once
#ifndef Tr2SwapChainALStub_H
#define Tr2SwapChainALStub_H

#if TRINITY_PLATFORM==TRINITY_STUB

#include "../Tr2AutoResetObjectAL.h"

class Tr2RenderContextAL;


class Tr2SwapChainAL: 
	public Tr2AutoResetObjectAL, 
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

	Tr2RenderTargetAL m_backBuffer;
private:
	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	Tr2WindowHandle m_windowHandle;
};

#endif // TRINITY_PLATFORM==TRINITY_STUB

#endif // Tr2SwapChainALStub_H