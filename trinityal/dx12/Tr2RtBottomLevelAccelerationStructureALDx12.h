////////////////////////////////////////////////////////////
//
//    Created:   November 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2RtBottomLevelAccelerationStructureAL.h"

namespace TrinityALImpl
{
	class Tr2RtBottomLevelAccelerationStructureAL : public Tr2DeviceResourceAL<Tr2RtBottomLevelAccelerationStructureAL>
	{
	public:
		Tr2RtBottomLevelAccelerationStructureAL();
		~Tr2RtBottomLevelAccelerationStructureAL();

		ALResult Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
		ALResult Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext );
		ALResult CompactBlas( Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;
		
		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

		ID3D12Resource* GetBuffer() const;
	
	private:
		CComPtr<ID3D12Resource> m_buffer;
		CComPtr<ID3D12Resource> m_scratch;
		D3D12_RAYTRACING_GEOMETRY_DESC m_geometryDesc;
		Tr2RtBuildFlags::Type m_buildFlags;
		Tr2PrimaryRenderContextAL* m_owner;
	};
}

#endif
