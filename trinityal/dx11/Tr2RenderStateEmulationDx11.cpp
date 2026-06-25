// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "Tr2RenderStateEmulationDx11.h"

TrackableStdUnorderedMap<D3D11_BLEND_DESC,
						 CComPtr<ID3D11BlendState>,
						 Tr2RenderStateEmulation::TBlendHash,
						 Tr2RenderStateEmulation::TBlendEq>
	Tr2RenderStateEmulation::s_blendStateCache( "Tr2RenderStateEmulation::s_blendStateCache" );

TrackableStdUnorderedMap<D3D11_DEPTH_STENCIL_DESC,
						 CComPtr<ID3D11DepthStencilState>,
						 Tr2RenderStateEmulation::TDepthStencilHash,
						 Tr2RenderStateEmulation::TDepthStencilEq>
	Tr2RenderStateEmulation::s_depthStencilStateCache( "Tr2RenderStateEmulation::s_depthStencilStateCache" );

TrackableStdUnorderedMap<D3D11_RASTERIZER_DESC,
						 CComPtr<ID3D11RasterizerState>,
						 Tr2RenderStateEmulation::TRasterizerHash,
						 Tr2RenderStateEmulation::TRasterizerEq>
	Tr2RenderStateEmulation::s_rasterizerCache( "Tr2RenderStateEmulation::s_rasterizerCache" );


long Tr2RenderStateEmulation::TBlendHash::operator()( const D3D11_BLEND_DESC& bd ) const
{
	return CcpHashFNV1( &bd, sizeof( bd ) );
}

bool Tr2RenderStateEmulation::TBlendEq::operator()( const D3D11_BLEND_DESC& bd1, const D3D11_BLEND_DESC& bd2 ) const
{
	return memcmp( &bd1, &bd2, sizeof( bd1 ) ) == 0;
}

long Tr2RenderStateEmulation::TDepthStencilHash::operator()( const D3D11_DEPTH_STENCIL_DESC& dsd ) const
{
	return CcpHashFNV1( &dsd, sizeof( dsd ) );
}

bool Tr2RenderStateEmulation::TDepthStencilEq::operator()( const D3D11_DEPTH_STENCIL_DESC& dsd1,
														   const D3D11_DEPTH_STENCIL_DESC& dsd2 ) const
{
	return memcmp( &dsd1, &dsd2, sizeof( dsd1 ) ) == 0;
}

long Tr2RenderStateEmulation::TRasterizerHash::operator()( const D3D11_RASTERIZER_DESC& rd ) const
{
	return CcpHashFNV1( &rd, sizeof( rd ) );
}

bool Tr2RenderStateEmulation::TRasterizerEq::operator()( const D3D11_RASTERIZER_DESC& rd1,
														 const D3D11_RASTERIZER_DESC& rd2 ) const
{
	return memcmp( &rd1, &rd2, sizeof( rd1 ) ) == 0;
}


ALResult Tr2RenderStateEmulation::GetBlendState( CComPtr<ID3D11BlendState>& blendState, const CComPtr<ID3D11Device>& device11 ) const
{
	auto blendCopy = m_currentBlend;
	if( !m_separateAlphaBlendEnabled )
	{
		auto& desc = blendCopy.RenderTarget[0];
		static D3D11_BLEND alphaBlendRemap[] = {
			D3D11_BLEND_ZERO,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_ONE,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_INV_SRC_ALPHA,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_INV_SRC_ALPHA,
			D3D11_BLEND_DEST_ALPHA,
			D3D11_BLEND_INV_DEST_ALPHA,
			D3D11_BLEND_DEST_ALPHA,
			D3D11_BLEND_INV_DEST_ALPHA,
			D3D11_BLEND_SRC_ALPHA_SAT,
			D3D11_BLEND_BLEND_FACTOR,
			D3D11_BLEND_INV_BLEND_FACTOR,

			D3D11_BLEND_SRC1_ALPHA,
			D3D11_BLEND_INV_SRC1_ALPHA,
			D3D11_BLEND_SRC1_ALPHA,
			D3D11_BLEND_INV_SRC1_ALPHA
		};
		desc.BlendOpAlpha = desc.BlendOp;
		desc.SrcBlendAlpha = alphaBlendRemap[desc.SrcBlend];
		desc.DestBlendAlpha = alphaBlendRemap[desc.DestBlend];
	}

	auto cached = s_blendStateCache.find( blendCopy );

	if( cached != s_blendStateCache.end() )
	{
		blendState = cached->second;
		return S_OK;
	}

	const HRESULT hr = device11->CreateBlendState( &blendCopy, &blendState );
	if( hr == D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS )
	{
		// hit the 4K limit, we don't have a LRU mechanism, just clear the whole
		// thing and lazily recreate.
		s_blendStateCache.clear();

		CR_RETURN_HR( device11->CreateBlendState( &blendCopy, &blendState ) );
	}
	else if( FAILED( hr ) )
	{
		return hr;
	}

	s_blendStateCache[blendCopy] = blendState;
	return S_OK;
}

ALResult Tr2RenderStateEmulation::GetDepthStencilState( CComPtr<ID3D11DepthStencilState>& depthStencilState, const CComPtr<ID3D11Device>& device11 ) const
{
	auto cached = s_depthStencilStateCache.find( m_currentDepthStencil );

	if( cached != s_depthStencilStateCache.end() )
	{
		depthStencilState = cached->second;
		return S_OK;
	}

	const HRESULT hr = device11->CreateDepthStencilState( &m_currentDepthStencil, &depthStencilState );
	if( hr == D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS )
	{
		s_depthStencilStateCache.clear();
		CR_RETURN_HR(
			device11->CreateDepthStencilState( &m_currentDepthStencil,
											   &depthStencilState ) );
	}
	else if( FAILED( hr ) )
	{
		return hr;
	}

	s_depthStencilStateCache[m_currentDepthStencil] = depthStencilState;
	return S_OK;
}

ALResult Tr2RenderStateEmulation::GetRasterizerState( CComPtr<ID3D11RasterizerState>& rasterizerState, const CComPtr<ID3D11Device>& device11 ) const
{
	auto cached = s_rasterizerCache.find( m_currentRasterizer );

	if( cached != s_rasterizerCache.end() )
	{
		rasterizerState = cached->second;
		return S_OK;
	}

	const HRESULT hr = device11->CreateRasterizerState( &m_currentRasterizer,
														&rasterizerState );

	if( hr == D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS )
	{
		s_rasterizerCache.clear();
		CR_RETURN_HR( device11->CreateRasterizerState( &m_currentRasterizer,
													   &rasterizerState ) );
	}
	else if( FAILED( hr ) )
	{
		return hr;
	}

	s_rasterizerCache[m_currentRasterizer] = rasterizerState;
	return S_OK;
}

#endif //DX11?
