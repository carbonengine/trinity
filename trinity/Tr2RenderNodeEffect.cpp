// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderNodeEffect.h"
#include "Shader/Tr2Effect.h"
#include "Tr2GpuResourcePool.h"
#include "Tr2Renderer.h"


Tr2RenderNodeEffect::Tr2RenderNodeEffect( IRoot* lockobj ) :
	PARENTLOCK( m_inputNodes )
{
}

bool Tr2RenderNodeEffect::Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime )
{
	if( destDimensions.size == 0 )
	{
		CCP_ASSERT_M( false, "Tr2RenderNodeEffect requires at least one destination texture" );
		return false;
	}
	if( outputs.size > 0 )
	{
		CCP_LOGERR( "Tr2RenderNodeEffect does not support named outputs" );
		return false;
	}
	if( !m_effect || !m_effect->GetShaderStateInterface() )
	{
		return false;
	}
	if( m_viewport && ( m_viewport->width <= 0 || m_viewport->height <= 0 ) )
	{
		return false;
	}
	for( auto& source : m_sources )
	{
		Tr2BitmapDimensions dim = *destDimensions.data;
		if( m_viewport )
		{
			dim = Tr2BitmapDimensions( std::max( 0, m_viewport->width ), std::max( 0, m_viewport->height ), 1, destDimensions.data->GetFormat() );
		}

		if( !source.node->Validate( { &dim, 1 }, { source.outputNames.data(), source.outputNames.size() }, realTime, simTime ) )
		{
			return false;
		}
	}
	return true;
}

void Tr2RenderNodeEffect::Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	std::vector<Tr2GpuResourcePool::Texture> tempTextures;
	for( auto& source : m_sources )
	{
		char nameBuffer[256];
		sprintf_s( nameBuffer, "Tr2RenderNodeUnaryEffect_Source_%u", uint32_t( tempTextures.size() ) );

		auto width = destinations.data->GetWidth();
		auto height = destinations.data->GetHeight();
		if( m_viewport )
		{
			width = std::max( 0, m_viewport->width );
			height = std::max( 0, m_viewport->height );
		}

		auto sourceTarget = GetGlobalGpuResourcePool().GetTempTexture(
			nameBuffer,
			width,
			height,
			destinations.data->GetFormat(),
			Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET );

		source.node->Execute( { &sourceTarget.Get(), 1 }, { source.outputs.data(), source.outputs.size() }, realTime, simTime, rootTimer, renderContext );

		for( auto& param : source.params )
		{
			if( param.outputName.empty() )
			{
				m_effect->SetParameter( param.paramName, sourceTarget );
			}
			else
			{
				m_effect->SetParameter( param.paramName, source.outputs[param.outputIndex].texture );
			}
		}
		tempTextures.push_back( sourceTarget );
	}
	for( size_t i = 0; i < destinations.size; ++i )
	{
		renderContext.m_esm.SetRenderTarget( uint32_t( i ), destinations[i] );
	}
	renderContext.m_esm.SetDepthStencilBuffer( {} );
	renderContext.m_esm.SetFullScreenViewport();
	if( m_viewport )
	{
		renderContext.m_esm.SetViewport( *m_viewport );
	}
	renderContext.m_esm.ApplyStandardStates( m_renderingMode );
	Tr2Renderer::DrawTexture( renderContext, m_effect, { 0, 0 }, { 1, 1 } );
	for( auto& source : m_sources )
	{
		for( auto& param : source.params )
		{
			m_effect->SetParameter( param.paramName, Tr2TextureAL{} );
		}
	}
	for( auto& source : m_sources )
	{
		for( auto& output : source.outputs )
		{
			output.texture = {};
		}
	}
}

void Tr2RenderNodeEffect::AddSource( const char* name, ITr2RenderNode* source, const char* outputName )
{
	auto found = find_if( begin( m_sources ), end( m_sources ), [&]( auto& x ) { return x.node == source; } );
	if( found == end( m_sources ) )
	{
		m_sources.push_back( { source } );
		found = m_sources.end() - 1;
	}
	found->params.push_back( { BlueSharedString( name ), BlueSharedString( outputName ) } );

	found->outputNames.clear();
	found->outputs.clear();
	for( auto& param : found->params )
	{
		if( !param.outputName.empty() )
		{
			auto foundOutput = find( begin( found->outputNames ), end( found->outputNames ), param.outputName );
			if( foundOutput == end( found->outputNames ) )
			{
				found->outputNames.push_back( param.outputName );
				found->outputs.push_back( { param.outputName } );
			}
		}
	}

	m_inputNodes.Append( source );
}
