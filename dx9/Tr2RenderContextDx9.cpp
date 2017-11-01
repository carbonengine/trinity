#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2RenderContextDx9.h"
#include "ITr2RenderContextEvents.h"
#include "ALLog.h"
#include "Tr2HalHelperStructures.h"
#include "Tr2VertexBufferALDx9.h"
#include "Tr2IndexBufferALDx9.h"
#include "Tr2ConstantBufferALDx9.h"
#include "Tr2SamplerStateALDx9.h"
#include "Tr2DepthStencilALDx9.h"
#include "Tr2RenderTargetALDx9.h"
#include "Tr2VideoAdapterInfoALDx9.h"
#include "Tr2VertexLayoutALDx9.h"
#include "Tr2ShaderALDx9.h"
#include "Tr2AdapterStructures.h"

using namespace Tr2RenderContextEnum;
#pragma warning( disable: 4189 )	// Scopeguard

CCP_STATS_DECLARE( primitiveCount		, "Trinity/AL/primitiveCount"		, true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount			, "Trinity/AL/vertexCount"			, true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount	, "Trinity/AL/sceneDrawcallCount"	, true, CST_COUNTER_LOW,  "Number of DrawPrimitive calls." );


namespace
{

Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
{
	static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
	return primaryRenderContext;
}

}

// --------------------------------------------------------------------------------------
// Description:
//   A simple Blitter to copy texture contents onto a render target.
// --------------------------------------------------------------------------------------
struct Tr2RenderContextAL::Blitter
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Initializes blitter.
	// Arguments:
	//   device - valid D3D device
	// Return Value:
	//   HRESULT of the call.
	// ----------------------------------------------------------------------------------
	HRESULT Create( IDirect3DDevice9* device )
	{
		Destroy();

		float coords[] = {
			-1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f,
			-1.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 1.0f, 0.0f, 1.0f };
		CR_RETURN_HR( device->CreateVertexBuffer( sizeof( float ) * 16, 0, 0, D3DPOOL_MANAGED, &m_vb, nullptr ) );
		void* data;
		CR_RETURN_HR( m_vb->Lock( 0, 0, &data, 0 ) );
		memcpy( data, coords, sizeof( coords ) );
		m_vb->Unlock();

		D3DVERTEXELEMENT9 elements[] = {
			{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			D3DDECL_END()
		};
		CR_RETURN_HR( device->CreateVertexDeclaration( elements, &m_decl ) );

		// Compiled byte codes for vertex shader:
		// float4 vs( float4 pos: POSITION, out float2 texCoord: TEXCOORD0 ): POSITION
		// {
		//     texCoord = pos.xy * float2( 0.5, -0.5 ) + 0.5;
		//     return pos;
		// }
		const BYTE vsCompiled[] = {
			  0,   3, 254, 255, 254, 255, 
			 22,   0,  67,  84,  65,  66, 
			 28,   0,   0,   0,  35,   0, 
			  0,   0,   0,   3, 254, 255, 
			  0,   0,   0,   0,   0,   0, 
			  0,   0,   0,   1,   0,   0, 
			 28,   0,   0,   0, 118, 115, 
			 95,  51,  95,  48,   0,  77, 
			105,  99, 114, 111, 115, 111, 
			102, 116,  32,  40,  82,  41, 
			 32,  72,  76,  83,  76,  32, 
			 83, 104,  97, 100, 101, 114, 
			 32,  67, 111, 109, 112, 105, 
			108, 101, 114,  32,  57,  46, 
			 50,  57,  46,  57,  53,  50, 
			 46,  51,  49,  49,  49,   0, 
			 81,   0,   0,   5,   0,   0, 
			 15, 160,   0,   0,   0,  63, 
			  0,   0,   0, 191,   0,   0, 
			  0,   0,   0,   0,   0,   0, 
			 31,   0,   0,   2,   0,   0, 
			  0, 128,   0,   0,  15, 144, 
			 31,   0,   0,   2,   0,   0, 
			  0, 128,   0,   0,  15, 224, 
			 31,   0,   0,   2,   5,   0, 
			  0, 128,   1,   0,   3, 224, 
			  4,   0,   0,   4,   1,   0, 
			  3, 224,   0,   0, 228, 144, 
			  0,   0, 228, 160,   0,   0, 
			  0, 160,   1,   0,   0,   2, 
			  0,   0,  15, 224,   0,   0, 
			228, 144, 255, 255,   0,   0
			};
		CR_RETURN_HR( device->CreateVertexShader( reinterpret_cast<const DWORD*>( vsCompiled ), &m_vertexShader ) );
		// Compiled byte codes for vertex shader:
		// sampler2D src: register(s0);
		// float4 viewport: register(c0);
		// float4 ps( float2 pos: TEXCOORD0 ): COLOR
		// {
		//     return tex2D( src, pos + viewport.xy );
		// }
		const BYTE psCompiled[] = {
			  0,   3, 255, 255, 254, 255, 
			 44,   0,  67,  84,  65,  66, 
			 28,   0,   0,   0, 123,   0, 
			  0,   0,   0,   3, 255, 255, 
			  2,   0,   0,   0,  28,   0, 
			  0,   0,   0,   1,   0,   0, 
			116,   0,   0,   0,  68,   0, 
			  0,   0,   3,   0,   0,   0, 
			  1,   0,   2,   0,  72,   0, 
			  0,   0,   0,   0,   0,   0, 
			 88,   0,   0,   0,   2,   0, 
			  0,   0,   1,   0,   2,   0, 
			100,   0,   0,   0,   0,   0, 
			  0,   0, 115, 114,  99,   0, 
			  4,   0,  12,   0,   1,   0, 
			  1,   0,   1,   0,   0,   0, 
			  0,   0,   0,   0, 118, 105, 
			101, 119, 112, 111, 114, 116, 
			  0, 171, 171, 171,   1,   0, 
			  3,   0,   1,   0,   4,   0, 
			  1,   0,   0,   0,   0,   0, 
			  0,   0, 112, 115,  95,  51, 
			 95,  48,   0,  77, 105,  99, 
			114, 111, 115, 111, 102, 116, 
			 32,  40,  82,  41,  32,  72, 
			 76,  83,  76,  32,  83, 104, 
			 97, 100, 101, 114,  32,  67, 
			111, 109, 112, 105, 108, 101, 
			114,  32,  57,  46,  50,  57, 
			 46,  57,  53,  50,  46,  51, 
			 49,  49,  49,   0,  31,   0, 
			  0,   2,   5,   0,   0, 128, 
			  0,   0,   3, 144,  31,   0, 
			  0,   2,   0,   0,   0, 144, 
			  0,   8,  15, 160,   2,   0, 
			  0,   3,   0,   0,   3, 128, 
			  0,   0, 228, 160,   0,   0, 
			228, 144,  66,   0,   0,   3, 
			  0,   8,  15, 128,   0,   0, 
			228, 128,   0,   8, 228, 160, 
			255, 255,   0,   0
			};
		CR_RETURN_HR( device->CreatePixelShader( reinterpret_cast<const DWORD*>( psCompiled ), &m_pixelShader ) );
		return S_OK;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Destroys the blitter.
	// ----------------------------------------------------------------------------------
	void Destroy()
	{
		m_vb = nullptr;
		m_decl = nullptr;
		m_vertexShader = nullptr;
		m_pixelShader = nullptr;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Blits a texture onto a render target surface. Tries to not to mess D3D state in
	//   the process.
	// Arguments:
	//   destination - destination render target surface
	//   source - source texture (2D)
	//   width - destination surface width
	//   height - destination surface height
	//   device - valid D3D device
	// Return Value:
	//   HRESULT of the call.
	// ----------------------------------------------------------------------------------
	HRESULT Blit( IDirect3DSurface9* destination, IDirect3DBaseTexture9* source, uint32_t width, uint32_t height, IDirect3DDevice9* device )
	{
		if( !m_vb || !m_decl || !m_vertexShader || !m_pixelShader )
		{
			CR_RETURN_HR( Create( device ) );
		}

		CComPtr<IDirect3DSurface9>	oldRT, oldDS;

		device->GetRenderTarget( 0, &oldRT );
		device->GetDepthStencilSurface( &oldDS );

		CR_RETURN_HR( device->BeginScene() );
		ON_BLOCK_EXIT( [&]{ device->EndScene(); } );

		D3DVIEWPORT9 vpOld;
		CR( device->GetViewport( &vpOld ) );
		ON_BLOCK_EXIT( [&]{ device->SetViewport( &vpOld ); } );

		CR_RETURN_HR( device->SetRenderTarget( 0, destination ) );
		ON_BLOCK_EXIT( [&]{ device->SetRenderTarget( 0, oldRT ); } );
		CR_RETURN_HR( device->SetDepthStencilSurface( nullptr ) );
		ON_BLOCK_EXIT( [&]{ device->SetDepthStencilSurface( oldDS ); } );

#define PUSH_RENDER_STATE( rs, value ) DWORD CCP_CONCATENATE(old_, rs);						\
		CR( device->GetRenderState( rs, &CCP_CONCATENATE(old_, rs) ) );						\
		ON_BLOCK_EXIT( [&]{ device->SetRenderState( rs, CCP_CONCATENATE(old_, rs) ); } );	\
		device->SetRenderState( rs, value );

#define PUSH_SAMPLER_STATE( ss, value ) DWORD CCP_CONCATENATE(old_, ss);						\
		CR( device->GetSamplerState( 0, ss, &CCP_CONCATENATE(old_, ss) ) );						\
		ON_BLOCK_EXIT( [&]{ device->SetSamplerState( 0, ss, CCP_CONCATENATE(old_, ss) ); } );	\
		device->SetSamplerState( 0, ss, value );

		PUSH_RENDER_STATE( D3DRS_CULLMODE, D3DCULL_NONE );
		PUSH_RENDER_STATE( D3DRS_ALPHABLENDENABLE, FALSE );
		PUSH_RENDER_STATE( D3DRS_ALPHATESTENABLE, FALSE );

		CComPtr<IDirect3DVertexDeclaration9> oldDecl;
		CR( device->GetVertexDeclaration( &oldDecl ) );
		ON_BLOCK_EXIT( [&]{ device->SetVertexDeclaration( oldDecl ); } );
		CR_RETURN_HR( device->SetVertexDeclaration( m_decl ) );

		CComPtr<IDirect3DVertexBuffer9> oldVB;
		UINT oldOffset, oldStride;
		CR( device->GetStreamSource( 0, &oldVB, &oldOffset, &oldStride ) );
		ON_BLOCK_EXIT( [&]{ device->SetStreamSource( 0, oldVB, oldOffset, oldStride ); } );
		CR_RETURN_HR( device->SetStreamSource( 0, m_vb, 0, sizeof( float ) * 4 ) );

		CComPtr<IDirect3DVertexShader9> oldVS;
		CR( device->GetVertexShader( &oldVS ) );
		ON_BLOCK_EXIT( [&]{ device->SetVertexShader( oldVS ); } );
		CR_RETURN_HR( device->SetVertexShader( m_vertexShader ) );

		CComPtr<IDirect3DPixelShader9> oldPS;
		CR( device->GetPixelShader( &oldPS ) );
		ON_BLOCK_EXIT( [&]{ device->SetPixelShader( oldPS ); } );
		CR_RETURN_HR( device->SetPixelShader( m_pixelShader ) );

		float oldPSConstant[4];
		CR( device->GetPixelShaderConstantF( 0, oldPSConstant, 1 ) );
		ON_BLOCK_EXIT( [&]{ device->SetPixelShaderConstantF(  0, oldPSConstant, 1 ); } );
		float lod[4] = { 0.5f / float( width ), 0.5f / float( height ), 0.0f, 0.0f };
		CR_RETURN_HR( device->SetPixelShaderConstantF( 0, lod, 1 ) );

		CComPtr<IDirect3DBaseTexture9> oldTexture;
		CR( device->GetTexture( 0, &oldTexture ) );
		ON_BLOCK_EXIT( [&]{ device->SetTexture( 0, oldTexture ); } );
		CR_RETURN_HR( device->SetTexture( 0, source ) );

		PUSH_SAMPLER_STATE( D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
		PUSH_SAMPLER_STATE( D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
		PUSH_SAMPLER_STATE( D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		PUSH_SAMPLER_STATE( D3DSAMP_MINFILTER, D3DTEXF_POINT );
		PUSH_SAMPLER_STATE( D3DSAMP_MIPFILTER, D3DTEXF_NONE ); // TODO: this should be POINT, but fixing it means possible changes to the game(
		PUSH_SAMPLER_STATE( D3DSAMP_MIPMAPLODBIAS, 0 );
		PUSH_SAMPLER_STATE( D3DSAMP_SRGBTEXTURE, 0 );

		return device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
	}

	CComPtr<IDirect3DVertexBuffer9> m_vb;
	CComPtr<IDirect3DVertexDeclaration9> m_decl;
	CComPtr<IDirect3DVertexShader9> m_vertexShader;
	CComPtr<IDirect3DPixelShader9> m_pixelShader;
};

Tr2RenderContextAL::Tr2RenderContextAL()
	: m_usingEXDevice( false )
	, m_topology( D3DPT_TRIANGLELIST )	
	, m_stackDS( "Tr2RenderContextAL::m_stackDS" )
	, m_queryDustbin( "Tr2RenderContextAL::m_queryDustbin" )
	, m_depthStencilFormat( DSFMT_UNKNOWN )
	, m_blitter( nullptr )
	, m_events( nullptr )
	, m_adapter( 0 )
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() == nullptr );
	::GetPrimaryRenderContextPointer() = this;

	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = nullptr;
		m_stackRT[i].SetName( "Tr2RenderContextAL::m_stackRT" );
	}
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
	Destroy();
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContext )
{
	::GetPrimaryRenderContextPointer() = renderContext;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() );
	return *GetPrimaryRenderContextPointer();
}

Tr2PrimaryRenderContextAL* Tr2RenderContextAL::GetPrimaryRenderContextPointer()
{
	return ::GetPrimaryRenderContextPointer();
}

void Tr2RenderContextAL::Destroy()
{
	ReleaseDeviceResources();

	m_topology = D3DPT_TRIANGLELIST;

	m_usingEXDevice = false;
	
	m_d3dDevice9 = nullptr;
	m_depthStencilFormat = DSFMT_UNKNOWN;
}

ALResult Tr2RenderContextAL::ReportIfFailure( long hr, const char* message )
{
#if defined(_DEBUG) || defined(TRINITYDEV)
	if( FAILED(hr) )
	{
		ReportHresultError( __FILE__, __LINE__, message, hr );
		BreakInDebugger();
	}
#else
	(message);
#endif
	return hr;
}

ALResult Tr2RenderContextAL::SetStreamSource( 
	uint32_t stream, 
	const Tr2VertexBufferAL & buffer, 
	uint32_t offset, 
	uint32_t stride )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	if( !buffer.IsValid() )
	{
		if( &buffer == &nullVB )
		{
			return ReportIfFailure( 
							m_d3dDevice9->SetStreamSource( stream, nullptr, 0, 0 )
							, "SetStreamSource" );
		}
		return E_INVALIDARG;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	return ReportIfFailure( 
							m_d3dDevice9->SetStreamSource( stream, buffer.m_buffer, offset, stride )
							, "SetStreamSource" );
}

ALResult Tr2RenderContextAL::Clear(	
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil,
	uint32_t slot )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	uint32_t d3dFlags = 0;
	if( clearFlags & CLEARFLAGS_TARGET )
	{
		d3dFlags |= D3DCLEAR_TARGET;
	}
	if( clearFlags & CLEARFLAGS_ZBUFFER )
	{
		d3dFlags |= D3DCLEAR_ZBUFFER;
	}
	if( clearFlags & CLEARFLAGS_STENCIL )
	{
		if( HasStencilBuffer() )
		{
			d3dFlags |= D3DCLEAR_STENCIL;
		}
	}

	return ReportIfFailure( m_d3dDevice9->Clear(0, NULL, d3dFlags, color, depth, stencil ), "Clear" );
}

// Version of SetIndices that accepts a nullpointer, in which case the currently bound index buffer is un-set.
ALResult Tr2RenderContextAL::SetIndices( const Tr2IndexBufferAL& buffer )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	if( !buffer.IsValid() )
	{
		if( &buffer == &nullIB )
		{
			return ReportIfFailure( m_d3dDevice9->SetIndices( nullptr ), "SetIndices" );
		}
		return E_INVALIDARG;
	}
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	return ReportIfFailure( m_d3dDevice9->SetIndices( buffer.m_buffer ), "SetIndices" );
}

ALResult Tr2RenderContextAL::SetTopology( long topology )
{
	m_topology = topology;
	return S_OK;
}

namespace {

	static D3DPRIMITIVETYPE lookup[ TOP_MAX_TOPOLOGY ] = 
	{
		D3DPT_POINTLIST,		// invalid.. least likely to explode

		D3DPT_TRIANGLELIST,
		D3DPT_TRIANGLESTRIP,
		D3DPT_TRIANGLEFAN,

		D3DPT_LINELIST,
		D3DPT_LINESTRIP,

		D3DPT_POINTLIST		
	};

}

ALResult Tr2RenderContextAL::DrawIndexedPrimitive(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t minimumIndex )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, numVertices );
	CCP_STATS_INC( sceneDrawcallCount );

	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	return ReportIfFailure( 
					m_d3dDevice9->DrawIndexedPrimitive( 
							lookup[ m_topology ], 
							0, minimumIndex, numVertices, 
							startIndex, primitiveCount )
					, "DrawIndexedPrimitive" );
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t numInstances )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount * numInstances );
	CCP_STATS_ADD( vertexCount, numVertices * numInstances );
	CCP_STATS_INC( sceneDrawcallCount );

	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	m_d3dDevice9->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA  | numInstances );			
	m_d3dDevice9->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | 1u );

	HRESULT hr = ReportIfFailure( 
					m_d3dDevice9->DrawIndexedPrimitive( 
							lookup[ m_topology ], 
							0, 0, numVertices, startIndex, 
							primitiveCount )
					, "DrawIndexedInstanced" );

	m_d3dDevice9->SetStreamSourceFreq( 0, 1 );
	m_d3dDevice9->SetStreamSourceFreq( 1, 1 );

	return hr;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	return ReportIfFailure( 
					m_d3dDevice9->DrawPrimitive( lookup[ m_topology ], startVertex, primitiveCount )
					, "DrawIndexed" );
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP( 
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	return ReportIfFailure( 
				m_d3dDevice9->DrawPrimitiveUP( 
						lookup[ m_topology ], 
						primitiveCount, 
						vertexStreamZeroData, 
						vertexStreamZeroStride )
				, "DrawIndexedUP" );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	return ReportIfFailure( 
				m_d3dDevice9->DrawIndexedPrimitiveUP(	lookup[ m_topology ], 0, 
														numVertices, primitiveCount, 
														indexData, D3DFMT_INDEX32, 
														vertexStreamZeroData, vertexStreamZeroStride )
				, "DrawIndexedPrimitiveUP" );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	
	if( m_topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}

	return ReportIfFailure( 
				m_d3dDevice9->DrawIndexedPrimitiveUP(	
										lookup[ m_topology ], 0, 
										numVertices, primitiveCount, 
										indexData, D3DFMT_INDEX16, 
										vertexStreamZeroData, vertexStreamZeroStride )
				, "DrawIndexedPrimitiveUP" );
}

ALResult Tr2RenderContextAL::SetConstants(
	Tr2ConstantBufferAL& buffer, 
	ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t maxRegisterCount )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	if( !buffer.IsValid() )
	{
		if( &buffer == &nullCB )
		{
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	CCP_ASSERT( ( buffer.GetSize() % 16 ) == 0);

	AL_UPDATE_RESOURCE_FRAME_USAGE( buffer );
	switch( constantType )
	{
	case VERTEX_SHADER:
		return m_d3dDevice9->SetVertexShaderConstantF( 
								registerIndex, 
								(float*)buffer.m_shadowCopy.get(), 
								(uint32_t)buffer.GetSize() / ( 4 * sizeof(float) ) );

	case PIXEL_SHADER:
		{
			const uint32_t maxNumberOfPSRegistersFOnSM30 = 224;
			const uint32_t hardLimit = maxNumberOfPSRegistersFOnSM30 - registerIndex;

			maxRegisterCount = maxRegisterCount ? std::min( maxRegisterCount, hardLimit ) : hardLimit;

			return m_d3dDevice9->SetPixelShaderConstantF( 
								registerIndex, 
								(float*)buffer.m_shadowCopy.get(), 
								std::min(	maxRegisterCount, 
											(uint32_t)(buffer.GetSize() / ( 4 * sizeof(float) )) ) );
		}
	}

	CCP_AL_LOGERR( "Trying to use something other than pixel or vertex shader constants with DirectX9" );
	return E_INVALIDARG;
}

ALResult Tr2RenderContextAL::SetSamplerState( 
	const Tr2SamplerStateAL& samplerState, 
	ShaderType /*inputType*/, 
	uint32_t registerNumber )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( samplerState );
	for( int i = Tr2SamplerStateAL::SAMPLER_STATE_MIN; i < Tr2SamplerStateAL::SAMPLER_STATE_COUNT; ++i )
	{
		HRESULT hr = m_d3dDevice9->SetSamplerState(		registerNumber, 
														D3DSAMPLERSTATETYPE( i ), 
														samplerState.m_states[i] );
		if( FAILED( hr ) )
		{
			return hr;
		}
	}
	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool /*enable*/ ) 
{
}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return false;
}

ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2DepthStencilAL& depthStencil )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	if( !depthStencil.IsValid() )
	{
		return m_d3dDevice9->SetDepthStencilSurface( nullptr );
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( depthStencil );
	if( depthStencil.m_depthStencil )
	{
		return m_d3dDevice9->SetDepthStencilSurface( depthStencil.m_depthStencil );
	}
	if( depthStencil.m_depthStencilREADABLE )
	{
		CComPtr<IDirect3DSurface9> surf;
		CR_RETURN_HR( depthStencil.m_depthStencilREADABLE->GetSurfaceLevel( 0, &surf ) );
		return m_d3dDevice9->SetDepthStencilSurface( surf );
	}
	return E_FAIL;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	if( &renderTarget == &nullRT && slot == 0 )
	{
		m_boundRenderTarget[slot] = nullptr;
		if( !m_d3dDevice9 )
		{
			return E_FAIL;
		}
		return m_d3dDevice9->SetRenderTarget( slot, m_nullRT );
	}
	else
	{
		AL_UPDATE_RESOURCE_FRAME_USAGE( renderTarget );
		// This can get complicated enough that we just go and ask the RT to do it.
		HRESULT hr = renderTarget.Bind( slot, *this );
		if( SUCCEEDED( hr ) )
		{
			m_boundRenderTarget[slot] = &renderTarget;
		}
		return hr;
	}
}

/*Tr2RenderTargetAL Tr2RenderContextAL::GetRenderTarget( unsigned slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return nullRT;
	}

	return m_boundRenderTarget[slot];
}*/

ALResult Tr2RenderContextAL::CreateDevice(	
	uint32_t Adapter, 
	Tr2WindowHandle  hFocusWindow, 
	const Tr2PresentParametersAL& presentationParameters )
{
	if( !Tr2VideoAdapterInfo::GetDirect3D() )
	{
		return E_FAIL;
	}

	auto d3d9 = Tr2VideoAdapterInfo::GetDirect3D();

	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL; 
	const uint32_t behaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_HARDWARE_VERTEXPROCESSING;

#if NVPERFHUD
	// If it is present, override default settings
	uint32_t ac = 0;
	if( SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterCount( ac ) ) )
	{
		for( uint32_t search=0;search<ac; search++ )
		{
			Tr2AdapterInfo info;
			if( SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterInfo( search, info ) ) )
			{
				if( info.description == L"NVIDIA PerfHUD" )
				{
					Adapter=search;
					deviceType=D3DDEVTYPE_REF;
					break;
				}
			} 
		}
	}
#endif

	extern bool g_usingEXDevice;
	extern bool g_useManagedDX9Buffers;
	extern bool g_preloadTextureToDeviceOnPrepare;

	D3DPRESENT_PARAMETERS pp;
	pp.BackBufferWidth = presentationParameters.mode.width;
	pp.BackBufferHeight = presentationParameters.mode.height;
	pp.BackBufferFormat = ConvertToD3D9Format( presentationParameters.mode.format );
	pp.BackBufferCount = presentationParameters.backBufferCount;
	pp.MultiSampleType = D3DMULTISAMPLE_TYPE( presentationParameters.msaaType );
    pp.MultiSampleQuality = presentationParameters.msaaQuality;
	pp.SwapEffect = presentationParameters.swapEffect == SWAP_EFFECT_DISCARD ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	pp.hDeviceWindow = Tr2WindowHandle( presentationParameters.outputWindow );
    pp.Windowed = presentationParameters.windowed ? TRUE : FALSE;
	pp.EnableAutoDepthStencil = presentationParameters.depthStencilFormat != DSFMT_UNKNOWN;
	pp.AutoDepthStencilFormat = ConvertToD3D9DepthStencilFormat( presentationParameters.depthStencilFormat );
    pp.Flags = 0;
	pp.FullScreen_RefreshRateInHz = presentationParameters.windowed ? 0 : presentationParameters.mode.refreshRateDenominator;
	switch( presentationParameters.presentInterval )
	{
	case PRESENT_INTERVAL_IMMEDIATE:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		break;
	case PRESENT_INTERVAL_TWO:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_TWO;
		break;
	case PRESENT_INTERVAL_THREE:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_THREE;
		break;
	case PRESENT_INTERVAL_FOUR:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_FOUR;
		break;
	default:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	}

	HRESULT hr;
	if( g_usingEXDevice )
	{
		// We can not use managed buffers with the ex device.
		g_useManagedDX9Buffers = false;
		g_preloadTextureToDeviceOnPrepare = false;
		hr = ((IDirect3D9Ex*)&*d3d9)->CreateDeviceEx(	Adapter, 
														deviceType, 
														hFocusWindow, 
														behaviorFlags, 
														&pp, 
														0, 
														(IDirect3DDevice9Ex**)&m_d3dDevice9);
	}
	else
	{
		hr = d3d9->CreateDevice(	Adapter, 
									deviceType, 
									hFocusWindow, 
									behaviorFlags, 
									&pp, 
									&m_d3dDevice9 );
	}

	if( SUCCEEDED( hr ) && m_d3dDevice9 )
	{
		m_caps.QueryCaps( m_d3dDevice9 );

		m_depthStencilFormat = presentationParameters.depthStencilFormat;

		m_adapter = Adapter;
	}

	SetPresentParameters( Adapter, presentationParameters );
	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	return hr;
}

PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const
{
	if( !m_d3dDevice9 )
	{
		return PIXEL_FORMAT_UNKNOWN;
	}


	if( !m_d3dDevice9 )
	{
		return PIXEL_FORMAT_UNKNOWN;
	}

	CComPtr<IDirect3DSurface9>	rt;
	CR_RETURN_VAL( m_d3dDevice9->GetRenderTarget( 0, &rt ), PIXEL_FORMAT_UNKNOWN );
	
	D3DSURFACE_DESC desc;
	CR_RETURN_VAL( rt->GetDesc( &desc ), PIXEL_FORMAT_UNKNOWN );

	return ConvertD3DBackBufferFormat( desc.Format );
}

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& /*pPresentationParameters*/ )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	//TODO reset device here.

	CComPtr<IDirect3DSurface9> backBuffer;
	CR_RETURN_HR( m_d3dDevice9->GetRenderTarget( 0, &backBuffer ) );
	CR_RETURN_HR( m_defaultBackBuffer.Attach( backBuffer, *this ) );

	m_nullRT = nullptr;
	CR_RETURN_HR( m_d3dDevice9->CreateRenderTarget(		1, 1, 
														D3DFMT_A8R8G8B8, 
														D3DMULTISAMPLE_NONE, 0, 
														FALSE, &m_nullRT, nullptr ) );

	m_adapter = adapter;

	return S_OK;
}

const Tr2CapsAL& Tr2RenderContextAL::GetCaps() const
{
	return m_caps;
}

ALResult Tr2RenderContextAL::BeginScene()
{
	return m_d3dDevice9 ? m_d3dDevice9->BeginScene() : S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	return m_d3dDevice9 ? m_d3dDevice9->EndScene() : S_OK;
}

ALResult Tr2RenderContextAL::Present()
{
#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
	extern uint64_t g_trackCurrentFrame;
	++g_trackCurrentFrame;
#endif
	if( m_d3dDevice9 )
	{
		CComPtr<IDirect3DSwapChain9> chain;
		HRESULT hr = m_d3dDevice9->GetSwapChain( 0, &chain );
		if( FAILED( hr ) )
		{
			CCP_AL_LOGERR( "Tr2RenderContext::Present: GetSwapChain failed");
			return hr;
		}
		if( chain )
		{
			hr = chain->Present( nullptr, nullptr, nullptr, nullptr, 0 );
	
			if( FAILED( hr ) )
			{
				CCP_AL_LOGERR( "Tr2RenderContext::Present: Present failed");
				return hr;
			}
		}
	}

	ProcessOrphanedQueries( false );

	m_frameDelayedDX9Objects.clear();
	return S_OK;
}

bool Tr2RenderContextAL::IsValid()
{
	return m_d3dDevice9 != nullptr;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout )
{
	if( !m_d3dDevice9 || !layout.IsValid() )
	{
		return E_FAIL;
	}
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( layout );
	return layout.SetLayout( nullptr, *this );
}

ALResult Tr2RenderContextAL::SetShader( const Tr2ShaderAL& shader )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( shader );
	return shader.Apply( *this );
}

ALResult Tr2RenderContextAL::SetRenderState( RenderState state, uint32_t value )
{
	static_assert(	sizeof( D3DRENDERSTATETYPE ) == sizeof( uint32_t ), 
					"Incorrect size, cast won't work" );

#if !defined( NDEBUG )
	return m_d3dDevice9 ? m_d3dDevice9->SetRenderState( static_cast<D3DRENDERSTATETYPE>( state ), value ) 
						: E_FAIL;
#else
	return m_d3dDevice9->SetRenderState( static_cast<D3DRENDERSTATETYPE>( state ), value );
#endif
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	for(	uint32_t i = 0; 
			( count != 0 && i != count ) || ( count == 0 && *stateValuePairs ); 
			++i, stateValuePairs += 2 )
	{
		m_d3dDevice9->SetRenderState( 
							static_cast<D3DRENDERSTATETYPE>( stateValuePairs[0] ), 
							stateValuePairs[1] );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::GetRenderState( RenderState state, uint32_t* value )
{
	static_assert( sizeof( uint32_t ) == sizeof( uint32_t ), "Incorrect size, cast won't work" );

#if !defined( NDEBUG )
	return m_d3dDevice9 ? m_d3dDevice9->GetRenderState( 
							static_cast<D3DRENDERSTATETYPE>( state ),
							reinterpret_cast<DWORD*>( value ) ) 
						: E_FAIL;
#else
	return m_d3dDevice9->GetRenderState( 
							static_cast<D3DRENDERSTATETYPE>( state ),
							reinterpret_cast<DWORD*>( value ) );
#endif
}

ALResult Tr2RenderContextAL::SetClipPlane( uint32_t planeIndex, const float* planeEq )
{
#if !defined( NDEBUG )
	return m_d3dDevice9 ? m_d3dDevice9->SetClipPlane( planeIndex, planeEq ) : E_FAIL;
#else
	return m_d3dDevice9->SetClipPlane( planeIndex, planeEq );
#endif
}

ALResult Tr2RenderContextAL::SetScissorRect(	
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom )
{
	RECT rect;
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

#if !defined( NDEBUG )
	return m_d3dDevice9 ? m_d3dDevice9->SetScissorRect( &rect ) : E_FAIL;
#else
	return m_d3dDevice9->SetScissorRect( &rect );
#endif
}

ALResult Tr2RenderContextAL::SetShaderBuffer(		
	ShaderType /* inputType */, 
	uint32_t /* slot */, 
	const Tr2GpuBufferAL& /* buffer */ )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::SetTexture(	
	ShaderType inputType, 
	uint32_t slot, 
	const Tr2TextureAL& texture, 
	Tr2RenderContextEnum::ColorSpace colorSpace )
{
#if !defined( NDEBUG )
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
#endif
	switch( inputType )
	{
	case VERTEX_SHADER:
		if( slot > 3 )
		{
			return E_FAIL;
		}
		slot += D3DVERTEXTEXTURESAMPLER0;
		break;
	case PIXEL_SHADER:
		break;
	default:
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( texture );
	CR_RETURN_HR( m_d3dDevice9->SetSamplerState( slot, D3DSAMP_SRGBTEXTURE, colorSpace == COLOR_SPACE_SRGB ? TRUE : FALSE ) );
	return m_d3dDevice9->SetTexture( slot, texture.m_texture );
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights )
{
	if( m_d3dDevice9 )
	{
		int lights = numLights;
		return m_d3dDevice9->SetPixelShaderConstantI( 15, &lights, 1 );
	}
	return E_FAIL;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	D3DVIEWPORT9 vp;
	vp.X		= (uint32_t)viewport.m_x;
	vp.Y		= (uint32_t)viewport.m_y;
	vp.Width	= (uint32_t)viewport.m_width;
	vp.Height	= (uint32_t)viewport.m_height;
	vp.MinZ		= viewport.m_minZ;
	vp.MaxZ		= viewport.m_maxZ;
	return m_d3dDevice9->SetViewport( &vp );
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	D3DVIEWPORT9 vp;
	HRESULT hr = m_d3dDevice9->GetViewport( &vp );
	if( SUCCEEDED( hr ) )
	{
		viewport.m_x		= (float)vp.X;
		viewport.m_y		= (float)vp.Y;
		viewport.m_width	= (float)vp.Width;
		viewport.m_height	= (float)vp.Height;
		viewport.m_minZ		= vp.MinZ;
		viewport.m_maxZ		= vp.MaxZ;
	}
	return hr;
}

bool Tr2RenderContextAL::IsSupportedRenderTargetFormat( PixelFormat format, bool withAutoGenMipmap )
{
	const D3DFORMAT format9 = ConvertToD3D9Format( format );
	if( format9 == D3DFMT_UNKNOWN )
	{
		return false;
	}
	
	Tr2DisplayModeInfo displayMode;
	if( FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( m_adapter, displayMode ) ) )
	{
		return false;
	}
	
	return Tr2VideoAdapterInfo::SupportsRenderTargetFormat( m_adapter, 
															displayMode.format, 
															format, 
															withAutoGenMipmap );
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( !m_d3dDevice9 || slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	
	IDirect3DSurface9* oldRenderTarget;
	HRESULT hr = m_d3dDevice9->GetRenderTarget( slot, &oldRenderTarget );

	// D3DERR_NOTFOUND is returned if the current RT is NULL (this is valid)
	if( FAILED(hr) && hr != D3DERR_NOTFOUND )
	{
		CCP_AL_LOGERR( "PushRenderTarget failed getting the original render target" );
#if defined(_DEBUG) || defined(TRINITYDEV)	
		BreakInDebugger();
#endif
		return hr;
	}

	m_stackRT[slot].push( oldRenderTarget );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( !m_d3dDevice9 || slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	CCP_ASSERT( !m_stackRT[slot].empty() );
	if( m_stackRT[slot].empty() )
	{
		return E_FAIL;
	}

	HRESULT hr = m_d3dDevice9->SetRenderTarget( slot, m_stackRT[slot].top() );
	if( m_stackRT[slot].top() )
	{
		m_stackRT[slot].top()->Release();
	}
	m_stackRT[slot].pop();
	return hr;
}
	
ALResult Tr2RenderContextAL::PushDepthStencil()
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}

	IDirect3DSurface9* oldTarget;

	HRESULT hr = m_d3dDevice9->GetDepthStencilSurface( &oldTarget );

	// D3DERR_NOTFOUND is returned if the current DS is NULL (this is valid)
	if( FAILED(hr) && hr != D3DERR_NOTFOUND )
	{
		CCP_AL_LOGERR( "PushDepthStencil failed getting the original depth buffer" );
#if defined(_DEBUG) || defined(TRINITYDEV)	
		BreakInDebugger();
#endif
		return hr;
	}

	m_stackDS.push( oldTarget );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil()
{
	CCP_ASSERT( !m_stackDS.empty() );
	if( !m_d3dDevice9 || m_stackDS.empty() )
	{
		return E_FAIL;
	}
	HRESULT hr = m_d3dDevice9->SetDepthStencilSurface( m_stackDS.top() );
	if( !SUCCEEDED( hr ) )
	{
		CCP_AL_LOGERR( "Failed to set depth/stencil buffer");		
	}
	if( m_stackDS.top() )
	{
		m_stackDS.top()->Release();
	}
	m_stackDS.pop();
	return hr;
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot )
{
	if( !m_d3dDevice9 || slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	/*	 TODO this is not correct.. until everything goes through the context, we can't use RTs in the stack; so when the stack
	pushes/pops, we don't know anymore what's currently bound, and this is wrong. */
	/*
	if( m_boundRenderTarget[slot].IsValid() )
	{
		width  = m_boundRenderTarget[slot].GetWidth();
		height = m_boundRenderTarget[slot].GetHeight();
		return S_OK;
	}*/

	CComPtr<IDirect3DSurface9> surface;
	HRESULT hr = m_d3dDevice9->GetRenderTarget( slot, &surface );
	if( SUCCEEDED( hr ) )
	{
		if( surface == m_nullRT )
		{
			surface = nullptr;
			hr = m_d3dDevice9->GetDepthStencilSurface( &surface );
		}
		if( SUCCEEDED( hr ) )
		{
			if( surface == nullptr )
			{
				width = height = 0;
			}
			else
			{
				D3DSURFACE_DESC desc;
				hr = surface->GetDesc( &desc );
				if( SUCCEEDED( hr ) )
				{
					width = desc.Width;
					height = desc.Height;
				}
			}
		}
	}
	return hr;
}

D3DFORMAT Tr2RenderContextAL::ConvertToD3D9Format( PixelFormat format )
{
	return Tr2RenderContextEnum::ConvertToD3D9Format( format );
}

PixelFormat Tr2RenderContextAL::ConvertFromD3D9Format( D3DFORMAT format )
{
	return Tr2RenderContextEnum::ConvertFromD3D9Format( format );
}

/////////////////////////////////////////////////////////////////////////
// Orphaned queries 
// D3D has a bug, in that if a Query object is released before it
// has moved into its signaled state (that is, the results are ready)
// it can crash the program when the results finally come in.  This
// Trash system here, takes old queries and stores them until the results
// are ready.  It then releases them.
/////////////////////////////////////////////////////////////////////////
void Tr2RenderContextAL::TrashQuery( IDirect3DQuery9* query )
{
	if( query )
	{
		m_queryDustbin.push_back(query);
	}
}

bool Tr2RenderContextAL::ProcessOrphanedQueries( bool flush )
{
	for( size_t i = 0; i < m_queryDustbin.size(); ++i )
	{
		uint32_t flags = ( flush ? D3DGETDATA_FLUSH : 0 );

		//S_FALSE means query isn't ready, any failure means it must be destroyed.
		//S_OK means that data is ready and the query can be destroyed
		if( flush )
		{
			while( S_FALSE == m_queryDustbin[i]->GetData( 0, 0, flags ) )
			{
			}
			m_queryDustbin.erase( m_queryDustbin.begin() + i );
			--i;
		}
		else
		{
			HRESULT h = m_queryDustbin[i]->GetData( 0, 0, flags );
			if( h != S_FALSE ) 
			{
				m_queryDustbin.erase( m_queryDustbin.begin() + i );
				--i;
			}
		}
	}
	return m_queryDustbin.empty();
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	if( !ProcessOrphanedQueries( true ) )
	{
		CCP_AL_LOGWARN( "****** Failed to empty query buffer!! ********" );
	}

	delete m_blitter;
	m_blitter = nullptr;

	m_frameDelayedDX9Objects.clear();

	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		while( !m_stackRT[i].empty() )
		{
			if( m_stackRT[i].top() )
			{
				m_stackRT[i].top()->Release();
			}
			m_stackRT[i].pop();
		}

		if( m_boundRenderTarget[i] )
		{
			m_boundRenderTarget[i] = nullptr;
		}
	}
	while( !m_stackDS.empty() )
	{
		if( m_stackDS.top() )
		{
			m_stackDS.top()->Release();
		}
		m_stackDS.pop();
	}

	m_defaultBackBuffer.Destroy();
	m_nullRT = nullptr;
}

namespace {

inline bool CheckStencilBufferFormat( D3DFORMAT fmt )
{
	switch( (uint32_t)fmt )
	{
	case D3DFMT_D15S1:
		return true;
	case D3DFMT_D24S8:
		return true;
	case D3DFMT_D24X4S4:
		return true;
	case D3DFMT_D24FS8:
		return true;
	case MAKEFOURCC( 'I', 'N', 'T', 'Z' ):
		return true;
	}
	return false;
}

}	// ns

bool Tr2RenderContextAL::HasStencilBuffer()
{
	if( !m_d3dDevice9 )
	{
		return false;
	}

	CComPtr<IDirect3DSurface9> curStencil;
	CR_RETURN_VAL( m_d3dDevice9->GetDepthStencilSurface( &curStencil ), false );

	if( curStencil )
	{
		D3DSURFACE_DESC desc;

		HRESULT hr = curStencil->GetDesc( &desc );
		if( SUCCEEDED( hr ) )
		{
			return CheckStencilBufferFormat( desc.Format );
		}
	}

	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Performs texture blit onto a render target surface. Uses point sampling. Used for 
//   render target locking. Do not use this function outside AL.
// Arguments:
//   destination - Destination surface (needs to be a render target)
//   source - Source texture (2D)
//   width - Destination width
//   height - Destination height
// Return Value:
//   AL result of the call.
// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::InternalBlit( IDirect3DSurface9* destination, IDirect3DBaseTexture9* source, uint32_t width, uint32_t height )
{
	if( !m_d3dDevice9 )
	{
		return E_FAIL;
	}
	if( !m_blitter )
	{
		m_blitter = new Blitter;
	}
	return m_blitter->Blit( destination, source, width, height, m_d3dDevice9 );
}

// --------------------------------------------------------------------------------------
void Tr2RenderContextAL::AddGpuMarker( const char* )
{
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus&, std::string& ) const
{
	return E_FAIL;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuPageFaultResource(
	Tr2RenderContextEnum::PixelFormat&,
	uint64_t&,
	uint32_t&,
	uint32_t&,
	uint32_t&,
	uint32_t& ) const
{
	return E_FAIL;
}

#endif
