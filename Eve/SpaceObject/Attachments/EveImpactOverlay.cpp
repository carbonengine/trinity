////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "include/TriMath.h"
#include "Curves/TriCurveSet.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshBase.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Eve/EveUpdateContext.h"

EveImpactOverlay::EveImpactOverlay( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	m_display( true ),
	m_overallShieldImpact( -1.f ),
	m_maxShieldImpacts( 128 ),
	m_shieldEllipsoidCenter( 0.f, 0.f, 0.f ),
	m_shieldEllipsoidRadii( 1.f, 1.f, 1.f ),
	m_shieldImpactDataNextIdx( 1 ),
	m_armorImpactDataNextIdx( 1 ),
	m_dataTextureBlockID( -1 )
{
}

EveImpactOverlay::~EveImpactOverlay()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveImpactOverlay::Initialize()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// do we have something to do at all?
	if( !HasActivity() )
	{
		m_dataTextureBlockID = -1;
		return;
	}

	// temporary
	CCP_ASSERT( m_impactTexelData.size() == std::max( m_shieldImpactData.size(), m_armorImpactData.size() ) );

	// this comes from the scene via EveUpdateContext
	Tr2DataTextureManagerPtr dataTextureMgr = updateContext.GetDataTextureManager();

	// the block header is the first column in the data texture, set it!
	DataRow header;
	header.v[0] = Vector4( float( m_shieldImpactData.size() ), m_overallShieldImpact, 0.f, 0.f );
	header.v[1] = Vector4( 0.f, 0.f, 0.f, 0.f );
	header.v[2] = Vector4( float( m_armorImpactData.size() ), 0.f, 0.f, 0.f );
	header.v[3] = Vector4( 0.f, 0.f, 0.f, 0.f );

	// update block data
	m_dataTextureBlockID = dataTextureMgr->requestBlockData( &header.v[0], m_impactTexelData.size(), m_impactTexelData.empty() ? nullptr : &m_impactTexelData[0].v[0] );
}

// --------------------------------------------------------------------------------
// Description:
//   Do all the math-heavy conversion here async
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// first take out the dead ones
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); )
	{
		sidit->second.timeLeft -= updateContext.GetDeltaT();
		if( sidit->second.timeLeft <= 0.f )
		{
			m_shieldImpactData.erase(sidit++);
		}
		else
		{
			++sidit;
		}
	}

	// resize the texture data array based on both shield and armor impact
	m_impactTexelData.resize( std::max( m_shieldImpactData.size(), m_armorImpactData.size() ) );

	// no activity?
	if( !HasActivity() )
	{
		return;
	}

	// get parent's bounding ellipsoid shape
	parent->GetShapeEllipsoid( m_shieldEllipsoidCenter, m_shieldEllipsoidRadii );

	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	if( !D3DXMatrixInverse( &parentInverseWorldTransform, nullptr, &parentWorldTransform ) )
	{
		parentInverseWorldTransform = parentWorldTransform;
	}
	
	size_t i = 0;
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); ++sidit )
	{
		ShieldImpactData* shieldData = &sidit->second;
		DataRow* texelData = &m_impactTexelData[i];

		// get worldpos of damagelocator from parent
		Vector3 tgtPosWS( 0.f, 0.f, 0.f );
		parent->GetDamageLocatorPosition( &tgtPosWS, shieldData->damageLocatorIndex );
		// convert position and direction into object space
		Vector3 tgtPosOS, dirOS;
		D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
		D3DXVec3TransformNormal( &dirOS, &shieldData->direction, &parentInverseWorldTransform );
		// intersections
		Vector3 p( 0.f, 0.f, 0.f );
		IntersectEllipsoidRay( p, m_shieldEllipsoidCenter, m_shieldEllipsoidRadii, tgtPosOS, dirOS );
		// "encode" it in texels
		texelData->v[0] = Vector4( p, shieldData->timeLeft );
		texelData->v[1] = Vector4( 0.f, 0.f, 0.f, shieldData->lifeTime );
		// also need this intercept position in WS
		D3DXVec3TransformCoord( &shieldData->interceptPosition, &p, &parentWorldTransform );
	
		++i;
	}

	// armor
	i = 0;
	for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
	{
		ArmorImpactData* armorData = &aidit->second;
		DataRow* texelData = &m_impactTexelData[i];

		// get position from damage locator
		Vector3 tgtPosWS( 0.f, 0.f, 0.f );
		parent->GetDamageLocatorPosition( &tgtPosWS, armorData->damageLocatorIndex );
		// convert position and direction into object space
		Vector3 tgtPosOS;
		D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
		texelData->v[2] = Vector4( tgtPosOS, 0.f );
		texelData->v[3] = Vector4( 0.f, 0.f, 0.f, 0.f );

		++i;
	}

	// don't forget the curves
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Update( time, time );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( !m_display )
	{
		return;
	}
	if( !m_mesh )
	{
		return;
	}
	if( m_dataTextureBlockID == -1 )
	{
		return;
	}

	// anything on shields?
	if( HasActivity() )
	{
		const Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
		m_mesh->GetBatches( accumulator, areas, perObjectData );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if this impact overlay is active
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasActivity() const
{
	return !m_armorImpactData.empty() || !m_shieldImpactData.empty() || ( m_overallShieldImpact > 0.f );
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal animation curves
// --------------------------------------------------------------------------------
void EveImpactOverlay::PlayCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal animation curves
// --------------------------------------------------------------------------------
void EveImpactOverlay::StopAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Stop();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new shield impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime )
{
	// fill our struct, but keep it in world space
	ShieldImpactData sid;
	sid.direction = direction;
	D3DXVec3Normalize( &sid.direction, &sid.direction );
	sid.damageLocatorIndex = damageLocatorIndex;
	sid.interceptPosition = Vector3( 0.f, 0.f, 0.f );
	sid.lifeTime = sid.timeLeft = 2.f * lifeTime;
	m_shieldImpactData[ m_shieldImpactDataNextIdx ] = sid;
	return m_shieldImpactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Shield impacts are special, they need constant updating with the direction
//   to the target
// --------------------------------------------------------------------------------
bool EveImpactOverlay::UpdateShieldImpact( const Vector3& direction, int shieldImpactIndex )
{
	auto finder = m_shieldImpactData.find( shieldImpactIndex );
	if( finder == m_shieldImpactData.end() )
	{
		return false;
	}
	D3DXVec3Normalize( &finder->second.direction, &direction );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the intercept position of a given impact
// --------------------------------------------------------------------------------
bool EveImpactOverlay::GetShieldImpactPosition( Vector3& out, int shieldImpactIndex ) const
{
	auto finder = m_shieldImpactData.find( shieldImpactIndex );
	if( finder == m_shieldImpactData.end() )
	{
		return false;
	}
	out = finder->second.interceptPosition;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new armor impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateArmorImpact( int damageLocatorIndex )
{
	// fill our struct, but keep it in world space
	ArmorImpactData aid;
	aid.damageLocatorIndex = damageLocatorIndex;
	aid.timeLeft = 0.f;
	m_armorImpactData[ m_armorImpactDataNextIdx ] = aid;
	return m_armorImpactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor efects
// --------------------------------------------------------------------------------
Tr2EffectPtr EveImpactOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	// no activity?
	if( !HasActivity() )
	{
		return nullptr;
	}

	if( batchType == TRIBATCHTYPE_DECAL )
	{
		return m_armorDamageShader;
	}
	return nullptr;
}




