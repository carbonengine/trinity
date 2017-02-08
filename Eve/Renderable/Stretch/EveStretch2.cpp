////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "EveStretch2.h"
#include "Curves/TriCurveSet.h"
#include "Eve/EveUpdateContext.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Tr2PointLight.h"
#include "Shader/Tr2Effect.h"


namespace
{

	class StretchPerObjectData : public Tr2PerObjectData
	{
	public:
		virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
		{
			FillAndSetConstants( 
				*buffers[Tr2RenderContextEnum::VERTEX_SHADER], 
				m_data, 
				m_size, 
				Tr2RenderContextEnum::VERTEX_SHADER, 
				Tr2Renderer::GetPerObjectVSStartRegister(), 
				renderContext );
		}

		void* m_data;
		size_t m_size;
	};

	struct Vertex
	{
		float quadIndex;
		float cornerIndex;
	};


}

EveStretch2::EveStretch2( IRoot* lockObj )
	:m_source( 0.f, 0.f, 0.f ),
	m_destination( 0.f, 0.f, 0.f ),
	m_currentDestinationScale( 1.f ),
	m_destinationScale( 1.f ),
	m_visible( true ),
	m_quadCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{

}

bool EveStretch2::Initialize()
{
	PrepareResources();
	return true;
}

bool EveStretch2::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_quadCount ) )
	{
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();
	}
	return true;
}

void EveStretch2::SetDestObjectScale( float scale )
{
	m_destinationScale = scale;
	m_currentDestinationScale = scale;
}

void EveStretch2::StartMoving()
{
}

float EveStretch2::GetCurveDuration()
{
	float duration = 0;
	if( m_start )
	{
		duration = std::max( duration, m_start->GetMaxCurveDuration() );
	}
	if( m_loop )
	{
		duration = std::max( duration, m_loop->GetMaxCurveDuration() );
	}
	return duration;
}

void EveStretch2::StartFiring( float delay )
{
	if( m_start )
	{
		m_start->PlayFrom( -delay );
	}
	if( m_loop )
	{
		m_loop->PlayFrom( -delay );
	}
	if( m_end )
	{
		m_end->Stop();
	}
}

void EveStretch2::StopFiring()
{
	if( m_start )
	{
		m_start->Stop();
	}
	if( m_loop )
	{
		m_loop->Stop();
	}
	if( m_end )
	{
		m_end->Play();
	}
}

void EveStretch2::SetFiringTransform( const Matrix& source, const Vector3& dest )
{
	m_source = source.GetTranslation();
	m_destination = dest;
}

void EveStretch2::SetFiringTransform( const Vector3& source, const Vector3& dest )
{
	m_source = source;
	m_destination = dest;
}

void EveStretch2::DisplayEndPoints( bool displaySource, bool displayDest )
{
	m_currentDestinationScale = displayDest ? m_destinationScale : 0;
}

void EveStretch2::Update( EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	if( m_start )
	{
		m_start->Update( TimeAsDouble( time ) );
	}
	if( m_loop )
	{
		m_loop->Update( TimeAsDouble( time ) );
	}
	if( m_end )
	{
		m_end->Update( TimeAsDouble( time ) );
	}
}

void EveStretch2::UpdateInactive( EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	if( m_start )
	{
		m_start->Update( TimeAsDouble( time ) );
	}
	if( m_loop )
	{
		m_loop->Update( TimeAsDouble( time ) );
	}
	if( m_end )
	{
		m_end->Update( TimeAsDouble( time ) );
	}
}

void EveStretch2::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
}

void EveStretch2::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_visible )
	{
		renderables.push_back( this );
	}
}


Tr2PerObjectData* EveStretch2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	auto data = accumulator->Allocate<StretchPerObjectData>();
	if( data )
	{
		data->m_data = &m_source;
		data->m_size = 2 * sizeof( Vector4 );
	}
	return data;
}

void EveStretch2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( batchType == TRIBATCHTYPE_ADDITIVE && m_effect && m_effect->GetShaderStateInterface() && m_vb.IsValid() )
	{
		TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
		if( batch )
		{
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( m_effect );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}
	}
}

bool EveStretch2::HasTransparentBatches()
{
	return false;
}

float EveStretch2::GetSortValue()
{
	return 0;
}

void EveStretch2::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vb, 0, sizeof( Vertex ) );
	renderContext.m_esm.ApplyIndexBuffer( *Tr2Renderer::GetQuadListIndexBuffer( m_quadCount ) );
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( 4 * m_quadCount, 0, 2 * m_quadCount );
}

void EveStretch2::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	if( m_vb.GetMemoryClass() & s )
	{
		m_vb.Destroy();
	}
}

bool EveStretch2::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION );

	m_vertexDeclHandle = renderContext.m_esm.GetVertexDeclarationHandle( def );

	if( m_quadCount && !m_vb.IsValid() )
	{
		std::vector<Vertex> data( m_quadCount * 4 );
		for( uint32_t i = 0; i < m_quadCount; ++i )
		{
			for( uint32_t j = 0; j < 4; ++j )
			{
				data[i * 4 + j].quadIndex = float( i );
				data[i * 4 + j].cornerIndex = float( j );
			}
		}
		m_vb.Create( m_quadCount * 4 * sizeof( Vertex ), Tr2RenderContextEnum::USAGE_IMMUTABLE, &data[0], renderContext );
	}
	return true;
}

void EveStretch2::GetLights( Tr2LightManager& lightManager ) const
{
	if( m_sourceLight )
	{
		m_sourceLight->AddLight( lightManager, XMMatrixTranslation( m_source.x, m_source.y, m_source.z ), 1.0f );
	}
	if( m_destinationLight )
	{
		m_destinationLight->AddLight( lightManager, XMMatrixTranslation( m_destination.x, m_destination.y, m_destination.z ), m_currentDestinationScale );
	}
}