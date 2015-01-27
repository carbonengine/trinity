#include "StdAfx.h"

#include "Tr2InteriorCell.h"
#include "Utilities/BoundingSphere.h"
#include "Resources/TriGeometryRes.h"

#include "Tr2SHProbeRes.h"
#include "Tr2IntSkinnedObject.h"
#include "Tr2InteriorStatic.h"
#include "TriLineSet.h"


BLUE_DEFINE_INTERFACE( ITr2InteriorDynamic );

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCell::Tr2InteriorCell( IRoot* lockobj ) :
	PARENTLOCK( m_statics ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_dynamics ),
	PARENTLOCK( m_skinnedObjects ),
	m_isDirty( true ),
	m_drawBoundingBox( false ),
	m_shScale( 1.0f ),
	m_reflectionMapPath( "" )
{
	// default size
	m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	m_boundingBoxReady = false;
	m_isUnbounded = false;

	m_statics.SetNotify( this );

	m_variableStore.CreateInstance();
	m_variableStore->RegisterVariable( "ReflectionMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "IrradianceMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "DirectionalIrradianceMap", (TriTextureRes*)NULL );
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCell::~Tr2InteriorCell()
{
	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->SetParentCell( NULL );
	}

	// Clear out the lights
	m_lights.Clear();

	// Clear out the dynamics
	m_dynamics.Clear();

	// Clear out the skinned objects
	m_skinnedObjects.Clear();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::Initialize()
{
	SetSHProbeResource();
	SetReflectionMapPath();

	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(* it )->SetParentCell( this );
	}

	m_isDirty = true;

	if( !m_irradianceTexturePath.empty() )
	{
		BeResMan->GetResource( m_irradianceTexturePath, "", m_irradianceTexture );
	}
	if( !m_directionalIrradianceTexturePath.empty() )
	{
		BeResMan->GetResource( m_directionalIrradianceTexturePath, "", m_directionalIrradianceTexture );
	}

	RebuildInternalData();
	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetReflectionMapPath()
// ------------------------------------------------------------------------------------------------------
{
	if( m_reflectionMapRes )
	{
		m_reflectionMapRes.Unlock();
	}

	BeResMan->GetResource( m_reflectionMapPath.c_str(), "", m_reflectionMapRes );

	if( m_reflectionMapRes )
	{
		m_reflectionMapRes->SetName( "Tr2InteriorCell ReflectionMap" );
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::OnModified( Be::Var* value )
{
	if( value == ( Be::Var* )&m_reflectionMapPath )
	{
		SetReflectionMapPath();
	}
	else if( value == ( Be::Var* )&m_shProbeResPath )
	{
		SetSHProbeResource();
	}
	else if( value == (Be::Var*)&m_isUnbounded )
	{
		m_isDirty = true;
	}
	else if( IsMatch( value, m_irradianceTexturePath ) )
	{
		m_irradianceTexture.Unlock();
		if( !m_irradianceTexturePath.empty() )
		{
			BeResMan->GetResource( m_irradianceTexturePath, "", m_irradianceTexture );
		}
		m_variableStore->RegisterVariable( "IrradianceMap", m_irradianceTexture );
		return true;
	}
	else if( IsMatch( value, m_directionalIrradianceTexturePath ) )
	{
		m_directionalIrradianceTexture.Unlock();
		if( !m_directionalIrradianceTexturePath.empty() )
		{
			BeResMan->GetResource( m_directionalIrradianceTexturePath, "", m_directionalIrradianceTexture );
		}
		m_variableStore->RegisterVariable( "DirectionalIrradianceMap", m_directionalIrradianceTexture );
		return true;
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( theList == &m_statics )
	{
		if( (event & BELIST_LOADING) == 0  )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				{
					m_isDirty = true;

					Tr2InteriorStaticPtr object = BlueCastPtr( value );
					if( value )
					{
						object->SetParentCell( this );
						object->BindLowLevelShaders();
					}
				}
				break;
			case BELIST_REMOVED:
				{
					//	If an entry is being removed from the statics list, ensure that it has its parent system cleared
					Tr2InteriorStaticPtr object = BlueCastPtr( value );
					if( value )
					{
						object->SetParentCell( NULL );
					}

					RebuildBoundingBox();
					m_isDirty = true;
				}
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns true if the cell is dirty or any of its systems are dirty.  Returns false
//   otherwise.
// Return Value:
//   true, if the cell or any of its systems have changed
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsDirty( void )
{
	// Return true if the cell is dirty
	if( m_isDirty )
	{
		return true;
	}

	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		// Check the dirty flag
		if( (*it)->IsDirty() )
		{
			return true;
		}
	}

	// Cell isn't dirty, none of the systems are either, so return false
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the dirty flag on the cell and all of the contained systems.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::ResetDirtyFlag( void )
{
	m_isDirty = false;

	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->ResetDirtyFlag();
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds )
{
	minBounds = m_minBounds;
	maxBounds = m_maxBounds;

	return m_boundingBoxReady;
}

// --------------------------------------------------------------------------------------
void Tr2InteriorCell::UpdateBoundingBox( void )
{
	if( !m_boundingBoxReady )
	{
		RebuildBoundingBox();
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsBoundingBoxReady( void ) const
{
	return m_boundingBoxReady;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsUnbounded( void ) const
{
	return m_isUnbounded;
}

bool Tr2InteriorCell::HasSHProbes() const
{
	return m_shProbeResource && m_shProbeResource->IsGood();
}

void Tr2InteriorCell::GetSHProbe( const Vector3& position, Matrix& red, Matrix& green, Matrix& blue )
{
	if( m_shProbeResource->GetProbeSetCount() == 0 )
	{
		memset( &red, 0, sizeof( Matrix ) );
		memset( &green, 0, sizeof( Matrix ) );
		memset( &blue, 0, sizeof( Matrix ) );
		return;
	}
	int sx, sy, sz;
	Matrix transform;
	const Matrix* shData;
	m_shProbeResource->GetProbeSet( 0, sx, sy, sz, transform, shData );
	Vector3 localPos( XMVector3TransformCoord( position, transform ) );
	localPos += Vector3( 0.5f, 0.5f, 0.5f );
	localPos.x *= sx;
	localPos.y *= sy;
	localPos.z *= sz;
	auto clamp = [&]( float& v, int maxV ) -> std::pair<int, int>
	{
		if( v < 0 )
		{
			v = 0;
			return std::make_pair( 0, 0 );
		}
		if( v > maxV - 1 )
		{
			v = 0;
			return std::make_pair( maxV - 1, maxV - 1 );
		}
		int tmp = int( v );
		v = v - floor( v );
		return std::make_pair( tmp, tmp + 1 );
	};
	auto x = clamp( localPos.x, sx );
	auto y = clamp( localPos.y, sy );
	auto z = clamp( localPos.z, sz );
	auto cubeLerp = [&]( Matrix& result, uint32_t index )
	{
		result = 
			( 1 - localPos.x ) * ( 1 - localPos.y ) * ( 1 - localPos.z ) * shData[( x.first + y.first * sx + z.first * sx * sy ) * 3 + index] +
			localPos.x * ( 1 - localPos.y ) * ( 1 - localPos.z ) * shData[( x.second + y.first * sx + z.first * sx * sy ) * 3 + index] +
			( 1 - localPos.x ) * localPos.y * ( 1 - localPos.z ) * shData[( x.first + y.second * sx + z.first * sx * sy ) * 3 + index] +
			localPos.x * localPos.y * ( 1 - localPos.z ) * shData[( x.second + y.second * sx + z.first * sx * sy ) * 3 + index] +
			( 1 - localPos.x ) * ( 1 - localPos.y ) * localPos.z * shData[( x.first + y.first * sx + z.second * sx * sy ) * 3 + index] +
			localPos.x * ( 1 - localPos.y ) * localPos.z * shData[( x.second + y.first * sx + z.second * sx * sy ) * 3 + index] +
			( 1 - localPos.x ) * localPos.y * localPos.z * shData[( x.first + y.second * sx + z.second * sx * sy ) * 3 + index] +
			localPos.x * localPos.y * localPos.z * shData[( x.second + y.second * sx + z.second * sx * sy ) * 3 + index]
			;
	};
	cubeLerp( red, 0 );
	cubeLerp( green, 1 );
	cubeLerp( blue, 2 );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::Update( Be::Time time )
{
	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->Update( time );
	}

	m_variableStore->RegisterVariable( "ReflectionMap", m_reflectionMapRes );
	m_variableStore->RegisterVariable( "IrradianceMap", m_irradianceTexture );
	m_variableStore->RegisterVariable( "DirectionalIrradianceMap", m_directionalIrradianceTexture );
}

// ------------------------------------------------------------------------------------------------------
TriTextureRes* Tr2InteriorCell::GetReflectionMap()
{
	return m_reflectionMapRes;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RebuildInternalData()
{
	RebuildBoundingBox();
}

// --------------------------------------------------------------------------------------
// Description:
//   Enlarges cell's bounding box to enclose the given bounding box. 
// Arguments:
//   minBounds - min bounds of the box to enclose
//   maxBounds - max bounds of the box to enclose
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	Vector3 oldMinBounds = m_minBounds;
	Vector3 oldMaxBounds = m_maxBounds;
	BoundingBoxUpdate( m_minBounds, m_maxBounds, minBounds, maxBounds );

	m_boundingBoxReady = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a dynamic object to the cell.  If the dynamic is already in the cell's
//   internal list, it is not added again.  In this case, shadows are marked as dirty.
//   This function should be called by the Tr2InteriorScene containing the cell, and
//   not by any other caller.  Adding a dynamic to a cell ensures it receives correct
//   indirect lighting from light probes, which are managed by the cell.
//   It is an error to add a NULL dynamic to a cell, and a log message is printed if that
//   occurs.
// Arguments:
//   dynamic - The dynamic to add to the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::AddDynamic( ITr2InteriorDynamic* dynamic )
{
	// Bail out early if
	if( !dynamic )
	{
		CCP_LOGERR( "Attempt to add a NULL dynamic to interior cell!" );
		return false;
	}

	// put it in our list of dynamics
	bool added = false;
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos == -1 )
	{
		m_dynamics.Insert( -1, dynamic );
		added = true;
	}

	// See if it's a skinned object - we maintain a separate list of those for
	// performance reasons
	Tr2IntSkinnedObject* skinned = dynamic_cast<Tr2IntSkinnedObject*>( dynamic );
	if( skinned )
	{
		pos = m_skinnedObjects.FindKey( dynamic );
		if( pos == -1 )
		{
			m_skinnedObjects.Insert( -1, dynamic );
		}
	}

	MarkShadowsDirtyForDynamic( dynamic );

	return added;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a dynamic object from the cell.  If the dynamic is in the list prior to
//   removal, shadows are flagged as dirty.  It is an error to remove a NULL dynamic, and 
//   a log message is printed if that occurs.
// Arguments:
//   dynamic - The dynamic object to remove from the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::RemoveDynamic( ITr2InteriorDynamic* dynamic )
{
	// Bail out early if the dynamic is NULL
	if( !dynamic )
	{
		CCP_LOGERR( "Attempt to remove NULL dynamic from interior cell!" );
		return false;
	}

	bool removed = false;

	// find this one
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos != -1 )
	{
		MarkShadowsDirtyForDynamic( dynamic );

		// remove it
		m_dynamics.Remove( pos );

		removed = true;

		pos = m_skinnedObjects.FindKey( dynamic );
		if( pos != -1 )
		{
			m_skinnedObjects.Remove( pos );
		}
	}

	return removed;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds an interior light source to the cell.  An individual light source can only be 
//   added to a cell once.  It is an error to add a NULL light 
//   source, and a log message is printed if that occurs.
// Arguments:
//   light - The light source to add to the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::AddLight( ITr2InteriorLight* light )
{
	// Bail out early if the light source is NULL
	if( !light )
	{
		CCP_LOGERR( "Attempt to add NULL light source to interior cell!" );
		return false;
	}

	// put it in our list of lights
	ssize_t key = m_lights.FindKey( light );
	if( key == -1 )
	{
		m_lights.Insert( -1, light );

		// Added the light, return success
		return true;
	}
	
	// Didn't add the light (because it's already in our list), return failure
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes an interior light source from the cell.  It is an error 
//   to remove a NULL light source, and a log message is printed if that occurs.
// Arguments:
//   lightSource - The light source to remove from the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::RemoveLight( ITr2InteriorLight* light )
{
	// Bail out early if the light source is NULL
	if( !light )
	{
		CCP_LOGERR( "Attempt to remove NULL light from interior cell!" );
		return;
	}

	ssize_t key = m_lights.FindKey( light );
	if( key != -1 )
	{
		m_lights.Remove( key );
	}
}

// --------------------------------------------------------------------------------------
// Description
//   Mark spotlight shadows as dirty for lights that affect the given dynamic object.
// Arguments:
//   dynamic - Dynamic object that has moved/changed.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::MarkShadowsDirtyForDynamic( ITr2InteriorDynamic* dynamic )
{
	if( !dynamic->IsShadowCaster() )
	{
		return;
	}
	for( ITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		if( (*it)->IsDynamicContributingToShadows( dynamic ) )
		{
			Vector3 min, max;
			dynamic->GetWorldBoundingBox( min, max );
			(*it)->MarkShadowsDirtyForBounds( min, max );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   Mark spotlight shadows as dirty for lights that affect any of the cell's skinned 
//   objects.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::MarkShadowsDirtyForSkinnedObjects()
{
	for( ITr2InteriorDynamicVector::iterator it = m_skinnedObjects.begin(); 
		it != m_skinnedObjects.end(); ++it )
	{
		MarkShadowsDirtyForDynamic( *it );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RenderDebugInfo( TriLineSetPtr lines )
{
	// bounding boxes?
	if( m_drawBoundingBox )
	{
		lines->AddBox( m_minBounds, m_maxBounds, 0x800000ff );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetSHScale( float scale )
{
	m_shScale = scale;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetSHProbeResource()
{
	if( m_shProbeResource )
	{
		m_shProbeResource.Unlock();
	}

	if( !m_shProbeResPath.empty() )
	{
		BeResMan->GetResource( m_shProbeResPath.c_str(), "", m_shProbeResource );
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsVolumeReady() const
{
	return m_boundingBoxReady;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RebuildBoundingBox( void )
{
	Vector3 oldMinBounds = m_minBounds;
	Vector3 oldMaxBounds = m_maxBounds;

	// rest bbox
	m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	m_boundingBoxReady = false;

	// add all sub-cells
	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		Vector3 mn, mx;
		if( (*it)->GetBoundingBox( mn, mx ) )
		{
			BoundingBoxTransform( mn, mx, (*it)->GetTransform() );
			BoundingBoxUpdate( m_minBounds, m_maxBounds, mn, mx );
			m_boundingBoxReady = true;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a static object to the interior cell.  If the system has a valid
//   parent cell, the cell recomputes its bounding box after the static is
//   added.  It is an error to add a NULL static, and a log message is printed if that
//   occurs
// Arguments:
//   interiorStatic - The static object to add to the system (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::AddStatic( Tr2InteriorStatic* interiorStatic )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Bail out early if the static is NULL
	if( !interiorStatic )
	{
		CCP_LOGERR( "Attempt to add a NULL static object to interior cell!" );
		return;
	}

	// put in our statics list
	m_statics.Insert( -1, interiorStatic->GetRawRoot() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a static object from the cell.  It is an error to remove a NULL
//   static, and a log message is printed if that occurs.
// Arguments:
//   interiorStatic - The static object to remove
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::RemoveStatic( Tr2InteriorStatic* interiorStatic )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Bail out early if the static is NULL
	if( !interiorStatic )
	{
		CCP_LOGERR( "Attempt to remove a NULL static object from interior cell!" );
		return;
	}

	// find this one

	IRoot *key =  interiorStatic->GetRawRoot();
	ssize_t pos = m_statics.FindKey( key );

	if( pos == -1 )
	{
		CCP_LOGERR("Tr2InteriorCell::RemoveStatic() - interiorStatic %p not found in this system %s!", ( void* )key, m_name.c_str() );
		return;
	}

	m_statics.Remove( pos );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::ClearStatics()
{
	// clean-up all statics
	for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->RemoveFromCell();
	}
	// clear vector
	while( m_statics.size() )
	{
		m_statics.Remove( 0 );
	}

	RebuildBoundingBox();

	m_isDirty = true;
}

// -------------------------------------------------------------
// Description:
//   Tests if a point is inside the cell's union of bounding boxes
// Arguments:
//   testPoint - the point to test in worldspace
// -------------------------------------------------------------
bool Tr2InteriorCell::ContainsPoint( const Vector3& testPoint, float epsilon )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	return BoundingBoxIsInside( cellMinBounds, cellMaxBounds, testPoint, epsilon );
}

// -------------------------------------------------------------
// Description:
//   Tests if an oriented bounding box intersects the cell's union of bounding boxes
// Arguments:
//   boxCenter, boxExtents, boxOrientation - the definition of the oriented bounding box
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsOBB( const Vector3& boxCenter, const Vector3& boxExtents, const Quaternion& boxOrientation )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
		//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
		//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	return IntersectOrientedBoxAxisAlignedBox( 
		boxCenter, 
		boxExtents, 
		boxOrientation, 
		cellMinBounds, 
		cellMaxBounds );
}

// -------------------------------------------------------------
// Description:
//   Tests if an axisaligned bounding box intersects the cell's union of bounding boxes
// Arguments:
//   boxMin, boxMax - the axis-aligned bounding box extents of the tested object in worldspace
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsAABB( const Vector3& minBounds, const Vector3& maxBounds )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	return IntersectAxisAlignedBoxAxisAlignedBox( cellMinBounds, cellMaxBounds, minBounds, maxBounds );
}


// -------------------------------------------------------------
// Description:
//   Tests if a sphere intersects the cell's union of bounding boxes
// Arguments:
//   center - Sphere center in worldspace
//	 radius - Sphere radius in worldspace units
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsSphere( const Vector3& center, float radius )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}
	Vector4 sphere( center.x, center.y, center.z, radius );
	return IntersectSphereAxisAlignedBox( sphere, cellMinBounds, cellMaxBounds );
}

void Tr2InteriorCell::SetVisualizeMethod( VisualizeMethod method )
{
	for( auto it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->SetVisualizeMethod( method );
	}
}

