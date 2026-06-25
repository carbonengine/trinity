// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2RenderStateEmulationDx11_h_
#define Tr2RenderStateEmulationDx11_h_

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "../ALResult.h"

// -------------------------------------------------------------
// Description:
//   Keeps track of all currently set renderstates that need to
//   emulated, and also hosts a static cache of already created
//   D3D objects to implement the emulation.  Moving it out here
//   makes it a little easier to see what's going on, also multiple
//   renderContexts (secondary contexts) can share the same cached
//   objects.
// -------------------------------------------------------------
struct Tr2RenderStateEmulation
{
	// sum of current SetRenderState calls for blending, may or may not be what's on the device.  We only
	// track one render target blend because DX9 can't do independent blending, so we don't need to support it.  In the
	// future we want client code to directly build and set their own blend state blocks anyway.
	D3D11_BLEND_DESC m_currentBlend;
	D3D11_DEPTH_STENCIL_DESC m_currentDepthStencil;
	D3D11_RASTERIZER_DESC m_currentRasterizer;
	uint32_t m_currentStencilRef;
	bool m_separateAlphaBlendEnabled;


	ALResult GetBlendState( CComPtr<ID3D11BlendState>& blendState, const CComPtr<ID3D11Device>& device11 ) const;
	ALResult GetDepthStencilState( CComPtr<ID3D11DepthStencilState>& depthStencilState, const CComPtr<ID3D11Device>& device11 ) const;
	ALResult GetRasterizerState( CComPtr<ID3D11RasterizerState>& rasterizerState, const CComPtr<ID3D11Device>& device11 ) const;



	struct TBlendHash
	{
		long operator()( const D3D11_BLEND_DESC& bd ) const;
	};
	struct TBlendEq
	{
		bool operator()( const D3D11_BLEND_DESC& bd1, const D3D11_BLEND_DESC& bd2 ) const;
	};
	static TrackableStdUnorderedMap<D3D11_BLEND_DESC, CComPtr<ID3D11BlendState>, TBlendHash, TBlendEq> s_blendStateCache;

	struct TDepthStencilHash
	{
		long operator()( const D3D11_DEPTH_STENCIL_DESC& dsd ) const;
	};
	struct TDepthStencilEq
	{
		bool operator()( const D3D11_DEPTH_STENCIL_DESC& dsd1, const D3D11_DEPTH_STENCIL_DESC& dsd2 ) const;
	};
	static TrackableStdUnorderedMap<D3D11_DEPTH_STENCIL_DESC, CComPtr<ID3D11DepthStencilState>, TDepthStencilHash, TDepthStencilEq> s_depthStencilStateCache;

	struct TRasterizerHash
	{
		long operator()( const D3D11_RASTERIZER_DESC& rd ) const;
	};
	struct TRasterizerEq
	{
		bool operator()( const D3D11_RASTERIZER_DESC& rd1, const D3D11_RASTERIZER_DESC& rd2 ) const;
	};
	static TrackableStdUnorderedMap<D3D11_RASTERIZER_DESC, CComPtr<ID3D11RasterizerState>, TRasterizerHash, TRasterizerEq> s_rasterizerCache;
};

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2RenderStateEmulationDx11_h_
