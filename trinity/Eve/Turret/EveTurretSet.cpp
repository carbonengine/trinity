// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "EveTurretSet.h"

#include "Shader/Tr2Effect.h"
#include "Resources/TriGeometryRes.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustumOrtho.h"
#include "TriObserverLocal.h"
#include "TriRenderBatch.h"
#include "Tr2Renderer.h"

#include <ITr2AudEmitter.h>
#include "Eve/EveUpdateContext.h"
#include "Eve/Turret/EveTurretTarget.h"
#include "EveTurretFiringFX.h"
#include "include/TriMath.h"
#include "Tr2QuadRenderer.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;
extern bool g_brokenMacOSNvidiaDrivers;


// names of system bones like they are in the granny file
static std::string s_systemBoneSkeletonNames[] = {
	"invalid", // SYSBONE_INVALID
	"Sys_Rotation_Arm", // SYSBONE_ROTATION
	"Sys_Rotation_Arm01", // SYSBONE_ROTATION1
	"Sys_Rotation_Arm02", // SYSBONE_ROTATION2
	"Sys_CounterRotation", // SYSBONE_COUNTER_ROTATION
	"Sys_Pitch_Barrel", // SYSBONE_PITCH
	"Sys_Pitch_Barrel1", // SYSBONE_PITCH1
	"Sys_Pitch_Barrel2", // SYSBONE_PITCH2
	"Sys_Height", // SYSBONE_SCALED_HEIGHT
	"Sys_Pitch_Arm01", // SYSBONE_SCALED_PITCH01
	"Sys_Pitch_Arm02", // SYSBONE_SCALED_PITCH02
	"Sys_Pitch_Arm03", // SYSBONE_SCALED_PITCH03
	"Sys_Pitch_Arm04", // SYSBONE_SCALED_PITCH04
	"Sys_Pitch_Arm05", // SYSBONE_SCALED_PITCH05
	"Sys_Pitch_Arm06", // SYSBONE_SCALED_PITCH06
};

// invalids
const unsigned int INVALID_BONE_INDEX = 0xffffffff;
const unsigned int INVALID_TURRET_INDEX = 0xffffffff;

// some very static timings, no need to confuse artists by exposing them
const float TRACKING_FADE_TIME = 1.f;


namespace
{
bool IsUsingCMF( TriGeometryRes* geometryResource )
{
	// The only reason the null pointer check is here, is to be able to call IsUsingCMF.
	// Do not rely on this function to test for geometryResource's existance in other places!
	// This function will be removed in the future, alongside all granny code.
	return !geometryResource || !geometryResource->IsGood() || geometryResource->IsUsingCMF();
}
}


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to inlavid/empty and call
//   ::PrepareResouce(), which will create a vertex decleration and a
//   special istance buffer for the instance rendering. Also load the
//   shader for shadow generation
// --------------------------------------------------------------------------------
EveTurretSet::EveTurretSet( IRoot* lockobj ) :
	m_display( true ),
	m_displayEffects( true ),
	m_isOnline( true ),
	m_updatePitchPose( false ),
	m_useDynamicBounds( false ),
	m_visibleCount( 0 ),
	m_estimatedPixelDiameter( -1.f ),
	m_lodLevel( LOD_INVALID ),
	m_trackingInfluence( 0.f ),
	m_trackingInfluenceDelta( 0.f ),
	m_delayToFadeOutTracking( 0.f ),
	m_delayToFadeInTracking( 0.f ),
	m_maxTrackingTime( 1.f ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_bottomClipHeight( 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_turretVertexDeclElementCount( 0 ),
#if WITH_GRANNY
	m_grnModel( NULL ),
	m_grnMeshBinding( NULL ),
#endif
	m_skeleton( nullptr ),
	m_useRandomFiringDelay( true ),
	m_randomFiringDelay( 0.f ),
	m_maxCyclingFirePos( 1 ),
	m_cyclingFireGroupCount( 1 ),
	m_currentCyclingFiresPos( 0 ),
	m_sysBoneHeight( 1.f ),
	m_sysBonePitchOffset( 0.f ),
	m_sysBonePitchFactor( 1.f ),
	m_sysBonePitchMin( 0.f ),
	m_sysBonePitchMax( 90.f ),
	m_sysBonePitch01Offset( 0.f ),
	m_sysBonePitch01Factor( 1.f ),
	m_sysBonePitch02Offset( 0.f ),
	m_sysBonePitch02Factor( 1.f ),
	m_sysBonePitch03Offset( 0.f ),
	m_sysBonePitch03Factor( 1.f ),
	m_state( STATE_IDLE ),
	m_activeTurret( INVALID_TURRET_INDEX ),
	m_recheckTimeLeft( -1.f ),
	m_laserMissBehaviour( false ),
	m_projectileMissBehaviour( false ),
	m_impactSize( 0.f ),
	m_firingEffectMuzzlePosSet( false ),
	m_slotNumber( -1 ),
	m_swarmID( 0 ),
	m_parentShLighting( nullptr ),
	m_chooseRandomLocator( true ),
	m_randomizeExplosionRotation( true ),
	m_ambientEffectEditingMode( false ),
	m_ambientOffsetMatrix( IdentityMatrix() ),
	m_lowLodFiringEffectTranslation( 0, 0, 0 ),
	m_lowLodFiringEffectScale( 1, 1, 1 ),
	m_lowLodFiringEffectRotation( 0, 0, 0, 1 ),
	m_useLowLodFiringTransform( false ),
	m_impactBehaviour( ImpactBehaviour::DAMAGE_LOCATOR ),
	m_playMovementSound( true ),
	m_idleToTargetingMovementAudioEvent( L"" ),
	m_targetingToIdleMovementAudioEvent( L"" )
{
	// 0
	memset( &m_parentData, 0, sizeof( EveSpaceObject2::ParentData ) );
	m_parentData.transform = IdentityMatrix();
	m_shipTransformPrev = IdentityMatrix();

	//m_rtMeshArea.reset( new Tr2RaytracingMeshArea( 0 ) );
	//m_rtMesh.reset( new Tr2RaytracingMesh() );

	for( unsigned int i = 0; i < SYSBONE_MAX; ++i )
	{
		m_systemBoneID[i] = INVALID_BONE_INDEX;
	}

	// create target data
	m_target.CreateInstance();

	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveTurretSet::~EveTurretSet()
{
	if( m_firingEffect )
	{
		m_firingEffect->CleanUp();
	}

	Cleanup();

	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we have the name of the granny2, so we
//   create a geometry resource
// --------------------------------------------------------------------------------
bool EveTurretSet::Initialize()
{
	// pass down some user-defined data into sub-modules we don't save out
	m_target->SetBehaviour( m_laserMissBehaviour, m_projectileMissBehaviour, m_impactSize, m_impactBehaviour );

	// geom path is here, so load it
	InitializeGeometryResource();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   If someone changed the name of the granny2 file, we must re-create the
//   geometry resource
// --------------------------------------------------------------------------------
bool EveTurretSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	else if( IsMatch( value, m_geomResPath ) )
	{
		ReRegister();
		// new gr2 file specified -> reload!
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_ambientEffectEditingMode ) )
	{
		SetAmbientEffect( m_ambientEffect );
	}
	else if( IsMatch( value, m_laserMissBehaviour ) || IsMatch( value, m_projectileMissBehaviour ) || IsMatch( value, m_impactSize ) || IsMatch( value, m_impactBehaviour ) )
	{
		m_target->SetBehaviour( m_laserMissBehaviour, m_projectileMissBehaviour, m_impactSize, m_impactBehaviour );
	}
	else if( IsMatch( value, m_useDynamicBounds ) )
	{
		if( IsUsingCMF( m_geometryResource ) )
		{
			InitializeDynamicBounds( nullptr, nullptr );
		}
#if WITH_GRANNY
		else
		{
			InitializeGrannyDynamicBounds( nullptr, nullptr );
		}
#endif
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveTurretSet::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		registry->RegisterComponent<IEveShadowCaster>( this );

		if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
		{
			entity->Register( registry );
		}

		auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
		if( EveEntityPtr entity = BlueCastPtr( ambientEffect ) )
		{
			entity->Register( registry );
		}
	}
}

void EveTurretSet::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
		{
			entity->UnRegister( registry );
		}

		auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
		if( EveEntityPtr entity = BlueCastPtr( ambientEffect ) )
		{
			entity->UnRegister( registry );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Load the geometry resource, might be a re-load
// SeeAlso:
//   IBlueResource, TriGeometryRes
// --------------------------------------------------------------------------------
void EveTurretSet::InitializeGeometryResource()
{
	// Remove existing callback setup if any
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}

	// old geometry resource is gone, do some cleanup
	Cleanup();

	// get new geometry resource: path is LOD dependent!
	std::string resPath = "";
	switch( m_lodLevel )
	{
	case LOD_DISABLED:
	case LOD_HIGHEST:
		resPath = m_geomResPath;
		break;

	default:
		break;
	}

	// get new geometry resource, if res-path provided
	if( !resPath.empty() )
	{
		BeResMan->GetResource( resPath.c_str(), "", BlueInterfaceIID<TriGeometryRes>(), (void**)&m_geometryResource );
	}

	// attach callback, so ::RebuildCachedData() will be called when it finished loading
	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Create a vertex buffer, which acts as a instance buffer for the instanced
//   rendering we use for turrets. Fill it just with an index for each instance.
//   An instance is basiclly a single turret of this set.
// --------------------------------------------------------------------------------
void EveTurretSet::InitializeInstanceBuffer()
{
	// release old and create new and fill it with 1, 2, 3, 4, ...
	float pBufferData[EVE_MAX_TURRETS_PER_SET];
	for( unsigned int i = 0; i < EVE_MAX_TURRETS_PER_SET; ++i )
	{
		pBufferData[i] = (float)i;
	}
	USE_MAIN_THREAD_RENDER_CONTEXT();

	CR_RETURN( g_sharedBuffer.Allocate(
		sizeof( float ),
		EVE_MAX_TURRETS_PER_SET,
		pBufferData,
		renderContext,
		m_instanceBuffer ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Sets up the attached firing effect with data from the turret model, like
//   bone positions, IDs, etc. WARNING: m_firingEffect must exist and model
//   must be fully loaded!
// --------------------------------------------------------------------------------
void EveTurretSet::InitializeFiringEffect()
{
	// check for effect
	if( !m_firingEffect )
	{
		return;
	}
	m_firingEffect->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
	// check for model geometry
	if( m_geometryResource )
	{
		if( m_geometryResource->GetSkeletonCount() )
		{
			TriGeometryResSkeletonData* skeletonData = m_geometryResource->GetSkeletonData( 0 );
			if( skeletonData )
			{
				// find all the granny bone indices and store them
				for( unsigned int i = 0; i < m_firingEffect->GetPerMuzzleEffectCount(); ++i )
				{
					if( i == EveTurretFiringFX::MUZZLECOUNT_MAX )
					{
						CCP_LOGERR( "Upper limit of firing bones is %d, this turret has %d", EveTurretFiringFX::MUZZLECOUNT_MAX, m_firingEffect->GetPerMuzzleEffectCount() );
						break;
					}

					// firing bones should always be on the format Pos_FireXX where XX can range form 01 to 99
					char boneNameBuffer[20];
					int boneNameIndex = i + 1;
					sprintf_s( boneNameBuffer, "%s%.2d", m_firingEffect->GetFiringBoneName(), boneNameIndex );

					// in case we don't find positional bone, ::FindJoint() returns 0xffffffff
					m_firingEffect->SetMuzzleBoneID( i, skeletonData->FindJoint( boneNameBuffer ) );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets up the attached ambient effect.
//   The ambient effect will be distributed across the turret locators via a
//   EveChildInstanceContainer
// --------------------------------------------------------------------------------
void EveTurretSet::InitializeAmbientEffect()
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_generatedDistributedAmbientEffect ) )
	{
		entity->UnRegister( registry );
	}
	m_generatedDistributedAmbientEffect = nullptr;

	if( nullptr == m_ambientEffect )
	{
		return;
	}

	m_ambientOffsetMatrix = IdentityMatrix();
	if( m_ambientEffectEditingMode && !m_singleTurrets.empty() )
	{
		auto& firstTurret = m_singleTurrets[0];
		// calculate the offset to place the ambient effect, if we are in editing mode so we can see the effect on the first turret
		m_ambientOffsetMatrix = firstTurret.localMatrix;
	}

	m_generatedDistributedAmbientEffect.CreateInstance();
	m_generatedDistributedAmbientEffect->SetSourceEffect( m_ambientEffect );

	for( auto it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		auto& turret = *it;
		// Add the instance to the generated effect
		// Note that we don't want this to be attached to a bone
		m_generatedDistributedAmbientEffect->AddInstanceTransform( Vector3( 1, 1, 1 ), turret.localQuaternion, turret.localPosition.GetXYZ() );
	}

	// This will either return the source or the generated effect
	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();

	if( m_ambientEffectEditingMode )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_ambientEffect ) )
		{
			entity->Register( registry );
		}
	}
	else
	{
		if( EveEntityPtr entity = BlueCastPtr( m_generatedDistributedAmbientEffect ) )
		{
			entity->Register( registry );
		}
	}

	switch( m_state )
	{
	case STATE_FIRING:
		ambientEffect->SetControllerVariable( "TurretState", float( STATE_TARGETING ) );
		SetAmbientEffectControllerVariableOnInstance( m_activeTurret, "TurretState", float( m_state ) );
		break;
	default:
		ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
	}

	ambientEffect->StartControllers();
}

bool EveTurretSet::IsAmbientVisible() const
{
	return m_display && m_displayEffects && abs( m_parentData.clipRadiusSq ) < 0.05;
}


IEveSpaceObjectChild* EveTurretSet::GetAmbientEffectOrGeneratedEffect() const
{
	if( m_ambientEffectEditingMode )
	{
		return m_ambientEffect;
	}
	return m_generatedDistributedAmbientEffect;
}

void EveTurretSet::SetAmbientEffectControllerVariableOnInstance( int index, const char* name, float value )
{
	if( m_ambientEffectEditingMode )
	{
		m_ambientEffect->SetControllerVariable( name, value );
		return;
	}
	m_generatedDistributedAmbientEffect->SetControllerVariableForInstance( index, name, value );
}

// --------------------------------------------------------------------------------
// Description:
//   Free all of the granny animation helper stuff, we allocated
// SeeAlso:
//   SingleTurretData
// --------------------------------------------------------------------------------
void EveTurretSet::Cleanup()
{
	// vertex decl element array is no longer valid
	m_turretVertexDeclElementCount = 0;

	// system bone ids are no longer valid
	for( unsigned int i = 0; i < SYSBONE_MAX; ++i )
	{
		m_systemBoneID[i] = INVALID_BONE_INDEX;
	}

	m_skeleton = nullptr;
	m_skeletonBoneIndices.clear();
	// free and null cmf data per turret
	for( auto& turret : m_singleTurrets )
	{
		turret.sequencer.reset();
		turret.pose.boneTransforms.clear();
		turret.pose.skeleton = nullptr;
		turret.worldTransforms.clear();
	}

#if WITH_GRANNY
	// granny invalid
	m_grnModel = NULL;
	// granny
	if( m_grnMeshBinding )
	{
		GrannyFreeMeshBinding( m_grnMeshBinding );
		m_grnMeshBinding = NULL;
	}
	// free and null granny data per turret
	for( auto& turret : m_singleTurrets )
	{
		// free the parts that were alocted
		if( turret.grnModelInstance )
		{
			// free all animation controls of this instance
			for( granny_model_control_binding* b = GrannyModelControlsBegin( turret.grnModelInstance ); b != GrannyModelControlsEnd( turret.grnModelInstance ); )
			{
				granny_control* control = GrannyGetControlFromBinding( b );
				b = GrannyModelControlsNext( b );
				GrannyFreeControl( control );
			}
			GrannyFreeModelInstance( turret.grnModelInstance );
			turret.grnModelInstance = NULL;
		}
		if( turret.grnLocalPose )
		{
			GrannyFreeLocalPose( turret.grnLocalPose );
			turret.grnLocalPose = NULL;
		}
		if( turret.grnWorldPose )
		{
			GrannyFreeWorldPose( turret.grnWorldPose );
			turret.grnWorldPose = NULL;
		}
		// zero/invalidate other data
		turret.grnSkeleton = NULL;
	}
#endif
	m_boneBounds.clear();
}

// --------------------------------------------------------------------------------
void EveTurretSet::InitializeDynamicBounds( const cmf::Data* cmfData, const cmf::Skeleton* skeleton )
{
	CCP_ASSERT( !m_geometryResource || m_geometryResource->IsUsingCMF() );

	m_boneBounds.clear();
	if( !m_useDynamicBounds )
	{
		return;
	}

	if( !cmfData )
	{
		if( !m_geometryResource )
		{
			return;
		}
		cmfData = m_geometryResource->GetCMFData();
		if( !cmfData )
		{
			return;
		}
	}

	if( !skeleton )
	{
		if( m_singleTurrets.empty() || !m_singleTurrets[0].sequencer )
		{
			return;
		}
		skeleton = m_skeleton;
	}

	if( cmfData->skeletons.empty() )
	{
		return;
	}

	for( const auto& mesh : cmfData->meshes )
	{
		if( mesh.skeleton == 0 )
		{
			for( int boneIdx = 0; boneIdx < skeleton->bones.size(); boneIdx++ )
			{
				auto name = skeleton->bones[boneIdx];
				auto boneBinding = std::find_if( mesh.boneBindings.begin(), mesh.boneBindings.end(), [name]( const cmf::BoneBinding& bb ) {
					return name == bb.name;
				} );
				if( boneBinding != mesh.boneBindings.end() )
				{
					GrannyBoneBindingBounds bounds;
					bounds.m_boneIndex = boneIdx;

					Vector3 minBounds( boneBinding->bounds.m_min );
					Vector3 maxBounds( boneBinding->bounds.m_max );
					bounds.m_corners[0] = minBounds;
					bounds.m_corners[1] = maxBounds;
					bounds.m_corners[2] = Vector3( minBounds.x, minBounds.y, maxBounds.z );
					bounds.m_corners[3] = Vector3( minBounds.x, maxBounds.y, minBounds.z );
					bounds.m_corners[4] = Vector3( minBounds.x, maxBounds.y, maxBounds.z );
					bounds.m_corners[5] = Vector3( maxBounds.x, minBounds.y, minBounds.z );
					bounds.m_corners[6] = Vector3( maxBounds.x, minBounds.y, maxBounds.z );
					bounds.m_corners[7] = Vector3( maxBounds.x, maxBounds.y, minBounds.z );
					m_boneBounds.push_back( bounds );
				}
			}
		}
	}
}

#if WITH_GRANNY
// --------------------------------------------------------------------------------
void EveTurretSet::InitializeGrannyDynamicBounds( granny_file_info* fi, granny_skeleton* skeleton )
{
	CCP_ASSERT( !m_geometryResource || !m_geometryResource->IsUsingCMF() );

	m_boneBounds.clear();
	if( !m_useDynamicBounds )
	{
		return;
	}

	if( !fi )
	{
		if( !m_geometryResource )
		{
			return;
		}
		fi = m_geometryResource->GetGrannyInfo();
		if( !fi )
		{
			return;
		}
	}

	if( !skeleton )
	{
		if( m_singleTurrets.size() == 0 || !m_singleTurrets[0].grnModelInstance )
		{
			return;
		}
		skeleton = m_singleTurrets[0].grnSkeleton;
	}
	if( fi->ModelCount )
	{
		for( int32_t j = 0; j < fi->Models[0]->MeshBindingCount; ++j )
		{
			granny_model* model = fi->Models[0];
			granny_model_mesh_binding& meshBinding = model->MeshBindings[j];
			granny_bone_binding* bb = meshBinding.Mesh->BoneBindings;
			for( int boneIdx = 0; boneIdx < meshBinding.Mesh->BoneBindingCount; boneIdx++ )
			{
				GrannyBoneBindingBounds bounds;
				GrannyFindBoneByName( skeleton, bb[boneIdx].BoneName, &bounds.m_boneIndex );

				Vector3 minBounds( *reinterpret_cast<Vector3*>( bb[boneIdx].OBBMin ) );
				Vector3 maxBounds( *reinterpret_cast<Vector3*>( bb[boneIdx].OBBMax ) );
				bounds.m_corners[0] = minBounds;
				bounds.m_corners[1] = maxBounds;
				bounds.m_corners[2] = Vector3( minBounds.x, minBounds.y, maxBounds.z );
				bounds.m_corners[3] = Vector3( minBounds.x, maxBounds.y, minBounds.z );
				bounds.m_corners[4] = Vector3( minBounds.x, maxBounds.y, maxBounds.z );
				bounds.m_corners[5] = Vector3( maxBounds.x, minBounds.y, minBounds.z );
				bounds.m_corners[6] = Vector3( maxBounds.x, minBounds.y, maxBounds.z );
				bounds.m_corners[7] = Vector3( maxBounds.x, maxBounds.y, minBounds.z );
				m_boneBounds.push_back( bounds );
			}
		}
	}
}
#endif

// --------------------------------------------------------------------------------
bool EveTurretSet::GetDynamicBounds( const SingleTurretData& turret, Vector4* boundingSphere, Vector3* aabbMin, Vector3* aabbMax ) const
{
	Vector3 transformed[8];
	if( m_boneBounds.empty() || !m_geometryResource )
	{
		return false;
	}

	if( IsUsingCMF( m_geometryResource ) )
	{
		if( !turret.sequencer )
		{
			return false;
		}

		if( boundingSphere )
		{
			BoundingSphereInitialize( *boundingSphere );
		}
		if( aabbMin && aabbMax )
		{
			BoundingBoxInitialize( *aabbMin, *aabbMax );
		}

		CCP_ASSERT( !turret.worldTransforms.empty() );

		for( const auto& bb : m_boneBounds )
		{
			const Matrix& mat = turret.worldTransforms[bb.m_boneIndex];

			for( size_t point = 0; point < 8; point++ )
			{
				transformed[point] = TransformCoord( bb.m_corners[point], mat );
				if( aabbMin && aabbMax )
				{
					BoundingBoxUpdate( *aabbMin, *aabbMax, transformed[point] );
				}
				if( boundingSphere )
				{
					BoundingSphereUpdate( transformed[point], *boundingSphere );
				}
			}
		}
	}
#if WITH_GRANNY
	else
	{
		if( !turret.grnModelInstance )
		{
			return false;
		}

		if( boundingSphere )
		{
			BoundingSphereInitialize( *boundingSphere );
		}
		if( aabbMin && aabbMax )
		{
			BoundingBoxInitialize( *aabbMin, *aabbMax );
		}

		for( const auto& bb : m_boneBounds )
		{
			const Matrix* mat = reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( turret.grnWorldPose, bb.m_boneIndex ) );

			for( size_t point = 0; point < 8; point++ )
			{
				transformed[point] = TransformCoord( bb.m_corners[point], *mat );
				if( aabbMin && aabbMax )
				{
					BoundingBoxUpdate( *aabbMin, *aabbMax, transformed[point] );
				}
				if( boundingSphere )
				{
					BoundingSphereUpdate( transformed[point], *boundingSphere );
				}
			}
		}

		const granny_file_info* fi = m_geometryResource->GetGrannyInfo();
		if( fi && fi->ModelCount )
		{
			if( aabbMin && aabbMax )
			{
				*aabbMin += *reinterpret_cast<Vector3*>( fi->Models[0]->InitialPlacement.Position );
				*aabbMax += *reinterpret_cast<Vector3*>( fi->Models[0]->InitialPlacement.Position );
			}
			if( boundingSphere )
			{

				( *boundingSphere ).x += fi->Models[0]->InitialPlacement.Position[0];
				( *boundingSphere ).y += fi->Models[0]->InitialPlacement.Position[1];
				( *boundingSphere ).z += fi->Models[0]->InitialPlacement.Position[2];
			}
		}
	}
#else
	else
	{
		return false;
	}
#endif

	return true;
}

// --------------------------------------------------------------------------------
void EveTurretSet::RenderDynamicBounds()
{
	Vector3 transformed[8];
	if( m_boneBounds.empty() || !m_geometryResource )
	{
		return;
	}

	if( IsUsingCMF( m_geometryResource ) )
	{
		for( const auto& turret : m_singleTurrets )
		{
			Vector4 boundingSphere;
			Vector3 aabbMin, aabbMax;
			BoundingBoxInitialize( aabbMin, aabbMax );
			bool initialized = false;

			if( !turret.sequencer )
			{
				return;
			}

			Matrix modelTransform = turret.worldMatrix;

			for( const auto& bb : m_boneBounds )
			{
				Matrix mat = turret.worldTransforms[bb.m_boneIndex] * modelTransform;

				for( unsigned point = 0; point < 8; point++ )
				{
					transformed[point] = TransformCoord( bb.m_corners[point], mat );
					BoundingBoxUpdate( aabbMin, aabbMax, transformed[point] );
					if( !initialized )
					{
						boundingSphere.x = transformed[point].x;
						boundingSphere.y = transformed[point].y;
						boundingSphere.z = transformed[point].z;
						boundingSphere.w = 0;
						initialized = true;
					}
					else
					{
						BoundingSphereUpdate( transformed[point], boundingSphere );
					}
				}

				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[2], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[5], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[5], 0xff0000ff, transformed[6], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[2], 0xff0000ff, transformed[6], 0xff0000ff );

				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[7], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[4], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[3], 0xff0000ff, transformed[4], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[3], 0xff0000ff, transformed[7], 0xff0000ff );

				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[6], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[3], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[7], 0xff0000ff, transformed[5], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[4], 0xff0000ff, transformed[2], 0xff0000ff );
			}

			Tr2Renderer::DrawSphere( boundingSphere, 8, 0xffff0000 );
			Tr2Renderer::DrawBox( aabbMin, aabbMax, 0xffff0000 );
		}
	}
#if WITH_GRANNY
	else
	{
		const granny_file_info* fi = m_geometryResource->GetGrannyInfo();

		for( const auto& turret : m_singleTurrets )
		{
			Vector4 boundingSphere;
			Vector3 aabbMin, aabbMax;
			BoundingBoxInitialize( aabbMin, aabbMax );
			bool initialized = false;

			Vector3 initialPlacement( 0, 0, 0 );
			Matrix initialTranslation;

			if( !turret.grnModelInstance )
			{
				return;
			}

			Matrix modelTransform = turret.worldMatrix;

			if( fi )
			{
				initialPlacement = *reinterpret_cast<Vector3*>( fi->Models[0]->InitialPlacement.Position );
			}
			initialTranslation = TranslationMatrix( initialPlacement );

			for( const auto& bb : m_boneBounds )
			{
				Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( turret.grnWorldPose, bb.m_boneIndex ) ) * modelTransform * initialTranslation;

				for( unsigned point = 0; point < 8; point++ )
				{
					transformed[point] = TransformCoord( bb.m_corners[point], mat );
					BoundingBoxUpdate( aabbMin, aabbMax, transformed[point] );
					if( !initialized )
					{
						boundingSphere.x = transformed[point].x;
						boundingSphere.y = transformed[point].y;
						boundingSphere.z = transformed[point].z;
						boundingSphere.w = 0;
						initialized = true;
					}
					else
					{
						BoundingSphereUpdate( transformed[point], boundingSphere );
					}
				}

				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[2], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[5], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[5], 0xff0000ff, transformed[6], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[2], 0xff0000ff, transformed[6], 0xff0000ff );

				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[7], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[4], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[3], 0xff0000ff, transformed[4], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[3], 0xff0000ff, transformed[7], 0xff0000ff );

				Tr2Renderer::DrawLine( transformed[1], 0xff0000ff, transformed[6], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[0], 0xff0000ff, transformed[3], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[7], 0xff0000ff, transformed[5], 0xff0000ff );
				Tr2Renderer::DrawLine( transformed[4], 0xff0000ff, transformed[2], 0xff0000ff );
			}

			Tr2Renderer::DrawSphere( boundingSphere, 8, 0xffff0000 );
			Tr2Renderer::DrawBox( aabbMin, aabbMax, 0xffff0000 );
		}
	}
#endif
}

void EveTurretSet::InitializeAnimation()
{
	if( IsUsingCMF( m_geometryResource ) )
	{
		// get a model, a meshbinding and animation stuff from the resource
		const cmf::Data* cmfData = m_geometryResource->GetCMFData();
		if( cmfData )
		{
			if( cmfData->skeletons.size() )
			{
				auto mesh = std::find_if( cmfData->meshes.begin(), cmfData->meshes.end(), []( const cmf::Mesh& m ) {
					return m.skeleton == 0;
				} );

				if( mesh != cmfData->meshes.end() && mesh->boneBindings.size() )
				{
					if( m_skeletonBoneIndices.empty() )
					{
						// create one mesh binding for all turrets
						// and remember model
						m_skeleton = &cmfData->skeletons[0];

						// create animations for all turrets
						for( auto& turret : m_singleTurrets )
						{
							if( !turret.sequencer )
							{
								turret.sequencer = std::make_unique<cmf::AnimationSequencer>( *m_skeleton );
								cmf::RestPose( turret.pose, *m_skeleton );
							}
						}

						if( m_singleTurrets.size() > 0 )
						{
							InitializeDynamicBounds( cmfData, m_skeleton );
						}

						m_skeletonBoneIndices = Tr2GrannyAnimationUtils::CreateMapping( *m_skeleton, mesh->boneBindings, (uint32_t)mesh->boneBindings.size() );
					}
				}
			}
		}
	}
#if WITH_GRANNY
	else
	{
		// get a model, a meshbinding and animation stuff from the resource
		granny_file_info* grannyFileInfo = m_geometryResource->GetGrannyInfo();
		if( grannyFileInfo )
		{
			if( grannyFileInfo->ModelCount )
			{
				if( grannyFileInfo->Models[0]->MeshBindingCount )
				{
					if( grannyFileInfo->SkeletonCount )
					{
						if( m_grnMeshBinding == NULL )
						{
							// create one mesh binding for all turrets
							m_grnMeshBinding = GrannyNewMeshBinding( grannyFileInfo->Models[0]->MeshBindings[0].Mesh, grannyFileInfo->Skeletons[0], grannyFileInfo->Skeletons[0] );
							// and remember model
							m_grnModel = grannyFileInfo->Models[0];

							// create animations for all turrets
							for( auto& turret : m_singleTurrets )
							{
								if( turret.grnModelInstance == NULL )
								{
									turret.grnModelInstance = GrannyInstantiateModel( m_grnModel );
									turret.grnSkeleton = GrannyGetSourceSkeleton( turret.grnModelInstance );
									turret.grnLocalPose = GrannyNewLocalPose( turret.grnSkeleton->BoneCount );
									turret.grnWorldPose = GrannyNewWorldPose( turret.grnSkeleton->BoneCount );
								}
							}

							if( m_singleTurrets.size() > 0 )
							{
								InitializeGrannyDynamicBounds( grannyFileInfo, m_singleTurrets[0].grnSkeleton );
							}
						}
					}
				}
			}
		}
	}
#endif
}

// --------------------------------------------------------------------------------
// Description:
//   This gets called when the geometry data has finished loading, so we have a
//   whole lot of init's to do:
//   - take the original vertex declaration, extend by one member for the 2nd
//     stream, and create a new one
//   - grab bounding sphere from mesh
//   - analyze the skeleton: find the system bones and remember their indices, so
//     we can later manipulate them from tracking
//   - create/allocate all the granny stuff you need to play back animations
//   - start a possible animation request
//   - Initialize the ambient effect (if available)
// SeeAlso:
//   TriGeometryResMeshData, Tr2EffectStateManager, TriGeometryResSkeletonData
// --------------------------------------------------------------------------------
void EveTurretSet::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		// finished loading the turret geometry resource, so grab vertex decl and bounding sphere
		if( m_geometryResource->GetMeshCount() )
		{
			const TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( 0 );
			if( meshData )
			{
				// gemoetry's original vertex-decl must exist
				if( meshData->m_vertexDeclarationHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
				{
					// get gemoetry's original vertex-decl...
					Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclarationHandle, m_turretVertexDecl );
					// ...expand it with instances stream elements...
					auto& item = m_turretVertexDecl.Add( m_turretVertexDecl.FLOAT32_1, m_turretVertexDecl.TEXCOORD, 2, 1, 1 );
					item.m_offset = 0;
					// ...and create new vertex dcel again
					m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( m_turretVertexDecl );
					m_turretVertexDeclElementCount = (unsigned int)m_turretVertexDecl.m_items.size();
				}

				// get a bounding box for visibilty detection, if this is not already set in the redfile
				if( m_boundingSphere.w == 0.f )
				{
					m_geometryResource->RecalculateBoundingSphere();
					m_geometryResource->GetBoundingSphere( 0, m_boundingSphere );
				}
			}
		}

		// hunt for the system-controlled bones and positional bones
		if( m_geometryResource->GetSkeletonCount() )
		{
			TriGeometryResSkeletonData* skeletonData = m_geometryResource->GetSkeletonData( 0 );
			if( skeletonData )
			{
				for( int i = 0; i < SYSBONE_MAX; ++i )
				{
					// in case we don't find system bone, ::FindJoint() returns 0xffffffff
					m_systemBoneID[i] = skeletonData->FindJoint( s_systemBoneSkeletonNames[i].c_str() );
				}

				// try to link an existing firing effect to skeleton's bones
				InitializeFiringEffect();
			}
		}

		InitializeAnimation();

		// animation already requested in a queue?
		if( !m_animationQueue.empty() )
		{
			std::vector<AnimationRequest> queueSnapshot = m_animationQueue;
			for( const auto& request : queueSnapshot )
			{
				PlayAnimation( request.turretIndex, request.animName, request.animNameIdle );
			}
			m_animationQueue.clear();
		}
		else
		{
			// force an anim based on a state
			ForceIdleAnimation();
		}

		InitializeAmbientEffect();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Zero some memebers and free granny's animation data, cause the granny file
//   is no longer valid.
// --------------------------------------------------------------------------------
void EveTurretSet::ReleaseCachedData( BlueAsyncRes* p )
{
	// mem release
	Cleanup();
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   the instance buffer
// --------------------------------------------------------------------------------
void EveTurretSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	g_sharedBuffer.Free( m_instanceBuffer );
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering and create and fill the instance vertex buffer
// --------------------------------------------------------------------------------
bool EveTurretSet::OnPrepareResources()
{
	// already loaded?
	if( m_turretVertexDeclElementCount )
	{
		// create vertex decl
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( m_turretVertexDecl );
			if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
			{
				return false;
			}
		}
	}
	// create instance buffer
	InitializeInstanceBuffer();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Here we check if the current LOD is still ok, based on m_estimatedPixelDiameter.
//   It will signal a change in the LOD level.
// Return Value:
//   True if a LOD level change did happen
// --------------------------------------------------------------------------------
bool EveTurretSet::UpdateLOD( const EveUpdateContext& updateContext )
{
	// is LODing enabled?
	if( m_lodLevel == LOD_DISABLED )
	{
		return false;
	}

	LOD oldLOD = m_lodLevel;

	if( g_brokenMacOSNvidiaDrivers && m_singleTurrets.size() > 1 )
	{
		m_lodLevel = LOD_EMPTY;
		return oldLOD != m_lodLevel;
	}

	// if the pixeldiameter is negative that means it is not a reliable value, so
	// don't do lod calculations with it
	if( m_estimatedPixelDiameter < 0.f )
	{
		return false;
	}

	if( m_estimatedPixelDiameter < 2.f * updateContext.GetVisibilityThreshold() )
	{
		// totally EMPTY: no turret visible at all
		m_lodLevel = LOD_EMPTY;
	}
	else
	{
		// HIGHEST
		m_lodLevel = LOD_HIGHEST;
	}

	m_estimatedPixelDiameter = -1.f;

	// return if change of LOD?
	return ( oldLOD != m_lodLevel );
}

void EveTurretSet::UpdateSyncronous( const EveUpdateContext& updateContext, const Matrix* parentMatrix )
{
	float deltaT = updateContext.GetDeltaT();
	Be::Time time = updateContext.GetTime();

	if( !m_singleTurrets.size() )
	{
		return;
	}

	// LODing
	if( UpdateLOD( updateContext ) )
	{
		// LOD change, so just call ::InitializeGeometryResource(), takes care of everything
		InitializeGeometryResource();

		// LOD change: toggle source dest effect of the attached firingFX
		if( m_firingEffect )
		{
			switch( m_lodLevel )
			{
			case LOD_DISABLED:
			case LOD_HIGHEST:
				m_firingEffect->SetDisplaySourceObject( true );
				break;
			default:
				m_firingEffect->SetDisplaySourceObject( false );
				break;
			}
		}
	}

	if( IsUsingCMF( m_geometryResource ) )
	{
		for( auto& turret : m_singleTurrets )
		{
			// mesh animation
			if( turret.sequencer )
			{
				turret.sequencer->RemoveFinishedAnimations( Tr2Renderer::GetAnimationTime() );
			}
		}
	}
#if WITH_GRANNY
	else
	{
		for( const auto& turret : m_singleTurrets )
		{
			// mesh animation
			if( turret.grnModelInstance )
			{
				GrannySetModelClock( turret.grnModelInstance, Tr2Renderer::GetAnimationTime() );
				GrannyFreeCompletedModelControls( turret.grnModelInstance );
			}
		}
	}
#endif

	// setup and update attached firing effect
	if( m_firingEffect )
	{
		if( m_activeTurret != INVALID_TURRET_INDEX )
		{
			// if the attached firing effect is looping, then we must recheck if active turret is still the best,
			if( m_firingEffect->IsLooping() )
			{
				if( m_state == STATE_FIRING )
				{
					// don't do it every frame, cause this will result in popping
					m_recheckTimeLeft -= deltaT;
					if( m_recheckTimeLeft < 0.f )
					{
						unsigned int currentClosestTurret = INVALID_TURRET_INDEX;
						int currentClosestLocator = -1;
						if( GetClosestTurretAndLocator( currentClosestTurret, currentClosestLocator ) )
						{
							if( ( currentClosestTurret != m_activeTurret ) || ( currentClosestLocator != m_target->GetLocator() ) )
							{
								// Set up the firing states correctly
								SetupFiringState();
							}
						}
						// recheck every 2 seconds
						m_recheckTimeLeft = 2.f;
					}
				}
			}
		}
		m_firingEffect->UpdateSynchronous( updateContext );
	}

	// update the target locator position
	Vector3 p = m_parentData.transform.GetTranslation();
	if( m_firingEffect )
	{
		m_firingEffect->GetStartPosition( p );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		EveChildUpdateParams params;
		params.isVisible = IsAmbientVisible();
		params.localToWorldTransform = m_ambientOffsetMatrix * m_parentData.transform;
		ambientEffect->UpdateSyncronous( updateContext, params );
	}

	m_target->Update( deltaT, &p );

	if( m_turretMovementObserver != nullptr && m_singleTurrets.size() > 0 )
	{
		m_turretMovementObserver->Update( m_singleTurrets[0].worldMatrix );
	}
}

void EveTurretSet::UpdateSingleTurrets()
{
	if( IsUsingCMF( m_geometryResource ) )
	{
		// update animation of each single turret
		for( auto& turret : m_singleTurrets )
		{
			// mesh animation
			if( turret.sequencer )
			{
				turret.sequencer->Sample( turret.pose, Tr2Renderer::GetAnimationTime() );

				if( turret.valid )
				{
					if( m_trackingInfluence != 0.f )
					{
						// transform target pos (which is in world space) into objectspace
						Vector3 targetPosOS = TransformCoord( *m_target->GetTrackingPosition(), Inverse( turret.worldMatrix ) );

						// "do" all the system bones, we have found
						for( unsigned int bone = 0; bone < SYSBONE_MAX; ++bone )
						{
							if( m_systemBoneID[bone] != INVALID_BONE_INDEX && m_systemBoneID[bone] < turret.pose.boneTransforms.size() )
							{
								const Matrix* localTransformPtr = nullptr;
								// Currently only do this for the main pitch bones. May want to extend this for others
								// if we find a case that uses them.
								if( bone >= SYSBONE_PITCH && bone <= SYSBONE_PITCH2 && m_updatePitchPose )
								{
									cmf::ComputeWorldTransforms( turret.worldTransforms, turret.pose );
									localTransformPtr = &turret.worldTransforms[m_systemBoneID[bone]];
								}
								// modify this bone's transform data
								cmf::Transform& boneTransform = turret.pose.boneTransforms[m_systemBoneID[bone]];
								ModifySystemBoneTransform( (SystemBones)bone, &targetPosOS, localTransformPtr, boneTransform.position, boneTransform.rotation );
							}
						}
					}
				}

				cmf::ComputeWorldTransforms( turret.worldTransforms, turret.pose );
			}
		}
	}
#if WITH_GRANNY
	else
	{
		// update animation of each single turret
		for( const auto& turret : m_singleTurrets )
		{
			// mesh animation
			if( turret.grnModelInstance )
			{
				GrannySampleModelAnimations( turret.grnModelInstance, 0, turret.grnSkeleton->BoneCount, turret.grnLocalPose );

				if( turret.valid )
				{
					if( m_trackingInfluence != 0.f )
					{
						// transform target pos (which is in world space) into objectspace
						Vector3 targetPosOS = TransformCoord( *m_target->GetTrackingPosition(), Inverse( turret.worldMatrix ) );

						// "do" all the system bones, we have found
						for( unsigned int bone = 0; bone < SYSBONE_MAX; ++bone )
						{
							if( m_systemBoneID[bone] != INVALID_BONE_INDEX )
							{
								Matrix* localTransformPtr = nullptr;
								Matrix localTransform;
								// Currently only do this for the main pitch bones. May want to extend this for others
								// if we find a case that uses them.
								if( bone >= SYSBONE_PITCH && bone <= SYSBONE_PITCH2 && m_updatePitchPose )
								{
									Matrix id = IdentityMatrix();
									GrannyBuildWorldPose( turret.grnSkeleton, 0, turret.grnSkeleton->BoneCount, turret.grnLocalPose, &id.m[0][0], turret.grnWorldPose );
									granny_real32* boneWorldTransform = GrannyGetWorldPose4x4( turret.grnWorldPose, m_systemBoneID[bone] );

									localTransform = *reinterpret_cast<const Matrix*>( boneWorldTransform ) * id;
									localTransformPtr = &localTransform;
								}
								// modify this bone's transform data
								granny_transform* boneTransform = GrannyGetLocalPoseTransform( turret.grnLocalPose, m_systemBoneID[bone] );
								if( boneTransform )
								{
									Vector3 position = Vector3( boneTransform->Position[0], boneTransform->Position[1], boneTransform->Position[2] );
									Quaternion rotation = *reinterpret_cast<Quaternion*>( boneTransform->Orientation );
									ModifySystemBoneTransform( (SystemBones)bone, &targetPosOS, localTransformPtr, position, rotation );
									GrannySetTransform( boneTransform, &position.x, (float*)&rotation, (float*)boneTransform->ScaleShear );
								}
							}
						}
					}
				}

				Matrix id = IdentityMatrix();
				GrannyBuildWorldPose( turret.grnSkeleton, 0, turret.grnSkeleton->BoneCount, turret.grnLocalPose, &id.m[0][0], turret.grnWorldPose );
			}
		}
	}
#endif
}

// --------------------------------------------------------------------------------
// Description:
//   First thing to do is to keep the world-matrices of all turrets up to date,
//   cause most likely the ship has moved. Then sample the granny animation
//   for each single turret of this set, cause they are animated independently.
//   Just before collapsing the skeleton matrices, sneek in a bone modification
//   for the auto tracking.
//   Also smoothly do some fading between states and tracking positions.
// SeeAlso:
//   SingleTurretData, ModifySystemBoneTransform
// Arguments:
//   updateContext - scene update context
//   parentData - parent object data
// --------------------------------------------------------------------------------
void EveTurretSet::UpdateAsyncronous( const EveUpdateContext& updateContext, const IEveSpaceObject2::ParentData* parentData )
{
	m_boneOffsets.AdvanceFrame();

	m_shipTransformPrev = m_parentData.transform;

	// keep parent's transform
	m_parentData = *parentData;

	if( !m_singleTurrets.size() )
	{
		return;
	}

	float deltaT = updateContext.GetDeltaT();
	UpdateTurretTransforms( &parentData->transform );

	// handle fading of turret tracking
	if( m_trackingInfluenceDelta != 0.f )
	{
		m_trackingInfluence += m_trackingInfluenceDelta * deltaT;
		if( m_trackingInfluence > m_maxTrackingTime )
		{
			m_trackingInfluence = m_maxTrackingTime;
			m_trackingInfluenceDelta = 0.f;
		}
		else if( m_trackingInfluence < 0.f )
		{
			m_trackingInfluence = 0.f;
			m_trackingInfluenceDelta = 0.f;
		}
	}

	if( m_delayToFadeOutTracking > 0.f )
	{
		m_delayToFadeOutTracking -= deltaT;
		if( m_delayToFadeOutTracking <= 0.f )
		{
			m_delayToFadeOutTracking = 0.f;
			m_trackingInfluenceDelta = -1.f / TRACKING_FADE_TIME;
		}
	}

	if( m_delayToFadeInTracking > 0.f )
	{
		m_delayToFadeInTracking -= deltaT;
		if( m_delayToFadeInTracking <= 0.f )
		{
			m_delayToFadeInTracking = 0.f;
			m_trackingInfluenceDelta = 1.f / TRACKING_FADE_TIME;
		}
	}

	UpdateSingleTurrets();

	// setup and update attached firing effect
	if( m_firingEffect )
	{
		if( m_activeTurret != INVALID_TURRET_INDEX )
		{
			// update all muzzle points in the firing effect
			for( unsigned int i = 0; i < m_firingEffect->GetPerMuzzleEffectCount(); ++i )
			{
				// get world transform of this muzzle bone
				Matrix m = GetFiringBoneWorldTransform( i );
				// and set it to the muzzle
				m_firingEffect->SetMuzzleTransform( i, &m );
			}
			m_firingEffectMuzzlePosSet = true;
		}

		m_firingEffect->SetEndPosition( m_target->GetTargetPosition() );

		// time update (return value tells us if effect is ready to fire!)
		if( m_firingEffect->UpdateAsynchronous( updateContext ) )
		{
			// if we haven't initialised muzzle positions, do it now
			// this can happen, and if we don't do this all effects originate from
			// the player ship until turret geometry is loaded and muzzle positions
			// properly set
			if( !m_firingEffectMuzzlePosSet )
			{

				for( unsigned int i = 0; i < m_firingEffect->GetPerMuzzleEffectCount(); ++i )
				{
					// use something relatively sensible, even absent geometry
					m_firingEffect->SetMuzzleTransform( i, &m_parentData.transform );
				}

				m_firingEffectMuzzlePosSet = true;
			}
			m_firingEffect->SetDisplayDestObject( m_target->ShowDestObject() );
		}
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		EveChildUpdateParams params;
		params.isVisible = m_display;
		params.localToWorldTransform = m_ambientOffsetMatrix * m_parentData.transform;
		ambientEffect->UpdateAsyncronous( updateContext, params );
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Updates the turret world and inv world matrices and validates the turret tata
// SeeAlso:
//   UpdateAsyncronous, EveMobile::UpdateSyncronous
// Arguments:
//   turretTransformMatrix - The transform matrix of the turret on the EveMobile object
// --------------------------------------------------------------------------------
void EveTurretSet::UpdateTurretTransforms( const Matrix* turretTransformMatrix )
{
	for( auto& turret : m_singleTurrets )
	{
		// first parent matrix (ship or station), then local matrix (locator position)
		turret.worldMatrix = turret.localMatrix * *turretTransformMatrix;
		// this validates this turret
		turret.valid = true;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate the world matrix for a firing bone currently firing.
// Arguments:
//   muzzle - number of the firing bone in the turret
//   worldTransform - returns the world matrix
// Return Value:
//   True if worldTransform received valid data
// --------------------------------------------------------------------------------
Matrix EveTurretSet::GetFiringBoneWorldTransform( unsigned int muzzle ) const
{
	// there MUST be an avtive turret aka a "firing turret"!
	unsigned int closestTurret = m_activeTurret;
	// so if we don't have one, calc one temporarily
	if( closestTurret == INVALID_TURRET_INDEX )
	{
		int closestLocator = -1;
		GetClosestTurretAndLocator( closestTurret, closestLocator );
	}

	// this is a problem, only thing to do is to return the parent's worldtransform
	if( closestTurret == INVALID_TURRET_INDEX )
	{
		return m_parentData.transform;
	}

	// source comes from position bones or, if we don't have any, from center of turret
	Matrix m = m_singleTurrets[closestTurret].worldMatrix;

	// get the boneID for that muzzle from firing effect
	if( !m_firingEffect )
	{
		return m;
	}
	unsigned int boneID = m_firingEffect->GetPerMuzzleBoneID( muzzle );
	return GetTurretBoneTransform( closestTurret, boneID );
}

// --------------------------------------------------------------------------------
Matrix EveTurretSet::GetTurretBoneTransform( uint32_t closestTurret, uint32_t boneID ) const
{
	if( IsUsingCMF( m_geometryResource ) )
	{
		// source comes from position bones or, if we don't have any, from center of turret
		Matrix m = m_singleTurrets[closestTurret].worldMatrix;
		Matrix lowLodTransform = IdentityMatrix();

		if( m_useLowLodFiringTransform )
		{
			lowLodTransform = TransformationMatrix( m_lowLodFiringEffectScale, m_lowLodFiringEffectRotation, m_lowLodFiringEffectTranslation );
		}

		if( boneID == INVALID_BONE_INDEX )
		{
			return lowLodTransform * m;
		}

		// valid pose? (bone positions are stored "in" that thing)
		if( m_singleTurrets[closestTurret].pose.skeleton )
		{
			// firing-bone position in turret-space
			const Matrix& boneTransform = m_singleTurrets[closestTurret].worldTransforms[boneID];
			// create this pos in worldspace
			m = boneTransform * m;
		}
		else if( m_useLowLodFiringTransform )
		{
			// if we are too far out from the turret to have the cmf loaded we have the option to offset/scale/rotate the effect statically
			m = lowLodTransform * m;
		}
		else
		{
			// if we don't have a valid pose, position is fine but orientation? depends on launcher settings...
			if( m_sysBonePitchMin < 45.f )
			{
				// aiming directly at target, because target cone is large
				Vector3 nrmToTarget = *m_target->GetTrackingPosition() - m.GetTranslation();
				// get direct rotation quaternion
				Quaternion directRotationQuaternion;
				static const Vector3 zAxis( 0.f, 0.f, 1.f );
				TriQuaternionRotationArc( &directRotationQuaternion, &zAxis, &nrmToTarget );
				// now combine the translation from the worldmatrix and the rotation from the direct targeting
				Vector3 tr = m.GetTranslation();
				m = RotationMatrix( directRotationQuaternion ) * TranslationMatrix( tr );
			}
			else
			{
				// eject straight out of ship, because target cone is very small
				// attn: locator up is +Y, but missile eject is in +Z! so rotate around X
				Matrix y2xMatrix = RotationXMatrix( -0.5f * XM_PI );
				// apply this rotation to "before" world transform
				m = y2xMatrix * m;
			}
		}
		return m;
	}
#if WITH_GRANNY
	else
	{
		// source comes from position bones or, if we don't have any, from center of turret
		Matrix m = m_singleTurrets[closestTurret].worldMatrix;
		Matrix lowLodTransform = IdentityMatrix();

		if( m_useLowLodFiringTransform )
		{
			lowLodTransform = TransformationMatrix( m_lowLodFiringEffectScale, m_lowLodFiringEffectRotation, m_lowLodFiringEffectTranslation );
		}

		if( boneID == INVALID_BONE_INDEX )
		{
			return lowLodTransform * m;
		}

		// valid granny pose? (bone positions are stored "in" that thing)
		if( m_singleTurrets[closestTurret].grnWorldPose )
		{
			// granny tells us firing-bone position in turret-space
			granny_real32* boneTransform = GrannyGetWorldPose4x4( m_singleTurrets[closestTurret].grnWorldPose, boneID );
			// create this pos in worldspace
			m = *reinterpret_cast<const Matrix*>( boneTransform ) * m;
		}
		else if( m_useLowLodFiringTransform )
		{
			// if we are too far out from the turret to have the granny loaded we have the option to offset/scale/rotate the effect statically
			m = lowLodTransform * m;
		}
		else
		{
			// if we don't have a valid granny pose, position is fine but orientation? depends on launcher settings...
			if( m_sysBonePitchMin < 45.f )
			{
				// aiming directly at target, because target cone is large
				Vector3 nrmToTarget = *m_target->GetTrackingPosition() - m.GetTranslation();
				// get direct rotation quaternion
				Quaternion directRotationQuaternion;
				static const Vector3 zAxis( 0.f, 0.f, 1.f );
				TriQuaternionRotationArc( &directRotationQuaternion, &zAxis, &nrmToTarget );
				// now combine the translation from the worldmatrix and the rotation from the direct targeting
				Vector3 tr = m.GetTranslation();
				m = RotationMatrix( directRotationQuaternion ) * TranslationMatrix( tr );
			}
			else
			{
				// eject straight out of ship, because target cone is very small
				// attn: locator up is +Y, but missile eject is in +Z! so rotate around X
				Matrix y2xMatrix = RotationXMatrix( -0.5f * XM_PI );
				// apply this rotation to "before" world transform
				m = y2xMatrix * m;
			}
		}
		return m;
	}
#else
	else
	{
		return Matrix();
	}
#endif
}

// --------------------------------------------------------------------------------
// Description:
//   Depending on the type of the system bone, we calculate a new transform for
//   it and apply it. All of this is highly "hard-coded", but there are some
//   variables in there, so we can customize the tracking to individual turrets.
//   The modification should pay attention the amount of modification needed,
//   stored in m_trackingInfluence
// Arguments:
//   bone - type of system bone
//   target - position of target in "turret"-space
//   position - the bone position that needs to get modified
//   rotation - the bone rotation that needs to get modified
// --------------------------------------------------------------------------------
void EveTurretSet::ModifySystemBoneTransform( SystemBones bone, const Vector3* target, const Matrix* localTransform, Vector3& position, Quaternion& rotation ) const
{
	switch( bone )
	{
	case SYSBONE_INVALID:
		break;
	case SYSBONE_ROTATION:
	case SYSBONE_ROTATION01:
	case SYSBONE_ROTATION02: {
		// rotation of turret 360 degress, alpha is between -pi and pi
		float alpha = atan2( target->x, target->z );
		// never forget do apply influence!
		alpha *= m_trackingInfluence;
		// 1st: make quaternion
		Quaternion quat = RotationQuaternion( alpha, 0.f, 0.f );
		// 2nd: apply this quat after the original one
		quat = rotation * quat;
		// 3rd: make granny_transform from quat
		rotation = quat;
	}
	break;
	case SYSBONE_COUNTER_ROTATION: {
		// inverse(!!) rotation of turret 360 degress, alpha is between -pi and pi
		float alpha = -1.f * atan2( target->x, target->z );
		// never forget do apply influence!
		alpha *= m_trackingInfluence;
		// 1st: make quaternion
		Quaternion quat = RotationQuaternion( alpha, 0.f, 0.f );
		// 2nd: apply this quat after the original one
		quat = rotation * quat;
		// 3rd: make granny_transform from quat
		rotation = quat;
	}
	break;
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2: {
		CalcTransformForPitchBone( target, XMConvertToRadians( m_sysBonePitchMin ), XMConvertToRadians( m_sysBonePitchMax ), bone, localTransform, rotation );
	}
	break;
	case SYSBONE_SCALED_HEIGHT: {
		// pitch of barrel 90 degrees
		Vector3 dirNrm = Normalize( *target );
		float height = TriClamp( dirNrm.y, 0.f, 1.f );
		// never forget do apply influence!
		height *= m_trackingInfluence;
		// it's a pos extension with a scale
		Vector3 pos = Vector3( 0.f, height * m_sysBoneHeight, 0.f ) + position;
		position = pos;
	}
	break;
	case SYSBONE_SCALED_PITCH01:
	case SYSBONE_SCALED_PITCH02:
	case SYSBONE_SCALED_PITCH03:
	case SYSBONE_SCALED_PITCH04:
	case SYSBONE_SCALED_PITCH05:
	case SYSBONE_SCALED_PITCH06: {
		CalcTransformForPitchBone( target, 0.f, XMConvertToRadians( m_sysBonePitchMax ), bone, nullptr, rotation );
	}
	break;
	default:
		break;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This sets up a turret on a ship to a give position. The total number of
//   turrets in this set is dynamically changed here. Also allocate some
//   animation related granny stuff, if we have the model already loaded.
//   If we need to allocate new animation stuff, automatically play the idle
//   anim, which puts new turrets into idle state.
// Arguments:
//   turretIndex - index of turret in this set
//   localMatrix - position/orientation of single turret in object-space
// --------------------------------------------------------------------------------
void EveTurretSet::SetLocalTransform( unsigned int turretIndex, const Matrix* localMatrix )
{
	// should never be more than MAX_TURRETS_PER_SET
	if( turretIndex >= EVE_MAX_TURRETS_PER_SET )
	{
		return;
	}

	if( IsUsingCMF( m_geometryResource ) )
	{
		// keep list up to size
		if( m_singleTurrets.size() <= turretIndex )
		{
			// fill up list with new entries for turrets to meet requirement
			for( unsigned int i = (unsigned int)m_singleTurrets.size(); i <= turretIndex; ++i )
			{
				SingleTurretData data;
				if( m_skeleton )
				{
					data.sequencer = std::make_unique<cmf::AnimationSequencer>( *m_skeleton );
					cmf::RestPose( data.pose, *m_skeleton );
				}
				else
				{
					data.sequencer.reset();
					data.pose.skeleton = NULL;
					data.pose.boneTransforms.clear();
				}
				data.worldMatrix = IdentityMatrix();
				data.valid = false;
				data.visible = false;

				m_singleTurrets.push_back( std::move( data ) );

				PlayAnimation( i, "", "Active" );
			}
		}
	}
#if WITH_GRANNY
	else
	{
		// keep list up to size
		if( m_singleTurrets.size() <= turretIndex )
		{
			// fill up list with new entries for turrets to meet requirement
			for( unsigned int i = (unsigned int)m_singleTurrets.size(); i <= turretIndex; ++i )
			{
				SingleTurretData data;
				if( m_grnModel )
				{
					data.grnModelInstance = GrannyInstantiateModel( m_grnModel );
					data.grnSkeleton = GrannyGetSourceSkeleton( data.grnModelInstance );
					data.grnLocalPose = GrannyNewLocalPose( data.grnSkeleton->BoneCount );
					data.grnWorldPose = GrannyNewWorldPose( data.grnSkeleton->BoneCount );
				}
				else
				{
					data.grnModelInstance = NULL;
					data.grnSkeleton = NULL;
					data.grnLocalPose = NULL;
					data.grnWorldPose = NULL;
				}
				data.worldMatrix = IdentityMatrix();
				data.valid = false;
				data.visible = false;

				m_singleTurrets.push_back( std::move( data ) );

				PlayAnimation( i, "", "Active" );
			}
		}
	}
#endif

	Matrix noScaleLocalMatrix = IdentityMatrix();

	// remove scaling: this matrix comes from a locator which can have scaling, but we don't want it!
	TriMatrixRemoveScaling( &noScaleLocalMatrix, localMatrix );

	Vector3 translation, scale;
	Decompose( scale, m_singleTurrets[turretIndex].localQuaternion, translation, noScaleLocalMatrix );
	m_singleTurrets[turretIndex].localPosition = Vector4( translation, 1.0f );
	m_singleTurrets[turretIndex].localMatrix = noScaleLocalMatrix;

	// new one is not yet valid, cause it needs to get all calculated
	m_singleTurrets[turretIndex].valid = false;

	if( m_generatedDistributedAmbientEffect )
	{
		m_generatedDistributedAmbientEffect->UpdateInstance( turretIndex, Vector3( 1, 1, 1 ), m_singleTurrets[turretIndex].localQuaternion, translation );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Each turret set holds a unique locator name, which must match the locators
//   on a ship. To get different positions for different turrets, a character
//   is appended to the locator name (done by EveShip2!) and then compared
//   to the ship's locators (characters are: 'a', 'b', 'c', etc.)
// SeeAlso:
//   EveShip2
// --------------------------------------------------------------------------------
const char* EveTurretSet::GetLocatorName() const
{
	return m_locatorName.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   The turret gets a slot number from the application.
// --------------------------------------------------------------------------------
int EveTurretSet::GetSlotNumber() const
{
	return m_slotNumber;
}

// --------------------------------------------------------------------------------
bool EveTurretSet::GetLocalBoundingBox( Vector3& aabbMin, Vector3& aabbMax )
{
	// Currently only really need this in the case where we have unusual bounds(moon drill)
	// In most cases the bounds fall within the bounding box of the parent so we ignore this.
	if( !m_useDynamicBounds )
	{
		return false;
	}

	BoundingBoxInitialize( aabbMin, aabbMax );
	Vector3 minBounds, maxBounds;
	bool valid = false;
	for( const auto& turret : m_singleTurrets )
	{
		if( turret.valid )
		{
			if( GetDynamicBounds( turret, nullptr, &minBounds, &maxBounds ) )
			{
				BoundingBoxUpdate( aabbMin, aabbMax, minBounds, maxBounds );
				valid = true;
			}
		}
	}

	return valid;
}

// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this turret set
// --------------------------------------------------------------------------------
void EveTurretSet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "TurretBounds" ) )
	{
		// draw bounding sphere for every turret in this set
		for( const auto& turret : m_singleTurrets )
		{
			if( turret.valid )
			{
				Vector3 center = TransformCoord( m_boundingSphere.GetXYZ(), turret.worldMatrix );
				renderer.DrawSphere( this, center, m_boundingSphere.w, 10, Tr2DebugRenderer::Wireframe, 0xffffff00 );
			}
		}
		RenderDynamicBounds();
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		if( auto debuggable = dynamic_cast<ITr2DebugRenderable*>( ambientEffect ) )
		{
			debuggable->RenderDebugInfo( renderer );
		}
	}
	if( m_firingEffect )
	{
		m_firingEffect->RenderDebugInfo( renderer );
	}

	if( m_turretMovementObserver && m_playMovementSound )
	{
		m_turretMovementObserver->RenderDebugInfo( renderer );
	}
}

void EveTurretSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "TurretBounds" );

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		if( auto debuggable = dynamic_cast<ITr2DebugRenderable*>( ambientEffect ) )
		{
			debuggable->GetDebugOptions( options );
		}
	}

	if( m_firingEffect )
	{
		m_firingEffect->GetDebugOptions( options );
	}

	if( m_turretMovementObserver )
	{
		m_turretMovementObserver->GetDebugOptions( options );
	}
}


int EveTurretSet::GetState() const
{
	return m_state;
}

// --------------------------------------------------------------------------------
// Description:
//   Check if the object is casting a shadow in the camera/shadow frustums
// --------------------------------------------------------------------------------
bool EveTurretSet::IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const
{
	if( !m_display || !m_geometryResource )
	{
		return false;
	}

	if( renderReason == TR2RENDERREASON_REFLECTION )
	{
		return false;
	}

	sizeInShadow = 0;

	for( auto& turret : m_singleTurrets )
	{
		Vector4 transformedBoundingSphere = m_boundingSphere;
		// transform bounding sphere into world space to check against frustum
		BoundingSphereTransform( turret.worldMatrix, transformedBoundingSphere );

		if( transformedBoundingSphere.w > 0.0f )
		{
			if( shadowFrustum.IsVisible( cameraFrustum, transformedBoundingSphere ) )
			{
				sizeInShadow = max( sizeInShadow, shadowFrustum.GetSizeInShadow( transformedBoundingSphere ) );
			}
		}
	}
	return sizeInShadow > 5.f;
}

void EveTurretSet::UpdateVisibility( const EveUpdateContext& updateContext )
{
	m_parentShLighting = nullptr;

	// check visibility for each single turret and keep MAX on-screen pixel diameter.
	// The pixel diameter is reset in UpdateLOD on next frame - this is to account for
	// multiple views. We need to keep updating turrets as long as they're visible in any
	// view, not just the last one rendered.
	m_visibleCount = 0;

	// display?
	if( !m_display )
	{
		return;
	}
	auto& frustum = updateContext.GetFrustum();

	for( auto& turret : m_singleTurrets )
	{
		// transform bounding sphere into world space to check against frustum
		Vector4 transformedBoundingSphere = m_boundingSphere;
		GetDynamicBounds( turret, &transformedBoundingSphere, nullptr, nullptr );
		BoundingSphereTransform( turret.worldMatrix, transformedBoundingSphere );
		turret.visible = frustum.IsSphereVisible( &transformedBoundingSphere );
		// count visible turrets of this set, mainly for debug purposes
		if( turret.visible )
		{
			// one more visible turret
			++m_visibleCount;
			// how big is it on screen? Attention: we want the max diameter, since we only have one lodlevel per multiple instanced turrets!
			float currentPixelDiameter = frustum.GetPixelSizeAccross( &transformedBoundingSphere );
			if( currentPixelDiameter > m_estimatedPixelDiameter )
			{
				m_estimatedPixelDiameter = currentPixelDiameter;
			}
		}
	}

	if( m_displayEffects && m_firingEffect )
	{
		m_firingEffect->UpdateVisibility( updateContext );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		Tr2Lod currentLod = Tr2Lod::TR2_LOD_HIGH;
		if( m_lodLevel == LOD::LOD_EMPTY )
		{
			currentLod = Tr2Lod::TR2_LOD_LOW;
		}
		else if( m_lodLevel == LOD::LOD_INVALID )
		{
			currentLod = Tr2Lod::TR2_LOD_UNSPECIFIED;
		}

		ambientEffect->UpdateVisibility( updateContext, m_parentData.transform, currentLod );
	}

	if( updateContext.m_raytracingEnabled )
	{
		UpdateRtMesh();
		UpdateRtSkeleton();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Put this object on the list, since it
//   is an ITr2Renderable.
//   Also get the renderables from the fire effects, if we are firing.
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveStretch
// --------------------------------------------------------------------------------
void EveTurretSet::GetRenderables( std::vector<ITr2Renderable*>& renderables, const Vector4* shLighting )
{
	m_parentShLighting = nullptr;

	if( !m_display )
	{
		return;
	}

	// add this object (which is a renderable), if it is visible
	if( m_geometryResource && m_visibleCount && m_turretEffect )
	{
		m_parentShLighting = shLighting;
		renderables.push_back( this );
	}

	// add firing effects, we should add some lodding for this
	if( m_displayEffects && m_firingEffect )
	{
		m_firingEffect->GetRenderables( renderables );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect && IsAmbientVisible() )
	{
		ambientEffect->GetRenderables( renderables );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency.
// --------------------------------------------------------------------------------
bool EveTurretSet::HasTransparentBatches()
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Only have opaque batches via a geometry provider, since we are using
//   instanced rendering.
// --------------------------------------------------------------------------------
void EveTurretSet::GetBatches( ITriRenderBatchAccumulator* batches,
							   TriBatchType batchType,
							   const Tr2PerObjectData* perObjectData,
							   Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_OPAQUE )
	{
		return;
	}
	if( !m_display || !m_visibleCount )
	{
		return;
	}
	if( !m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( !m_geometryResource || !m_geometryResource->IsGood() )
	{
		return;
	}
	const TriGeometryResLodData* lod = m_geometryResource->GetMeshLod( 0, 0 );
	if( !lod || !lod->m_allocationsValid )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_turretEffect );
	batch.SetGeometry( m_vertexDeclHandle, lod->m_vertexAllocation, m_instanceBuffer, lod->m_indexAllocation );
	batch.SetPerObjectData( perObjectData );
	batch.SetDrawIndexedInstanced(
		lod->m_primitiveCount * 3,
		m_visibleCount,
		lod->m_indexAllocation.GetStartIndex(),
		lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
		m_instanceBuffer.GetOffset() / m_instanceBuffer.GetStride() );

	if( batch )
	{
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get batches only for shadow generation
// --------------------------------------------------------------------------------
void EveTurretSet::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
	if( !m_display || !m_visibleCount )
	{
		return;
	}
	if( !m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( !m_geometryResource || !m_geometryResource->IsGood() )
	{
		return;
	}
	const TriGeometryResLodData* lod = m_geometryResource->GetMeshLod( 0, 0 );
	if( !lod || !lod->m_allocationsValid )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_turretEffect );
	batch.SetGeometry( m_vertexDeclHandle, lod->m_vertexAllocation, m_instanceBuffer, lod->m_indexAllocation );
	batch.SetPerObjectData( perObjectData );

	batch.SetDrawIndexedInstanced(
		lod->m_primitiveCount * 3,
		m_visibleCount,
		lod->m_indexAllocation.GetStartIndex(),
		lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
		m_instanceBuffer.GetOffset() / m_instanceBuffer.GetStride() );

	batches->Commit( batch );
}

// --------------------------------------------------------------------------------
// Description:
//   No sorting. Everything is NonSorted
// --------------------------------------------------------------------------------
float EveTurretSet::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill the per-object data. First the world matrix of the parent-ship, then
//   up to x matrices for each single turret (for deformation in the
//   vertex shader). Pass both pose and world matrix to vertex shader, so
//   we can pose the mesh in the shader and then "cut-off" (clip) all z < 0
//   pixels to avoid intersecting mesh geom for inactive turrets on ship's wings.
// SeeAlso:
//   EveTurretSetPerObjectData, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveTurretSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( !m_geometryResource )
	{
		return NULL;
	}

	if( !m_geometryResource->IsGood() )
	{
		return NULL;
	}

	if( m_geometryResource->GetMeshCount() < 1 )
	{
		return NULL;
	}

	// allocate only once
	auto perObjectData = accumulator->Allocate<EveTurretSetPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}


	if( IsUsingCMF( m_geometryResource ) )
	{
		Vector3 scale, translation;

		// tell "parent"-ship matrix
		perObjectData->m_vsData.m_shipMatrix = Transpose( m_parentData.transform );
		perObjectData->m_vsData.m_prevShipMatrix = Transpose( m_shipTransformPrev );

		// put together clip-data, so we can have a clip plane to cut off the turrets base to avoid intersections
		perObjectData->m_vsData.m_baseCutoffData = Vector4( m_bottomClipHeight, 0.f, 0.f, 0.f );

		// fill with data
		if( !m_singleTurrets.empty() )
		{
			unsigned int defaultBonesPerTurret = 3;
			// to-bone-index-mapping for the shader (is the same for all turrets of the set)
			unsigned int boneCount = m_skeletonBoneIndices.size() ? (unsigned int)m_skeletonBoneIndices.size() : defaultBonesPerTurret;

			// Index of the bone in the big translation and rotation
			unsigned int turretIndex = 0;
			uint32_t boneIndex = 0;

			// put all single turret's positions and rotations in the array
			for( const auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					// actual turret position and rotation
					if( turret.valid )
					{
						perObjectData->m_vsData.m_turretRotation[turretIndex] = turret.localQuaternion;
						perObjectData->m_vsData.m_turretTranslation[turretIndex] = turret.localPosition;
					}
					else
					{
						perObjectData->m_vsData.m_turretTranslation[turretIndex] = Vector4( 0, 0, 0, 1 );
						perObjectData->m_vsData.m_turretRotation[turretIndex] = Quaternion( 0, 0, 0, 1 );
					}

					++turretIndex;
				}
			}

			if( m_boneOffsets.GetCurrentFrameOffset() == Tr2RingBufferOffsets::INVALID_OFFSET )
			{
				auto transforms = static_cast<Float4x3*>( accumulator->Allocate( sizeof( Float4x3 ) * m_singleTurrets.size() * boneCount ) );

				for( const auto& turret : m_singleTurrets )
				{
					if( turret.visible )
					{
						// get animation matrices here, they are not the same for all turrets of the set
						auto& worldTransforms = turret.worldTransforms;

						// Construct all turret bone translations and rotations
						for( unsigned int j = 0; j < boneCount; ++j )
						{
							Matrix bone;
							if( turret.valid && m_skeletonBoneIndices.size() && worldTransforms.size() )
							{
								int32_t skelBoneIdx = m_skeletonBoneIndices[j];
								if( skelBoneIdx >= 0 )
								{
									CCP_ASSERT( skelBoneIdx < m_skeleton->invBindTransforms.size() );
									CCP_ASSERT( skelBoneIdx < worldTransforms.size() );
									bone = m_skeleton->invBindTransforms[skelBoneIdx] * worldTransforms[skelBoneIdx];
								}
								else
								{
									bone = IdentityMatrix();
								}
							}
							else
							{
								bone = IdentityMatrix();
							}
							transforms[boneIndex++] = Float4x3( bone );
						}
					}
				}

				if( boneIndex != 0 )
				{
					m_boneOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Float4x3>(), transforms, boneIndex );
				}
			}

			perObjectData->m_vsData.m_currentBoneOffset = m_boneOffsets.GetCurrentFrameOffset();
			perObjectData->m_vsData.m_prevBoneOffset = m_boneOffsets.GetPreviousFrameOffset();

			// store how many bones in in the turrets, so we can correctly read from the buffer in the shader
			perObjectData->m_vsData.m_turretSetData = Vector4( (float)boneCount, 0, 0, 0 );

			// ps data
			perObjectData->m_psData.m_shipData = m_parentData.shipData;
			perObjectData->m_psData.m_clipData1 = Vector4( m_parentData.clipSphereCenter, m_parentData.clipRadiusSq );
			perObjectData->m_psData.m_clipRadius2Sq = m_parentData.clipRadius2Sq;
			if( m_parentShLighting )
			{
				memcpy( perObjectData->m_psData.m_shLightingCoefficients, m_parentShLighting, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
			}
			else
			{
				memset( perObjectData->m_psData.m_shLightingCoefficients, 0, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
			}
		}

		return perObjectData;
	}
#if WITH_GRANNY
	else
	{
		Vector3 scale, translation;

		// tell "parent"-ship matrix
		perObjectData->m_vsData.m_shipMatrix = Transpose( m_parentData.transform );
		perObjectData->m_vsData.m_prevShipMatrix = Transpose( m_shipTransformPrev );

		// put together clip-data, so we can have a clip plane to cut off the turrets base to avoid intersections
		perObjectData->m_vsData.m_baseCutoffData = Vector4( m_bottomClipHeight, 0.f, 0.f, 0.f );

		// fill with data
		if( !m_singleTurrets.empty() )
		{
			unsigned int defaultBonesPerTurret = 3;
			// to-bone-index-mapping for the shader (is the same for all turrets of the set)
			const int* toBoneIndices = m_grnMeshBinding ? GrannyGetMeshBindingToBoneIndices( m_grnMeshBinding ) : NULL;
			unsigned int boneCount = m_grnMeshBinding ? GrannyGetMeshBindingBoneCount( m_grnMeshBinding ) : defaultBonesPerTurret;

			// Index of the bone in the big translation and rotation
			unsigned int turretIndex = 0;
			uint32_t boneIndex = 0;

			// put all single turret's positions and rotations in the array
			for( const auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					// actual turret position and rotation
					if( turret.valid )
					{
						perObjectData->m_vsData.m_turretRotation[turretIndex] = turret.localQuaternion;
						perObjectData->m_vsData.m_turretTranslation[turretIndex] = turret.localPosition;
					}
					else
					{
						perObjectData->m_vsData.m_turretTranslation[turretIndex] = Vector4( 0, 0, 0, 1 );
						perObjectData->m_vsData.m_turretRotation[turretIndex] = Quaternion( 0, 0, 0, 1 );
					}

					++turretIndex;
				}
			}

			if( m_boneOffsets.GetCurrentFrameOffset() == Tr2RingBufferOffsets::INVALID_OFFSET )
			{
				auto transforms = static_cast<Float4x3*>( accumulator->Allocate( sizeof( Float4x3 ) * m_singleTurrets.size() * boneCount ) );

				for( const auto& turret : m_singleTurrets )
				{
					if( turret.visible )
					{
						// get animation matrices here, they are not the same for all turrets of the set
						const Matrix* compositeMatrixArray = turret.grnWorldPose ?
							reinterpret_cast<const Matrix*>( GrannyGetWorldPoseComposite4x4Array( turret.grnWorldPose ) ) :
							nullptr;

						// Construct all turret bone translations and rotations
						for( unsigned int j = 0; j < boneCount; ++j )
						{
							Matrix bone;
							if( turret.valid && toBoneIndices && compositeMatrixArray )
							{
								bone = compositeMatrixArray[toBoneIndices[j]];
							}
							else
							{
								bone = IdentityMatrix();
							}
							transforms[boneIndex++] = Float4x3( bone );
						}
					}
				}

				if( boneIndex != 0 )
				{
					m_boneOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Float4x3>(), transforms, boneIndex );
				}
			}

			perObjectData->m_vsData.m_currentBoneOffset = m_boneOffsets.GetCurrentFrameOffset();
			perObjectData->m_vsData.m_prevBoneOffset = m_boneOffsets.GetPreviousFrameOffset();

			// store how many bones in in the turrets, so we can correctly read from the buffer in the shader
			perObjectData->m_vsData.m_turretSetData = Vector4( (float)boneCount, 0, 0, 0 );

			// ps data
			perObjectData->m_psData.m_shipData = m_parentData.shipData;
			perObjectData->m_psData.m_clipData1 = Vector4( m_parentData.clipSphereCenter, m_parentData.clipRadiusSq );
			perObjectData->m_psData.m_clipRadius2Sq = m_parentData.clipRadius2Sq;
			if( m_parentShLighting )
			{
				memcpy( perObjectData->m_psData.m_shLightingCoefficients, m_parentShLighting, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
			}
			else
			{
				memset( perObjectData->m_psData.m_shLightingCoefficients, 0, sizeof( perObjectData->m_psData.m_shLightingCoefficients ) );
			}
		}

		return perObjectData;
	}
#else
	else
	{
		return nullptr;
	}
#endif
}

Tr2PerObjectData* EveTurretSet::GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectData( accumulator );
}

void EveTurretSet::PushRtGeometry( Tr2RaytracingManager& rtManager ) const
{
	if( !m_display )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();


	for( auto it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		if( it->visible && it->valid && it->rtMesh && it->rtMesh->IsGood() && it->rtMeshArea )
		{
			if( !it->rtPerObjectData.IsValid() )
			{
				it->rtPerObjectData.Create( sizeof( EveSpaceObjectPSData ), renderContext );
			}

			EveTurretSetPSData* perObjectData;

			it->rtPerObjectData.Lock( (void**)&perObjectData, renderContext );

			perObjectData->m_shipData = m_parentData.shipData;
			perObjectData->m_clipData1 = Vector4( TransformCoord( m_parentData.clipSphereCenter, Inverse( it->localMatrix ) ), m_parentData.clipRadiusSq );
			perObjectData->m_clipRadius2Sq = m_parentData.clipRadius2Sq;
			if( m_parentShLighting )
			{
				memcpy( perObjectData->m_shLightingCoefficients, m_parentShLighting, sizeof( perObjectData->m_shLightingCoefficients ) );
			}
			else
			{
				memset( perObjectData->m_shLightingCoefficients, 0, sizeof( perObjectData->m_shLightingCoefficients ) );
			}

			it->rtPerObjectData.Unlock( renderContext );

			rtManager.GetGeometry().AddGeometry( *it->rtMesh, *it->rtMeshArea, m_turretEffect, &it->rtPerObjectData, nullptr, it->worldMatrix );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Play an animation to a single turret
// Arguments:
//   turretIndex - which turret is going to get this anim
//   animName - animation name like it says in the gr2 file, which will be played now
//   animNameIdle - animation name like it says in the gr2 file, which will be played in loop after animName
//   delay - wait time in seconds until anims kick in (usually 0.0)
// Return Value:
//   Length of animation in seconds or 0.f if error
// --------------------------------------------------------------------------------
float EveTurretSet::PlayAnimation( unsigned int turretIndex, const std::string& animName, const std::string& animNameIdle, float delay )
{
	float animLength = 0.f;

	// if we don't have a geometry or a shader, animation is useless and probably unwanted
	if( !m_geometryResource || !m_turretEffect )
	{
		return 0.f;
	}

	// if this one still is getting loaded, buffer the animation play request
	if( !m_geometryResource->IsPrepared() )
	{
		AnimationRequest ar;
		ar.turretIndex = turretIndex;
		ar.animName = animName;
		ar.animNameIdle = animNameIdle;
		m_animationQueue.push_back( ar );
		return 0.f;
	}

	if( !m_geometryResource->IsGood() )
	{
		return 0.f;
	}

	if( IsUsingCMF( m_geometryResource ) )
	{
		auto cmfData = m_geometryResource->GetCMFData();
		if( cmfData )
		{
			// there can be more animations in one res, so find right one
			size_t animIx = cmfData->animations.size();
			if( !animName.empty() )
			{
				auto animation = std::find_if( cmfData->animations.begin(), cmfData->animations.end(), [&animName]( const cmf::Animation& anim ) {
					return cmf::ToStdStringView( anim.name ) == animName;
				} );
				if( animation == cmfData->animations.end() )
				{
					return 0.f;
				}
				animIx = std::distance( cmfData->animations.begin(), animation );
			}

			size_t idleIx = cmfData->animations.size();
			if( !animNameIdle.empty() )
			{
				auto animation = std::find_if( cmfData->animations.begin(), cmfData->animations.end(), [&animNameIdle]( const cmf::Animation& anim ) {
					return cmf::ToStdStringView( anim.name ) == animNameIdle;
				} );
				if( animation == cmfData->animations.end() )
				{
					return 0.f;
				}
				idleIx = std::distance( cmfData->animations.begin(), animation );
			}

			// stop all animation
			StopAnimation( turretIndex, delay );

			// play both (2nd with delay)
			if( m_singleTurrets.size() > turretIndex )
			{
				if( m_singleTurrets[turretIndex].sequencer )
				{
					// granny, play first anim once, if provided & found
					if( animIx != cmfData->animations.size() )
					{
						auto& animation = cmfData->animations[animIx];
						animLength = animation.duration;

						auto player = m_singleTurrets[turretIndex].sequencer->PlayAnimation( animation );
						player->SetStartTime( delay + Tr2Renderer::GetAnimationTime() );
						player->SetLoopCount( 1 );
						player->SetSpeed( 1.f );
						player->SetStopTime( animLength + delay + Tr2Renderer::GetAnimationTime() );
					}

					// then play idle anim on loop (after delay), if provided & found
					if( idleIx != cmfData->animations.size() )
					{
						auto& animation = cmfData->animations[idleIx];

						auto player = m_singleTurrets[turretIndex].sequencer->PlayAnimation( animation );
						player->SetStartTime( animLength + delay + Tr2Renderer::GetAnimationTime() );
						player->SetLoopCount( 0 );
						player->SetSpeed( 1.f );
					}
				}
			}
		}

		return animLength;
	}
#if WITH_GRANNY
	else
	{
		granny_file_info* grannyFileInfo = m_geometryResource->GetGrannyInfo();
		if( grannyFileInfo )
		{
			// there can be more animations in one res, so find right one
			int animIx = grannyFileInfo->AnimationCount;
			if( !animName.empty() )
			{
				for( int i = 0; i < grannyFileInfo->AnimationCount; ++i )
				{
					if( strcmp( grannyFileInfo->Animations[i]->Name, animName.c_str() ) == 0 )
					{
						animIx = i;
						break;
					}
				}
				if( animIx == grannyFileInfo->AnimationCount )
				{
					return 0.f;
				}
			}

			int idleIx = grannyFileInfo->AnimationCount;
			if( !animNameIdle.empty() )
			{
				for( int i = 0; i < grannyFileInfo->AnimationCount; ++i )
				{
					if( strcmp( grannyFileInfo->Animations[i]->Name, animNameIdle.c_str() ) == 0 )
					{
						idleIx = i;
						break;
					}
				}
				if( idleIx == grannyFileInfo->AnimationCount )
				{
					return 0.f;
				}
			}

			// stop all animation
			StopAnimation( turretIndex, delay );

			// play both (2nd with delay)
			if( m_singleTurrets.size() > turretIndex )
			{
				if( m_singleTurrets[turretIndex].grnModelInstance )
				{
					// granny, play first anim once, if provided & found
					if( animIx != grannyFileInfo->AnimationCount )
					{
						animLength = grannyFileInfo->Animations[animIx]->Duration;

						granny_control* controlAnim = GrannyPlayControlledAnimation( delay + Tr2Renderer::GetAnimationTime(), grannyFileInfo->Animations[animIx], m_singleTurrets[turretIndex].grnModelInstance );
						GrannyEaseControlIn( controlAnim, 0.f, false );
						GrannySetControlLoopCount( controlAnim, 1 );
						GrannySetControlSpeed( controlAnim, 1.f );
						GrannySetControlClock( controlAnim, Tr2Renderer::GetAnimationTime() );
						GrannyCompleteControlAt( controlAnim, animLength + delay + Tr2Renderer::GetAnimationTime() );
					}

					// then play idle anim on loop (after delay), if provided & found
					if( idleIx != grannyFileInfo->AnimationCount )
					{
						granny_control* controlIdle = GrannyPlayControlledAnimation( animLength + delay + Tr2Renderer::GetAnimationTime(), grannyFileInfo->Animations[idleIx], m_singleTurrets[turretIndex].grnModelInstance );
						GrannyEaseControlIn( controlIdle, 0.f, false );
						GrannySetControlLoopCount( controlIdle, 0 );
						GrannySetControlSpeed( controlIdle, 1.f );
						GrannySetControlClock( controlIdle, Tr2Renderer::GetAnimationTime() );
					}
				}
			}
		}

		return animLength;
	}
#else
	else
	{
		return 0.f;
	}
#endif
}

// --------------------------------------------------------------------------------
// Description:
//   Stop all animations to a single turret
// Arguments:
//   turretIndex - which turret's animations to stop
//   delay - wait time in seconds until anims stop (usually 0.0)
// --------------------------------------------------------------------------------
void EveTurretSet::StopAnimation( unsigned int turretIndex, float delay )
{
	// if we don't have a geometry or a shader, animation is useless and probably unwanted
	if( !m_geometryResource || !m_turretEffect )
	{
		return;
	}

	// empty queue, so no more buffered requests
	m_animationQueue.clear();

	if( IsUsingCMF( m_geometryResource ) )
	{
		// stop
		if( m_singleTurrets.size() > turretIndex )
		{
			if( m_singleTurrets[turretIndex].sequencer )
			{
				m_singleTurrets[turretIndex].sequencer->EnumerateAnimations( [&]( const std::shared_ptr<cmf::AnimationPlayer>& player ) {
					player->SetStopTime( delay + Tr2Renderer::GetAnimationTime() );
				} );

				m_singleTurrets[turretIndex].sequencer->RemoveFinishedAnimations( Tr2Renderer::GetAnimationTime() );
			}
		}
	}
#if WITH_GRANNY
	else
	{
		// stop
		if( m_singleTurrets.size() > turretIndex )
		{
			if( m_singleTurrets[turretIndex].grnModelInstance )
			{
				for( granny_model_control_binding* b = GrannyModelControlsBegin( m_singleTurrets[turretIndex].grnModelInstance ); b != GrannyModelControlsEnd( m_singleTurrets[turretIndex].grnModelInstance ); )
				{
					granny_control* control = GrannyGetControlFromBinding( b );
					b = GrannyModelControlsNext( b );
					GrannyCompleteControlAt( control, delay + Tr2Renderer::GetAnimationTime() );
					GrannyFreeControlIfComplete( control );
				}
			}
		}
	}
#endif
}

// --------------------------------------------------------------------------------
// Description:
//   Usually animation names are just constant strings like "deploy" or "idle".
//   But in the firing case, we support more than one for cycling through them
//   to make the firing look more lively.
// --------------------------------------------------------------------------------
std::string EveTurretSet::GetFireAnimationName() const
{
	// if m_currentCyclingFiresPos is 0, it's just "Fire"
	std::string res = "Fire";
	if( m_currentCyclingFiresPos > 0 )
	{
		res.push_back( '0' );
		res.push_back( '0' + m_currentCyclingFiresPos / m_cyclingFireGroupCount );
	}

	return res;
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to put this turret set into the deactive state. Depending
//   on the current state we trigger different anims, wait times. etc.
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateDeactive()
{
	// what state are we in?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// do nothing if we are already in this state
		break;
	case STATE_IDLE:
	case STATE_RELOADING:
		// no fadeout of tracking, just play deactive anim and then the deactive loop
		m_trackingInfluence = 0.f;
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Pack", "Inactive" );
			m_delayToFadeOutTracking = 0.f;
		}
		break;
	case STATE_FIRING:
		// stop shooting
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		// DON'T break, just continue with stopping things:
	case STATE_TARGETING:
		// fadeout the tracking, play deactive anim and then the deactive loop
		m_delayToFadeOutTracking = 0.0001f;
		m_activeTurret = INVALID_TURRET_INDEX;
		m_target->StopFireAtLocator();
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Pack", "Inactive", TRACKING_FADE_TIME );
		}
		break;

	default:
		break;
	}
	// finally, we can set state
	m_state = STATE_DEACTIVE;

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to put this turret set into the idle state. Idle
//   means the guns are all facing forward but are deployed.
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateIdle()
{
	if( !m_isOnline )
	{
		return;
	}

	// what state are we in?
	switch( m_state )
	{
	case STATE_INVALID:
	case STATE_RELOADING:
		// just play active loop
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active" );
		}
		break;
	case STATE_DEACTIVE:
		// start unpack animation, disable tracking and then into active loop
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Deploy", "Active" );
		}
		m_trackingInfluence = 0.f;
		break;
	case STATE_IDLE:
		// do nothing here
		break;
	case STATE_TARGETING:
	case STATE_FIRING:
		// stop shooting, fadout tracking, then into active loop
		m_delayToFadeOutTracking = 0.0001f;
		m_activeTurret = INVALID_TURRET_INDEX;
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active", TRACKING_FADE_TIME );
		}

		if( m_playMovementSound && !m_targetingToIdleMovementAudioEvent.empty() )
		{
			SendEventToAudEmitter( m_turretMovementObserver, m_targetingToIdleMovementAudioEvent );
		}
		break;
	}
	// finally, we can set state
	m_state = STATE_IDLE;

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Todo
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateTargeting()
{
	float animLength = 0.f;
	if( !m_isOnline )
	{
		return;
	}

	// what state are we in?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// play deplpoy anim, then active loop and fade in tracking
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			animLength = PlayAnimation( i, "Deploy", "Active", TRACKING_FADE_TIME );
		}
		// fade in tracking
		m_delayToFadeInTracking = animLength + 0.0001f;
		break;
	case STATE_IDLE:
	case STATE_RELOADING:
		// fadein tracking, play active loop
		m_delayToFadeInTracking = 0.0001f;
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active", TRACKING_FADE_TIME );
		}
		break;
	case STATE_TARGETING:
		break;
	case STATE_FIRING:
		// stop shooting, then into active loop
		m_activeTurret = INVALID_TURRET_INDEX;
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active", 0.f );
		}
		break;

	default:
		break;
	}

	// finally, we can set state
	m_state = STATE_TARGETING;

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to put this turret set into the firing state. It will
//   start playing the fireanim and then go into standard active loop until
//   this function is called again.
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateFiring()
{
	if( !SetupFiringState() )
	{
		return;
	}

	// only if we are in firing mode, call ::StopFiring() on the effect right before
	// we call ::PrepareFiring(), it'll clean things up in the effect
	if( m_firingEffect && m_state == STATE_FIRING )
	{
		if( m_firingEffect->IsLooping() )
		{
			// We don't want to start and stop the curves when the turret is looping and firing
			m_firingEffect->PrepareFiringEffectMoveObjects();
			return;
		}
		m_firingEffect->StopFiring();
	}

	// We're starting a firing sequence, we need to set up our firing effect time-delays
	if( m_firingEffect )
	{
		if( m_maxCyclingFirePos > 1 )
		{
			m_firingEffect->PrepareFiring( m_randomFiringDelay, m_currentCyclingFiresPos, m_cyclingFireGroupCount );
		}
		else
		{
			m_firingEffect->PrepareFiring( m_randomFiringDelay );
		}

		if( m_target != nullptr )
		{
			m_firingEffect->SetImpactConfiguration( m_target->GetImpactConfiguration() );
		}
	}

	// finally, we can set state
	m_state = STATE_FIRING;
}


// --------------------------------------------------------------------------------
// Description:
// Sets up information required for the firing state to render correctly
// This does not change the state or start/stop the firing effect
// --------------------------------------------------------------------------------
bool EveTurretSet::SetupFiringState()
{
	if( m_state == STATE_DEACTIVE )
	{
		// this state change is forbidden!
		CCP_LOGERR( "EveTurretSet %s wants to fire but is in deactive state.", m_name.c_str() );
		return false;
	}
	// find the best single turret and damage locator pair
	unsigned int closestTurret = INVALID_TURRET_INDEX;
	int closestLocator = -1;
	GetClosestTurretAndLocator( closestTurret, closestLocator );

	// if this turret is set to cycle through the muzzles for firing, do it here
	if( m_maxCyclingFirePos > 1 )
	{
		m_currentCyclingFiresPos += m_cyclingFireGroupCount;
		if( m_currentCyclingFiresPos >= m_maxCyclingFirePos * m_cyclingFireGroupCount )
		{
			m_currentCyclingFiresPos = 0;
		}
	}

	// timing: apply a randomized fire delay
	CalcRandomDelay();

	// timing: is the length of the firing effect known?
	float effectTotalTime = m_firingEffect ? m_firingEffect->GetFiringDuration() : 0.f;
	float effectPeakTime = m_firingEffect ? m_firingEffect->GetFiringPeakTime() : 0.f;

	Vector3 source = m_parentData.transform.GetTranslation();

	// what state are we in?
	switch( m_state )
	{
	case STATE_IDLE:
	case STATE_RELOADING:
		// and delay the effect until we are facing target
		m_randomFiringDelay += m_maxTrackingTime;
		// fadein tracking, play fire anim (only one the firing turret!) and then the active anim
		m_delayToFadeInTracking = 0.0001f;

		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			if( closestTurret == i )
			{
				// this one shoots!
				PlayAnimation( i, GetFireAnimationName(), "Active", m_randomFiringDelay );
			}
			else
			{
				// this one just idles!
				PlayAnimation( i, "", "Active", m_randomFiringDelay );
			}
		}
		// assign locator and turret
		m_target->StartFireAtLocator( closestLocator, m_randomFiringDelay + effectPeakTime, effectTotalTime - effectPeakTime, &source );
		m_activeTurret = closestTurret;
		break;
	case STATE_FIRING:
	case STATE_TARGETING:
		// directly go into firing loop
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			if( closestTurret == i )
			{
				// this one shoots!
				PlayAnimation( i, GetFireAnimationName(), "Active", m_randomFiringDelay );
			}
			else
			{
				// this one just idles!
				PlayAnimation( i, "", "Active", m_randomFiringDelay );
			}
		}
		// switch to new location
		m_activeTurret = closestTurret;
		m_target->StartFireAtLocator( closestLocator, m_randomFiringDelay + effectPeakTime, effectTotalTime - effectPeakTime, &source );
		break;

	default:
		break;
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		if( m_state == STATE_FIRING )
		{
			ambientEffect->SetControllerVariable( "TurretState", STATE_TARGETING );
		}
		else
		{
			ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
		}

		SetAmbientEffectControllerVariableOnInstance( m_activeTurret, "TurretState", STATE_FIRING );
		SetAmbientEffectControllerVariableOnInstance( m_activeTurret, "FiringDelay", m_randomFiringDelay );
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to put this turret set into the reloading state. It will
//   play the reloading animation follow then by idle
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateReloading()
{
	// what state are we in?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// ingnore this state change: when the turret is inactive, no reload state can be shown!
		break;
	case STATE_INVALID:
	case STATE_IDLE:
	case STATE_RELOADING:
		// just play reloading anim and then loop
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Reload", "Active", 0.f );
		}
		break;
	case STATE_TARGETING:
	case STATE_FIRING:
		// stop shooting, fadout tracking, then into active loop
		m_delayToFadeOutTracking = 0.0001f;
		m_activeTurret = INVALID_TURRET_INDEX;
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Reload", "Active", TRACKING_FADE_TIME );
		}
		break;

	default:
		break;
	}
	// finally, we can set state
	m_state = STATE_RELOADING;

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetControllerVariable( "TurretState", float( m_state ) );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to force this turret set into the deactive state. No nice
//   animations are played, no smooth transitions, no nothing, just "Force"
// --------------------------------------------------------------------------------
void EveTurretSet::ForceStateDeactive()
{
	// turn it all off
	m_trackingInfluence = 0.f;
	m_delayToFadeOutTracking = 0.f;
	m_activeTurret = INVALID_TURRET_INDEX;
	m_target->StopFireAtLocator();
	if( m_firingEffect )
	{
		m_firingEffect->StopFiring();
	}
	// finally, we can set state
	m_state = STATE_DEACTIVE;

	// now force-play the deactive anim for this state
	ForceIdleAnimation();
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to force this turret set into the targeting state. No nice
//   animations are played, no smooth transitions, no nothing, just "Force"
// --------------------------------------------------------------------------------
void EveTurretSet::ForceStateTargeting()
{
	m_trackingInfluence = m_maxTrackingTime;
	m_trackingInfluenceDelta = 0.f;
	unsigned int closestTurret = INVALID_TURRET_INDEX;
	int closestLocator = -1;
	GetClosestTurretAndLocator( closestTurret, closestLocator );
	m_activeTurret = closestTurret;

	// finally, we can set state
	m_state = STATE_TARGETING;

	// now force-play the deactive anim for this state
	PlayAnimation( m_activeTurret, "", "Active", 0.f );
}

// --------------------------------------------------------------------------------
// Description:
//   This one forces an idle animation on a turret based on the current state.
// --------------------------------------------------------------------------------
void EveTurretSet::ForceIdleAnimation()
{
	std::string idleAnimName = "";
	// what state?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		idleAnimName = "Inactive";
		break;
	case STATE_IDLE:
	case STATE_TARGETING:
	case STATE_FIRING:
		idleAnimName = "Active";
		break;

	default:
		break;
	}

	// set it to all turrets in this set
	if( idleAnimName.length() > 0 )
	{
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", idleAnimName, 0.f );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to trigger a recalculation of the stored bounding data
// --------------------------------------------------------------------------------
void EveTurretSet::RebuildBoundingSphere()
{
	// if there is geometry there, just ask it to rebuild
	if( m_geometryResource )
	{
		if( m_geometryResource->GetMeshCount() )
		{
			m_geometryResource->RecalculateBoundingSphere();
			m_geometryResource->GetBoundingSphere( 0, m_boundingSphere );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Puts this turret set into highest LOD and disables any LODing. Usually used
//   for photoservice or any non-space UI rendering.
// --------------------------------------------------------------------------------
void EveTurretSet::FreezeHighDetailLOD()
{
	// disable LOD
	m_lodLevel = LOD_DISABLED;
	// init high geom
	InitializeGeometryResource();
}

// --------------------------------------------------------------------------------
// Description:
//   Turretactions like shooting or activating all have a small random time delay,
//   so they overall look more alive when there are multiple on one ship. This can
//   be disabled!
// --------------------------------------------------------------------------------
void EveTurretSet::CalcRandomDelay()
{
	// enabled?
	if( m_useRandomFiringDelay )
	{
		m_randomFiringDelay = GetShotTimeVariance() * TriFloatRandom01();
	}
	else
	{
		m_randomFiringDelay = 0.f;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Calculate which turret out of this set is the closest one to the
//   m_targetPosition and (if target has them) which damage locator
//   is closest by.
// SeeAlso:
//   ITriTargetable
// Arguments:
//   closestTurretIx - reference to the turret index, which will get the result
//   closestLocatorIx - reference to the locator index, which will get the result
// Return value:
//   Returns true if calculation was successfull.
// --------------------------------------------------------------------------------
bool EveTurretSet::GetClosestTurretAndLocator( unsigned int& closestTurretIx, int& closestLocatorIx ) const
{
	// find the best single turret and damage locator pair
	float closestAngle = -1.f;
	unsigned int closestTurret = INVALID_TURRET_INDEX;
	int closestLocator = -1;

	// empty?
	if( m_singleTurrets.empty() )
	{
		return false;
	}

	// check all individual turrets find closest!
	for( unsigned int i = 0; i < m_singleTurrets.size(); i++ )
	{
		if( m_singleTurrets[i].valid )
		{
			Vector3 source = m_singleTurrets[i].worldMatrix.GetTranslation();
			Vector3 position = source;
			// get position of closest damagelocator
			int locatorIx = m_target->FindClosestLocator( &source, &position );

			// find normal from turret to target
			Vector3 nrmToTarget = Normalize( position - source );
			// find "up" normal of turret
			Vector3 nrmUp = TransformNormal( Vector3( 0.f, 1.f, 0.f ), m_singleTurrets[i].worldMatrix );
			float angle = Dot( nrmToTarget, nrmUp );
			if( angle > closestAngle )
			{
				closestTurret = i;
				closestLocator = locatorIx;
				closestAngle = angle;
			}
		}
	}

	if( closestTurret != INVALID_TURRET_INDEX && m_chooseRandomLocator )
	{
		Vector3 locatorPosition;
		int randomLocator = m_target->FindRandomValidLocator( m_singleTurrets[closestTurret].worldMatrix.GetTranslation(), locatorPosition );
		if( randomLocator != closestLocator && randomLocator != -1 )
		{
			closestLocator = randomLocator;
			closestAngle = -1.f;

			// we may need to adjust the turret index now
			for( size_t i = 0; i < m_singleTurrets.size(); i++ )
			{
				if( m_singleTurrets[i].valid )
				{
					Vector3 source = m_singleTurrets[i].worldMatrix.GetTranslation();

					// find normal from turret to target
					Vector3 nrmToTarget = Normalize( locatorPosition - source );
					// find "up" normal of turret
					Vector3 nrmUp = TransformNormal( Vector3( 0.f, 1.f, 0.f ), m_singleTurrets[i].worldMatrix );
					float angle = Dot( nrmToTarget, nrmUp );
					if( angle > closestAngle )
					{
						closestTurret = unsigned( i );
						closestAngle = angle;
					}
				}
			}
		}
	}

	// "failure is NOT an option" !
	if( closestTurret == INVALID_TURRET_INDEX )
	{
		closestTurret = 0;
	}

	// no error
	closestTurretIx = closestTurret;
	closestLocatorIx = closestLocator;
	return true;
}

void EveTurretSet::UpdateRtMesh()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.GetCaps().SupportsRaytracing() )
	{
		return;
	}

	if( !m_visibleCount )
	{
		return;
	}
	if( !m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( !m_geometryResource || !m_geometryResource->IsGood() || !m_turretEffect )
	{
		return;
	}

	for( auto& turret : m_singleTurrets )
	{
		if( !turret.rtMesh )
		{
			turret.rtMesh = std::make_unique<Tr2RaytracingMesh>();
		}
		turret.rtMesh->UpdateRtMesh( m_geometryResource, 0, m_estimatedPixelDiameter );
		if( !turret.rtMeshArea )
		{
			turret.rtMeshArea = std::make_unique<Tr2RaytracingMeshArea>( 0 );
		}
	}
}

void EveTurretSet::UpdateRtSkeleton()
{
	// if we don't have a geometry or a shader, animation is useless and probably unwanted
	if( !m_geometryResource || !m_geometryResource->IsGood() || !m_turretEffect )
	{
		return;
	}


	if( IsUsingCMF( m_geometryResource ) )
	{
		// fill with data
		if( !m_singleTurrets.empty() )
		{
			unsigned int defaultBonesPerTurret = 3;
			// to-bone-index-mapping for the shader (is the same for all turrets of the set)
			unsigned int boneCount = m_skeletonBoneIndices.size() ? (unsigned int)m_skeletonBoneIndices.size() : defaultBonesPerTurret;

			std::vector<Float4x3> boneTransformList;

			boneTransformList.reserve( boneCount );

			// put all single turret's positions and rotations in the array
			for( const auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					// get animation matrices here, they are not the same for all turrets of the set
					auto& worldTransforms = turret.worldTransforms;

					// Construct all turret bone translations and rotations
					for( unsigned int j = 0; j < boneCount; ++j )
					{
						Matrix bone;
						if( turret.valid && m_skeletonBoneIndices.size() && worldTransforms.size() )
						{
							int32_t skelBoneIdx = m_skeletonBoneIndices[j];
							if( skelBoneIdx >= 0 )
							{
								CCP_ASSERT( skelBoneIdx < m_skeleton->invBindTransforms.size() );
								CCP_ASSERT( skelBoneIdx < worldTransforms.size() );
								bone = m_skeleton->invBindTransforms[skelBoneIdx] * worldTransforms[skelBoneIdx];
							}
							else
							{
								bone = IdentityMatrix();
							}
						}
						else
						{
							bone = IdentityMatrix();
						}

						boneTransformList.push_back( Float4x3( bone ) );
					}
				}
			}

			if( boneTransformList.size() == 0 )
			{
				return;
			}

			m_boneOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Float4x3>(), boneTransformList.data(), uint32_t( boneTransformList.size() ) );
			uint32_t offset = 0;

			for( auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					if( !turret.rtMesh )
					{
						turret.rtMesh = std::make_unique<Tr2RaytracingMesh>();
					}
					if( !turret.rtMeshArea )
					{
						turret.rtMeshArea = std::make_unique<Tr2RaytracingMeshArea>( 0 );
					}
					if( turret.rtMesh->SetBoneTransforms( boneCount, reinterpret_cast<const Float4x3*>( boneTransformList.data() + offset ), m_boneOffsets.GetCurrentFrameOffset() + offset ) )
					{
						turret.rtMeshArea->MarkBlasOutdated();
					}
					offset += boneCount;
				}
			}
		}
	}
#if WITH_GRANNY
	else
	{
		// fill with data
		if( !m_singleTurrets.empty() )
		{
			unsigned int defaultBonesPerTurret = 3;
			// to-bone-index-mapping for the shader (is the same for all turrets of the set)
			const int* toBoneIndices = m_grnMeshBinding ? GrannyGetMeshBindingToBoneIndices( m_grnMeshBinding ) : NULL;
			unsigned int boneCount = m_grnMeshBinding ? GrannyGetMeshBindingBoneCount( m_grnMeshBinding ) : defaultBonesPerTurret;

			std::vector<Float4x3> boneTransformList;

			boneTransformList.reserve( boneCount );

			// put all single turret's positions and rotations in the array
			for( const auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					// get animation matrices here, they are not the same for all turrets of the set
					const Matrix* compositeMatrixArray = turret.grnWorldPose ?
						reinterpret_cast<const Matrix*>( GrannyGetWorldPoseComposite4x4Array( turret.grnWorldPose ) ) :
						nullptr;

					// Construct all turret bone translations and rotations
					for( unsigned int j = 0; j < boneCount; ++j )
					{
						Matrix bone;
						if( turret.valid && toBoneIndices && compositeMatrixArray )
						{
							bone = compositeMatrixArray[toBoneIndices[j]];
						}
						else
						{
							bone = IdentityMatrix();
						}

						boneTransformList.push_back( Float4x3( bone ) );
					}
				}
			}

			if( boneTransformList.size() == 0 )
			{
				return;
			}

			m_boneOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Float4x3>(), boneTransformList.data(), uint32_t( boneTransformList.size() ) );
			uint32_t offset = 0;

			for( auto& turret : m_singleTurrets )
			{
				if( turret.visible )
				{
					if( !turret.rtMesh )
					{
						turret.rtMesh = std::make_unique<Tr2RaytracingMesh>();
					}
					if( !turret.rtMeshArea )
					{
						turret.rtMeshArea = std::make_unique<Tr2RaytracingMeshArea>( 0 );
					}
					if( turret.rtMesh->SetBoneTransforms( boneCount, reinterpret_cast<const Float4x3*>( boneTransformList.data() + offset ), m_boneOffsets.GetCurrentFrameOffset() + offset ) )
					{
						turret.rtMeshArea->MarkBlasOutdated();
					}
					offset += boneCount;
				}
			}
		}
	}
#endif
}

EveTurretFiringFX* EveTurretSet::GetFiringEffect()
{
	return m_firingEffect;
}

void EveTurretSet::SetFiringEffect( EveTurretFiringFX* firingEffect )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
	{
		entity->UnRegister( registry );
	}
	m_firingEffect = firingEffect;
	if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
	{
		entity->Register( registry );
	}
	InitializeFiringEffect();
}

IEveSpaceObjectChild* EveTurretSet::GetAmbientEffect()
{
	return m_ambientEffect;
}

void EveTurretSet::SetAmbientEffect( IEveSpaceObjectChild* ambientEffect )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_ambientEffect ) )
	{
		entity->UnRegister( registry );
	}
	m_ambientEffect = ambientEffect;
	InitializeAmbientEffect();
}


// --------------------------------------------------------------------------------
// Description:
//   Make the shader accessable to the outside (const version)
// --------------------------------------------------------------------------------
const Tr2Effect* EveTurretSet::GetShader() const
{
	return m_turretEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Make the shader accessable to the outside (non-const version)
// --------------------------------------------------------------------------------
Tr2Effect* EveTurretSet::GetShader()
{
	return m_turretEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a hit/miss to the shot queue.
// --------------------------------------------------------------------------------
void EveTurretSet::SetShotMissed( const bool missed )
{
	m_target->SetShotMissed( missed );
}

// --------------------------------------------------------------------------------
// Description:
//   Return the size of the miss queue
// --------------------------------------------------------------------------------
size_t EveTurretSet::MissQueueSize() const
{
	return m_target->MissQueueSize();
}

// --------------------------------------------------------------------------------
// Description:
//   Return the time of the last shot.
// --------------------------------------------------------------------------------
double EveTurretSet::GetLastShotTime() const
{
	return m_target->GetLastShotTime();
}

// --------------------------------------------------------------------------------
// Description:
//   Sets the scale of the firing effect using the target's radius.
// --------------------------------------------------------------------------------
void EveTurretSet::SetTargetScale()
{
	if( m_firingEffect )
	{
		float radius = m_target->GetRadius();
		m_firingEffect->SetScaleByRadius( radius );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Sets the target object. This is part of the targetObject property.
// --------------------------------------------------------------------------------
void EveTurretSet::SetTargetObject( IRoot* target )
{
	if( !target )
	{
		return;
	}
	ITriTargetablePtr oldTargetPtr = m_target->GetTargetable();

	// attach to target
	m_target->SetTargetable( target );

	if( m_playMovementSound && !m_idleToTargetingMovementAudioEvent.empty() )
	{
		// Always trigger movement sounds if coming from IDLE state, otherwise trigger it only if you're targeting a new object.
		if( m_state == STATE_IDLE || !oldTargetPtr.IsEqualObject( m_target->GetTargetable() ) )
		{
			SendEventToAudEmitter( m_turretMovementObserver, m_idleToTargetingMovementAudioEvent );
		}
	}

	// update the firing effect we have one
	SetTargetScale();
}

// --------------------------------------------------------------------------------
// Description:
//   Sets the target object. This is part of the targetObject property.
// --------------------------------------------------------------------------------
ITriTargetablePtr EveTurretSet::GetTargetObject()
{
	return m_target->GetTargetable();
}

// --------------------------------------------------------------------------------
// Description:
//   Calculates the transform for a pitch bone
// --------------------------------------------------------------------------------
void EveTurretSet::CalcTransformForPitchBone( const Vector3* target, float minPitch, float maxPitch, unsigned int boneIndex, const Matrix* localTransform, Quaternion& rotation ) const
{
	float pitchOffset = GetBonePitchOffset( boneIndex );
	float pitchFactor = GetBonePitchFactor( boneIndex );
	// pitch of barrel 90 degrees
	Vector3 bone_position( 0.f, 0.f, 0.f );

	if( localTransform )
	{
		bone_position = localTransform->GetTranslation();
	}

	Vector3 relTarget = *target - bone_position;
	Vector3 dirNrm = Normalize( relTarget );
	float radians = asinf( dirNrm.y );

	if( localTransform )
	{
		Vector3 bone_direction = Normalize( bone_position );
		float d = Dot( bone_direction, *target );
		if( d < Length( bone_position ) )
		{
			// Assuming up is enough for now to avoid cross products
			radians = TriFloatSign( relTarget.y ) * XM_PI - radians;
		}
	}

	float alpha = TriClamp( radians, minPitch, maxPitch );
	// modify!
	alpha = pitchFactor * alpha + XMConvertToRadians( pitchOffset );
	// never forget do apply influence!
	alpha *= m_trackingInfluence;
	// 1st: make quaternion
	Quaternion quat = RotationQuaternion( 0.f, -alpha, 0.f );
	// 2nd: apply this quat after the original one
	quat = rotation * quat;
	// 2nd: make granny_transform from quat
	rotation = quat;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the correct bone pitch factor based on the bone index
//   If the bone index does not have a specific bone pitch factor, a default 1.0 is returned
// --------------------------------------------------------------------------------
float EveTurretSet::GetBonePitchFactor( unsigned int boneIndex ) const
{
	switch( boneIndex )
	{
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2:
		return m_sysBonePitchFactor;
	case SYSBONE_SCALED_PITCH01:
		return m_sysBonePitch01Factor;
	case SYSBONE_SCALED_PITCH02:
		return m_sysBonePitch02Factor;
	case SYSBONE_SCALED_PITCH03:
		return m_sysBonePitch03Factor;
	default:
		return 1.0f;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the correct bone pitch offset based on the bone index
//   If the bone index does not have a specific bone pitch offset, a default 0.0 is returned
// --------------------------------------------------------------------------------
float EveTurretSet::GetBonePitchOffset( unsigned int boneIndex ) const
{
	switch( boneIndex )
	{
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2:
		return m_sysBonePitchOffset;
	case SYSBONE_SCALED_PITCH01:
		return m_sysBonePitch01Offset;
	case SYSBONE_SCALED_PITCH02:
		return m_sysBonePitch02Offset;
	case SYSBONE_SCALED_PITCH03:
		return m_sysBonePitch03Offset;
	default:
		return 0.0f;
	}
}

// --------------------------------------------------------------------------------
void EveTurretSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( nullptr != m_firingEffect )
	{
		m_firingEffect->RegisterWithQuadRenderer( quadRenderer );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
void EveTurretSet::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_display )
	{
		return;
	}

	if( nullptr != m_firingEffect )
	{
		m_firingEffect->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect && IsAmbientVisible() )
	{
		ambientEffect->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}


void EveTurretSet::SetControllerVariable( const char* name, float value )
{
	if( m_firingEffect )
	{
		m_firingEffect->SetControllerVariable( name, value );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetControllerVariable( name, value );
	}
}

void EveTurretSet::HandleControllerEvent( const char* name )
{
	if( m_firingEffect )
	{
		m_firingEffect->HandleControllerEvent( name );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->HandleControllerEvent( name );
	}
}

void EveTurretSet::StartControllers()
{
	if( m_firingEffect )
	{
		m_firingEffect->StartControllers();
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->StartControllers();
	}
}

void EveTurretSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_turretEffect )
	{
		m_turretEffect->SetOption( name, value );
	}

	auto ambientEffect = GetAmbientEffectOrGeneratedEffect();
	if( ambientEffect )
	{
		ambientEffect->SetShaderOption( name, value );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the data to HW
// --------------------------------------------------------------------------------
void EveTurretSetPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_vsData, sizeof( EveTurretSetVSData ), VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );

	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_psData, sizeof( EveTurretSetPSData ), PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}
void EveTurretSetPerObjectData::ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const
{
	writer.SetPerObjectData( VERTEX_SHADER, &m_vsData, sizeof( EveTurretSetVSData ) );
	writer.SetPerObjectData( PIXEL_SHADER, &m_psData, sizeof( EveTurretSetPSData ) );
}
