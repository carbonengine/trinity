//
//  Tr2RtPipelineStateALMetal.cpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "../ALLog.h"
/*
namespace
{
    class DataStoreCache
    {
    public:
        DataStoreCache()
            :m_dataOffset( PAGE_SIZE )
        {
        }
        void* AddData( size_t size )
        {
            if( m_dataOffset + size > PAGE_SIZE )
            {
                std::unique_ptr<uint8_t[]> page( new uint8_t[std::max( size, PAGE_SIZE )] );
                m_dataPages.emplace_back( std::move( page ) );
                m_dataOffset = size;
                return m_dataPages.back().get();
            }
            auto result = m_dataPages.back().get() + m_dataOffset;
            m_dataOffset += size;
            return result;
        }

        template <typename T>
        T* AddData( size_t count = 1)
        {
            return static_cast<T*>(AddData( sizeof( T ) * count ));
        }
    private:
        std::vector<std::unique_ptr<uint8_t[]>> m_dataPages;
        static const size_t PAGE_SIZE = 1024;
        size_t m_dataOffset;
    };
}
*/

namespace TrinityALImpl
{
    Tr2RtPipelineStateAL::Tr2RtPipelineStateAL()
    {
    }

    Tr2RtPipelineStateAL::~Tr2RtPipelineStateAL()
    {
    }

    ALResult Tr2RtPipelineStateAL::CreateRtPipelineState( const Tr2RtPipelineStateDescriptionAL& desc, Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
    }

    bool Tr2RtPipelineStateAL::IsValid() const
    {
        return true;
    }

    Tr2ALMemoryType Tr2RtPipelineStateAL::GetMemoryClass() const
    {
        return AL_MEMORY_MANAGED;
    }

    void Tr2RtPipelineStateAL::Destroy()
    {
        
    }
    
    void Tr2RtPipelineStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
    {
        description["type"] = "Tr2RtPipelineStateAL";
    }
/*
    const TrinityALImpl::Tr2RootSignatureAL& Tr2RtPipelineStateAL::GetGlobalRootSignature() const
    {
        return m_globalSignature;
    }

    ALResult Tr2RtPipelineStateAL::CreateRootSignature( TrinityALImpl::Tr2RootSignatureAL& rootSignature, const Tr2ShaderSignatureAL& signature, Tr2PrimaryRenderContextAL& renderContext )
    {
        return S_OK;
    }*/
}

#endif
