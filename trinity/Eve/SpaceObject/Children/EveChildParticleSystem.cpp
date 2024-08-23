////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildParticleSystem.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Resources/TriGeometryRes.h"
#include "Eve/EveTransform.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2InstancedMesh.h"
#include "TriFrustum.h"
#include "Eve/EveUpdateContext.h"

#include "Particle/Tr2ParticleSystem.h"
#include "Particle/ITr2GenericEmitter.h"

#include "Utilities/BoundingSphere.h"


extern float g_eveSpaceSceneLODFactor;


EveChildParticleSystem::EveChildParticleSystem( IRoot* lockobj ):
	EveChildTransform(),
	PARENTLOCK( m_particleEmitters ),
	PARENTLOCK( m_particleSystems ),
	PARENTLOCK( m_transformModifiers ),
	m_boundingSphere( 0, 0, 0, -1 ),
	m_lodSphere( 0, 0, 0, -1 ),
	m_display( true ),
	m_isVisible( true ),
	m_useDynamicLod( false ),
	m_lodFactorMedium( 0.25 ),
	m_lodFactorLow( 0.125 ),
	m_lodClampLow( 5 ),
	m_lodSphereRadius( 0.0f ),
	m_minScreenSize( 0.0f ),
	m_currentScreenSize( -1 ),
	m_reflectionMode( EntityComponents::REFLECT_NEVER )
{
}

EveChildParticleSystem::~EveChildParticleSystem()
{
}

bool EveChildParticleSystem::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}
	return true;
}


// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildParticleSystem::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		if( EntityComponents::ShouldReflect( m_reflectionMode ) && m_display )
		{
			registry->RegisterComponent<ITr2Renderable>( this );
		}
	}
}


bool EveChildParticleSystem::OnModified( Be::Var* value)
{
	if( IsMatch(value, m_reflectionMode) || IsMatch(value, m_display))
	{
		ReRegister();
	}
	return true;
}

const char* EveChildParticleSystem::GetName() const
{
	return m_name.c_str();
}

void EveChildParticleSystem::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

// --------------------------------------------------------------------------------
// Description:
//   Setup function to set data from outside, in this case just pass it to
//   function of base class
// --------------------------------------------------------------------------------
void EveChildParticleSystem::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildParticleSystem::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_isVisible = m_display && frustum.IsSphereVisible( &m_boundingSphere );
	if( m_isVisible )
	{
		m_currentScreenSize = frustum.GetPixelSizeAccrossEst( &m_lodSphere );
		m_isVisible &= m_currentScreenSize >= m_minScreenSize * g_eveSpaceSceneLODFactor;
	}
	else
	{
		m_currentScreenSize = -1;
	}

	if( m_isVisible )
	{
		for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
		{
			(*it)->UpdateViewDependentData( &frustum, m_worldTransform );
		}
	}
}

bool EveChildParticleSystem::IsVisible( const TriFrustum& frustum ) const
{
	return frustum.IsSphereVisible( &m_boundingSphere ) && frustum.GetPixelSizeAccrossEst( &m_lodSphere ) >= m_minScreenSize * g_eveSpaceSceneLODFactor;
}


void EveChildParticleSystem::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_isVisible )
	{
		return;
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->SortParticles();
	}

	renderables.push_back( this );
}

bool EveChildParticleSystem::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_boundingSphere.w == -1 ) 
	{
		return false;
	}
	sphere = m_boundingSphere;
	return true;
}

bool EveChildParticleSystem::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void EveChildParticleSystem::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_display && m_mesh && m_mesh->GetAreas(batchType)->size() != 0 )
	{
		if( reason == Tr2RenderReason::TR2RENDERREASON_REFLECTION )
		{
			// rendering into the reflection cubemap is lefthanded, so we need to reverse all the areas 
			auto areas = m_mesh->GetAreas( batchType );
			for( auto it = areas->begin(); it != areas->end(); ++it )
			{
				( *it )->SetReversed( !( *it )->IsReversed() );
			}
		}

		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );

		if( reason == Tr2RenderReason::TR2RENDERREASON_REFLECTION )
		{
			// reverse them again!
			auto areas = m_mesh->GetAreas( batchType );
			for( auto it = areas->begin(); it != areas->end(); ++it )
			{
				( *it )->SetReversed( !( *it )->IsReversed() );
			}
		}
	}
}

float EveChildParticleSystem::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance;
}

Tr2PerObjectData* EveChildParticleSystem::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveBasicPerObjectData* data = accumulator->Allocate<EveBasicPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	// column_major for shaders
	data->m_world = Transpose( m_worldTransform );
	data->m_worldInverseTranspose = Inverse( m_worldTransform );

	return data;
}

void EveChildParticleSystem::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
}

void EveChildParticleSystem::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform;
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if ( params.spaceObjectParent )
	{
		params.spaceObjectParent ->GetLocalToWorldTransform( localToWorldTransform );
	}
	else 
	{
		return;
	}
	UpdateTransform( localToWorldTransform );
	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); ++it )
	{
		m_worldTransform = (*it)->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}
	
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( m_boundingSphere, minBounds, maxBounds, &m_worldTransform );
	}

	if( m_lodSphereRadius > 0 )
	{
		m_lodSphere.x = 0.0f;
		m_lodSphere.y = 0.0f;
		m_lodSphere.z = 0.0f;		

		m_lodSphere.w = m_lodSphereRadius;
		BoundingSphereTransform( m_worldTransform, m_lodSphere );
	}
	else
	{
		m_lodSphere.w = -1.0f;
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateTransform( m_worldTransform );
	}
	if( !m_particleEmitters.empty() )
	{
		float emitCountFactor = 1.f;
		if( !params.isVisible || !m_display )
		{
			emitCountFactor = 0;
		}
		else
		{
			if( m_minScreenSize > 0.f && m_lodSphereRadius > 0.f && m_reflectionMode == EntityComponents::REFLECT_NEVER )
			{
				TriFrustum frustum;
				frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &Tr2Renderer::GetViewPosition(), &Tr2Renderer::GetProjectionRawTransform(), Tr2Renderer::GetViewport() );
				if( frustum.GetPixelSizeAccrossEst( &m_lodSphere ) < m_minScreenSize * g_eveSpaceSceneLODFactor )
				{
					emitCountFactor = 0.f;
				}
			}
		}
		ITr2GenericEmitter::UpdateArguments args( 
			updateContext.GetTime(), 
			updateContext.GetGpuParticleSystem(), 
			m_worldTransform, 
			updateContext.GetOriginShift(),
			emitCountFactor );
		for( auto it = m_particleEmitters.begin(); it != m_particleEmitters.end(); ++it )
		{
			(*it)->Update( args );
		}
	}
	if( !m_particleSystems.empty() )
	{
		ITr2GenericEmitter::UpdateArguments args(
			updateContext.GetTime(),
			updateContext.GetGpuParticleSystem(),
			IdentityMatrix(),
			updateContext.GetOriginShift() );
		for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
		{
			( *it )->Update( args );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the Local to World transformation matrix
// --------------------------------------------------------------------------------
void EveChildParticleSystem::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

// --------------------------------------------------------------------------------
// Description:
//   Called if the parent's LOD changed
// --------------------------------------------------------------------------------
void EveChildParticleSystem::ChangeLOD( Tr2Lod lod )
{
	if ( !m_useDynamicLod )
	{
		return;
	}

	for ( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		unsigned original = (*it)->GetOriginalMaxParticles();
		unsigned particleCount = original;

		if ( lod == TR2_LOD_LOW )
		{
			particleCount = min( m_lodClampLow, (unsigned)( original * m_lodFactorLow ));
		}
		else if ( lod == TR2_LOD_MEDIUM )
		{
			particleCount = (int)( original * m_lodFactorMedium );
		}
		
		(*it)->SetMaxParticleCount( particleCount );
	}
}

void EveChildParticleSystem::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( m_mesh )
	{
		m_mesh->GetDebugOptions( options );
	}
}

void EveChildParticleSystem::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( m_display && m_mesh )
	{
		m_mesh->RenderDebugInfo( m_worldTransform, renderer );
	}
}

void EveChildParticleSystem::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}