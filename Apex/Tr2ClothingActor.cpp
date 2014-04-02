#include "StdAfx.h"

#if APEX_ENABLED

#include "Tr2ClothingActor.h"
#include "Tr2ClothingRes.h"
#include "Tr2ApexScene.h"
#include "TriSettingsRegistrar.h"
#include "Resources/TriGrannyRes.h"

#include "Apex.h"

static bool s_apexHardwareClothEnabled = true;
TRI_REGISTER_SETTING( "apexHardwareClothEnabled", s_apexHardwareClothEnabled );

Tr2ClothingActor::Tr2ClothingActor( IRoot* lockobj ) :
	m_clothingActor( NULL ),
	m_clothingMaterial( NULL ),
	m_morphResMeshIndex( 0 ),
	m_morphResEpsilon( 0.0f ),
	m_isInScene( false ),
	m_bones( NULL ),
	m_boneCount( 0 ),
	m_boneMap( "Tr2ClothingActor/m_boneMap" ),
	m_lastBoundBoneList( NULL ),
	m_windDirection( 1.0f, 0.0f, 0.0f ),
	m_windStrength( 0.0f ),
	m_blendVelocity( 0.1f ),
	m_useTransparentBatches( false ),
	m_useSHLighting( false ),
	m_resetCloth( false ),
	m_maxDistance( 100.0f ),
	m_distanceWeight( 1.0f ),
	m_bias( 0.0f ),
	m_benefitBias( 0.0f ),
	m_stretchingStiffness( 1.0f ),
	m_bendingStiffness( 0.5f ),
	m_orthoBending( false ),
	m_damping( 0.1f ),
	m_comDamping( false ),
	m_friction( 0.5f ),
	m_solverIterations( 5 ),
	m_gravityScale( 1.0f ),
	m_hardStretchLimitation( 1.1f ),
	m_maxDistanceBias( 0.0f ),
	m_maxDistanceScale( 1.0f ),
	m_hierarchicalSolverIterations( 0 )
{
}

Tr2ClothingActor::~Tr2ClothingActor()
{
	Cleanup();

	if( m_clothingRes )
	{
		m_clothingRes->RemoveNotifyTarget( this );
	}

}

bool Tr2ClothingActor::Initialize()
{
	Cleanup();

	if( m_clothingRes )
	{
		m_clothingRes->RemoveNotifyTarget( this );
		m_clothingRes.Unlock();
	}

	BeResMan->GetResource( m_resPath, "cloth", m_clothingRes );

	if( m_clothingRes )
	{
		m_clothingRes->AddNotifyTarget( this );
	}	

	return true;
}

void Tr2ClothingActor::AddToApexScene( Tr2ApexScene* apexScene )
{
	if( !apexScene || !apexScene->GetApexScene() )
	{
		CCP_LOGERR( "AddToApexScene: null scene" );
		return;
	}

	m_isInScene = true;

	if( !m_clothingRes )
	{
		// No clothing resource, can't create the actor.
		return;
	}

	if( m_clothingRes->IsLoading() )
	{
		// Not finished loading - we'll come back once it is.
		return;
	}

	if( m_morphRes && m_morphRes->IsLoading() )
	{
		return;
	}

	physx::apex::NxClothingAsset* asset = m_clothingRes->GetAsset();
	if( !asset )
	{
		CCP_LOGERR( "Tr2ClothingActor::AddToApexScene: %S has no clothing asset", m_clothingRes->GetPath() );
		return;
	}

	Cleanup();

	m_boneCount = asset->getNumUsedBones();
	m_bones = CCP_NEW( "Tr2ClothingActor/m_bones" ) physx::PxMat44[m_boneCount];

	// Initialize to identity - default constructor doesn't do that.
	// This ensures that UpdateState won't pass garbage to the simulation
	// even if some bones aren't mapped.
	for( unsigned int i = 0; i < m_boneCount; ++i )
	{
		m_bones[i] = physx::PxMat44::createIdentity();
	}

	m_lastBoundBoneList = NULL;
	NxParameterized::Interface* actorDesc = asset->getDefaultActorDesc();
	CCP_ASSERT( actorDesc != NULL );
	if( !actorDesc )
	{
		return;
	}

	bool ok = NxParameterized::setParamBool( *actorDesc, "useHardwareCloth", s_apexHardwareClothEnabled );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamBool( *actorDesc, "useInternalBoneOrder", true );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamBool( *actorDesc, "flags.ParallelCpuSkinning", true );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamMat44( *actorDesc, "globalPose", *m_bones );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamBool( *actorDesc, "fallbackSkinning", g_Tr2Apex->GetFallbackSkinning() );
	CCP_ASSERT(ok);

	{
		#define VERIFY_PARAM(_A) { NxParameterized::ErrorType error = _A; CCP_ASSERT(error == NxParameterized::ERROR_NONE); }

		NxParameterized::Handle actorHandle(*actorDesc);

		// No util method for this
		VERIFY_PARAM( actorHandle.getParameter("boneMatrices" ) );
		VERIFY_PARAM( actorHandle.resizeArray(m_boneCount ) );
		VERIFY_PARAM( actorHandle.setParamMat44Array( m_bones, m_boneCount ) );
	}

	if( m_morphRes )
	{
		// Create a mapping from the granny vertices to the cloth vertices
		std::vector<float> xyz;
		if( m_morphRes->GetVertexPositions( m_morphResMeshIndex, xyz ) && !xyz.empty() )
		{
			const unsigned failedCount = asset->prepareMorphTargetMapping( (physx::pubfnd3::PxVec3*)&xyz[0], (physx::PxU32)xyz.size()/3, m_morphResEpsilon );
			if( failedCount )
			{
				CCP_LOGWARN( "prepareMorphTargetMapping: %d vertices could not be matched (out of %d)", failedCount, xyz.size()/3 );
			}
		
			// Get the scaled and summed blend deltas and feed them into Apex.
			std::vector<float> deltaXyz;			
			if( m_morphRes->GetBlendDeltas( m_morphResMeshIndex, m_morphResWeights, deltaXyz ) )
			{
				if( deltaXyz.size() == xyz.size() )
				{
					NxParameterized::Handle md( *actorDesc, "morphDisplacements" );
					md.resizeArray( (unsigned int)deltaXyz.size() / 3 );
					md.setParamVec3Array( (physx::pubfnd3::PxVec3*)&deltaXyz[0], (unsigned int)deltaXyz.size()/3 );
				}
				else
				{
					CCP_LOGWARN( "prepareMorphTargetMapping: mapping has %d vertices but morph target has %d.", xyz.size()/3, deltaXyz.size()/3 );
				}
			}
			else
			{
				CCP_LOGWARN( "prepareMorphTargetMapping: getting the blend delta's failed, likely wrong number of weights." );
			}
		}
	}

	// create the actor
	physx::apex::NxApexActor* apexActor = asset->createApexActor( *actorDesc, *apexScene->GetApexScene() );
	CCP_ASSERT( apexActor );

	m_clothingActor = static_cast<physx::apex::NxClothingActor*>(apexActor);
}

void Tr2ClothingActor::RemoveFromApexScene()
{
	m_isInScene = false;
	Cleanup();
}

void Tr2ClothingActor::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}

void Tr2ClothingActor::RebuildCachedData( BlueAsyncRes* p )
{
	if( m_isInScene && m_clothingActor )
	{
		Cleanup();
		// The next call to update will re-AddToApexScene() for us;
		// so we can avoid the need to hang on to the scene we're in
		// to be able to do the re-add here.
	}
}

const std::string& Tr2ClothingActor::GetResPath() const
{
	return m_resPath;
}

void Tr2ClothingActor::SetResPath( const std::string& val )
{
	m_resPath = val;
	Initialize();
}

void Tr2ClothingActor::Cleanup()
{
	if( m_clothingActor )
	{
		if( g_Tr2Apex->IsClothSimInProgress() )
		{
			g_Tr2Apex->ApexDelayReleaseActor( m_clothingActor );
		}
		else
		{
			m_clothingActor->release();
		}
		m_clothingActor = NULL;
	}

	CCP_DELETE [] m_bones;
	m_bones = NULL;
	m_boneCount = 0;
}

physx::apex::NxApexRenderable* Tr2ClothingActor::GetApexRenderable()
{
	return m_clothingActor;
}

float Tr2ClothingActor::GetMaxDistanceBlendTime() const
{
	if( m_clothingActor )
	{
		return m_clothingActor->getMaxDistanceBlendTime();
	}

	return 0.0f;
}

void Tr2ClothingActor::SetForcedLod( float lod )
{
	if( m_clothingActor )
	{
		m_clothingActor->forcePhysicalLod( lod );
	}
}

void Tr2ClothingActor::UpdateState( const Matrix& globalPose, float* skinningMatrices, unsigned int skinningMatrixCount, UpdateStateFlag continuousFlag, Tr2ApexScene* apexScene )
{
	if( !m_isInScene && apexScene )
	{
		AddToApexScene( apexScene );
	}

	if( !m_clothingActor )
	{
		return;
	}

	// We ought to have a valid clothing res if the actor is valid
	CCP_ASSERT( m_clothingRes );
	if( !m_clothingRes )
	{
		return;
	}

	NxParameterized::Interface* actorDesc = m_clothingActor->getActorDesc();
	CCP_ASSERT( actorDesc );
	if( !actorDesc )
	{
		return;
	}
	bool ok;
	ok = NxParameterized::setParamBool( *actorDesc, "flags.ParallelCpuSkinning", g_Tr2Apex->ApexGetParallelCpuSkinning() );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamVec3( *actorDesc, "windParams.Velocity", *(const physx::PxVec3*)&m_windDirection );
	CCP_ASSERT(ok);
	ok = NxParameterized::setParamF32(  *actorDesc, "windParams.Adaption", m_windStrength );
	CCP_ASSERT(ok);



	m_clothingActor->setLODWeights( m_maxDistance, m_distanceWeight, m_bias, m_benefitBias );

	physx::PxMat44& m = *(physx::PxMat44*)&globalPose;

	for( unsigned int i = 0; i < m_boneCount; ++i )
	{
		unsigned int srcIx = m_boneMap[i];
		CCP_ASSERT( srcIx < skinningMatrixCount );

		physx::PxMat44& jointMatrix = m_bones[i];

		float* src = skinningMatrices + 12*srcIx;

		// TODO: do this more efficiently. Apex changed to using 4x4, so look into
		// skinned objects again - probably makes sense to keep the 4x4s coming from
		// the animation updater rather than converting back and forth.
		jointMatrix.column0.x = src[0];
		jointMatrix.column0.y = src[4];
		jointMatrix.column0.z = src[8];
		jointMatrix.column0.w = 0.0f;

		jointMatrix.column1.x = src[1];
		jointMatrix.column1.y = src[5];
		jointMatrix.column1.z = src[9];
		jointMatrix.column1.w = 0.0f;

		jointMatrix.column2.x = src[2];
		jointMatrix.column2.y = src[6];
		jointMatrix.column2.z = src[10];
		jointMatrix.column2.w = 0.0f;

		jointMatrix.column3.x = src[3];
		jointMatrix.column3.y = src[7];
		jointMatrix.column3.z = src[11];
		jointMatrix.column3.w = 1.0f;
	}


//	physx::PxMat44& baseMatrix = *m_bones;
//	CCP_LOG("RootPos: %0.5f,%0.5f,%0.5f BaseLocation: %0.5f,%0.5f,%0.5f\r\n", m.column3.x, m.column3.y, m.column3.z, baseMatrix.column3.x, baseMatrix.column3.y, baseMatrix.column3.z );

	bool isContinuous = !m_resetCloth && (continuousFlag == IS_CONTINUOUS);

	m_clothingActor->updateState( m, 
								m_bones, 
								sizeof( physx::PxMat44 ), 
								m_boneCount, 
								isContinuous ? physx::apex::NxClothingActor::TM_Continuous : physx::apex::NxClothingActor::TM_Teleport );

	m_resetCloth = false;
}

void Tr2ClothingActor::BindToRig( const std::string* boneList, unsigned int numBones, bool forceRebind )
{
	if( m_clothingRes && ( forceRebind || boneList != m_lastBoundBoneList) )
	{
		m_clothingRes->BindToRig( boneList, numBones, m_boneMap );
		m_lastBoundBoneList = boneList;
	}
}

void Tr2ClothingActor::SetVisualize( bool b )
{
	CCP_ASSERT(m_clothingActor);
	if ( m_clothingActor )
	{
		NxParameterized::Interface *actorDesc = m_clothingActor->getActorDesc();
		CCP_ASSERT(actorDesc);
		if ( actorDesc )
		{
			bool ok = NxParameterized::setParamBool(*actorDesc,"flags.Visualize",b);
			CCP_ASSERT(ok);
		}
	}
}

bool Tr2ClothingActor::GetVisualize()
{
	bool ret = false;
	CCP_ASSERT(m_clothingActor);
	if ( m_clothingActor )
	{
		NxParameterized::Interface *actorDesc = m_clothingActor->getActorDesc();
		CCP_ASSERT(actorDesc);
		if ( actorDesc )
		{
			bool ok = NxParameterized::getParamBool(*actorDesc,"flags.Visualize",ret);
			CCP_ASSERT(ok);
		}
	}
	return ret;
}

ITr2ShaderMaterial* Tr2ClothingActor::GetEffect()
{
	return m_effect;
}

ITr2ShaderMaterial* Tr2ClothingActor::GetDepthEffect()
{
	return m_depthEffect;
}

ITr2ShaderMaterial* Tr2ClothingActor::GetDepthNormalEffect()
{
	return m_depthNormalEffect;
}

// -------------------------------------------------------------
// Description:
//   Returns an effect to use with reversed order of triangle 
//   rendering for forward rendering.
// Return value:
//   An effect to use with reversed triangle order for forward redering 
// -------------------------------------------------------------
ITr2ShaderMaterial* Tr2ClothingActor::GetEffectReversed()
{
	return m_effectReversed;
}

// -------------------------------------------------------------
// Description:
//   Returns an effect to use with reversed order of triangle 
//   rendering for shadow rendering.
// Return value:
//   An effect to use with reversed triangle order for shadow redering 
// -------------------------------------------------------------
ITr2ShaderMaterial* Tr2ClothingActor::GetDepthEffectReversed()
{
	return m_depthEffectReversed;
}

// -------------------------------------------------------------
// Description:
//   Returns an effect to use with reversed order of triangle 
//   rendering for prepass depth/normal rendering.
// Return value:
//   An effect to use with reversed triangle order for prepass 
//   depth/normal redering 
// -------------------------------------------------------------
ITr2ShaderMaterial* Tr2ClothingActor::GetDepthNormalEffectReversed()
{
	return m_depthNormalEffectReversed;
}

float Tr2ClothingActor::GetStretchingStiffness() const
{
	return m_stretchingStiffness;
}

static NxParameterized::Handle getMaterialHandle(physx::apex::NxClothingActor *actor,const char* name)
{
	CCP_ASSERT(actor);
	physx::apex::NxClothingAsset *asset = static_cast< physx::apex::NxClothingAsset *>(actor->getOwner());
	CCP_ASSERT(asset);
	NxParameterized::Interface* paramAsset = (NxParameterized::Interface*) asset->getAssetNxParameterized();
	CCP_ASSERT(paramAsset);
	NxParameterized::Interface* paramMaterialLib = NULL;
	NxParameterized::getParamRef(*paramAsset,"materialLibrary", paramMaterialLib);
	CCP_ASSERT(paramMaterialLib);
	unsigned int materialIndex = 0;
	NxParameterized::ErrorType err;
	bool ok;
	ok = NxParameterized::getParamU32(*paramAsset,"materialIndex", materialIndex);
	CCP_ASSERT( ok );
	NxParameterized::Handle libHandle(*paramMaterialLib,"materials");
	err = libHandle.set(materialIndex); // select the correct material
	CCP_ASSERT( err == NxParameterized::ERROR_NONE );
	physx::PxI32 elementIndex = 0;
	const NxParameterized::Definition *d = libHandle.parameterDefinition();
	CCP_ASSERT(d);
	d->child(name, elementIndex);
	err = libHandle.set(elementIndex);
	CCP_ASSERT( err == NxParameterized::ERROR_NONE );
	return libHandle;
}


static void setMaterialParam(physx::apex::NxClothingActor *actor,const char* name, physx::PxF32 value)
{
	NxParameterized::Handle libHandle = getMaterialHandle(actor,name);
	NxParameterized::ErrorType err = libHandle.setParamF32(value);
	CCP_ASSERT( err == NxParameterized::ERROR_NONE );
}

static void setMaterialParam(physx::apex::NxClothingActor *actor,const char* name,bool value)
{
	NxParameterized::Handle libHandle = getMaterialHandle(actor,name);
	NxParameterized::ErrorType err = libHandle.setParamBool(value);
	CCP_ASSERT( err == NxParameterized::ERROR_NONE );
}

static void setMaterialParam(physx::apex::NxClothingActor *actor,const char* name,physx::PxU32 value)
{
	NxParameterized::Handle libHandle = getMaterialHandle(actor,name);
	NxParameterized::ErrorType err = libHandle.setParamU32(value);
	CCP_ASSERT( err == NxParameterized::ERROR_NONE );
}



void Tr2ClothingActor::SetStretchingStiffness( float val )
{
	m_stretchingStiffness = val;
	setMaterialParam(m_clothingActor,"stretchingStiffness",val);
}

float Tr2ClothingActor::GetBendingStiffness() const
{
	return m_bendingStiffness;
}

void Tr2ClothingActor::SetBendingStiffness( float val )
{
	m_bendingStiffness = val;
	setMaterialParam(m_clothingActor,"bendingStiffness",val);
}

bool Tr2ClothingActor::GetOrthoBending() const
{
	return m_orthoBending;
}

void Tr2ClothingActor::SetOrthoBending( bool val )
{
	m_orthoBending = val;
	setMaterialParam(m_clothingActor,"orthoBending",val);
}

float Tr2ClothingActor::GetDamping() const
{
	return m_damping;
}

void Tr2ClothingActor::SetDamping( float val )
{
	m_damping = val;
	setMaterialParam(m_clothingActor,"damping",val);
}

bool Tr2ClothingActor::GetComDamping() const
{
	return m_comDamping;
}

void Tr2ClothingActor::SetComDamping( bool val )
{
	m_comDamping = val;
	setMaterialParam(m_clothingActor,"comDamping",val);
}

float Tr2ClothingActor::GetFriction() const
{
	return m_friction;
}

void Tr2ClothingActor::SetFriction( float val )
{
	m_friction = val;
	setMaterialParam(m_clothingActor,"friction",val);
}

unsigned int Tr2ClothingActor::GetSolverIterations() const
{
	return m_solverIterations;
}

void Tr2ClothingActor::SetSolverIterations( unsigned int val )
{
	m_solverIterations = val;
	setMaterialParam(m_clothingActor,"solverIterations",val);
}

float Tr2ClothingActor::GetGravityScale() const
{
	return m_gravityScale;
}

void Tr2ClothingActor::SetGravityScale( float val )
{
	m_gravityScale = val;
	setMaterialParam(m_clothingActor,"gravityScale",val);
}

float Tr2ClothingActor::GetHardStretchLimitation() const
{
	return m_hardStretchLimitation;
}

void Tr2ClothingActor::SetHardStretchLimitation( float val )
{
	m_hardStretchLimitation = val;
	setMaterialParam(m_clothingActor,"hardStretchLimitation",val);
}

float Tr2ClothingActor::GetMaxDistanceBias() const
{
	return m_maxDistanceBias;
}

void Tr2ClothingActor::SetMaxDistanceBias( float val )
{
	m_maxDistanceBias = val;
	setMaterialParam(m_clothingActor,"maxDistanceBias",val);
}

float Tr2ClothingActor::GetMaxDistanceScale() const
{
	return m_maxDistanceScale;
}

void Tr2ClothingActor::SetMaxDistanceScale( float val )
{
	m_maxDistanceScale = std::min( 1.0f, std::max( 0.0f, val ) );
	if( m_clothingActor != NULL )
	{
		m_clothingActor->updateMaxDistanceScale( m_maxDistanceScale, true );
	}
}

unsigned int Tr2ClothingActor::GetHierarchicalSolverIterations() const
{
	return m_hierarchicalSolverIterations;
}

void Tr2ClothingActor::SetHierarchicalSolverIterations( unsigned int val )
{
	m_hierarchicalSolverIterations = val;
	setMaterialParam(m_clothingActor,"hierarchicalSolverIterations",val);
}

bool Tr2ClothingActor::GetUseTransparentBatches() const
{
	return m_useTransparentBatches;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a flag indicating that the mesh requires SH lighting coefficients for 
//   rendering instead of normal direct lighting. Only used for transparent meshes
//   (when GetUseTransparentBatches() is true).
// Return Value:
//   true If mesh requires SH lighting
//   false If mesh requires direct lighting
// --------------------------------------------------------------------------------------
bool Tr2ClothingActor::GetUseSHLighting() const
{
	return m_useSHLighting;
}

AxisAlignedBoundingBox Tr2ClothingActor::GetWorldBoundingBox() const
{
	if( m_clothingRes != NULL && m_clothingRes->GetAsset() != NULL && m_clothingActor != NULL )
	{
		const physx::PxBounds3 bb = m_clothingRes->GetAsset()->getBoundingBox();
		AxisAlignedBoundingBox result;
		result.m_min = Vector3( bb.minimum.x, bb.minimum.y, bb.minimum.z );
		result.m_max = Vector3( bb.maximum.x, bb.maximum.y, bb.maximum.z );

		const physx::PxMat44 globalPose = m_clothingActor->getGlobalPose();

		Matrix m;
		for( unsigned j = 0; j != 4; ++j )
			for( unsigned i = 0; i != 4; ++i )
				m.m[j][i] = globalPose[j][i];
		
		result.Transform( m );
		return result;
	}
	return AxisAlignedBoundingBox( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
}

#endif
