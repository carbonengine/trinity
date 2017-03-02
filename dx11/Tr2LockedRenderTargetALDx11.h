#pragma once
#ifndef Tr2LockedRenderTargetALDx11_H
#define Tr2LockedRenderTargetALDx11_H

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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

	CComPtr<ID3D11Texture2D> m_staging;
};

#endif

#endif