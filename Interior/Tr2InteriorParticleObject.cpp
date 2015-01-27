////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2InteriorParticleObject.h"
#include "Particle/ITr2GenericEmitter.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2InteriorCell.h"
#include "Tr2Mesh.h"
#include "TriLineSet.h"
#include "Curves/TriCurveSet.h"
#include "Tr2InteriorConstantBufferFormats.h"

Tr2InteriorParticleObject::Tr2InteriorParticleObject( IRoot* lockobj )
:	PARENTLOCK( m_particleSystems ),
	PARENTLOCK( m_emitters ),
	PARENTLOCK( m_meshes ),
	m_shSolver( nullptr ),
	m_maxParticleRadius( 0.f ),
	m_shBoundsMin( 0.f, 0.f, 0.f ),
	m_shBoundsMax( 0.f, 0.f, 0.f ),
	m_depthOffset( 0.f ),
	m_isVisible( true ),
	m_renderDebugInfo( false ),
	PARENTLOCK( m_curveSets )
{
	D3DXMatrixIdentity( &m_transform );
	D3DXMatrixIdentity( &m_mirrorToWorldMatrix );
}

Tr2InteriorParticleObject::~Tr2InteriorParticleObject()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Re-binds low level shaders on all meshes.
// Return value:
//   true Always
// --------------------------------------------------------------------------------------
bool Tr2InteriorParticleObject::Initialize()
{
	BindLowLevelShaders();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets visibility flag for the object.
// Arguments:
//   bVisible - Visibility flag: true if the object should render, false if the object
//              should be hidden
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns visibility flag.
// Return Value:
//   true If the object should render
//   false if the object should be hidden
// --------------------------------------------------------------------------------------
bool Tr2InteriorParticleObject::IsVisible( void ) const
{
	return m_isVisible;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object using a per-instance light-set override and 
//    an arbitrary object-to-world matrix.  
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorParticleObject::GetPerObjectDataWithPerInstanceLighting( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet, 
	const Matrix& objectToWorldMatrix, 
	const Matrix& mirrorToWorldMatrix 
)
{
	// standard vertex shader data
	Tr2PerObjectParticleVSData perObjectVSBuffer;

	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	Matrix worldView = m_transform * mirrorMatrix;
	worldView = worldView * Tr2Renderer::GetViewTransform();
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &worldView );
	D3DXMatrixTranspose( &perObjectVSBuffer.InvViewMat, &Tr2Renderer::GetInverseViewTransform() );

	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();


	if( !data )
	{
		return NULL;
	}

	// Pixel Shader Light information
	Tr2InteriorPerObjectPSData perObjectPSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );

	float zNear = Tr2Renderer::GetFrontClip();
	float zFar = Tr2Renderer::GetBackClip();
	perObjectPSBuffer.shadowCaster0.x = zFar / ( zFar - zNear );
	perObjectPSBuffer.shadowCaster0.y = zFar * zNear / ( zNear - zFar );

	// Copy the SH matrices
	memset( &perObjectPSBuffer.redMat, 0, sizeof( Matrix ) );
	memset( &perObjectPSBuffer.greenMat, 0, sizeof( Matrix ) );
	memset( &perObjectPSBuffer.blueMat, 0, sizeof( Matrix ) );

	D3DXMatrixTranspose( &perObjectPSBuffer.redMat, &m_redProbeMatrix );
	D3DXMatrixTranspose( &perObjectPSBuffer.greenMat, &m_greenProbeMatrix );
	D3DXMatrixTranspose( &perObjectPSBuffer.blueMat, &m_blueProbeMatrix );

	// Copy the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	D3DXMatrixInverse( &m_mirrorToWorldMatrix, NULL, D3DXMatrixTranspose( &m_mirrorToWorldMatrix, &mirrorToWorldMatrix ) );

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		( *it )->UpdateViewDependentData( m_mirrorToWorldMatrix );
	}
	return data;
}

namespace
{
	// ----------------------------------------------------------------------------------
	//  Description:
	//    Per-batch/area callback for Tr2Mesh::GetBatches. Assigns batch depth and adds 
	//    area bounding box to SH solver if needed.  
	//  Return Value:
	//    The allocated per-object data, or NULL if the memory allocation failed.
	// ----------------------------------------------------------------------------------
	struct Tr2InteriorParticleObjectBatchCallback: public ITr2MeshBatchCallback
	{
		Tr2InteriorParticleObjectBatchCallback( 
			ITr2InteriorSHLightingSolver* solver,
			ITriRenderBatchAccumulator* batches,
			unsigned int depth,
			Vector3 shBoundsMin, 
			Vector3 shBoundsMax,
			Matrix transform )
		:	m_shSolver( solver),
			m_batches( batches ),
			m_shData( nullptr ),
			m_depth( depth ),
			m_shBoundsMin( shBoundsMin ),
			m_shBoundsMax( shBoundsMax ),
			m_transform( transform )
		{
		}

		// ------------------------------------------------------------------------------
		//  Description:
		//    Per-batch/area callback for Tr2Mesh::GetBatches. Assigns batch depth and  
		//    adds area bounding box to SH solver if needed.  
		//  Arguments:
		//    area - Mesh area for which the batch is generated
		//    batch - New batch to be added to accumulator
		//  Return Value:
		//    true Always.
		// ------------------------------------------------------------------------------
		bool ProcessBatch( Tr2MeshArea* area, TriRenderBatch* batch )
		{
			if( m_shSolver && area->GetUseSHLighting() && m_shData == nullptr )
			{
				if( m_shData == nullptr )
				{
					m_shData = m_batches->Allocate<Tr2PerAreaSHLightingData>();
					m_shData->SetPerObjectData( batch->GetPerObjectData() );
					m_shSolver->AddVolume( m_shBoundsMin, m_shBoundsMax, m_transform, m_shData );
				}
				if( m_shData )
				{
					batch->SetPerObjectData( m_shData );
				}
			}
			batch->SetDepth( m_depth );

			return true;
		}

		// SH lighting solver for transparent rendering
		ITr2InteriorSHLightingSolver *m_shSolver;
		// Batch accumulator
		ITriRenderBatchAccumulator* m_batches;
		// Per-object SH lighting data
		Tr2PerAreaSHLightingData* m_shData;
		// Encoded object depth
		unsigned int m_depth;
		// SH box bounds
		Vector3 m_shBoundsMin, m_shBoundsMax;
		// SH box transform
		Matrix m_transform;
	};
}

// --------------------------------------------------------------------------------------
//  Description:
//    Accumulates batches for rendering.
//  Arguments:
//    batches - The batch accumulator used to allocate memory for per-object data
//    batchType - Type batches to collect
//    perObjectData - Per-object data for this object
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::GetBatches( ITriRenderBatchAccumulator* batches, 
						 TriBatchType batchType, 
						 const Tr2PerObjectData* perObjectData )
{
	unsigned int depth = 0;
	if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		// Compute the depth

		float maxDepth = Tr2Renderer::GetFrustumRadius();
		Matrix instanceToWorld = m_transform * m_mirrorToWorldMatrix;

		Vector3 center;
		Vector3 minBounds, maxBounds;
		if( GetLocalBoundingBox( minBounds, maxBounds ) )
		{
			center = 0.5f * ( minBounds + maxBounds );
			D3DXVec3TransformCoord( &center, &center, &instanceToWorld );
			center -= Tr2Renderer::GetViewPosition();
			float z = std::min( std::max( ( D3DXVec3Length( &center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

			depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );
		}
	}

	Tr2InteriorParticleObjectBatchCallback callback( m_shSolver, batches, depth, m_shBoundsMin, m_shBoundsMax, m_transform );


	for( auto mit = m_meshes.begin(); mit != m_meshes.end(); ++mit )
	{
		Tr2Mesh* mesh = *mit;
		if( !mesh->IsHidden() )
		{
			Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );
			if( areas )
			{
				mesh->GetBatches( batches, areas, perObjectData, &callback );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Checks if the mesh has transparent areas.  
//  Return Value:
//    true Always.
// --------------------------------------------------------------------------------------
bool Tr2InteriorParticleObject::HasTransparentBatches()
{
	return true;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Implements ITr2Renderable interface. Does nothing.  
//  Return Value:
//    0.0f always.
// --------------------------------------------------------------------------------------
float Tr2InteriorParticleObject::GetSortValue()
{
	return 0.0f;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object.  
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorParticleObject::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectDataWithPerInstanceLighting( accumulator, NULL, m_transform, Tr2Renderer::GetIdentityTransform() );
}

// --------------------------------------------------------------------------------------
//  Description:
//    Renders debug info: SH lighting bounding box.
//  Arguments:
//    lines - Line rendering object
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::RenderDebugInfo( TriLineSet* lines ) const
{
	if( m_renderDebugInfo )
	{
		lines->AddOrientedBox( m_transform, m_shBoundsMin, m_shBoundsMax );
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Rebinds low level shaders on all meshes.
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::BindLowLevelShaders()
{
	std::vector<unsigned int> localFlags;
	unsigned int h = CcpHashFNV1( "ParticleObject", strlen( "ParticleObject" ) );
	localFlags.push_back( h );
	h = CcpHashFNV1( "Interior", strlen( "Interior" ) );
	localFlags.push_back( h );

	for( auto it = m_meshes.begin(); it != m_meshes.end(); ++it )
	{
		( *it )->BindLowLevelShaders( localFlags );
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Implements IBluePlacementObserver. Updates object's transform matrix based on
//	  provided position/rotation.
//  Arguments:
//    front - "front" facing vector (assuming it's local Z direction)
//    top - "up" facing vector (assuming it's local Y direction)
//    pos - object position
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	// Terrible workaround since the UpdatePlacement takes in a position and a front vector. 
	
	D3DXVec3Normalize( &m_transform.GetY(), &top );
	D3DXVec3Normalize( &m_transform.GetX(), D3DXVec3Cross( &m_transform.GetX(), &front, &top ) );
	D3DXVec3Cross( &m_transform.GetZ(), &m_transform.GetX(), &m_transform.GetY() );
	m_transform.GetTranslation() = pos;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Assigns SH lighting solver to each particle system.
//  Arguments:
//    solver - SH lighting solver
// --------------------------------------------------------------------------------------
void Tr2InteriorParticleObject::SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver )
{
	m_shSolver = solver;
}

// -------------------------------------------------------------
// Description:
//   Returns bounding box in local space for this object.
// Arguments:
//   minBounds (out) - Bounding box min bounds
//   maxBounds (out) - Bounding box max bounds
// Return value:
//   true If the bounding box is ready
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::GetLocalBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	bool validBox = false;
	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		Vector3 min, max;
		if( ( *it )->GetBoundingBox( min, max ) )
		{
			if( validBox )
			{
				BoundingBoxUpdate( minBounds, maxBounds, min, max );
			}
			else
			{
				minBounds = min;
				maxBounds = max;
				validBox = true;
			}
		}
	}
	if( validBox )
	{
		minBounds -= Vector3( m_maxParticleRadius, m_maxParticleRadius, m_maxParticleRadius );
		maxBounds += Vector3( m_maxParticleRadius, m_maxParticleRadius, m_maxParticleRadius );
	}
	return validBox;
}

// -------------------------------------------------------------
// Description:
//   Returns bounding box in world space for this object.
// Arguments:
//   minBounds (out) - Bounding box min bounds
//   maxBounds (out) - Bounding box max bounds
// Return value:
//   true If the bounding box is ready
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
{
	if( !GetLocalBoundingBox( min, max ) )
	{
		return false;
	}
	BoundingBoxTransform( min, max, m_transform );
	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns if bounding box is ready.
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::IsBoundingBoxReady( void ) const
{
	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		Vector3 min, max;
		if( ( *it )->GetBoundingBox( min, max ) )
		{
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//   Returns position of SH probe in world space.
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::GetShProbePosition( Vector3& position ) const
{
	position = ( m_shBoundsMin + m_shBoundsMax ) * 0.5f;
	D3DXVec3TransformCoord( &position, &position, &m_transform );
	return true;
}

// -------------------------------------------------------------
// Description:
//   Updates an object before running physics update. Does nothing.
// Arguments:
//   time - Current system time
// -------------------------------------------------------------
void Tr2InteriorParticleObject::PrePhysicsUpdate( Be::Time time )
{
}

// -------------------------------------------------------------
// Description:
//   Updates an object after running physics update. Updates all
//   particle emitters.
// Arguments:
//   time - Current system time
// -------------------------------------------------------------
void Tr2InteriorParticleObject::PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene )
{
	Matrix transformWithoutTranslation;
	D3DXMatrixIdentity( &transformWithoutTranslation );

	for( auto it = m_emitters.begin(); it != m_emitters.end(); ++it )
	{
		( *it )->Update( time );
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to red SH probe matrix.
// Return value:
//   Reference to red SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorParticleObject::GetRedLightProbeMatrix( void )
{
	return m_redProbeMatrix;
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to green SH probe matrix.
// Return value:
//   Reference to green SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorParticleObject::GetGreenLightProbeMatrix( void )
{
	return m_greenProbeMatrix;
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to blue SH probe matrix.
// Return value:
//   Reference to blue SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorParticleObject::GetBlueLightProbeMatrix( void )
{
	return m_blueProbeMatrix;
}

// -------------------------------------------------------------
// Description:
//   Called when object is added to the scene. 
// Return value:
//   true If the object has valid bounds and can be added to a cell
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::AddToScene( Tr2ApexScene *apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	return true;
}

// -------------------------------------------------------------
// Description:
//   Called when object is removed from the scene. 
// -------------------------------------------------------------
void Tr2InteriorParticleObject::RemoveFromScene( void )
{
}

// -------------------------------------------------------------
// Description:
//   Checks if the object intersects a cell and if it does adds
//   itself the the cell.
// Arguments:
//   cell - A cell to test intersection with
// Return value:
//   true If the object intersects the cell
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}


	// Get our bounding box
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		// Our bounding box is not ready, return false (no intersection)
		return false;
	}

	bool intersects = cell->IntersectsAABB( minBounds, maxBounds );

	// If we got an intersection, add to the cell
	if( intersects )
	{
		cell->AddDynamic( this );
	}
	else
	{
		cell->RemoveDynamic( this );
	}

	// Return the result of the intersection test
	return intersects;
}

// -------------------------------------------------------------
// Description:
//   Query if the object has dirty bounds. We assume that the
//   particle system always changes bounds.
// Return value:
//   true Always
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::IsDirty( void ) const
{
	return true;
}

// -------------------------------------------------------------
// Description:
//   Directly set the bounds dirty flag. Does
//   nothing.
// Arguments:
//   isDirty - New diry flag (not used)
// -------------------------------------------------------------
void Tr2InteriorParticleObject::SetDirtyFlag( bool isDirty )
{
}

// -------------------------------------------------------------
// Description:
//   Query if the object casts shadows . Particle
//   systems don't support that.
// Return value:
//   false Always
// -------------------------------------------------------------
bool Tr2InteriorParticleObject::IsShadowCaster( void ) const
{
	return false;
}

// -------------------------------------------------------------
// Description:
//   Assignes LOD based on camera. Not implemented yet.
// Arguments:
//   frustum - Camera frustum
// -------------------------------------------------------------
void Tr2InteriorParticleObject::SetLOD( const TriFrustum* frustum )
{
	// TODO_delder: implement LOD?
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in its
//   local coordinate space.
// Return value:
//   AABB for the object in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorParticleObject::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in the
//   world coordinate space.
// Return value:
//   AABB for the object in the world coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorParticleObject::GetBoundingBoxInWorldSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetWorldBoundingBox( result.m_min, result.m_max );
	return result;
}

#include "TriFrustum.h"

bool Tr2InteriorParticleObject::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		return false;
	}
	objectToWorld = m_transform;
	return frustum.IsBoxVisible( minBounds, maxBounds );
}

