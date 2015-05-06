////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveTurretSet.h"

#include "Tr2Effect.h"
#include "Resources/TriGeometryRes.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustumOrtho.h"
#include "TriRenderBatch.h"

#include "EveTurretFiringFX.h"
#include "include/TriMath.h"
#include "Utilities/ViewDistanceInfo.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

// names of system bones like they are in the granny file
static std::string s_systemBoneSkeletonNames[] = {
	"invalid",				// SYSBONE_INVALID
	"Sys_Rotation_Arm",		// SYSBONE_ROTATION
	"Sys_CounterRotation",	// SYSBONE_COUNTER_ROTATION
	"Sys_Pitch_Barrel",		// SYSBONE_PITCH
	"Sys_Pitch_Barrel1",	// SYSBONE_PITCH1
	"Sys_Height",			// SYSBONE_SCALED_HEIGHT
	"Sys_Pitch_Arm01",		// SYSBONE_SCALED_PITCH01
	"Sys_Pitch_Arm02",		// SYSBONE_SCALED_PITCH02
};

// names of position bones like they are in the granny file
static std::string s_positionBoneSkeletonNames[] = {
	"Pos_Fire01",
	"Pos_Fire02",
	"Pos_Fire03",
	"Pos_Fire04",
	"Pos_Fire05",
	"Pos_Fire06",
	"Pos_Fire07",
	"Pos_Fire08",
};

// invalids
const unsigned int INVALID_BONE_INDEX = 0xffffffff;
const unsigned int INVALID_TURRET_INDEX = 0xffffffff;

// shadow for turrets shader
static const char* SHADOW_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/Turret/Shadow.fx";

// use options visibility threshold when to turn off turret rendering, NOT the firingeffect
extern float g_eveSpaceSceneVisibilityThreshold;

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
	m_visibleCount( 0 ),
	m_estimatedPixelDiameter( -1.f ),
	m_lodLevel( LOD_INVALID ),
	m_targetLocator( -1 ),
	m_targetPosition( 0.f, 0.f, 0.f ),
	m_targetPositionMiss( 0.f, 0.f, 0.f ),
	m_trackingInfluence( 0.f ),
	m_targetPositionOldInfluence( -1.f ),
	m_targetPositionOld( 0.f, 0.f, 0.f ),
	m_trackingInfluenceDelta( 0.f ),
	m_delayToFadeOutTracking( 0.f ),
	m_delayToFadeInTracking( 0.f ),
	m_trackingFadeTime( 1.f ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_bottomClipHeight( 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_turretVertexDeclElementCount( 0 ),
	m_grnModel( NULL ),
	m_grnMeshBinding( NULL ),
	m_useRandomFiringDelay( true ),
	m_randomFiringDelay( 0.f ),
	m_alternateFiringAnimCount( 0 ),
	m_hasCyclingFiringPos( false ),
	m_currentCyclingFiresPos( 0 ),
	m_sysBoneHeight( 1.f ),
	m_sysBonePitchOffset( 0.f ),
	m_sysBonePitchFactor( 1.f ),
	m_sysBonePitchMin( 0.f ),
	m_sysBonePitch01Offset( 0.f ),
	m_sysBonePitch01Factor( 1.f ),
	m_sysBonePitch02Offset( 0.f ),
	m_sysBonePitch02Factor( 1.f ),
	m_state( STATE_IDLE ),
	m_activeTurret( INVALID_TURRET_INDEX ),
	m_recheckTimeLeft( -1.f ),
	m_missQueue( "EveTurretSet::m_missQueue" ),
	m_lastShotAccuracy( ACCURACY_INDETERMINATE ),
	m_lastShotTime( 0.0 ),
	m_laserMissBehaviour( false ),
	m_projectileMissBehaviour( false ),
	m_readyToFireEffect( false ),
	m_randomMissDistanceOffset( 0.5f ),
	m_randomMissPositionOffset( 0, 0, 0 ),
	m_trackMissPoint( false ),
	m_firingEffectMuzzlePosSet( false ),
	m_slotNumber( -1 ),
	m_parentShLighting( nullptr ),
	m_possibleTurretDisplayAmount( 0 )
{
	// 0
	memset( &m_parentData, 0, sizeof( ParentData ) );
	D3DXMatrixIdentity( &m_parentData.transform );
	for( unsigned int i = 0; i < SYSBONE_MAX; ++i )
	{
		m_systemBoneID[i] = INVALID_BONE_INDEX;
	}

	// create shaders for shadow-map rendering
	m_shadowEffect.CreateInstance();
	m_shadowEffect->SetEffectPathName( SHADOW_EFFECT_PATH );

	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveTurretSet::~EveTurretSet()
{
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
	if( IsMatch( value, m_geomResPath ) )
	{
		// new gr2 file specified -> reload!
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_firingEffect ) )
	{
		// attached firing effect has changed -> relink!
		InitializeFiringEffect();
	}
	return true;
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
	CR_RETURN( m_instanceBuffer.Create(		EVE_MAX_TURRETS_PER_SET * sizeof( float ), 
											USAGE_IMMUTABLE, 
											pBufferData, 
											renderContext ) );
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
					// in case we don't find positional bone, ::FindJoint() returns 0xffffffff
					m_firingEffect->SetMuzzleBoneID( i, skeletonData->FindJoint( s_positionBoneSkeletonNames[i].c_str() ) );
				}
			}
		}
	}
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

	// amount of renderable turrets is now invalid
	m_possibleTurretDisplayAmount = 0;

	// system bone ids are no longer valid
	for( unsigned int i = 0; i < SYSBONE_MAX; ++i )
	{
		m_systemBoneID[i] = INVALID_BONE_INDEX;
	}
	// granny invalid
	m_grnModel = NULL;
	// granny
	if( m_grnMeshBinding )
	{
		GrannyFreeMeshBinding( m_grnMeshBinding );
		m_grnMeshBinding = NULL;
	}
	// free and null granny data per turret
	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// free the parts that were alocted
		if( it->grnModelInstance )
		{
			// free all animation controls of this instance
			for( granny_model_control_binding* b = GrannyModelControlsBegin( it->grnModelInstance ); b != GrannyModelControlsEnd( it->grnModelInstance ); )
			{
				granny_control* control = GrannyGetControlFromBinding( b );
				b = GrannyModelControlsNext( b );
				GrannyFreeControl( control );
			}
			GrannyFreeModelInstance( it->grnModelInstance );
			it->grnModelInstance = NULL;
		}
		if( it->grnLocalPose )
		{
			GrannyFreeLocalPose( it->grnLocalPose );
			it->grnLocalPose = NULL;
		}
		if( it->grnWorldPose )
		{
			GrannyFreeWorldPose( it->grnWorldPose );
			it->grnWorldPose = NULL;
		}
		// zero/invalidate other data
		it->grnSkeleton = NULL;
	}
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
				if( meshData->m_vertexDeclaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
				{
					// get gemoetry's original vertex-decl...
					Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, m_turretVertexDecl );
					// ...expand it with instances stream elements...
					auto& item = m_turretVertexDecl.Add( m_turretVertexDecl.FLOAT32_1, m_turretVertexDecl.TEXCOORD, 1, 1, 1 );
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
							
							unsigned int boneCount = GrannyGetMeshBindingBoneCount( m_grnMeshBinding );
							m_possibleTurretDisplayAmount = EVE_MAX_TURRETS_PER_SET;
							if( boneCount != 0 )
							{
								m_possibleTurretDisplayAmount = EVE_MAX_TURRET_SET_BONES / boneCount;
							}							 

							// create animations for all turrets
							for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
							{
								if( it->grnModelInstance == NULL )
								{
									it->grnModelInstance = GrannyInstantiateModel( m_grnModel );
									it->grnSkeleton = GrannyGetSourceSkeleton( it->grnModelInstance );
									it->grnLocalPose = GrannyNewLocalPose( it->grnSkeleton->BoneCount );
									it->grnWorldPose = GrannyNewWorldPose( it->grnSkeleton->BoneCount );
								}
							}

							// remove the turrets that are not able to be displayed
							if( m_possibleTurretDisplayAmount > 0 && m_singleTurrets.size() > m_possibleTurretDisplayAmount )
							{
								CCP_LOGWARN( "Turretset '%s' has more turrets (%d) than the shader can handle (%d) due to the amount of bones for the model (%d)", 
									grannyFileInfo->FromFileName,
									m_singleTurrets.size(), 
									m_possibleTurretDisplayAmount,  
									EVE_MAX_TURRET_SET_BONES/m_possibleTurretDisplayAmount
								);

								m_singleTurrets.resize( m_possibleTurretDisplayAmount );
							}
						}
					}
				}
			}
		}

		// animation already requested in a queue?
		if( !m_animationQueue.empty() )
		{
			std::vector<AnimationRequest> queueSnapshot = m_animationQueue;
			for( std::vector<AnimationRequest>::const_iterator it = queueSnapshot.begin(); it != queueSnapshot.end(); ++it )
			{
				PlayAnimation( it->turretIndex, it->animName, it->animNameIdle, 0.f );
			}
			m_animationQueue.clear();
		}
		else
		{
			// force an anim based on a state
			ForceIdleAnimation();
		}
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
	// CComPtr, this is safe!
	m_instanceBuffer.Destroy();
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
bool EveTurretSet::UpdateLOD()
{
	// is LODing enabled?
	if( m_lodLevel == LOD_DISABLED )
	{
		return false;
	}
	// if the pixeldiameter is negative that means it is not a reliable value, so
	// don't do lod calculations with it
	if( m_estimatedPixelDiameter < 0.f )
	{
		return false;
	}

	LOD oldLOD = m_lodLevel;
	if( m_estimatedPixelDiameter < 2.f * g_eveSpaceSceneVisibilityThreshold )
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

void EveTurretSet::UpdateSyncronous( float deltaT, Be::Time time, const Matrix* parentMatrix )
{
	if( !m_singleTurrets.size() )
	{
		return;
	}

	// LODing
	if( UpdateLOD() )
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

	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// mesh animation
		if( it->grnModelInstance )
		{
			GrannySetModelClock( it->grnModelInstance, Tr2Renderer::GetAnimationTime() );
			GrannyFreeCompletedModelControls( it->grnModelInstance );
		}
	}

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
							if( ( currentClosestTurret != m_activeTurret ) || ( currentClosestLocator != m_targetLocator ) )
							{
								// re-enter firing state, should take care of everything
								EnterStateFiring();
							}
						}
						// recheck every 2 seconds
						m_recheckTimeLeft = 2.f;
					}
				}
			}
		}
	}
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
//   time - time delta since last frame
//   parentMatrix - transform of parent object (usually a ship)
// --------------------------------------------------------------------------------
void EveTurretSet::UpdateAsyncronous( float deltaT, Be::Time time, const ParentData* parentData )
{
	if( !m_singleTurrets.size() )
	{
		return;
	}

	// keep parent's transform
	m_parentData = *parentData;

	// update single turret list
	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// first parent matrix (ship or station), then local matrix (locator position)
		D3DXMatrixMultiply( &it->worldMatrix, &it->localMatrix, &m_parentData.transform );
		// we need the inverse matrix for the tracking later
		D3DXMatrixInverse( &it->invWorldMatrix, NULL, &it->worldMatrix );
		// this validates this turret
		it->valid = true;
	}

	// handle fading of turret tracking
	if( m_trackingInfluenceDelta != 0.f )
	{
		m_trackingInfluence += m_trackingInfluenceDelta * deltaT;
		if( m_trackingInfluence > 1.f )
		{
			m_trackingInfluence = 1.f;
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
			m_trackingInfluenceDelta = -1.f / m_trackingFadeTime;
		}
	}

	if( m_delayToFadeInTracking > 0.f )
	{
		m_delayToFadeInTracking -= deltaT;
		if( m_delayToFadeInTracking <= 0.f )
		{
			m_delayToFadeInTracking = 0.f;
			m_trackingInfluenceDelta = 1.f / m_trackingFadeTime;
		}
	}

	// update animation of each singel turret
	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// mesh animation
		if( it->grnModelInstance )
		{
			GrannySampleModelAnimations( it->grnModelInstance, 0, it->grnSkeleton->BoneCount, it->grnLocalPose );

			if( it->valid )
			{
				if( m_trackingInfluence != 0.f )
				{
					// transform traget pos (which is in world space) into objectspace
					Vector3 targetPosOS;
					D3DXVec3TransformCoord( &targetPosOS, &m_targetPosition, &it->invWorldMatrix );

					// "do" all the system bones, we have found
					for( unsigned int bone = 0; bone < SYSBONE_MAX; ++bone )
					{
						if( m_systemBoneID[bone] != INVALID_BONE_INDEX )
						{
							// modify this bone's transform data
							granny_transform* boneTransform = GrannyGetLocalPoseTransform( it->grnLocalPose, m_systemBoneID[bone] );
							if( boneTransform )
							{
								ModifySystemBoneTransform( (SystemBones)bone, &targetPosOS, boneTransform );
							}
						}
					}
				}
			}

			GrannyBuildWorldPose( it->grnSkeleton, 0, it->grnSkeleton->BoneCount, it->grnLocalPose, &Tr2Renderer::GetIdentityTransform().m[0][0], it->grnWorldPose );
		}
	}

	// update the target locator position, but do it smoothly by using another locator (if set!)
	// to lerp between their positions
	if( m_targetObject )
	{
		m_targetObject->GetDamageLocatorPosition( &m_targetPosition, m_targetLocator );

		if( !m_trackMissPoint )
		{
			m_targetPositionMiss = m_targetPosition;
		}

		if( m_targetPositionOldInfluence > 0.f )
		{
			// lerp the old position "in"
			D3DXVec3Lerp( &m_targetPosition, &m_targetPosition, &m_targetPositionOld, m_targetPositionOldInfluence );
			// fadeout the influence
			m_targetPositionOldInfluence -= deltaT / m_trackingFadeTime;
		}

		if( m_laserMissBehaviour )
		{
			UpdateMissPosition( &m_parentData.transform );
		}
	}

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
		
		// is targeted! so did we miss or hit the target?
		SetEffectEndPoint();

		// time update (return value tells us if effect is ready to fire!)
		if( m_firingEffect->Update( time, deltaT ) )
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


			if( !m_laserMissBehaviour )
			{
				UpdateMissPosition( &m_parentData.transform );
			}
			PopShotMissed();
			SetEffectEndPoint();
			m_firingEffect->SetDisplayDestObject( !GetShotMissed() || m_projectileMissBehaviour );

			// pop miss state
			m_randomMissDistanceOffset = TriFloatRandom01();
			const float u = TriFloatRandom01(), v = TriFloatRandom01();
			const float phi = u * 3.14159f * 2.f;
			const float theta = acosf( 1.f - sqrtf(v) ) * 2.f;
			TriVectorSpherical( &m_randomMissPositionOffset, phi, theta, 3.f );
		}
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

	// this is a problem...
	if( closestTurret == INVALID_TURRET_INDEX )
	{ 
		return Tr2Renderer::GetIdentityTransform();
	}

	// source comes from position bones or, if we don't have any, from center of turret
	Matrix m = m_singleTurrets[closestTurret].worldMatrix;

	// get the boneID for that muzzle from firing effect
	if( !m_firingEffect )
	{
		return m;
	}
	unsigned int boneID = m_firingEffect->GetPerMuzzleBoneID( muzzle );
	// valid firing positionbone?
	if( boneID != INVALID_BONE_INDEX )
	{
		// valid granny pose? (bone positions are stored "in" that thing)
		if( m_singleTurrets[closestTurret].grnWorldPose )
		{
			// granny tells us firing-bone position in turret-space
			granny_real32* boneTransform = GrannyGetWorldPose4x4( m_singleTurrets[closestTurret].grnWorldPose, boneID );
			// create this pos in worldspace
			D3DXMatrixMultiply( &m, (const Matrix*)boneTransform, &m );
		}
		else
		{
			// if we don't have a valid granny pose, position is fine but orientation? depends on launcher settings...
			if( m_sysBonePitchMin < 45.f )
			{
				// aiming directly at target, because target cone is large
				Vector3 nrmToTarget = m_targetPosition - m.GetTranslation();
				// get direct rotation quaternion
				Quaternion directRotationQuaternion;
				static const Vector3 zAxis( 0.f, 0.f, 1.f );
				TriQuaternionRotationArc( &directRotationQuaternion, &zAxis, &nrmToTarget );
				// now combine the translation from the worldmatrix and the rotation from the direct targeting
				Vector3 tr = m.GetTranslation();
				D3DXMatrixAffineTransformation( &m, 1.f, NULL, &directRotationQuaternion, &tr );
			}
			else
			{
				// eject straight out of ship, because target cone is very small
				// attn: locator up is +Y, but missile eject is in +Z! so rotate around X
				Matrix y2xMatrix;
				D3DXMatrixRotationX( &y2xMatrix, -0.5f * XM_PI );
				// apply this rotation to "before" world transform
				D3DXMatrixMultiply( &m, &y2xMatrix, &m );
			}
		}
	}

	return m;
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
//   transform - the granny bone transform that needs to get modified
// --------------------------------------------------------------------------------
void EveTurretSet::ModifySystemBoneTransform( SystemBones bone, const Vector3* target, granny_transform* transform ) const
{
	switch( bone )
	{
	case SYSBONE_INVALID:
		break;
	case SYSBONE_ROTATION:
		if( transform )
		{
			// rotation of turret 360 degress, alpha is between -pi and pi
			float alpha = atan2( target->x, target->z );
			// never forget do apply influence!
			alpha *= m_trackingInfluence;
			// 1st: make quaternion
			Quaternion quat;
			D3DXQuaternionRotationYawPitchRoll( &quat, alpha, 0.f, 0.f );
			// 2nd: apply this quat after the original one
			D3DXQuaternionMultiply( &quat, (Quaternion*)&transform->Orientation, &quat );
			// 3rd: make granny_transform from quat
			GrannySetTransform( transform, transform->Position, (float*)&quat, (float*)transform->ScaleShear );
		}
		break;
	case SYSBONE_COUNTER_ROTATION:
		if( transform )
		{
			// inverse(!!) rotation of turret 360 degress, alpha is between -pi and pi
			float alpha = -1.f * atan2( target->x, target->z );
			// never forget do apply influence!
			alpha *= m_trackingInfluence;
			// 1st: make quaternion
			Quaternion quat;
			D3DXQuaternionRotationYawPitchRoll( &quat, alpha, 0.f, 0.f );
			// 2nd: apply this quat after the original one
			D3DXQuaternionMultiply( &quat, (Quaternion*)&transform->Orientation, &quat );
			// 3rd: make granny_transform from quat
			GrannySetTransform( transform, transform->Position, (float*)&quat, (float*)transform->ScaleShear );
		}
		break;
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
		if( transform )
		{
			// pitch of barrel 90 degrees
			Vector3 dirNrm;
			D3DXVec3Normalize( &dirNrm, target );
			float alpha = Clamp( asinf( dirNrm.y ), XMConvertToRadians( m_sysBonePitchMin ), XM_PI / 2.f );
			// modify!
			alpha = m_sysBonePitchFactor * alpha + XMConvertToRadians( m_sysBonePitchOffset );
			// never forget do apply influence!
			alpha *= m_trackingInfluence;
			// 1st: make quaternion
			Quaternion quat;
			D3DXQuaternionRotationYawPitchRoll( &quat, 0.f, -alpha, 0.f );
			// 2nd: apply this quat after the original one
			D3DXQuaternionMultiply( &quat, (Quaternion*)&transform->Orientation, &quat );
			// 2nd: make granny_transform from quat
			GrannySetTransform( transform, transform->Position, (float*)&quat, (float*)transform->ScaleShear );
		}
		break;
	case SYSBONE_SCALED_HEIGHT:
		if( transform )
		{
			// pitch of barrel 90 degrees
			Vector3 dirNrm;
			D3DXVec3Normalize( &dirNrm, target );
			float height = Clamp( dirNrm.y, 0.f, 1.f );
			// never forget do apply influence!
			height *= m_trackingInfluence;
			// it's a pos extension with a scale
			Vector3 pos = Vector3( 0.f, height * m_sysBoneHeight, 0.f ) + Vector3( transform->Position[0], transform->Position[1], transform->Position[2] );
			GrannySetTransform( transform, pos, transform->Orientation, (float*)transform->ScaleShear );
		}
		break;
	case SYSBONE_SCALED_PITCH01:
		if( transform )
		{
			// pitch of barrel 90 degrees
			Vector3 dirNrm;
			D3DXVec3Normalize( &dirNrm, target );
			float alpha = Clamp( asinf( dirNrm.y ), 0.f, XM_PI / 2.f );
			// modify!
			alpha = m_sysBonePitch01Factor * alpha + XMConvertToRadians( m_sysBonePitch01Offset );
			// never forget do apply influence!
			alpha *= m_trackingInfluence;
			// 1st: make quaternion
			Quaternion quat;
			D3DXQuaternionRotationYawPitchRoll( &quat, 0.f, -alpha, 0.f );
			// 2nd: apply this quat after the original one
			D3DXQuaternionMultiply( &quat, (Quaternion*)&transform->Orientation, &quat );
			// 2nd: make granny_transform from quat
			GrannySetTransform( transform, transform->Position, (float*)&quat, (float*)transform->ScaleShear );
		}
		break;
	case SYSBONE_SCALED_PITCH02:
		if( transform )
		{
			// pitch of barrel 90 degrees
			Vector3 dirNrm;
			D3DXVec3Normalize( &dirNrm, target );
			float alpha = Clamp( asinf( dirNrm.y ), 0.f, XM_PI / 2.f );
			// modify!
			alpha = m_sysBonePitch02Factor * alpha + XMConvertToRadians( m_sysBonePitch02Offset );
			// never forget do apply influence!
			alpha *= m_trackingInfluence;
			// 1st: make quaternion
			Quaternion quat;
			D3DXQuaternionRotationYawPitchRoll( &quat, 0.f, -alpha, 0.f );
			// 2nd: apply this quat after the original one
			D3DXQuaternionMultiply( &quat, (Quaternion*)&transform->Orientation, &quat );
			// 2nd: make granny_transform from quat
			GrannySetTransform( transform, transform->Position, (float*)&quat, (float*)transform->ScaleShear );
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
	if( turretIndex >= EVE_MAX_TURRETS_PER_SET || ( m_possibleTurretDisplayAmount != 0 && turretIndex >= m_possibleTurretDisplayAmount ))
	{
		return;
	}

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
			D3DXMatrixIdentity( &data.localMatrix );
			D3DXMatrixIdentity( &data.worldMatrix );
			D3DXMatrixIdentity( &data.invWorldMatrix );
			data.valid = false;
			data.visible = false;

			m_singleTurrets.push_back( data );

			PlayAnimation( i, "", "Active", 0.f );
		}
	}
	
	// remove scaling: this matrix comes from a locator which can have scaling, but we don't want it!
	TriMatrixRemoveScaling( &m_singleTurrets[turretIndex].localMatrix, localMatrix );

	Vector3 translation, scale;
	D3DXMatrixDecompose( &scale, &m_singleTurrets[turretIndex].localQuaternion, &translation, &m_singleTurrets[turretIndex].localMatrix );
	m_singleTurrets[turretIndex].localPosition = Vector4(translation, 1.0f);

	// new one is not yet valid, cause it needs to get all calculated
	m_singleTurrets[turretIndex].valid = false;
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
const int EveTurretSet::GetSlotNumber() const
{
	return m_slotNumber;
}

// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this turret set: bounding sphere
// --------------------------------------------------------------------------------
void EveTurretSet::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	// draw bounding sphere for every turret in this set
	for( std::vector<SingleTurretData>::const_iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		if( it->valid )
		{
			Vector3 center;
			D3DXVec3TransformCoord( &center, (Vector3*)&m_boundingSphere, &it->worldMatrix );
			Tr2Renderer::DrawSphere( center, m_boundingSphere.w, 10, 0xffffff00 );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Use a more specialized ::GetRenderables() function to get this renderable. We
//   need a special culling method for orthogonal frustum
//   And do not get the renderables from the fire effects for shadows!
// Arguments:
//   frustum - the current shadow frustum
//   renderables - a vector for all the renderable we want to render into shadowmap
// SeeAlso:
//   ITr2Renderable
// --------------------------------------------------------------------------------
void EveTurretSet::GetRenderablesCastingShadow( const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// check visibility for each turret
	bool renderForShadow = false;
	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// transform bounding sphere into world space to check against frustum
		Vector4 transformedBoundingSphere = m_boundingSphere;
		BoundingSphereTransform( it->worldMatrix, transformedBoundingSphere );
		bool insideLightFrustum = frustum.IsSphereVisibleAndInsideNearPlane( &transformedBoundingSphere );
		// if it is visible and needed for this shadowmap, render it
		renderForShadow |= ( it->visible && insideLightFrustum );
	}

	// add this object (which is a renderable), if it is needed
	if( renderForShadow )
	{
		if( m_turretEffect )
		{
			renderables.push_back( this );
		}
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
void EveTurretSet::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Vector4* shLighting )
{
	m_parentShLighting = nullptr;

	// display?
	if( !m_display )
	{
		return;
	}

	// check visibility for each single turret and keep MAX on-screen pixel diameter.
	// The pixel diameter is reset in UpdateLOD on next frame - this is to account for
	// multiple views. We need to keep updating turrets as long as they're visible in any
	// view, not just the last one rendered.
	m_visibleCount = 0;

	for( std::vector<SingleTurretData>::iterator it = m_singleTurrets.begin(); it != m_singleTurrets.end(); ++it )
	{
		// transform bounding sphere into world space to check against frustum
		Vector4 transformedBoundingSphere = m_boundingSphere;
		BoundingSphereTransform( it->worldMatrix, transformedBoundingSphere );
		it->visible = frustum.IsSphereVisible( &transformedBoundingSphere );
		// count visible turrets of this set, mainly for debug purposes
		if( it->visible )
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

	// add this object (which is a renderable), if it is visible
	if( m_visibleCount )
	{
		if( m_turretEffect )
		{
			m_parentShLighting = shLighting;
			renderables.push_back( this );
		}
	}

    // add firing effects, we should add some lodding for this
	if( m_displayEffects )
	{
		if( m_firingEffect )
		{
			m_firingEffect->GetRenderables( frustum, renderables );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info using this object's bounds. 
// --------------------------------------------------------------------------------
void EveTurretSet::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{ 
	if( !m_display )
	{
		return;
	}

	Vector4 v; 
	if( m_displayEffects )
	{
		if( m_firingEffect && m_firingEffect->GetBoundingSphere( v ) && frustum.IsSphereVisible( &v ) )
		{
			viewDistance.UpdateClipPlanes( v, frustum );
		}
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
							   const Tr2PerObjectData* perObjectData )
{
	if( batchType != TRIBATCHTYPE_OPAQUE )
	{
		return;
	}
	if( !m_display )
	{
		return;
	}
	if( !m_visibleCount )
	{
		return;
	}

	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_turretEffect );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get batches only for shadow generation
// --------------------------------------------------------------------------------
void EveTurretSet::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( !m_display )
	{
		return;
	}
	if( !m_visibleCount )
	{
		return;
	}

	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_shadowEffect );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
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

	// make an identity 3x4 matrix for use later
	Matrix idMatrix;
	D3DXMatrixIdentity( &idMatrix );

	Vector3 scale, translation;

	// tell "parent"-ship matrix
	D3DXMatrixTranspose( &perObjectData->m_shipMatrix, &m_parentData.transform );

	// put together clip-data, so we can have a clip plane to cut off the turrets base to avoid intersections
	perObjectData->m_baseCutoffData = Vector4( m_bottomClipHeight, 0.f, 0.f, 0.f );

	// fill with data
	if( !m_singleTurrets.empty() )
	{
		unsigned int maxBonesPerTurret = EVE_MAX_TURRET_SET_BONES / (unsigned int)m_singleTurrets.size();
		// to-bone-index-mapping for the shader (is the same for all turrets of the set)
		const int* toBoneIndices = m_grnMeshBinding ? GrannyGetMeshBindingToBoneIndices( m_grnMeshBinding ) : NULL;
		unsigned int boneCount = m_grnMeshBinding ? GrannyGetMeshBindingBoneCount( m_grnMeshBinding ) : maxBonesPerTurret;

		// Index of the bone in the big translation and rotation 
		unsigned int boneIndex = 0;
		unsigned int turretIndex = 0;
		
		// put all single turret's positions and rotations in the array
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			if( m_singleTurrets[i].visible )
			{
				boneIndex = turretIndex * boneCount;
				// get animation matrices here, they are not the same for all turrets of the set
				const Matrix* compositeMatrixArray = m_singleTurrets[i].grnWorldPose ? (Matrix*)GrannyGetWorldPoseComposite4x4Array( m_singleTurrets[i].grnWorldPose ) : NULL;
				
				// Construct all turret bone translations and rotations
				for( unsigned int j = 0; j < boneCount; ++j )
				{
					if( m_singleTurrets[i].valid && toBoneIndices && compositeMatrixArray)
					{
						Matrix m = compositeMatrixArray[toBoneIndices[j]];
						D3DXMatrixDecompose( &scale, &perObjectData->m_turretPoseRot[boneIndex], &translation, &m );
						perObjectData->m_turretPoseTrans[boneIndex] = Vector4( translation, 1.0f );
					}
					else
					{
						perObjectData->m_turretPoseTrans[boneIndex] = Vector4( 0, 0, 0, 1 );
						perObjectData->m_turretPoseRot[boneIndex] = Quaternion( 0, 0, 0, 1 );
					}

					// Increment the bone index so the index in the arrays is correct
					boneIndex++;
				}

				// actual turret position and rotation
				if( m_singleTurrets[i].valid )
				{
					perObjectData->m_turretRotation[turretIndex] = m_singleTurrets[i].localQuaternion;
					perObjectData->m_turretTranslation[turretIndex] = m_singleTurrets[i].localPosition;
				}
				else
				{
					perObjectData->m_turretTranslation[turretIndex] = Vector4( 0, 0, 0, 1 );
					perObjectData->m_turretRotation[turretIndex] = Quaternion( 0, 0, 0, 1 );
				}

				++turretIndex;
			}
		}

		// store how many bones in in the turrets, so we can correctly read from the buffer in the shader
		perObjectData->m_turretSetData = Vector4( (float)boneCount, 0, 0, 0 );

		// ps data
		perObjectData->m_shipData = m_parentData.shipData;
		perObjectData->m_clipData1 = m_parentData.clipData;
		perObjectData->m_clipData2 = m_parentData.clipDataEx;
		if( m_parentShLighting )
		{
			memcpy( perObjectData->m_shLightingCoefficients, m_parentShLighting, sizeof( perObjectData->m_shLightingCoefficients ) );
		}
		else
		{
			memset( perObjectData->m_shLightingCoefficients, 0, sizeof( perObjectData->m_shLightingCoefficients ) );
		}
	}

	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced reandering and call DIP
// --------------------------------------------------------------------------------
void EveTurretSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( !m_geometryResource )
	{
		return;
	}

	if( !m_geometryResource->IsGood() )
	{
		return;
	}

	if( m_geometryResource->GetMeshCount() < 1 )
	{
		return;
	}

	const TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( 0 );
	if( !meshData )
	{
		return;
	}

	if( !meshData->m_indexBuffer.IsValid() || !meshData->m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( !m_visibleCount )
	{
		return;
	}

	// render
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyIndexBuffer( meshData->m_indexBuffer );
	// Stream 0: "geometry": here: our turret geometry
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	// Stream 1: instance", here: instance index
	renderContext.m_esm.ApplyStreamSource( 1, m_instanceBuffer, 0, sizeof( float ) );

	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedInstanced( meshData->m_vertexCount, 0, meshData->m_primitiveCount, m_visibleCount );
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

	granny_file_info* grannyFileInfo = m_geometryResource->GetGrannyInfo();
	if( grannyFileInfo )
	{
		// there can be more animations in one res, so find right one
		int animIx = grannyFileInfo->AnimationCount;
		if( !animName.empty() )
		{
			for( int i = 0; i < grannyFileInfo->AnimationCount ; ++i )
			{
				if( strcmp( grannyFileInfo->Animations[i]->Name, animName.c_str() ) == 0 )
				{
					animIx = i;
					break;
				}
			}
			if( animIx == grannyFileInfo->AnimationCount )
			{
				CCP_LOGWARN( "Animation %s not found in %s!", animName.c_str(), m_name.c_str() );
				return 0.f;
			}
		}

		int idleIx = grannyFileInfo->AnimationCount;
		if( !animNameIdle.empty() )
		{
			for( int i = 0; i < grannyFileInfo->AnimationCount ; ++i )
			{
				if( strcmp( grannyFileInfo->Animations[i]->Name, animNameIdle.c_str() ) == 0 )
				{
					idleIx = i;
					break;
				}
			}
			if( idleIx == grannyFileInfo->AnimationCount )
			{
				CCP_LOGWARN( "Animation %s not found in %s!", animNameIdle.c_str(), m_name.c_str() );
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

// --------------------------------------------------------------------------------
// Description:
//   Usually animation names are just constant strings like "deploy" or "idle".
//   But in the firing case, we support alternate animations, chosen randomly,
//   to make the firing look more lively.
// --------------------------------------------------------------------------------
std::string EveTurretSet::GetFireAnimationName() const
{
	// firing anim "index"
	int idx = 0;

	// random!
	int rndNum = TriIntRandom( 0, m_alternateFiringAnimCount );

	// cycling?
	if( m_hasCyclingFiringPos )
	{
		idx = m_currentCyclingFiresPos * ( m_alternateFiringAnimCount + 1 ) + rndNum;
	}
	else
	{
		idx = rndNum;
	}

	// if idx is 0, it's just "Fire"
	std::string res = "Fire";
	if( idx )
	{
		res.push_back('0');
		res.push_back('0' + idx);
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
			PlayAnimation( i, "Pack", "Inactive", 0.f );
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
		m_targetLocator = -1;
		m_targetPositionOldInfluence = -1.f;
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Pack", "Inactive", m_trackingFadeTime );
		}

		ResetMissQueue();
		break;

	default:
		break;
	}
	// finally, we can set state
	m_state = STATE_DEACTIVE;
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
			PlayAnimation( i, "", "Active", 0.f );
		}
		break;
	case STATE_DEACTIVE:
		// start unpack animation, disable tracking and then into active loop
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Deploy", "Active", 0.f );
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
		m_targetLocator = -1;
		m_targetPositionOldInfluence = -1.f;
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active", m_trackingFadeTime );
		}

		ResetMissQueue();
		break;
	}
	// finally, we can set state
	m_state = STATE_IDLE;
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
			animLength = PlayAnimation( i, "Deploy", "Active", 0.f );
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
			PlayAnimation( i, "", "Active", m_trackingFadeTime );
		}
		break;
	case STATE_TARGETING:
		// are we already targeting something?
		if( m_targetObject )
		{
			// this most likely is new target, so trigger a smooth fade
			m_targetPositionOld = m_targetPosition;
			m_targetPositionOldInfluence = 1.f;
		}
		break;
	case STATE_FIRING:
		// stop shooting, then into active loop
		m_activeTurret = INVALID_TURRET_INDEX;
		m_targetLocator = -1;
		// we sure as hell were targeting something, so we were facing to a damage locator: fade back
		m_targetPositionOld = m_targetPosition;
		m_targetPositionOldInfluence = 1.f;
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "", "Active", 0.f );
		}
		
		ResetMissQueue();
		break;

	default:
		break;
	}

	// finally, we can set state
	m_state = STATE_TARGETING;
}

// --------------------------------------------------------------------------------
// Description:
//   Exposed function to put this turret set into the firing state. It will
//   start playing the fireanim and then go into standard active loop until
//   this function is called again.
// --------------------------------------------------------------------------------
void EveTurretSet::EnterStateFiring()
{
	// find the best single turret and damage locator pair
	unsigned int closestTurret = INVALID_TURRET_INDEX;
	int closestLocator = -1;
	GetClosestTurretAndLocator( closestTurret, closestLocator );

	// if this turret is set to cycle through the muzzles for firing, do it here
	if( m_hasCyclingFiringPos )
	{
		++m_currentCyclingFiresPos;
		if( m_currentCyclingFiresPos > 1 )
		{
			m_currentCyclingFiresPos = 0;
		}
	}

	// apply a randomized fire delay
	CalcRandomDelay();

	// what state are we in?
	switch( m_state )
	{
	// this state change is forbidden!
	case STATE_DEACTIVE:
		CCP_LOGERR( "EveTurretSet %s wants to fire but is in deactive state.", m_name.c_str() );
		return;
	case STATE_IDLE:
	case STATE_RELOADING:
		// and delay the effect until we are facing target
		m_randomFiringDelay += m_trackingFadeTime;
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
		m_targetLocator = closestLocator;
		m_targetPositionOldInfluence = -1.f;
		m_activeTurret = closestTurret;
		break;
	case STATE_FIRING:
		// only if we are in firing mode, call ::StopFiring() on the effect right before
		// we call ::PrepareFiring(), it'll clean things up in the effect
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		// attention: no break!
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
		m_targetLocator = closestLocator;
		break;

	default:
		break;
	}

	// We're starting a firing sequence, we need to set up our firing effect time-delays
	if( m_firingEffect )
	{
		if( m_hasCyclingFiringPos )
		{
			m_firingEffect->PrepareFiring( m_randomFiringDelay, m_currentCyclingFiresPos );
		}
		else
		{
			m_firingEffect->PrepareFiring( m_randomFiringDelay );
		}
	}

	// finally, we can set state
	m_state = STATE_FIRING;
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
		m_targetLocator = -1;
		m_targetPositionOldInfluence = -1.f;
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		for( unsigned int i = 0; i < m_singleTurrets.size(); ++i )
		{
			PlayAnimation( i, "Reload", "Active", m_trackingFadeTime );
		}

		ResetMissQueue();
		break;

	default:
		break;
	}
	// finally, we can set state
	m_state = STATE_RELOADING;
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
	m_targetLocator = -1;
	m_targetPositionOldInfluence = -1.f;
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
//   ITr2Targetable
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
			int locatorIx = -1;
			Vector3 position = m_targetPosition;
			// get position of closest damagelocator
			if( m_targetObject )
			{
				// the target object tells us the target position via damage locators
				position = m_singleTurrets[i].worldMatrix.GetTranslation();
				locatorIx = m_targetObject->GetClosestDamageLocatorIndex( &position );
				m_targetObject->GetDamageLocatorPosition( &position, locatorIx );
			}

			// find normal from turret to target
			Vector3 nrmToTarget = position - m_singleTurrets[i].worldMatrix.GetTranslation();
			D3DXVec3Normalize( &nrmToTarget, &nrmToTarget );
			// find "up" normal of turret
			Vector3 nrmUp = Vector3( 0.f, 1.f, 0.f );
			D3DXVec3TransformNormal( &nrmUp, &nrmUp, &m_singleTurrets[i].worldMatrix );
			float angle = D3DXVec3Dot( &nrmToTarget, &nrmUp );
			if( angle > closestAngle )
			{
				closestTurret = i;
				closestLocator = locatorIx;
				closestAngle = angle;
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
//   Clears the miss queue. Do this when changing target, disabling turrets etc.
// --------------------------------------------------------------------------------
void EveTurretSet::ResetMissQueue()
{
	m_lastShotAccuracy = ACCURACY_INDETERMINATE;
	m_trackMissPoint = false;
	while( !m_missQueue.empty() ) 
	{
		m_missQueue.pop_front();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Private method to pop from the miss queue.
// --------------------------------------------------------------------------------
void EveTurretSet::PopShotMissed() 
{ 
	m_lastShotAccuracy = m_missQueue.empty() ? ACCURACY_INDETERMINATE : (ShotAccuracy)m_missQueue.front();
	if( !m_missQueue.empty() )
		m_missQueue.pop_front(); 
}

// --------------------------------------------------------------------------------
// Description:
//   Get the hit/miss status of the last shot info.
// --------------------------------------------------------------------------------
bool EveTurretSet::GetShotMissed() const 
{ 
	if( m_lastShotAccuracy == ACCURACY_INDETERMINATE )
	{
		// take a guess as to whether it hit... or just default to something
		m_lastShotAccuracy = ACCURACY_HIT;
	}

	return m_lastShotAccuracy != ACCURACY_HIT;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a hit/miss to the shot queue.
// --------------------------------------------------------------------------------
void EveTurretSet::SetShotMissed( const bool missed ) 
{ 
	if( !m_trackMissPoint )
	{
		m_lastShotAccuracy = (ShotAccuracy)missed;
	}
	m_trackMissPoint = true;
	m_missQueue.push_back(missed); 
	m_lastShotTime = TimeAsDouble(BeOS->GetActualTime());
	//in case we get way behind, start dropping miss events,
	// rather than infinitely accumulating.
	//should still be representative.
	while( m_missQueue.size() > 4 )
	{
		m_missQueue.pop_front();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Still to come
// --------------------------------------------------------------------------------
void EveTurretSet::UpdateMissPosition( const Matrix *parentMatrix )
{
	//early out if we're not tracking miss positions
	if( !m_trackMissPoint || !m_targetObject )
	{
		m_targetPositionMiss = m_targetPosition;
		return;
	}


	const Vector3 muzzlePos = Vector3( parentMatrix->_41, parentMatrix->_42, parentMatrix->_43 );

	m_targetObject->GetMissPosition( &m_targetPosition, &muzzlePos, &m_targetPositionMiss );
	m_targetPositionMiss += m_randomMissPositionOffset;
	Vector3 direction = m_targetPositionMiss - muzzlePos;

	if( m_laserMissBehaviour )
	{
		D3DXVec3Normalize( &direction, &direction );
		m_targetPositionMiss += direction * 250000.f;
	}
	else
	{
		float dist = D3DXVec3Length( &direction );
		direction /= dist;
		m_targetPositionMiss += direction * (dist + 5000.f) * (1.f + 0.5f * m_randomMissDistanceOffset);
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Return the size of the miss queue
// --------------------------------------------------------------------------------
size_t EveTurretSet::MissQueueSize() const
{
	return m_missQueue.size();
}


// --------------------------------------------------------------------------------
// Description:
//   Sets the end point of the firingEffect
// --------------------------------------------------------------------------------
void EveTurretSet::SetEffectEndPoint()
{
	if( GetShotMissed() )
	{
		m_firingEffect->SetEndPosition( &m_targetPositionMiss );
	}
	else
	{
		m_firingEffect->SetEndPosition( &m_targetPosition );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the data to HW
// --------------------------------------------------------------------------------
void EveTurretSetPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	// add up constant count, see EveTurretSetPerObjectData
	int vsConstantCount =
		1 +															// Vector4 clip data
		1 +							   								// Vector4 extra data (x is bone count)
		4 +															// World matrix
		EVE_MAX_TURRETS_PER_SET +									// Vector4 array translation
		EVE_MAX_TURRETS_PER_SET +									// Vector4 array rotation quaternion
		EVE_MAX_TURRET_SET_BONES +									// Vector4 array for bone pose translation
		EVE_MAX_TURRET_SET_BONES;									// Vector4 array for bone pose rotation
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_baseCutoffData, vsConstantCount * 16, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );

	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_shipData, ( 3 + Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT ) * 16, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );

}






