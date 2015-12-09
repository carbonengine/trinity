////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "Utilities/StringUtils.h"
#include "include/TriMath.h"
#include "Curves/TriCurveSet.h"
#include "Curves/Fader/Tr2ScalarFader.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshBase.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2GpuUniqueEmitter.h"
#include "Shader/Tr2Effect.h"

// settings
extern bool g_eveSpaceObjectImpactEffectEnabled;

// consts
static const float IMPACT_HOLE_TO_ARMOR_DAMAGE_RATIO = 15.f;
static const float IMPACT_HOLE_TO_HULL_DAMAGE_RATIO = 0.f;
static const float IMPACT_ARMOR_SIZE_FACTOR = 0.0129f;
static const float IMPACT_ARMOR_SIZE_MAX = 10.f;
static const float IMPACT_SHIELD_SIZE_MAX = 2000.f;
static const float IMPACT_SHIELD_SIZE_MIN = 70.f;
static const float IMPACT_SHIELD_FADEOUT = 1.5f;
static const float IMPACT_ARMOR_PARTICLE_LOD_FACTOR = 400.f;


EveImpactOverlay::EveImpactOverlay( IRoot* lockobj ) :
	m_display( true ),
	m_configuration( IMPACT_INVALID ),
	m_overallShieldImpact( -1.f ),
	m_maxShieldImpacts( 8 ),
	m_impactDataNextIdx( 1 ),
	m_debugForceSpawnDebris( false ),
	m_renderPriority( 0.f ),
	m_isVisibleLast( false ),
	m_dataTextureBlockID( -1 ),
	m_dataTextureOffset( -1 ),
	m_armorImpactGoalCount( 0 ),
	m_armorImpactParentSize( 0.f ),
	m_shieldImpactColorFade( 0.f ),
	m_shieldImpactParentSize( 0.f ),
	m_hullDamageFactor( 0.f )
{
	// 0
	memset( &m_impactTexelHeader, 0, sizeof( DataRow ) );

	// create the faders
	m_armorHardening.CreateInstance();
	m_armorRepairing.CreateInstance();
	m_shieldBoosting.CreateInstance();
	m_shieldHardening.CreateInstance();
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
//   Do everyting non-threadsafe here
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// do we have something to do at all?
	if( !HasGeneralActivity() )
	{
		m_dataTextureBlockID = -1;
		return;
	}

	// this comes from the scene via EveUpdateContext
	Tr2DataTextureManagerPtr dataTextureMgr = updateContext.GetDataTextureManager();

	// what's our ofset in pixels for the data texture?
	m_dataTextureOffset = dataTextureMgr->GetTextureOffset( m_dataTextureBlockID );

	// update block data
	m_dataTextureBlockID = dataTextureMgr->RequestBlockData( &m_impactTexelHeader.v[0],
		(uint32_t)m_impactTexelData.size(),
		m_impactTexelData.empty() ? nullptr : &m_impactTexelData[0].v[0],
		m_renderPriority );

	// spawn armor impact particles?
	if( updateContext.GetGpuParticleSystem() )
	{
		if( m_armorImpactEmitter )
		{
			if( m_armorImpactParentSize > 0.f )
			{
				for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
				{
					if( aidit->second.requestSpawnDebris )
					{
						// where?
						Vector3 impactPosWS( 0.f, 0.f, 0.f );
						parent->GetDamageLocatorPosition( &impactPosWS, aidit->second.damageLocatorIndex );
						m_armorImpactEmitter->SetPosition( &impactPosWS );
						// facing?
						Vector3 impactDirWS( 0.f, 1.f, 0.f );
						parent->GetDamageLocatorDirection( &impactDirWS, aidit->second.damageLocatorIndex );
						m_armorImpactEmitter->SetDirection( &impactDirWS );
						// velocity?
						Vector3 parentVelocityWS;
						parent->GetWorldVelocity( parentVelocityWS );
						// scaling?
						float scale = aidit->second.size * m_armorImpactParentSize / ( IMPACT_ARMOR_SIZE_MAX / IMPACT_ARMOR_SIZE_FACTOR );
						// loding for emit rate?
						float rateModifier = Clamp( m_renderPriority / IMPACT_ARMOR_PARTICLE_LOD_FACTOR, 0.f, 1.f );
						// put together particle update info
						ITr2GenericEmitter::UpdateArguments args( updateContext.GetTime(), updateContext.GetGpuParticleSystem(), Tr2Renderer::GetIdentityTransform(), updateContext.GetOriginShift() );
						// do the spawn here once!
						m_armorImpactEmitter->SpawnOnce( args, parentVelocityWS, scale, rateModifier );
						aidit->second.requestSpawnDebris = false;
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Do all the math-heavy conversion here async
// --------------------------------------------------------------------------------
void EveImpactOverlay::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// first always reduce shield impacts
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
	// then check if the impact count goal is less than what we have
	if( m_armorImpactGoalCount < m_armorImpactData.size() )
	{
		// close up only the excess holes, so get am "advanced" map iterator
		auto aidit = m_armorImpactData.begin();
		std::advance( aidit, m_armorImpactGoalCount );
		// ok, we want to have less impacts, so close the holes
		while( aidit != m_armorImpactData.end() )
		{
			aidit->second.size -= 0.1f * updateContext.GetDeltaT();
			if( aidit->second.size <= 0.f )
			{
				m_armorImpactData.erase(aidit++);
			}
			else
			{
				++aidit;
			}
		}
	}

	// update the faders
	m_armorHardening->Update( updateContext );
	m_armorRepairing->Update( updateContext );
	m_shieldBoosting->Update( updateContext );
	m_shieldHardening->Update( updateContext );

	// resize the texture data array based on both shield and armor impact
	m_impactTexelData.resize( std::max( m_shieldImpactData.size(), m_armorImpactData.size() ) );

	// the block header is the first column in the data texture, set it!
	m_impactTexelHeader.v[0] = Vector4( float( m_shieldImpactData.size() ),
		m_overallShieldImpact,
		m_shieldImpactColorFade,
		m_shieldImpactParentSize );
	m_impactTexelHeader.v[1] = Vector4( m_shieldHardening->GetFaderValue(),
		m_shieldBoosting->GetFaderValue(),
		m_shieldHardening->GetKickInValue(),
		m_shieldBoosting->GetKickInValue() );
	m_impactTexelHeader.v[2] = Vector4( float( m_armorImpactData.size() ),
		m_armorImpactParentSize,
		0.f,
		0.f );
	m_impactTexelHeader.v[3] = Vector4( m_armorRepairing->GetFaderValue(),
		m_armorHardening->GetFaderValue(),
		m_armorRepairing->GetKickInValue(),
		m_armorHardening->GetKickInValue() );

	// no activity?
	if( !HasGeneralActivity() )
	{
		return;
	}

	// calculate render priority, but take into account the visibility of the last frame. To counter
	// the fact that the actual visibility data might not be correct (because of picking etc.)
	// needs fixing! Steve, 2015
	if( m_isVisibleLast )
	{
		m_renderPriority = parent->GetEstimatedPixelDiameter();
	}
	else
	{
		m_renderPriority = parent->IsInFrustum() ? parent->GetEstimatedPixelDiameter() : 0.f;
	}
	m_isVisibleLast = parent->IsInFrustum();

	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	if( !D3DXMatrixInverse( &parentInverseWorldTransform, nullptr, &parentWorldTransform ) )
	{
		parentInverseWorldTransform = parentWorldTransform;
	}

	// get parent's bounding sphere
	Vector4 parentBoundingSphere( 0.f, 0.f, 0.f, -1.f );
	parent->GetBoundingSphere( parentBoundingSphere );
	// cut off the parent size at some hard-coded size, so shield and armor impacts on giant ships get smaller
	m_armorImpactParentSize = std::min( parentBoundingSphere.w, IMPACT_ARMOR_SIZE_MAX / IMPACT_ARMOR_SIZE_FACTOR );
	m_shieldImpactParentSize = Clamp( parentBoundingSphere.w, IMPACT_SHIELD_SIZE_MIN, IMPACT_SHIELD_SIZE_MAX );

	if( !m_shieldImpactData.empty() )
	{
		// get parent's bounding ellipsoid shape
		Vector3 shieldEllipsoidRadii( 1.f, 1.f, 1.f ), shieldEllipsoidCenter( 0.f, 0., 0.f );
		parent->GetShapeEllipsoid( shieldEllipsoidCenter, shieldEllipsoidRadii );

		// shield
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
			IntersectEllipsoidRay( p, shieldEllipsoidCenter, shieldEllipsoidRadii, tgtPosOS, dirOS );
			// "encode" it in texels
			texelData->v[0] = Vector4( p, shieldData->timeLeft );
			texelData->v[1] = Vector4( shieldData->size, 0.f, 0.f, shieldData->lifeTime );
			// also need this intercept position in WS
			D3DXVec3TransformCoord( &shieldData->interceptPosition, &p, &parentWorldTransform );
	
			++i;
		}
	}

	if( !m_armorImpactData.empty() )
	{
		// armor
		size_t i = 0;
		for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
		{
			ArmorImpactData* armorData = &aidit->second;
			DataRow* texelData = &m_impactTexelData[i];

			// size of impact
			float size = armorData->size * IMPACT_ARMOR_SIZE_FACTOR * m_armorImpactParentSize;
			// get position from damage locator
			Vector3 tgtPosWS( 0.f, 0.f, 0.f );
			parent->GetDamageLocatorPosition( &tgtPosWS, armorData->damageLocatorIndex );
			// convert position and direction into object space
			Vector3 tgtPosOS;
			D3DXVec3TransformCoord( &tgtPosOS, &tgtPosWS, &parentInverseWorldTransform );
			texelData->v[2] = Vector4( tgtPosOS, 0.f );
			texelData->v[3] = Vector4( size, 0.f, 0.f, 0.f );

			++i;
		}
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
	if( ( m_dataTextureBlockID == -1 ) || ( m_dataTextureOffset == -1 ) )
	{
		return;
	}

	// anything on shields?
	if( HasShieldActivity() )
	{
		const Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
		m_mesh->GetBatches( accumulator, areas, perObjectData );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is shield activity
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasShieldActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// general shield display?
	if( m_overallShieldImpact > 0.f )
	{
		return true;
	}

	// shield?
	return ( !m_shieldImpactData.empty() || !m_shieldBoosting->IsKickInZero() || !m_shieldHardening->IsKickInZero() );
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is armor activity
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasArmorActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// armor?
	return ( !m_armorImpactData.empty() || !m_armorHardening->IsZero() || !m_armorRepairing->IsZero() );
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is general activity
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasGeneralActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// armor or shield?
	return HasArmorActivity() || HasShieldActivity();
}

// --------------------------------------------------------------------------------
// Description:
//   Just a getter for the texture offset. Nothing special. Move on.
// --------------------------------------------------------------------------------
int32_t EveImpactOverlay::GetDataTextureOffset() const
{
	return m_dataTextureOffset;
}

// --------------------------------------------------------------------------------
// Description:
//   Check if a certain type of defense is there
// --------------------------------------------------------------------------------
EveImpactOverlay::ImpactConfiguration EveImpactOverlay::GetImpactConfiguration() const
{
	return m_configuration;
}

// --------------------------------------------------------------------------------
// Description:
//   EveImpact overlays can modulate the activation strenth, to let the lights
//   flciker etc.
// --------------------------------------------------------------------------------
float EveImpactOverlay::GetActivationStrength( EveUpdateContext& updateContext ) const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return 1.f;
	}

	// comes from a curve ifwe have hull damage
	if( m_hullDamageFactor > 0.f )
	{
		if( m_hullDamageFlickerCurve )
		{
			float min = std::max( 0.f, 2.f * m_hullDamageFactor - 1.f );
			float max = std::min( 1.f, 2.f * m_hullDamageFactor );
			return TriLinearize( min, max, m_hullDamageFlickerCurve->Update( updateContext.GetTime() ) );
		}
	}

	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   This function changes this effect to be ready for animated parent objects
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetToSkinned()
{
	if( m_armorDamageShader )
	{
		std::string resPath( m_armorDamageShader->GetEffectPathName() );
		StringInsertStubAfter( resPath, "/", "Skinned_" );
		m_armorDamageShader->SetEffectPathName( resPath.c_str() );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal effects/faders
// --------------------------------------------------------------------------------
void EveImpactOverlay::ToggleEffect( const std::string& name, bool on )
{
	if( name == "shieldboost" )
	{
		m_shieldBoosting->StartFade( on );
	}
	else if( name == "shieldhardening" )
	{
		m_shieldHardening->StartFade( on );
	}
	else if( name == "armorhardening" )
	{
		m_armorHardening->StartFade( on );
	}
	else if( name == "armorrepair" )
	{
		m_armorRepairing->StartFade( on );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets how much percentage is left of your defensives and then calculates
//   the internal configuration
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts )
{
	// what's left?
	if( shield > 0.05 )
	{
		m_configuration = IMPACT_SHIELD;
	}
	else if( armor > 0.05 )
	{
		m_configuration = IMPACT_ARMOR;
	}
	else if( hull > 0.0 )
	{
		m_configuration = IMPACT_HULL;
	}

	// always calculate the expected/desired number of impact effects
	m_armorImpactGoalCount = (size_t)( IMPACT_HOLE_TO_ARMOR_DAMAGE_RATIO * Clamp( 1.f - armor, 0.f, 1.f ) + IMPACT_HOLE_TO_HULL_DAMAGE_RATIO * Clamp( 1.f - hull, 0.f, 1.f ) );

	// hull factor
	m_hullDamageFactor = TriLinearize( 0.9f, 0.1f, hull );

	// have a color fade between full shield and zero shield
	m_shieldImpactColorFade = Clamp( pow( 1.f - shield, 2.f ), 0.f, 1.f );

	// do we forcefully have to create the amror impact holes?
	if( doCreateArmorImpacts )
	{
		for( size_t i = m_shieldImpactData.size(); i < m_armorImpactGoalCount; ++i )
		{
			CreateArmorImpact( (int)i, 0.2f + 0.8f * TriRand(), m_debugForceSpawnDebris );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new impact effect. Internal states determines
//   what effect to use
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return -1;
	}

	// what's the situation?
	switch( m_configuration )
	{
	case IMPACT_SHIELD:
		return CreateShieldImpact( damageLocatorIndex, direction, lifeTime, size );
	case IMPACT_ARMOR:
	case IMPACT_HULL:
		return CreateArmorImpact( damageLocatorIndex, size, true );
	}

	return -1;
}

// --------------------------------------------------------------------------------
// Description:
//   Shield impacts are special, they need constant updating with the direction
//   to the target. Also it returns the actual impact position
// --------------------------------------------------------------------------------
bool EveImpactOverlay::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	// valid?
	if( impactIndex == -1 )
	{
		return false;
	}

	// is it a shield effect?
	auto shieldData = m_shieldImpactData.find( impactIndex );
	if( shieldData != m_shieldImpactData.end() )
	{
		// put new direction in there
		D3DXVec3Normalize( &shieldData->second.direction, &direction );
		// and return the old "intercept" position
		out = shieldData->second.interceptPosition;
		return true;
	}

	// is it an armor effect?
	auto armorData = m_armorImpactData.find( impactIndex );
	if( armorData != m_armorImpactData.end() )
	{
		// nothing to do here
		return true;
	}

	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new shield impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	// only need normal
	Vector3 nrmDir;
	D3DXVec3Normalize( &nrmDir, &direction );
	
	// be carefull: try to find an already existing impact, which is close enough! Preferably at the same damage locator...
	int closestImpactAtSameDmgLocIdx = -1, closestImpactAtAnyDmgLocIdx = -1;
	float closestImpactAtSameDmgLocAngle = -FLT_MAX, closestImpactAtAnyDmgLocAngle = -FLT_MAX;
	for( auto it = m_shieldImpactData.begin(); it != m_shieldImpactData.end(); ++it )
	{
		float a = D3DXVec3Dot( &nrmDir, &it->second.direction );
		if( a > closestImpactAtAnyDmgLocAngle )
		{
			closestImpactAtAnyDmgLocAngle = a;
			closestImpactAtAnyDmgLocIdx = it->first;
		}
		if( damageLocatorIndex == it->second.damageLocatorIndex )
		{
			if( a > closestImpactAtSameDmgLocAngle )
			{
				closestImpactAtSameDmgLocAngle = a;
				closestImpactAtSameDmgLocIdx = it->first;
			}
		}
	}
	// if we have one that is close enough, use it instead and hand back that index
	if( closestImpactAtSameDmgLocAngle > 0.95f )
	{
		ShieldImpactData* p = &m_shieldImpactData[ closestImpactAtSameDmgLocIdx ];
		p->direction = nrmDir;
		p->timeLeft = IMPACT_SHIELD_FADEOUT * lifeTime;
		p->size = std::max( size, p->size );
		return closestImpactAtSameDmgLocIdx;
	}

	// check size limitation
	if( m_shieldImpactData.size() >= m_maxShieldImpacts )
	{
		// if we have no more room, use one of the existing ones, no matter how good they are and what locator they hit
		if( closestImpactAtAnyDmgLocIdx != -1 )
		{
			ShieldImpactData* p = &m_shieldImpactData[ closestImpactAtAnyDmgLocIdx ];
			p->direction = nrmDir;
			p->timeLeft = IMPACT_SHIELD_FADEOUT * lifeTime;
			p->size = std::max( size, p->size );
		}
		return closestImpactAtAnyDmgLocIdx;
	}

	// fill our struct, but keep it in world space
	ShieldImpactData sid;
	sid.direction = direction;
	D3DXVec3Normalize( &sid.direction, &sid.direction );
	sid.damageLocatorIndex = damageLocatorIndex;
	sid.interceptPosition = Vector3( 0.f, 0.f, 0.f );
	sid.lifeTime = sid.timeLeft = IMPACT_SHIELD_FADEOUT * lifeTime;
	sid.size = size;
	m_shieldImpactData[ m_impactDataNextIdx ] = sid;
	return m_impactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new armor impact
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateArmorImpact( int damageLocatorIndex, float size, bool spawnEffects )
{
	// be carefull: try to find an already existing impact at this index
	for( auto it = m_armorImpactData.begin(); it != m_armorImpactData.end(); ++it )
	{
		if( damageLocatorIndex == it->second.damageLocatorIndex )
		{
			// only update the size when it is bigger, so smaller lasers won't shrink the hole
			it->second.size = std::max( size, it->second.size );
			// spawn debris depends on the quality setting
			it->second.requestSpawnDebris = spawnEffects && !Tr2Renderer::IsLowQuality();
			return it->first;
		}
	}

	// fill our struct, but keep it in world space
	ArmorImpactData aid;
	aid.damageLocatorIndex = damageLocatorIndex;
	aid.size = size;
	aid.requestSpawnDebris = spawnEffects && !Tr2Renderer::IsLowQuality();
	m_armorImpactData[ m_impactDataNextIdx ] = aid;
	return m_impactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor efects
// --------------------------------------------------------------------------------
Tr2EffectPtr EveImpactOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return nullptr;
	}
	// no activity?
	if( !HasArmorActivity() )
	{
		return nullptr;
	}
	if( ( m_dataTextureBlockID == -1 ) || ( m_dataTextureOffset == -1 ) )
	{
		return nullptr;
	}

	if( batchType == TRIBATCHTYPE_DECAL )
	{
		return m_armorDamageShader;
	}
	return nullptr;
}




