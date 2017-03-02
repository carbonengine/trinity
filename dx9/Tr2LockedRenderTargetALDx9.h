#pragma once
#ifndef Tr2LockedRenderTargetALDx9_H
#define Tr2LockedRenderTargetALDx9_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

class Tr2LockedRenderTargetAL
{
public:
	Tr2LockedRenderTargetAL();
	~Tr2LockedRenderTargetAL();

	void Destroy();
	bool IsValid() const;

	ALResult Lock( void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );
private:
	Tr2LockedRenderTargetAL( const Tr2LockedRenderTargetAL& ) /* = delete */;
	Tr2LockedRenderTargetAL& operator=( const Tr2LockedRenderTargetAL& ) /* = delete */;

	friend class Tr2RenderTargetAL;

	CComPtr<IDirect3DSurface9> m_sysMemLocked;
	bool m_hasLockedRect;
	uint32_t m_lockedRect[4];
};

#endif

#endif