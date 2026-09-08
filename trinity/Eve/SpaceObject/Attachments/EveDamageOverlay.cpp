// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveDamageOverlay.h"

#include "include/TriMath.h"
#include "Curves/Fader/Tr2ScalarFader.h"
#include "Tr2Renderer.h"
#include "Shader/Utils/Tr2DataTextureManager.h"
#include "Eve/EveUpdateContext.h"
#include "Shader/Tr2Effect.h"
#include "TriSequencer.h"
#include <random>

// settings
extern bool g_eveSpaceObjectImpactEffectEnabled;

// consts
static const float IMPACT_HOLE_TO_ARMOR_DAMAGE_RATIO = 12.f;
static const float IMPACT_HOLE_TO_HULL_DAMAGE_RATIO = 4.f;


EveDamageOverlay::EveDamageOverlay( IRoot* lockobj ) :
	m_display( true ),
	m_configuration( ITriTargetable::IMPACT_INVALID ),
	m_impactDataNextIdx( 1 ),
	m_debugForceSpawnDebris( false ),
	m_armorImpactLifeTime( 10.f ),
	m_seed( 0 ),
	m_damageLocatorCount( 0 ),
	m_lastDamageState( 1.f, 1.f, 1.f ),
	m_renderPriority( 0.f ),
	m_isVisibleLast( false ),
	m_dataTextureBlockID( -1 ),
	m_dataTextureOffset( -1 ),
	m_armorImpactGoalCount( 0 ),
	m_armorImpactParentSize( 0.f ),
	m_hullDamageFactor( 0.f )
{
	// 0
	memset( &m_impactTexelHeader, 0, sizeof( DataRow ) );

	// create the faders
	m_armorHardening.CreateInstance();
	m_armorRepairing.CreateInstance();
	m_hullRepairing.CreateInstance();
}

EveDamageOverlay::~EveDamageOverlay()
{
}

bool EveDamageOverlay::Initialize()
{
	return true;
}

void EveDamageOverlay::SetSeed( unsigned int seed )
{
	m_seed = seed;
}

void EveDamageOverlay::SetDamageLocatorCount( unsigned int count )
{
	m_damageLocatorCount = count;

	m_enabledDamageLocators.resize( count );
	for( uint32_t i = 0; i < count; i++ )
	{
		m_enabledDamageLocators[i] = i;
	}
}

void EveDamageOverlay::SetEnabledDamageLocators( std::vector<bool>::iterator begin, std::vector<bool>::iterator end )
{
	m_enabledDamageLocators.clear();
	m_enabledDamageLocators.reserve( m_damageLocatorCount );
	size_t filterSize = std::distance( begin, end );
	for( size_t i = 0; i < m_damageLocatorCount; i++ )
	{
		if( i < filterSize && !( *( begin + i ) ) )
		{
			continue;
		}
		m_enabledDamageLocators.push_back( uint32_t( i ) );
	}
}

void EveDamageOverlay::SetDebugForceSpawnDebris( bool value )
{
	m_debugForceSpawnDebris = value;
}

void EveDamageOverlay::SetHullDamageFactor( float factor )
{
	m_hullDamageFactor = factor;
}

void EveDamageOverlay::SetArmorDamageShaderEffect( Tr2Effect* shader )
{
	m_armorDamageShader = shader;
}

void EveDamageOverlay::SetHullDamageFlickerCurve( TriPerlinCurve* curve )
{
	m_hullDamageFlickerCurve = curve;
}

void EveDamageOverlay::SetArmorRepairing( Tr2ScalarFader* fader )
{
	m_armorRepairing = fader;
}

void EveDamageOverlay::SetArmorHardening( Tr2ScalarFader* fader )
{
	m_armorHardening = fader;
}

void EveDamageOverlay::SetHullRepairing( Tr2ScalarFader* fader )
{
	m_hullRepairing = fader;
}

void EveDamageOverlay::SetImpactIndexSource( EveDamageOverlay* source )
{
	m_impactIndexSource = source;
}

unsigned int EveDamageOverlay::GetSeed() const
{
	return m_seed;
}

int EveDamageOverlay::GetImpactDataNextIdx() const
{
	return m_impactDataNextIdx;
}

size_t EveDamageOverlay::GetArmorImpactGoalCount() const
{
	return m_armorImpactGoalCount;
}

float EveDamageOverlay::GetArmorImpactParentSize() const
{
	return m_armorImpactParentSize;
}

bool EveDamageOverlay::GetDebugForceSpawnDebris() const
{
	return m_debugForceSpawnDebris;
}

float EveDamageOverlay::GetHullDamageFactor() const
{
	return m_hullDamageFactor;
}

Tr2Effect* EveDamageOverlay::GetArmorDamageShaderEffect() const
{
	return m_armorDamageShader;
}

TriPerlinCurve* EveDamageOverlay::GetHullDamageFlickerCurve() const
{
	return m_hullDamageFlickerCurve;
}

Tr2ScalarFader* EveDamageOverlay::GetArmorRepairing() const
{
	return m_armorRepairing;
}

Tr2ScalarFader* EveDamageOverlay::GetArmorHardening() const
{
	return m_armorHardening;
}

Tr2ScalarFader* EveDamageOverlay::GetHullRepairing() const
{
	return m_hullRepairing;
}

float EveDamageOverlay::GetRenderPriority() const
{
	return m_renderPriority;
}

int32_t EveDamageOverlay::GetDataTextureOffset() const
{
	return m_dataTextureOffset;
}

int32_t EveDamageOverlay::GetDataTextureBlockID() const
{
	return m_dataTextureBlockID;
}

ITriTargetable::ImpactConfiguration EveDamageOverlay::GetImpactConfiguration() const
{
	return m_configuration;
}

float EveDamageOverlay::GetArmorImpactLifeTime() const
{
	return m_armorImpactLifeTime;
}

Vector3 EveDamageOverlay::GetLastDamageState() const
{
	return m_lastDamageState;
}

EveDamageOverlay::DataRow& EveDamageOverlay::HeaderRow()
{
	return m_impactTexelHeader;
}

EveDamageOverlay::DataRow& EveDamageOverlay::TexelRow( size_t index )
{
	return m_impactTexelData[index];
}

std::map<int, EveDamageOverlay::ArmorImpactData>& EveDamageOverlay::ArmorImpacts()
{
	return m_armorImpactData;
}

int EveDamageOverlay::AllocateImpactIndex()
{
	return m_impactIndexSource ? m_impactIndexSource->AllocateImpactIndex() : m_impactDataNextIdx++;
}

// --------------------------------------------------------------------------------
// Description:
//   Do all the math-heavy conversion here async
// --------------------------------------------------------------------------------
void EveDamageOverlay::UpdateAsyncronous( const EveUpdateContext& updateContext, const OwnerInfo& info, size_t minTexelRows, bool hasExternalActivity )
{
	// check if the impact count goal is less than what we have
	if( m_armorImpactGoalCount < m_armorImpactData.size() )
	{
		// close up only the excess holes, so get an "advanced" map iterator
		auto aidit = m_armorImpactData.begin();
		std::advance( aidit, m_armorImpactGoalCount );
		// ok, we want to have less impacts, so close the holes
		while( aidit != m_armorImpactData.end() )
		{
			aidit->second.size -= updateContext.GetDeltaT() / m_armorImpactLifeTime;
			if( aidit->second.size <= 0.f )
			{
				m_armorImpactData.erase( aidit++ );
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
	m_hullRepairing->Update( updateContext );

	// resize the texture data array based on both external (shield) and armor impacts
	m_impactTexelData.resize( std::max( minTexelRows, m_armorImpactData.size() ) );

	// the block header is the first column in the data texture, set our half of it
	m_impactTexelHeader.v[2] = Vector4( float( m_armorImpactData.size() ),
										m_armorImpactParentSize,
										m_hullRepairing->GetFaderValue(),
										m_hullRepairing->GetKickInValue() );
	m_impactTexelHeader.v[3] = Vector4( m_armorRepairing->GetFaderValue(),
										m_armorHardening->GetFaderValue(),
										m_armorRepairing->GetKickInValue(),
										m_armorHardening->GetKickInValue() );

	// no activity?
	if( !hasExternalActivity && !HasGeneralActivity() )
	{
		return;
	}

	// calculate render priority, but take into account the visibility of the last frame. To counter
	// the fact that the actual visibility data might not be correct (because of picking etc.)
	// needs fixing! Steve, 2015
	if( m_isVisibleLast )
	{
		m_renderPriority = info.estimatedPixelDiameter;
	}
	else
	{
		m_renderPriority = info.isInFrustum ? info.estimatedPixelDiameter : 0.f;
	}
	m_isVisibleLast = info.isInFrustum;

	// cut off the owner size at some hard-coded size, so armor impacts on giant ships get smaller
	m_armorImpactParentSize = std::min( info.boundingSphere.w, IMPACT_ARMOR_SIZE_MAX / IMPACT_ARMOR_SIZE_FACTOR );

	if( !m_armorImpactData.empty() && info.getDamageLocatorPositionOS )
	{
		// armor
		size_t i = 0;
		for( auto aidit = m_armorImpactData.begin(); aidit != m_armorImpactData.end(); ++aidit )
		{
			ArmorImpactData* armorData = &aidit->second;
			DataRow* texelData = &m_impactTexelData[i];

			// size of impact
			float size = armorData->size * IMPACT_ARMOR_SIZE_FACTOR * m_armorImpactParentSize;
			// get position from damage locator, in the owner's object space
			Vector3 tgtPosOS( 0.f, 0.f, 0.f );
			info.getDamageLocatorPositionOS( armorData->damageLocatorIndex, tgtPosOS );
			texelData->v[2] = Vector4( tgtPosOS, 0.f );
			texelData->v[3] = Vector4( size, 0.f, 0.f, 0.f );

			++i;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Do everything non-threadsafe here
// --------------------------------------------------------------------------------
void EveDamageOverlay::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	UpdateBlockData( updateContext.GetDataTextureManager(), HasGeneralActivity() );
}

void EveDamageOverlay::UpdateBlockData( Tr2DataTextureManager* dataTextureMgr, bool hasActivity )
{
	if( !hasActivity )
	{
		m_dataTextureBlockID = -1;
		return;
	}

	// what's our offset in pixels for the data texture?
	m_dataTextureOffset = dataTextureMgr->GetTextureOffset( m_dataTextureBlockID );

	// update block data
	m_dataTextureBlockID = dataTextureMgr->RequestBlockData( &m_impactTexelHeader.v[0],
															 (uint32_t)m_impactTexelData.size(),
															 m_impactTexelData.empty() ? nullptr : &m_impactTexelData[0].v[0],
															 m_renderPriority );
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is armor activity
// --------------------------------------------------------------------------------
bool EveDamageOverlay::HasArmorActivity() const
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
//   Small helper function that checks if there is hull activity
// --------------------------------------------------------------------------------
bool EveDamageOverlay::HasHullActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// hull?
	return !m_hullRepairing->IsZero();
}

// --------------------------------------------------------------------------------
// Description:
//   Small helper function that checks if there is general activity
// --------------------------------------------------------------------------------
bool EveDamageOverlay::HasGeneralActivity() const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return false;
	}

	// hull or armor?
	return HasHullActivity() || HasArmorActivity();
}

// --------------------------------------------------------------------------------
// Description:
//   Damage overlays can modulate the activation strength, to let the lights
//   flicker etc.
// --------------------------------------------------------------------------------
float EveDamageOverlay::GetActivationStrength( const EveUpdateContext& updateContext ) const
{
	// settings
	if( !g_eveSpaceObjectImpactEffectEnabled )
	{
		return 1.f;
	}

	// comes from a curve if we have hull damage
	if( m_hullDamageFactor > 0.f )
	{
		if( m_hullDamageFlickerCurve )
		{
			// Clamp the flicker curve so we don't get a zero value from the curve
			float result = TriClamp( m_hullDamageFlickerCurve->Update( updateContext.GetTime() ), 0.3f, 1.0f );
			return result / std::exp( m_hullDamageFactor );
		}
	}

	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Easy-to-use access to the internal effects/faders
// --------------------------------------------------------------------------------
void EveDamageOverlay::ToggleEffect( const char* name, bool on, float duration )
{
	if( strcmp( name, "armorhardening" ) == 0 )
	{
		m_armorHardening->StartFade( on, duration / 4.f );
	}
	else if( strcmp( name, "armorrepair" ) == 0 )
	{
		m_armorRepairing->StartFade( on, duration / 4.f );
	}
	else if( strcmp( name, "hullrepair" ) == 0 )
	{
		m_hullRepairing->StartFade( on, duration / 4.f );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets how much percentage is left of your defensives and then calculates
//   the internal configuration
// --------------------------------------------------------------------------------
void EveDamageOverlay::SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts )
{
	// what's left?
	if( shield > 0.05 )
	{
		m_configuration = ITriTargetable::IMPACT_SHIELD;
	}
	else if( armor > 0.05 )
	{
		m_configuration = ITriTargetable::IMPACT_ARMOR;
	}
	else if( hull > 0.0 )
	{
		m_configuration = ITriTargetable::IMPACT_HULL;
	}

	// always calculate the expected/desired number of impact effects
	m_armorImpactGoalCount = (size_t)( IMPACT_HOLE_TO_ARMOR_DAMAGE_RATIO * TriClamp( 1.f - armor, 0.f, 1.f ) + IMPACT_HOLE_TO_HULL_DAMAGE_RATIO * TriClamp( 1.f - hull, 0.f, 1.f ) );

	// hull factor
	m_hullDamageFactor = TriLinearize( 0.9f, 0.1f, hull );
	if( m_hullDamageFlickerCurve )
	{
		float flickerCurveModifier = TriLinearize( 1.0f, 0.0f, hull );
		// Modify the flickercurve so it scales with the damage factor
		m_hullDamageFlickerCurve->mScale = flickerCurveModifier;
		m_hullDamageFlickerCurve->mOffset = 1.0f - flickerCurveModifier;
	}

	// do we forcefully have to create the armor impact holes?
	if( doCreateArmorImpacts && !m_enabledDamageLocators.empty() )
	{
		// create a random seed that is m_seed and also the armor impact size (so we get some variation into the damage)
		auto generator = std::mt19937();
		generator.seed( m_seed + (unsigned)m_armorImpactData.size() );
		std::uniform_int_distribution<int> damageLocatorDistribution( 0, int( m_enabledDamageLocators.size() ) - 1 );
		std::uniform_real_distribution<float> damageSizeDistribution( 0.2f, 0.8f );

		for( size_t i = m_armorImpactData.size(); i < m_armorImpactGoalCount; ++i )
		{
			CreateImpact( m_enabledDamageLocators[damageLocatorDistribution( generator )], damageSizeDistribution( generator ), m_debugForceSpawnDebris );
		}
	}

	m_lastDamageState = Vector3( shield, armor, hull );
}

// --------------------------------------------------------------------------------
// Description:
//   Takes out all damage/impact effects. Heals the object.
// --------------------------------------------------------------------------------
void EveDamageOverlay::Clear()
{
	// remove all impacts
	m_armorImpactData.clear();
}

// --------------------------------------------------------------------------------
// Description:
//   Use this method to add a new armor impact
// --------------------------------------------------------------------------------
int EveDamageOverlay::CreateImpact( int damageLocatorIndex, float size, bool spawnEffects )
{
	// be careful: try to find an already existing impact at this index
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

	ArmorImpactData aid;
	aid.damageLocatorIndex = damageLocatorIndex;
	aid.size = size;
	aid.requestSpawnDebris = spawnEffects && !Tr2Renderer::IsLowQuality();
	int impactIndex = AllocateImpactIndex();
	m_armorImpactData[impactIndex] = aid;
	return impactIndex;
}

bool EveDamageOverlay::HasImpact( int impactIndex ) const
{
	return m_armorImpactData.find( impactIndex ) != m_armorImpactData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out the shader for armor effects
// --------------------------------------------------------------------------------
Tr2Effect* EveDamageOverlay::GetArmorDamageShader( TriBatchType batchType ) const
{
	if( !m_display )
	{
		return nullptr;
	}

	if( batchType != TRIBATCHTYPE_DECAL )
	{
		return nullptr;
	}

	if( ( m_dataTextureBlockID == -1 ) || ( m_dataTextureOffset == -1 ) )
	{
		return nullptr;
	}

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
	return m_armorDamageShader;
}
