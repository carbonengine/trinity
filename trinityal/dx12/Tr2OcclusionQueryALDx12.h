// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2OcclusionQueryAL.h"

namespace TrinityALImpl
{
class Tr2OcclusionQueryAL : public Tr2DeviceResourceAL<Tr2OcclusionQueryAL>
{
public:
	Tr2OcclusionQueryAL();
	~Tr2OcclusionQueryAL();

	ALResult Create( Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Begin( Tr2RenderContextAL& renderContext );
	ALResult End( Tr2RenderContextAL& renderContext );
	ALResult GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode );

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	Tr2OcclusionQueryAL( const Tr2OcclusionQueryAL& ) /* = delete */;
	Tr2OcclusionQueryAL& operator=( const Tr2OcclusionQueryAL& ) /* = delete */;

	CComPtr<ID3D12QueryHeap> m_query;
	CComPtr<ID3D12Resource> m_result;
	uint64_t m_frameIndex;
	Tr2PrimaryRenderContextAL* m_owner;
	std::string m_name;
};
}

#endif
