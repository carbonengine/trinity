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
#include "Lights/Tr2PointLight.h"
#include "Shader/Tr2Effect.h"
#include "Particle/Tr2GpuSharedEmitter.h"
#include "TriObserverLocal.h"
#include "Tr2Renderer.h"
#include "Resources/TriGeometryRes.h"


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
			FillAndSetConstants(
				*buffers[Tr2RenderContextEnum::PIXEL_SHADER],
				m_data,
				m_size,
				Tr2RenderContextEnum::PIXEL_SHADER,
				Tr2Renderer::GetPerObjectPSStartRegister(),
				renderContext );
		}

		void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override
		{
			writer.SetPerObjectData( Tr2RenderContextEnum::VERTEX_SHADER, m_data, m_size );
			writer.SetPerObjectData( Tr2RenderContextEnum::PIXEL_SHADER, m_data, m_size );
		}

		void* m_data;
		size_t m_size;
	};

	struct Vertex
	{
		float quadIndex;
		float cornerIndex;
	};

	const uint32_t MAX_QUAD_COUNT = 128;

	ALResult GetEveStretch2Quads( Tr2SuballocatedBuffer::Allocation& vb, Tr2PrimaryRenderContext& renderContext )
	{
		std::vector<Vertex> data( MAX_QUAD_COUNT * 4 );
		for( uint32_t i = 0; i < MAX_QUAD_COUNT; ++i )
		{
			for( uint32_t j = 0; j < 4; ++j )
			{
				data[i * 4 + j].quadIndex = float( i );
				data[i * 4 + j].cornerIndex = float( j );
			}
		}

		return g_sharedBuffer.Allocate( sizeof( Vertex ), MAX_QUAD_COUNT * 4, data.data(), renderContext, vb );
	}

}

EveStretch2::EveStretch2( IRoot* lockObj )
	:m_source( 0.f, 0.f, 0.f ),
	m_destination( 0.f, 0.f, 0.f ),
	m_currentDestinationScale( 1.f ),
	m_destinationScale( 1.f ),
	m_visible( true ),
	m_isInFrustum( true ),
	m_quadCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_startTime( 0 ),
	m_intensity( 1 ),
	m_boundingRadius( 100 ),
	m_sourceTransform( IdentityMatrix() ),
	m_destinationTransform( IdentityMatrix() ),
	m_vb( BlueSharedString( "EveStretch2VB" ), &GetEveStretch2Quads )
{
	m_effectData[0] = Vector4( 0, 0, 0, float( rand() ) / RAND_MAX );
	m_effectData[0] = Vector4( 1, 0, 0, 0 );
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
		CCP_ASSERT( m_quadCount <= MAX_QUAD_COUNT );
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
	m_effectData[0].w = float( rand() ) / RAND_MAX;
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

void EveStretch2::SetDisplay( bool display )
{
	bool prev = m_visible;
	m_visible = display;
	if( m_visible != prev )
	{
		ReRegister();	
	}
}

void EveStretch2::SetIntensity( float intensity )
{
	float prev = m_intensity;
	m_intensity = intensity;

	if( ( prev == 0 && intensity > 0 ) || (prev > 0 && intensity == 0 ) )
	{
		ReRegister();
	}
}

void EveStretch2::UpdateEffectSync( const EveUpdateContext& updateContext )
{
	// do nothing here
}

void EveStretch2::UpdateEffectAsync( const EveUpdateContext& updateContext )
{
	Update( updateContext );
}

void EveStretch2::Update( const EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	if( m_startTime == 0 )
	{
		m_startTime = time;
	}
	m_effectData[0].x = m_effectData[0].y = m_effectData[0].z = 0;
	if( m_start )
	{
		m_start->Update( TimeAsDouble( time - m_startTime ) );
		m_effectData[0].x = float( m_start->GetScaledTime() );
	}
	if( m_loop )
	{
		m_loop->Update( TimeAsDouble( time - m_startTime ) );
		m_effectData[0].y = float( m_loop->GetScaledTime() );
	}
	if( m_end )
	{
		m_end->Update( TimeAsDouble( time - m_startTime ) );
		m_effectData[0].z = float( m_end->GetScaledTime() );
	}

	GetEndPointTransforms( m_sourceTransform, m_destinationTransform );
	if( m_sourceObserver )
	{
		m_sourceObserver->Update( m_sourceTransform );
	}
	if( m_destinationObserver )
	{
		m_destinationObserver->Update( m_destinationTransform );
	}
	if( m_sourceEmitter )
	{
		ITr2GenericEmitter::UpdateArguments args(
			updateContext.GetTime(),
			updateContext.GetGpuParticleSystem(),
			m_sourceTransform,
			updateContext.GetOriginShift() );
		m_sourceEmitter->Update( args );
	}
	if( m_destinationEmitter )
	{
		ITr2GenericEmitter::UpdateArguments args(
			updateContext.GetTime(),
			updateContext.GetGpuParticleSystem(),
			m_destinationTransform,
			updateContext.GetOriginShift() );
		m_destinationEmitter->Update( args );
	}
}

void EveStretch2::GetEndPointTransforms( Matrix& source, Matrix& destination ) const
{
	auto direction = m_destination - m_source;
	float x = std::abs( direction.x );
	float y = std::abs( direction.y );
	float z = std::abs( direction.z );
	Vector3 up;
	if( x < y && x < z )
	{
		up = Vector3( 1, 0, 0 );
	}
	else if( y < x && y < z )
	{
		up = Vector3( 0, 1, 0 );
	}
	else
	{
		up = Vector3( 0, 0, 1 );
	}

	source = IdentityMatrix();
	source.GetZ() = XMVector3Normalize( direction );
	source.GetX() = XMVector3Normalize( XMVector3Cross( up, source.GetZ() ) );
	source.GetY() = XMVector3Cross( source.GetX(), source.GetZ() );
	source.GetTranslation() = m_source;

	destination = source;
	destination.GetZ() = -destination.GetZ();
	destination.GetX() = -destination.GetX();
	destination.GetTranslation() = m_destination;
}

void EveStretch2::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	if( m_visible && m_intensity > 0 )
	{
		CcpMath::AxisAlignedBox box( Vector3( -m_boundingRadius, -m_boundingRadius, -m_boundingRadius ), Vector3( m_boundingRadius, m_boundingRadius, Length( m_destination - m_source ) + m_boundingRadius ) );
		box.Transform( m_sourceTransform );

		m_isInFrustum = updateContext.GetFrustum().IsBoxVisible( box );
	}
	else
	{
		m_isInFrustum = false;
	}
}

void EveStretch2::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_visible && m_intensity > 0 && m_isInFrustum )
	{
		renderables.push_back( this );
	}
}


Tr2PerObjectData* EveStretch2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	auto data = accumulator->Allocate<StretchPerObjectData>();
	if( data )
	{
		m_effectData[1].x = m_intensity;
		data->m_data = &m_source;
		data->m_size = 4 * sizeof( Vector4 );
	}
	return data;
}

void EveStretch2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType == TRIBATCHTYPE_ADDITIVE && m_effect && m_effect->GetShaderStateInterface() && m_vb.GetSharedResource().IsValid() )
	{
		auto& ib = Tr2Renderer::GetQuadListIndexBuffer();
		if( !ib.IsValid() )
		{
			return;
		}

		auto& vb = m_vb.GetSharedResource();

		Tr2RenderBatch batch;
		batch.SetMaterial( m_effect );
		batch.SetPerObjectData( perObjectData );
		batch.SetGeometry( m_vertexDeclHandle, vb, ib );
		batch.SetDrawIndexedInstanced( 6 * m_quadCount, 1, ib.GetStartIndex(), vb.GetOffset() / vb.GetStride(), 0 );

		batches->Commit( batch );
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

void EveStretch2::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

bool EveStretch2::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION );

	m_vertexDeclHandle = renderContext.m_esm.GetVertexDeclarationHandle( def );

	Tr2Renderer::ReserveQuadListIndexBuffer( m_quadCount );
	return true;
}

void EveStretch2::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	bool isActive = m_visible && m_intensity > 0;
	bool hasLights = m_sourceLight || m_destinationLight;
	if( registry && isActive && hasLights )
	{
		registry->RegisterComponent<ITr2LightOwner>( this );
	}
}

void EveStretch2::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_visible || m_intensity == 0)
	{
		return;
	}
	if( !m_sourceLight && !m_destinationLight )
	{
		return;
	}
	if( m_sourceLight )
	{
		m_sourceLight->AddLight( lightManager, m_sourceTransform, 1.0f );
	}
	if( m_destinationLight )
	{
		m_destinationLight->AddLight( lightManager, m_destinationTransform, m_currentDestinationScale );
	}
}

void EveStretch2::GetDebugOptions( Tr2DebugRendererOptions& options ) 
{
	options.insert( "Stretch bounds" );
}

void EveStretch2::RenderDebugInfo( ITr2DebugRenderer2& renderer ) 
{
	if( renderer.HasOption( this, "Stretch bounds" ) )
	{
		CcpMath::AxisAlignedBox box( Vector3( -m_boundingRadius, -m_boundingRadius, -m_boundingRadius ), Vector3( m_boundingRadius, m_boundingRadius, Length( m_destination - m_source ) + m_boundingRadius ) );

		renderer.DrawBox( this, m_sourceTransform, box.m_min, box.m_max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
		box.Transform( m_sourceTransform );
		renderer.DrawBox( this, box.m_min, box.m_max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
	}
}
