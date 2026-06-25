// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildParticleSphere.h"
#include "Tr2InstancedMesh.h"
#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"
#include "TriFrustum.h"
#include "ITr2AttributeGenerator.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Utilities/BoundingSphere.h"


namespace
{

class EveChildParticleSpherePerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		static const unsigned mask = ( 1 << Tr2RenderContextEnum::VERTEX_SHADER );
		FillAndSetConstants(
			*buffers[m_bufferType],
			&m_data,
			sizeof( m_data ),
			mask,
			m_register,
			renderContext );
	}

	struct Data
	{
		Matrix m_world;
		Matrix m_worldInverseTranspose;
	} m_data;

	int32_t m_bufferType;
	int32_t m_register;
};
}


// -----------------------------------------------------------------------------
EveChildParticleSphere::EveChildParticleSphere( IRoot* lockobj ) :
	PARENTLOCK( m_generators ),
	m_worldTransform( IdentityMatrix() ),
	m_boundingSphere( 0, 0, 0, 500 ),
	m_bindStatus( BIND_PENDING ),
	m_previousOrigin( 500, 0, 0 ),
	m_radius( 500.f ),
	m_movementScale( 1.f ),
	m_maxSpeed( 0.f ),
	m_egoVelocity( 0, 0, 0 ),
	m_egoSpeed( 0 ),
	m_positionShift( 0 ),
	m_positionShiftMax( 0 ),
	m_positionShiftMin( 100 ),
	m_positionShiftNormalized( 0 ),
	m_positionShiftIncreaseSpeed( 1000.f ),
	m_positionShiftDecreaseSpeed( 1000.f ),
	m_positionElement(),
	m_velocityElement(),
	m_lifetimeElement(),
	m_useSpaceObjectData( true ),
	m_display( true )
{
}

// --------------------------------------------------------------------------------
const char* EveChildParticleSphere::GetName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------
void EveChildParticleSphere::SetName( const char* name )
{
	m_name = name;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_mesh && m_particleSystem && m_display && m_hasUpdated )
	{
		m_particleSystem->SortParticles();
		renderables.push_back( this );
	}
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix&, Tr2Lod )
{
	auto& frustum = updateContext.GetFrustum();
	auto isVisible = !( !m_display || !frustum.IsSphereVisible( &m_boundingSphere ) );
	if( isVisible && m_particleSystem )
	{
		m_particleSystem->UpdateViewDependentData( &frustum, m_worldTransform );
	}
}

// -----------------------------------------------------------------------------
bool EveChildParticleSphere::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery ) const
{
	sphere = m_boundingSphere;
	return true;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
	CheckBinding();

	Vector3 velocity( 0, 0, 0 );
	if( auto ballpark = updateContext.GetBallpark() )
	{
		ballpark->DeltaVel( &velocity, updateContext.GetTime() );
	}
	m_egoSpeed = Length( velocity );
	m_egoVelocity = velocity;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix parent = params.localToWorldTransform;

	m_worldTransform = TranslationMatrix( parent.GetTranslation() );

	m_boundingSphere = Vector4( 0, 0, 0, m_radius );
	BoundingSphereTransform( m_worldTransform, m_boundingSphere );

	Update( updateContext );
	m_hasUpdated = true;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::Setup( const Vector3*, const Quaternion*, const Vector3*, Tr2Lod )
{
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_display || !m_mesh || !m_mesh->GetDisplay() )
	{
		return;
	}

	if( Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType ) )
	{
		m_mesh->GetBatches( batches, areas, perObjectData );
	}
}

// -----------------------------------------------------------------------------
bool EveChildParticleSphere::HasTransparentBatches()
{
	if( !m_mesh || !m_particleSystem )
	{
		return false;
	}

	return !m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty();
}

// -----------------------------------------------------------------------------
float EveChildParticleSphere::GetSortValue()
{
	return 0;
}

// -----------------------------------------------------------------------------
Tr2PerObjectData* EveChildParticleSphere::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveChildParticleSpherePerObjectData* data = accumulator->Allocate<EveChildParticleSpherePerObjectData>();

	if( !data )
	{
		return NULL;
	}

	data->m_bufferType = Tr2RenderContextEnum::VERTEX_SHADER;
	data->m_register = Tr2Renderer::GetPerObjectVSStartRegister();
	data->m_data.m_world = Transpose( m_worldTransform );
	data->m_data.m_worldInverseTranspose = Inverse( m_worldTransform );
	return data;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::Update( const EveUpdateContext& updateContext )
{
	auto delta = updateContext.GetDeltaT();

	auto originShift = updateContext.GetOriginShift();
	if( m_maxSpeed > 0.f && Length( originShift ) > m_maxSpeed * delta )
	{
		originShift = Normalize( originShift ) * ( m_maxSpeed * delta );
	}
	originShift *= m_movementScale;

	if( m_bindStatus == BIND_VALID )
	{
		auto worldInv = Inverse( m_worldTransform );
		auto previousReferencePosition = TransformCoord( m_previousOrigin + originShift, worldInv );

		auto shift = Length( TransformCoord( m_previousOrigin, worldInv ) );
		m_positionShift = shift;
		auto positionShiftNormalized = std::min( std::max( ( m_positionShift - m_positionShiftMin ) / ( m_positionShiftMax - m_positionShiftMin ), 0.f ), 1.f );
		if( positionShiftNormalized < m_positionShiftNormalized )
		{
			m_positionShiftNormalized = std::max( positionShiftNormalized, m_positionShiftNormalized - std::max( 0.f, delta * m_positionShiftDecreaseSpeed ) );
		}
		else
		{
			m_positionShiftNormalized = std::min( positionShiftNormalized, m_positionShiftNormalized + std::max( 0.f, delta * m_positionShiftIncreaseSpeed ) );
		}

		ApplyConstraint( previousReferencePosition, Normalize( m_egoVelocity ) );
		AddParticles( previousReferencePosition, Normalize( m_egoVelocity ) );
	}

	m_previousOrigin = m_worldTransform.GetTranslation();

	if( m_particleSystem )
	{
		m_particleSystem->Update( ITr2GenericEmitter::UpdateArguments(
			updateContext.GetTime(),
			updateContext.GetGpuParticleSystem(),
			IdentityMatrix(),
			updateContext.GetOriginShift() ) );
	}
}

// -----------------------------------------------------------------------------
bool EveChildParticleSphere::CheckBinding()
{
	if( m_bindStatus == BIND_PENDING )
	{
		Refresh();
	}
	return m_bindStatus == BIND_VALID;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::Refresh()
{
	if( !m_particleSystem )
	{
		return;
	}

	m_particleSystem->UpdateElementDeclaration();

	const Tr2ParticleElementDataMap& declaration = m_particleSystem->GetElementDeclaration();
	std::set<Tr2ParticleElementDeclarationName> elements;

	auto Bind = [&]( Tr2ParticleElementDeclarationName::Type type ) -> Tr2ParticleElementData {
		Tr2ParticleElementDeclarationName element( type );
		if( elements.find( element ) == elements.end() )
		{
			auto i = declaration.find( element );
			if( i == declaration.end() )
			{
				return Tr2ParticleElementData::Invalid();
			}
			else
			{
				elements.insert( element );
				return i->second;
			}
		}
		else
		{
			return Tr2ParticleElementData::Invalid();
		}
	};

	for( auto it = begin( m_generators ); it != end( m_generators ); ++it )
	{
		if( !( *it )->Bind( declaration, elements ) )
		{
			m_bindStatus = BIND_INVALID;
			return;
		}
	}

	m_positionElement = Bind( Tr2ParticleElementDeclarationName::POSITION );
	m_velocityElement = Bind( Tr2ParticleElementDeclarationName::VELOCITY );
	m_lifetimeElement = Bind( Tr2ParticleElementDeclarationName::LIFETIME );

	for( auto it = declaration.begin(); it != declaration.end(); ++it )
	{
		if( elements.find( it->first ) == elements.end() )
		{
			CCP_LOGERR( "Unbound particle element %s in a near field", it->first.GetName().c_str() );
			m_bindStatus = BIND_INVALID;
			return;
		}
	}

	m_bindStatus = BIND_VALID;

	m_previousOrigin = Vector3( m_radius, 0, 0 );
	return;
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::ApplyConstraint( const Vector3& previousReferencePosition, const Vector3& velocityDirection )
{
	if( !m_particleSystem )
	{
		return;
	}

	auto constraint = [&]( float** particles, unsigned* strides, unsigned count ) {
		Tr2ParticleStreamIterator<XMFLOAT3> position( particles, strides, m_positionElement );
		Tr2ParticleStreamIterator<XMFLOAT3> velocity( particles, strides, m_velocityElement );
		Tr2ParticleStreamIterator<float> lifetime( particles, strides, m_lifetimeElement );

		XMVECTOR originShift = previousReferencePosition;
		XMVECTOR constVelocity = velocityDirection;
		XMVECTOR oneOverRadius2 = XMVectorReplicate( 1.f / ( m_radius * m_radius ) );

		for( unsigned i = 0; i < count; ++i, ++position, ++velocity, ++lifetime )
		{
			XMVECTOR localPosition = XMVectorAdd( XMLoadFloat3( position ), originShift );
			XMStoreFloat3( position, localPosition );

			XMStoreFloat(
				lifetime,
				XMVectorMultiply( XMVector3LengthSq( localPosition ), oneOverRadius2 ) );

			XMStoreFloat3( velocity, constVelocity );

			if( *lifetime.Get() >= 1 )
			{
				float* particle[Tr2ParticleElementData::COUNT];
				for( unsigned j = 0; j < Tr2ParticleElementData::COUNT; ++j )
				{
					particle[j] = particles[j] + strides[j] * i;
				}
				FillParticleData( particle, previousReferencePosition, velocityDirection );
			}
		}
	};

	m_particleSystem->ApplyConstraint( constraint );
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::FillParticleData( float** particle, const Vector3& previousReferencePosition, const Vector3& velocityDirection )
{
	auto randf = []() {
		return float( rand() ) / RAND_MAX;
	};

	if( m_positionElement.m_bufferType != Tr2ParticleElementData::COUNT )
	{
		float distance;
		Vector3 localPosition;
		for( int32_t i = 0; i < 10; ++i )
		{
			localPosition = Normalize( Vector3( randf() - 0.5f, randf() - 0.5f, randf() - 0.5f ) );
			if( i == 9 || previousReferencePosition == Vector3( 0, 0, 0 ) )
			{
				distance = m_radius * pow( randf(), 0.5f );
				localPosition *= distance;
			}
			else
			{
				float minDistance = std::max( 0.f, m_radius - Length( previousReferencePosition ) );
				distance = minDistance + pow( randf(), 0.5f ) * ( m_radius - minDistance );
				localPosition *= distance;
				if( Length( localPosition - previousReferencePosition ) < m_radius )
				{
					continue;
				}
			}
			break;
		}
		auto position = reinterpret_cast<Vector3*>( particle[m_positionElement.m_bufferType] + m_positionElement.m_offset );
		*position = localPosition;
		if( m_lifetimeElement.m_bufferType != Tr2ParticleElementData::COUNT )
		{
			auto lifetime = particle[m_lifetimeElement.m_bufferType] + m_lifetimeElement.m_offset;
			lifetime[0] = std::min( 1.f, distance * distance / m_radius / m_radius );
			lifetime[1] = std::numeric_limits<float>::max() / 10;
		}
	}
	else if( m_lifetimeElement.m_bufferType != Tr2ParticleElementData::COUNT )
	{
		auto lifetime = particle[m_lifetimeElement.m_bufferType] + m_lifetimeElement.m_offset;
		*lifetime = 1;
	}
	if( m_velocityElement.m_bufferType != Tr2ParticleElementData::COUNT )
	{
		auto velocity = reinterpret_cast<Vector3*>( particle[m_velocityElement.m_bufferType] + m_velocityElement.m_offset );
		*velocity = velocityDirection;
	}

	for( auto it = begin( m_generators ); it != end( m_generators ); ++it )
	{
		( *it )->Generate( nullptr, nullptr, particle );
	}
}

// -----------------------------------------------------------------------------
void EveChildParticleSphere::AddParticles( const Vector3& previousReferencePosition, const Vector3& velocityDirection )
{
	if( !m_particleSystem )
	{
		return;
	}

	auto randf = []() {
		return float( rand() ) / RAND_MAX;
	};

	float* particle[Tr2ParticleElementData::COUNT];
	while( m_particleSystem->InsertParticle( particle ) )
	{
		FillParticleData( particle, previousReferencePosition, velocityDirection );
		m_particleSystem->DoneInsertingParticle();
	}
}