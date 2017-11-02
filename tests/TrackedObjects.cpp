#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"
#include "RenderWindow.h"

#if TRACK_AL_RESOURCES

using namespace Tr2RenderContextEnum;

namespace
{
class LiveResourceCount
{
public:
    LiveResourceCount()
    :   m_count( 0 )
    {
    }
        
    void operator ()( ObjectType, const char*, const void*, const std::map<std::string, uint32_t>& )
    {
        ++m_count;
    }
        
    uint32_t GetCount() const
    {
        return m_count;
    }
private:
    uint32_t m_count;
};
    
uint32_t GetTotalLiveCount()
{
    LiveResourceCount count;
    Tr2TrackedALObjectBase::GetAllObjectDescriptions( AL_MEMORY_MANAGED | AL_MEMORY_VIDEO, count );
    return count.GetCount();
}

ALResult CreateSampleObject( Tr2ConstantBufferAL& cb, Tr2PrimaryRenderContextAL& renderContext )
{
	return cb.Create( 128, renderContext );
}

ALResult CreateSampleObject( Tr2DepthStencilAL& ds, Tr2PrimaryRenderContextAL& renderContext )
{
	return ds.Create( 128, 64, DSFMT_D24S8, Tr2MsaaDesc(), EX_NONE, renderContext );
}

ALResult CreateSampleObject( Tr2IndexBufferAL& ib, Tr2PrimaryRenderContextAL& renderContext )
{
	return ib.Create( 128, 0, IB_16BIT, nullptr, renderContext );
}

ALResult CreateSampleObject( Tr2RenderTargetAL& rt, Tr2PrimaryRenderContextAL& renderContext )
{
	return rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, EX_NONE, renderContext );
}

ALResult CreateSampleObject( Tr2SamplerStateAL& ss, Tr2PrimaryRenderContextAL& renderContext )
{
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc(
								TF_ANISOTROPIC,
								TF_LINEAR,
								TF_POINT,
								false,
								TA_WRAP,
								TA_CLAMP,
								TA_BORDER,
								0.5f,
								4,
								CMP_ALWAYS,
								borderColor,
								0.1f,
								3.2f );
	return ss.Create( renderContext, desc );
}

ALResult CreateSampleObject( Tr2TextureAL& texture, Tr2PrimaryRenderContextAL& renderContext )
{
	return texture.Create2D( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, 0, nullptr, renderContext );
}

ALResult CreateSampleObject( Tr2VertexBufferAL& vb, Tr2PrimaryRenderContextAL& renderContext )
{
	return vb.Create( 128, renderContext );
}

ALResult CreateSampleObject( Tr2VertexLayoutAL& layout, Tr2PrimaryRenderContextAL& renderContext )
{
	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	return layout.Create( def, renderContext );
}
    
ALResult CreateSampleObject( Tr2OcclusionQueryAL& query, Tr2PrimaryRenderContextAL& renderContext )
{
	return query.Create( renderContext );
}
    
ALResult CreateSampleObject( Tr2SwapChainAL& sc, Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.GetCaps().SupportsStandaloneSwapChain() )
	{
		return E_FAIL;
	}

	RenderWindow window;
	return sc.Create( window, renderContext );
}
    
ALResult CreateSampleObject( Tr2GpuBufferAL& buffer, Tr2PrimaryRenderContextAL& renderContext )
{
	return buffer.Create( 128, PIXEL_FORMAT_R32_FLOAT, 0, nullptr, renderContext );
}
    
ALResult CreateSampleObject( Tr2FenceAL& fence, Tr2PrimaryRenderContextAL& renderContext )
{
	return fence.Create( renderContext );
}

template <typename T>
bool IsObjectTypeAvailable( Tr2PrimaryRenderContextAL& renderContext )
{
	return true;
}

template <>
bool IsObjectTypeAvailable<Tr2SwapChainAL>( Tr2PrimaryRenderContextAL& renderContext )
{
	return renderContext.GetCaps().SupportsStandaloneSwapChain();
}

template <>
bool IsObjectTypeAvailable<Tr2GpuBufferAL>( Tr2PrimaryRenderContextAL& renderContext )
{
	return renderContext.GetCaps().SupportsGpuBuffer();
}


std::vector<std::string> s_logMessages;

void CustomLogEcho( CcpLogChannel_t& channel, CCP::LogType type, unsigned long userData, const char* message )
{
	s_logMessages.push_back( message );
}

bool LogContainsMessage( const char* message )
{
	for( auto it = std::begin( s_logMessages ); it != std::end( s_logMessages ); ++it )
	{
		if( it->find( message ) != std::string::npos )
		{
			return true;
		}
	}
	return false;
}

bool LogContainsLiveResourceAddress( const void* address )
{
	char buffer[64];
	sprintf_s( buffer, "0x%X", address );
	return LogContainsMessage( buffer );
}


template <typename T>
class TrackedObjectTest : public WithValidRenderContext
{
};

}

typedef ::testing::Types<
	Tr2ConstantBufferAL, 
	Tr2DepthStencilAL,
	Tr2IndexBufferAL,
	Tr2RenderTargetAL,
	Tr2SamplerStateAL,
	Tr2TextureAL,
	Tr2VertexBufferAL,
	Tr2VertexLayoutAL,
	Tr2OcclusionQueryAL,
	Tr2SwapChainAL,
	Tr2GpuBufferAL,
	Tr2FenceAL
	> TrackedObjectTypes;
TYPED_TEST_CASE( TrackedObjectTest, TrackedObjectTypes );

TYPED_TEST( TrackedObjectTest, IvalidObjectDoesNotCountAsLive ) 
{
    uint32_t initialCount = GetTotalLiveCount();
    TypeParam obj;
    uint32_t newCount = GetTotalLiveCount();

	EXPECT_EQ( initialCount, newCount );
}

TYPED_TEST( TrackedObjectTest, ValidObjectCountsAsLive ) 
{
	if( !IsObjectTypeAvailable<TypeParam>( *this->renderContext ) )
	{
		return;
	}
	uint32_t initialCount = GetTotalLiveCount();
	TypeParam object;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleObject( object, *this->renderContext ) );
	uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}


#endif