// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveImpactOverlay.h"

#include "Utilities/StringUtils.h"
#include "include/TriMath.h"
#include "Curves/Fader/Tr2ScalarFader.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshBase.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2GpuUniqueEmitter.h"
#include "Shader/Tr2Effect.h"
#include "TriSequencer.h"

// settings
extern bool g_eveSpaceObjectImpactEffectEnabled;

// consts
static const float IMPACT_SHIELD_SIZE_MAX = 2000.f;
static const float IMPACT_SHIELD_SIZE_MIN = 70.f;
static const float IMPACT_SHIELD_FADEOUT = 1.5f;
static const float IMPACT_ARMOR_PARTICLE_LOD_FACTOR = 400.f;


EveImpactOverlay::EveImpactOverlay( IRoot* lockobj ) :
	m_display( true ),
	m_overallShieldImpact( -1.f ),
	m_shieldIsEllipsoid( true ),
	m_maxShieldImpacts( 8 ),
	m_shieldImpactColorFade( 0.f ),
	m_shieldImpactParentSize( 0.f )
{
	m_damageOverlay.CreateInstance();

	// create the faders
	m_shieldBoosting.CreateInstance();
	m_shieldHardening.CreateInstance();
}

EveImpactOverlay::~EveImpactOverlay()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Setup this overlay with data
// --------------------------------------------------------------------------------
void EveImpactOverlay::Set( TriPerlinCurvePtr hullDamageFlickerCurve, Tr2GpuUniqueEmitterPtr armorDamageEmitter, Tr2GpuUniqueEmitterPtr hullImpactEmitter, Tr2EffectPtr armorDamageShader, Tr2MeshBase* shieldImpactMesh, bool shieldIsEllipsoid )
{
	m_shieldIsEllipsoid = shieldIsEllipsoid;
	m_armorImpactEmitter = armorDamageEmitter;
	m_hullImpactEmitter = hullImpactEmitter;
	m_mesh = shieldImpactMesh;
	m_damageOverlay->SetHullDamageFlickerCurve( hullDamageFlickerCurve );
	m_damageOverlay->SetArmorDamageShaderEffect( armorDamageShader );
}


// --------------------------------------------------------------------------------
// Description:
//   Sets the name of the impact overlay, this is used for seeding the randomness of
//	 the impacts between session changes
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetSeed( unsigned int seed )
{
	m_damageOverlay->SetSeed( seed );
}


// --------------------------------------------------------------------------------
// Description:
//   Sets the amount of damage locators, used for the randomness of the impacts
//   between session change
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetDamageLocatorCount( unsigned int count )
{
	m_damageOverlay->SetDamageLocatorCount( count );
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
void EveImpactOverlay::UpdateSyncronous( const EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// do we have something to do at all?
	if( !HasGeneralActivity() )
	{
		m_damageOverlay->UpdateBlockData( nullptr, false );
		return;
	}

	// this comes from the scene via EveUpdateContext
	Tr2DataTextureManagerPtr dataTextureMgr = updateContext.GetDataTextureManager();
	m_damageOverlay->UpdateBlockData( dataTextureMgr, true );

	// spawn armor impact particles?
	if( updateContext.GetGpuParticleSystem() )
	{
		if( m_armorImpactEmitter )
		{
			float armorImpactParentSize = m_damageOverlay->GetArmorImpactParentSize();
			if( armorImpactParentSize > 0.f )
			{
				auto& armorImpactData = m_damageOverlay->ArmorImpacts();
				for( auto aidit = armorImpactData.begin(); aidit != armorImpactData.end(); ++aidit )
				{
					if( aidit->second.requestSpawnDebris )
					{
						// where?
						Vector3 impactPosWS( 0.f, 0.f, 0.f );
						parent->GetDamageLocatorPosition( &impactPosWS, aidit->second.damageLocatorIndex, true );
						m_armorImpactEmitter->SetPosition( &impactPosWS );
						// facing?
						Vector3 impactDirWS( 0.f, 1.f, 0.f );
						parent->GetDamageLocatorDirection( &impactDirWS, aidit->second.damageLocatorIndex, true );
						m_armorImpactEmitter->SetDirection( &impactDirWS );
						// velocity?
						Vector3 parentVelocityWS;
						parent->GetWorldVelocity( parentVelocityWS );

						// scaling?
						float scale = aidit->second.size * armorImpactParentSize / ( IMPACT_ARMOR_SIZE_MAX / IMPACT_ARMOR_SIZE_FACTOR );
						// loding for emit rate?
						float rateModifier = TriClamp( m_damageOverlay->GetRenderPriority() / IMPACT_ARMOR_PARTICLE_LOD_FACTOR, 0.f, 1.f );
						// put together particle update info
						ITr2GenericEmitter::UpdateArguments args( updateContext.GetTime(), updateContext.GetGpuParticleSystem(), IdentityMatrix(), updateContext.GetOriginShift() );
						// do the spawn here once!
						m_armorImpactEmitter->SpawnOnce( args, parentVelocityWS, scale, rateModifier );
						aidit->second.requestSpawnDebris = false;

						if( m_hullImpactEmitter && m_damageOverlay->GetImpactConfiguration() == ITriTargetable::IMPACT_HULL )
						{
							m_hullImpactEmitter->SetPosition( &impactPosWS );
							m_hullImpactEmitter->SetDirection( &impactDirWS );

							// do the spawn here once!
							m_hullImpactEmitter->SpawnOnce( args, parentVelocityWS, scale, rateModifier );
						}
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
void EveImpactOverlay::UpdateAsyncronous( const EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	// first always reduce shield impacts
	for( auto sidit = m_shieldImpactData.begin(); sidit != m_shieldImpactData.end(); )
	{
		sidit->second.timeLeft -= updateContext.GetDeltaT();
		if( sidit->second.timeLeft <= 0.f )
		{
			m_shieldImpactData.erase( sidit++ );
		}
		else
		{
			++sidit;
		}
	}

	// update the faders
	m_shieldBoosting->Update( updateContext );
	m_shieldHardening->Update( updateContext );

	// armor/hull half, including the shared data texture block
	EveDamageOverlay::OwnerInfo info;
	parent->GetBoundingSphere( info.boundingSphere );
	info.estimatedPixelDiameter = parent->GetEstimatedPixelDiameter();
	info.isInFrustum = parent->IsInFrustum();
	info.getDamageLocatorPositionOS = [parent]( int index, Vector3& out ) {
		return parent->GetDamageLocatorPosition( &out, index, false );
	};
	m_damageOverlay->UpdateAsyncronous( updateContext, info, m_shieldImpactData.size(), HasShieldActivity() );

	// the shield half of the block header
	EveDamageOverlay::DataRow& header = m_damageOverlay->HeaderRow();
	header.v[0] = Vector4( float( m_shieldImpactData.size() ),
						   m_overallShieldImpact,
						   m_shieldImpactColorFade,
						   m_shieldImpactParentSize );
	header.v[1] = Vector4( m_shieldHardening->GetFaderValue(),
						   m_shieldBoosting->GetFaderValue(),
						   m_shieldHardening->GetKickInValue(),
						   m_shieldBoosting->GetKickInValue() );

	// no activity?
	if( !HasGeneralActivity() )
	{
		return;
	}

	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	parentInverseWorldTransform = Inverse( parentWorldTransform );

	m_shieldImpactParentSize = TriClamp( info.boundingSphere.w, IMPACT_SHIELD_SIZE_MIN, IMPACT_SHIELD_SIZE_MAX );

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
			EveDamageOverlay::DataRow& texelData = m_damageOverlay->TexelRow( i );

			// get worldpos of damagelocator from parent
			Vector3 tgtPosWS( 0.f, 0.f, 0.f );
			parent->GetDamageLocatorPosition( &tgtPosWS, shieldData->damageLocatorIndex, true );

			Vector3 pos = GetShieldImpactPosition( parentInverseWorldTransform, tgtPosWS, shieldData->direction, shieldEllipsoidCenter, shieldEllipsoidRadii );

			// "encode" it in texels
			texelData.v[0] = Vector4( pos, shieldData->timeLeft );
			texelData.v[1] = Vector4( shieldData->size, shieldData->intensity, 0.f, shieldData->lifeTime );
			// also need this intercept position in WS
			shieldData->interceptPosition = TransformCoord( pos, parentWorldTransform );

			++i;
		}
	}
}

Vector3 EveImpactOverlay::GetShieldImpactPosition( const Matrix& parentInverseWorldTransform, const Vector3& damageLocatorPosWS, const Vector3& impactDirection, const Vector3& shieldEllipsoidCenter, const Vector3& shieldEllipsoidRadii )
{
	// calculate point, but depends on shield type
	Vector3 p( 0.f, 0.f, 0.f );
	if( m_shieldIsEllipsoid )
	{
		// convert position and direction into object space
		Vector3 tgtPosOS, dirOS;
		tgtPosOS = TransformCoord( damageLocatorPosWS, parentInverseWorldTransform );
		dirOS = TransformNormal( impactDirection, parentInverseWorldTransform );
		// intersections
		IntersectEllipsoidRay( p, shieldEllipsoidCenter, shieldEllipsoidRadii, tgtPosOS, dirOS );
	}
	else
	{
		// just use locator pos, no ellipsoid
		p = TransformCoord( damageLocatorPosWS, parentInverseWorldTransform );
	}
	return p;
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveImpactOverlay::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, float screenSize )
{
	if( !m_display )
	{
		return;
	}
	if( !m_mesh )
	{
		return;
	}
	if( ( m_damageOverlay->GetDataTextureBlockID() == -1 ) || ( m_damageOverlay->GetDataTextureOffset() == -1 ) )
	{
		return;
	}

	// anything on shields?
	if( HasShieldActivity() )
	{
		const Tr2MeshAreaVector* areas = m_mesh->GetAreas( batchType );
		m_mesh->GetBatches( accumulator, areas, perObjectData, screenSize );
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

bool EveImpactOverlay::HasArmorActivity() const
{
	return m_damageOverlay->HasArmorActivity();
}

bool EveImpactOverlay::HasHullActivity() const
{
	return m_damageOverlay->HasHullActivity();
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is general activity
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasGeneralActivity() const
{
	// hull, armor or shield?
	return m_damageOverlay->HasGeneralActivity() || HasShieldActivity();
}

// --------------------------------------------------------------------------------
// Description:
//   Just a getter for the texture offset. Nothing special. Move on.
// --------------------------------------------------------------------------------
int32_t EveImpactOverlay::GetDataTextureOffset() const
{
	return m_damageOverlay->GetDataTextureOffset();
}

// --------------------------------------------------------------------------------
// Description:
//   Check if a certain type of defense is there
// --------------------------------------------------------------------------------
ITriTargetable::ImpactConfiguration EveImpactOverlay::GetImpactConfiguration() const
{
	return m_damageOverlay->GetImpactConfiguration();
}


// --------------------------------------------------------------------------------
// Description:
//   Check if a certain type of defense is there
// --------------------------------------------------------------------------------
bool EveImpactOverlay::HasShieldEllipsoid() const
{
	return m_shieldIsEllipsoid;
}


float EveImpactOverlay::GetActivationStrength( const EveUpdateContext& updateContext ) const
{
	return m_damageOverlay->GetActivationStrength( updateContext );
}

float EveImpactOverlay::GetArmorImpactLifeTime() const
{
	return m_damageOverlay->GetArmorImpactLifeTime();
}

Vector3 EveImpactOverlay::GetLastDamageState() const
{
	return m_damageOverlay->GetLastDamageState();
}

EveDamageOverlayPtr EveImpactOverlay::GetDamageOverlay() const
{
	return m_damageOverlay;
}

unsigned int EveImpactOverlay::GetSeed() const
{
	return m_damageOverlay->GetSeed();
}

int EveImpactOverlay::GetImpactDataNextIdx() const
{
	return m_damageOverlay->GetImpactDataNextIdx();
}

size_t EveImpactOverlay::GetArmorImpactGoalCount() const
{
	return m_damageOverlay->GetArmorImpactGoalCount();
}

float EveImpactOverlay::GetArmorImpactParentSize() const
{
	return m_damageOverlay->GetArmorImpactParentSize();
}

bool EveImpactOverlay::GetDebugForceSpawnDebris() const
{
	return m_damageOverlay->GetDebugForceSpawnDebris();
}

void EveImpactOverlay::SetDebugForceSpawnDebris( bool value )
{
	m_damageOverlay->SetDebugForceSpawnDebris( value );
}

float EveImpactOverlay::GetRenderPriority() const
{
	return m_damageOverlay->GetRenderPriority();
}

int32_t EveImpactOverlay::GetDataTextureBlockID() const
{
	return m_damageOverlay->GetDataTextureBlockID();
}

float EveImpactOverlay::GetHullDamageFactor() const
{
	return m_damageOverlay->GetHullDamageFactor();
}

void EveImpactOverlay::SetHullDamageFactor( float factor )
{
	m_damageOverlay->SetHullDamageFactor( factor );
}

Tr2Effect* EveImpactOverlay::GetArmorDamageShaderEffect() const
{
	return m_damageOverlay->GetArmorDamageShaderEffect();
}

void EveImpactOverlay::SetArmorDamageShaderEffect( Tr2Effect* shader )
{
	m_damageOverlay->SetArmorDamageShaderEffect( shader );
}

TriPerlinCurve* EveImpactOverlay::GetHullDamageFlickerCurve() const
{
	return m_damageOverlay->GetHullDamageFlickerCurve();
}

void EveImpactOverlay::SetHullDamageFlickerCurve( TriPerlinCurve* curve )
{
	m_damageOverlay->SetHullDamageFlickerCurve( curve );
}

Tr2ScalarFader* EveImpactOverlay::GetArmorRepairing() const
{
	return m_damageOverlay->GetArmorRepairing();
}

void EveImpactOverlay::SetArmorRepairing( Tr2ScalarFader* fader )
{
	m_damageOverlay->SetArmorRepairing( fader );
}

Tr2ScalarFader* EveImpactOverlay::GetArmorHardening() const
{
	return m_damageOverlay->GetArmorHardening();
}

void EveImpactOverlay::SetArmorHardening( Tr2ScalarFader* fader )
{
	m_damageOverlay->SetArmorHardening( fader );
}

Tr2ScalarFader* EveImpactOverlay::GetHullRepairing() const
{
	return m_damageOverlay->GetHullRepairing();
}

void EveImpactOverlay::SetHullRepairing( Tr2ScalarFader* fader )
{
	m_damageOverlay->SetHullRepairing( fader );
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal effects/faders
// --------------------------------------------------------------------------------
void EveImpactOverlay::ToggleEffect( const std::string& name, bool on, float duration )
{
	if( name == "shieldboost" )
	{
		m_shieldBoosting->StartFade( on, duration / 4.f );
	}
	else if( name == "shieldhardening" )
	{
		m_shieldHardening->StartFade( on, duration / 4.f );
	}
	else
	{
		m_damageOverlay->ToggleEffect( name.c_str(), on, duration );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets how much percentage is left of your defensives and then calculates
//   the internal configuration
// --------------------------------------------------------------------------------
void EveImpactOverlay::SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts )
{
	// have a color fade between full shield and zero shield
	m_shieldImpactColorFade = TriClamp( pow( 1.f - shield, 2.f ), 0.f, 1.f );

	m_damageOverlay->SetDamageState( shield, armor, hull, doCreateArmorImpacts );
}

// --------------------------------------------------------------------------------
// Description:
//   Takes out all damage/impact effects. Heals the ship.
// --------------------------------------------------------------------------------
void EveImpactOverlay::Clear()
{
	// remove all impacts
	m_shieldImpactData.clear();
	m_damageOverlay->Clear();
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new impact effect. Internal states determines
//   what effect to use
// --------------------------------------------------------------------------------
int EveImpactOverlay::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size, float intensity, Tr2Lod lod, EveSpaceObject2* parent )
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return -1;
	}

	// what's the situation?
	ITriTargetable::ImpactConfiguration configuration = m_damageOverlay->GetImpactConfiguration();
	if( configuration == ITriTargetable::IMPACT_SHIELD && lod != TR2_LOD_LOW )
	{
		return CreateShieldImpact( damageLocatorIndex, direction, lifeTime, size, intensity, parent );
	}
	else if( configuration == ITriTargetable::IMPACT_ARMOR || configuration == ITriTargetable::IMPACT_HULL )
	{
		bool spawnEffects = lod != TR2_LOD_LOW;
		return m_damageOverlay->CreateImpact( damageLocatorIndex, size, spawnEffects );
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
		shieldData->second.direction = direction;
		// and return the old "intercept" position
		out = shieldData->second.interceptPosition;
		return true;
	}

	// is it an armor effect?
	if( m_damageOverlay->HasImpact( impactIndex ) )
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
int EveImpactOverlay::CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size, float intensity, EveSpaceObject2* parent )
{
	// only need normal
	Vector3 nrmDir = Normalize( direction );

	// be carefull: try to find an already existing impact, which is close enough! Preferably at the same damage locator...
	int closestImpactAtSameDmgLocIdx = -1, closestImpactAtAnyDmgLocIdx = -1;
	float closestImpactAtSameDmgLocAngle = -FLT_MAX, closestImpactAtAnyDmgLocAngle = -FLT_MAX;
	for( auto it = m_shieldImpactData.begin(); it != m_shieldImpactData.end(); ++it )
	{
		float a = Dot( nrmDir, it->second.direction );
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
		ShieldImpactData* p = &m_shieldImpactData[closestImpactAtSameDmgLocIdx];
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
			ShieldImpactData* p = &m_shieldImpactData[closestImpactAtAnyDmgLocIdx];
			p->direction = nrmDir;
			p->timeLeft = IMPACT_SHIELD_FADEOUT * lifeTime;
			p->size = std::max( size, p->size );
		}
		return closestImpactAtAnyDmgLocIdx;
	}


	// need the inverse world matrix
	Matrix parentWorldTransform, parentInverseWorldTransform;
	parent->GetLocalToWorldTransform( parentWorldTransform );
	parentInverseWorldTransform = Inverse( parentWorldTransform );

	// get parent's bounding ellipsoid shape
	Vector3 shieldEllipsoidRadii( 1.f, 1.f, 1.f ), shieldEllipsoidCenter( 0.f, 0., 0.f );
	parent->GetShapeEllipsoid( shieldEllipsoidCenter, shieldEllipsoidRadii );

	// get worldpos of damagelocator from parent
	Vector3 tgtPosWS( 0.f, 0.f, 0.f );
	parent->GetDamageLocatorPosition( &tgtPosWS, damageLocatorIndex, true );

	Vector3 shieldImpact = GetShieldImpactPosition( parentInverseWorldTransform, tgtPosWS, Normalize( direction ), shieldEllipsoidCenter, shieldEllipsoidRadii );

	// fill our struct, but keep it in world space
	ShieldImpactData sid;
	sid.direction = Normalize( direction );
	sid.damageLocatorIndex = damageLocatorIndex;
	sid.interceptPosition = TransformCoord( shieldImpact, parentWorldTransform );
	sid.lifeTime = sid.timeLeft = IMPACT_SHIELD_FADEOUT * lifeTime;
	sid.size = size;
	sid.intensity = intensity;
	int impactIndex = m_damageOverlay->AllocateImpactIndex();
	m_shieldImpactData[impactIndex] = sid;
	return impactIndex;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor efects
// --------------------------------------------------------------------------------
Tr2Effect* EveImpactOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	return m_damageOverlay->GetArmorDamageShader( batchType );
}
