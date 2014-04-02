#include "StdAfx.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "include/ITr2DebugRenderer.h"

static const int MAX_JOINT_COUNT = 58;

Tr2GrannyAnimation::Tr2GrannyAnimation( IRoot* lockobj ) :
	PARENTLOCK( m_boneOffset ),
	m_animationQueue( "Tr2GrannyAnimation/m_animationQueue" ),
	m_boneList( "Tr2GrannyAnimation/m_boneList" ),
	m_skeleton( nullptr ),
	m_modelInstance( nullptr ),
	m_localPose( nullptr ),
	m_worldPose( nullptr ),
	m_meshBinding( nullptr ),
	m_meshBoneMatrixList( nullptr ),
	m_meshBoneCount( 0 ),
	m_useMeshBinding( false ),
	m_debugRenderSkeleton( false ),
	m_debugRenderJointNames( false )
{
}

Tr2GrannyAnimation::~Tr2GrannyAnimation()
{
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
	}	
}

void Tr2GrannyAnimation::SetSharedGeometryRes( TriGeometryResPtr res )
{
	m_geometryRes = res;
	m_resPath = "<EveSpaceObject2>";
}

bool Tr2GrannyAnimation::Initialize()
{
	Cleanup();

	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
		m_grannyRes.Unlock();
	}
	
	if( !m_geometryRes && !m_resPath.empty() )
	{
		BeResMan->GetResource( m_resPath.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&m_grannyRes );
	}

	if( m_grannyRes )
	{
		m_grannyRes->AddNotifyTarget( this );
	}
	
	return true;
}

void Tr2GrannyAnimation::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}

granny_file_info* Tr2GrannyAnimation::GetFileInfo()
{
	if( m_grannyRes )
	{
		// when using a standalone granny file, it's supposed to have an animation
		// track, so complain if it doesn't.
		granny_file_info* const fi = GrannyGetFileInfo( m_grannyRes->GetGrannyFile() );
		if( !fi )
		{
			CCP_LOGERR( "'%s' is not a valid Granny file", m_resPath.c_str() );
		}
		return fi;
	}

	// when using a shared geometryRes, there may not be an animation, or the
	// granny file isn't loaded yet.  Silently fail.
	if( m_geometryRes )
	{
		return m_geometryRes->GetGrannyInfo();
	}
	
	return nullptr;
}

void Tr2GrannyAnimation::RebuildCachedData( BlueAsyncRes* p )
{
	if( !m_grannyRes && !m_geometryRes )
	{
		return;
	}

	if( m_grannyRes && !m_grannyRes->GetGrannyFile() )
	{
		CCP_LOGERR( "'%s' not found or not a valid Granny file", m_resPath.c_str() );
		return;
	}

	const granny_file_info* const fi = GetFileInfo();
	if( !fi )
	{
		return;
	}
	
	if( fi->ModelCount > 0 && fi->AnimationCount > 0 )
	{
		// By default we take the first model in the file
		int modelIndex = 0;
		int meshBindingIndex = -1;

		if( m_useMeshBinding )
		{
			modelIndex = -1;
			for( int i = 0; i < fi->ModelCount ; ++i )
			{
				for( int j = 0; j < fi->Models[i]->MeshBindingCount ; ++j )
				{
					if( fi->Models[i]->MeshBindings[j].Mesh == fi->Meshes[0] )
					{
						modelIndex = i;
						meshBindingIndex = j;
						break;
					}
				}
				if( modelIndex != -1 )
				{
					break;
				}
			}
		}
		else
		if( !m_model.empty() )
		{
			// A named model is specified - look for its index
			modelIndex = -1;

			for( int i = 0; i < fi->ModelCount ; ++i )
			{
				if( m_model == fi->Models[i]->Name )
				{
					modelIndex = i;
					break;
				}
			}
		}
		
		if( modelIndex != -1 )
		{
			m_modelInstance = GrannyInstantiateModel ( fi->Models[ modelIndex ] );
			m_skeleton		= GrannyGetSourceSkeleton( m_modelInstance );
			m_localPose		= GrannyNewLocalPose	 ( m_skeleton->BoneCount );
			m_worldPose		= GrannyNewWorldPose	 ( m_skeleton->BoneCount );

			if( meshBindingIndex != -1 )
			{
				m_meshBinding = GrannyNewMeshBinding ( fi->Models[ modelIndex ]->MeshBindings[ meshBindingIndex ].Mesh, m_skeleton, m_skeleton );
			}
			else
			{
				m_meshBinding = nullptr;
			}

			for( int i = 0; i < m_skeleton->BoneCount; ++i )
			{
				m_boneList.push_back( m_skeleton->Bones[i].Name );
			}

			if( m_meshBinding )
			{
				m_meshBoneCount = GrannyGetMeshBindingBoneCount( m_meshBinding );
				if( m_meshBoneCount )
				{
					if( m_meshBoneCount >= MAX_JOINT_COUNT )
					{
						m_meshBoneCount = MAX_JOINT_COUNT;
					}
					m_meshBoneMatrixList = (granny_matrix_3x4*)CCP_ALIGNED_MALLOC( "Tr2GrannyAnimation/m_boneMatrixList", m_meshBoneCount * sizeof( granny_matrix_3x4 ), 16 );
				}
			}
		}
		else
		{
			CCP_LOGERR( "Model '%s' not found in '%s'", m_model.c_str(), m_resPath.c_str() );
			return;
		}
	}
	else
	{		
		m_modelInstance	= nullptr;
		m_skeleton		= nullptr;
		m_localPose		= nullptr;
		m_worldPose		= nullptr;

		if( !fi->ModelCount )
		{
			CCP_LOGERR( "No model to animate in '%s'", m_resPath.c_str() );
			return;
		}
		if( !fi->AnimationCount )
		{
			CCP_LOGERR( "No animations in '%s'", m_resPath.c_str() );
			return;
		}
	}

	// PlayAnimation will clear the animation queue if the replace flag is set.
	// We really should be searching backwards through the queue here to find the
	// last entry with replace set, then go forward from there. In reality we
	// rarely have more than one entry here, and this approach is much simpler:
	// Make a copy of the queue, iterate through that and be done with it...
	AnimationRequestList queueSnapshot = m_animationQueue;
	for( auto it = queueSnapshot.cbegin(); it != queueSnapshot.cend(); ++it )
	{
		PlayAnimation( it->m_animationName.c_str(), it->m_replace, it->m_loopCount, it->m_start, it->m_speed );
	}
	m_animationQueue.clear();
}

const std::string& Tr2GrannyAnimation::GetResPath() const
{
	return m_resPath;
}

void Tr2GrannyAnimation::SetResPath( const std::string& val )
{
	m_resPath = val;
	Initialize();
}

const std::string& Tr2GrannyAnimation::GetModel() const
{
	return m_model;
}

void Tr2GrannyAnimation::SetModel( const std::string& val )
{
	m_model = val;
	Initialize();
}

bool Tr2GrannyAnimation::PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone )
{
	if( ( !m_grannyRes  && !m_geometryRes )  ||		
		( m_grannyRes	&& !m_grannyRes->IsPrepared() ) ||
		( m_geometryRes	&& !m_geometryRes->IsPrepared() ) )
	{
		AnimationRequest ar;
		ar.m_animationName = animName;
		ar.m_replace = replace;
		ar.m_loopCount = loopCount;
		ar.m_start = delay;
		ar.m_speed = speed;

		m_animationQueue.push_back( ar );
		return true;
	}

	if( ( m_grannyRes && !m_grannyRes->IsGood() ) ||
		( m_geometryRes && !m_geometryRes->IsGood() ) )
	{
		CCP_LOGERR( "Animation resource failed to load!" );
		return false;
	}

	if( !m_modelInstance )
	{
		return false;
	}

	if( replace )
	{
		ClearAnimations();
	}

	const granny_file_info* const fi = GetFileInfo();
	if( !fi )
	{
		return false;
	}

	int animIx = fi->AnimationCount;

	for( int i = 0; i < fi->AnimationCount ; ++i )
	{
		if( strcmp( fi->Animations[i]->Name, animName ) == 0 )
		{
			animIx = i;
			break;
		}

	}
	if( animIx == fi->AnimationCount )
	{
		return false;
	}

	float startTime = Tr2Renderer::GetAnimationTime();

	if( !replace )
	{
		float maxRemaining = 0.0f;
		for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
				binding != GrannyModelControlsEnd( m_modelInstance ); 
				binding = GrannyModelControlsNext( binding ) )
		{
			granny_control *control = GrannyGetControlFromBinding( binding );

			// Force control to stop at the end of its current loop iteration
			// loopCount can be -1 if the animation hasn't started resulting in newLoopCount = 0
			// loop count 0 means loop forever so we'd get 'inf' time left
			int loopCount = max(0, GrannyGetControlLoopIndex( control ));
			int newLoopCount = loopCount + 1;

			GrannySetControlLoopCount( control, newLoopCount );

			float remaining = GrannyGetControlDurationLeft( control );
			GrannyCompleteControlAt( control, Tr2Renderer::GetAnimationTime() + remaining );

			if( remaining > maxRemaining )
			{
				maxRemaining = remaining;
			}
		}

		delay += maxRemaining;
	}

	startTime += delay;

	granny_control* control = GrannyPlayControlledAnimation( startTime, fi->Animations[animIx], m_modelInstance );

	GrannyEaseControlIn( control, 0.0f, false );
	GrannySetControlLoopCount( control, loopCount );
	GrannySetControlSpeed( control, speed );

	if( loopCount > 0 && clearWhenDone )
	{
		GrannyCompleteControlAt( control, Tr2Renderer::GetAnimationTime() + GrannyGetControlDurationLeft( control ) + delay );
	}

	GrannySetControlClock( control, Tr2Renderer::GetAnimationTime() );

	return true;
}

void Tr2GrannyAnimation::EndAnimation()
{
	if( !m_modelInstance )
	{
		m_animationQueue.clear();
		return;
	}

	for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
			binding != GrannyModelControlsEnd(m_modelInstance); 
			binding = GrannyModelControlsNext( binding ) )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		// Force control to stop at the end of its current loop iteration
		int newLoopCount;
		newLoopCount = max(0, GrannyGetControlLoopIndex( control )) + 1;
		GrannySetControlLoopCount( control, newLoopCount );
	}
	GrannyFreeCompletedModelControls( m_modelInstance );
}


void Tr2GrannyAnimation::ClearAnimations()
{	
	m_animationQueue.clear();

	if( !m_modelInstance )
	{
		return;
	}

	for( granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); binding != GrannyModelControlsEnd(m_modelInstance); )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		binding = GrannyModelControlsNext(binding);
		GrannyFreeControl( control );
	}
}

void Tr2GrannyAnimation::PrePhysicsAnimation( Be::Time time, const Matrix &modelTransform )
{
	if( m_modelInstance )
	{
		GrannySetModelClock( m_modelInstance, Tr2Renderer::GetAnimationTime() );
		GrannyFreeCompletedModelControls( m_modelInstance );

		// TODO: Should this be done here? Seems wasteful to sample animations and build the pose
		// for objects that are off-screen.
		GrannySampleModelAnimations( m_modelInstance, 0, m_skeleton->BoneCount, m_localPose );

		if( m_boneOffset.NeedRebind( m_skeleton->BoneCount ) && m_skeleton->BoneCount )
		{
			std::vector<std::string> bones( m_skeleton->BoneCount );			
			for( size_t i = 0; i < bones.size(); ++i )
				bones[ i ] = m_skeleton->Bones[ i ].Name;
			m_boneOffset.BindToRig( &bones[0], bones.size() );
		}

		if( !m_boneOffset.HaveTransforms() )
		{
			// build the worldpos out of the localpose using identity matrix as base
			GrannyBuildWorldPose( m_skeleton, 0, m_skeleton->BoneCount, m_localPose, &Tr2Renderer::GetIdentityTransform().m[0][0], m_worldPose );
			// construct the 3x4 matrix list, that will be passed to the shader, if we have a meshbinding at all
			if( m_meshBinding )
			{
				int const* meshToBone = GrannyGetMeshBindingToBoneIndices( m_meshBinding );
				if( m_meshBoneMatrixList && meshToBone && m_meshBoneCount )
				{
					GrannyBuildIndexedCompositeBufferTransposed( m_skeleton, m_worldPose, meshToBone, m_meshBoneCount, m_meshBoneMatrixList );
				}
			}
		}
		else
		{
			for( unsigned i = 0; i != m_skeleton->BoneCount; ++i )
			{
				granny_real32 localMatrix[16];
				GrannyBuildCompositeTransform4x4( GrannyGetLocalPoseTransform( m_localPose, i ), localMatrix );
				granny_real32    *worldMatrix    = GrannyGetWorldPose4x4( m_worldPose, i );

				const granny_int32 parentIndex = m_skeleton->Bones[i].ParentIndex;
				if( parentIndex != -1 )
				{
					const granny_real32    *parentWorldMatrix = GrannyGetWorldPose4x4( m_worldPose, parentIndex );

					if( !m_boneOffset.HaveTransforms() ||
						!m_boneOffset.Apply( worldMatrix, i, localMatrix, parentWorldMatrix ) )
					{
						GrannyColumnMatrixMultiply4x4( worldMatrix, localMatrix, parentWorldMatrix );
					}					
				}
				else
				{
					memcpy( worldMatrix, localMatrix, sizeof( granny_real32 ) * 16 );
				}
			}
		}
	
		extern ITr2DebugRendererPtr g_debugRenderer;
		if( g_debugRenderer )
		{
			if( m_debugRenderSkeleton )
			{
				for( int i = 0; i < m_skeleton->BoneCount; ++i )
				{
					int parentIx = m_skeleton->Bones[i].ParentIndex;
					if( parentIx != -1 )
					{
						Matrix fromMat = *reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4( m_worldPose, parentIx ) );
						Matrix toMat = *reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4( m_worldPose, i ) );

						// Transform to our world coordinates
						D3DXMatrixMultiply(&fromMat, &fromMat, &modelTransform);
						D3DXMatrixMultiply(&toMat, &toMat, &modelTransform);

						g_debugRenderer->DrawLine( fromMat.GetTranslation(), toMat.GetTranslation() );
					}
				}
			}
			if( m_debugRenderJointNames )
			{
				for( int i = 0; i < m_skeleton->BoneCount; ++i )
				{
					const char* name = m_skeleton->Bones[i].Name;
					Matrix m = *reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4( m_worldPose, i ) );

					// Transform to our world coordinates
					D3DXMatrixMultiply(&m, &m, &modelTransform);

					g_debugRenderer->Printf( m.GetTranslation(), 0xffffffff, name );
				}
			}
		}
	}
}

float Tr2GrannyAnimation::GetAnimationChainCompleteTime()
{
	float startTime = Tr2Renderer::GetAnimationTime();
	if( !m_modelInstance )
	{
		return startTime;
	}

	float maxRemaining = 0.0f;
	for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
			binding != GrannyModelControlsEnd( m_modelInstance ); 
			binding = GrannyModelControlsNext( binding ) )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );

		// Force control to stop at the end of its current loop iteration
		int loopCount = max(0, GrannyGetControlLoopIndex( control ));
		int newLoopCount = loopCount + 1;

		int loopsTotal = GrannyGetControlLoopCount( control );
		
		GrannySetControlLoopCount( control, newLoopCount );
		float remaining = GrannyGetControlDurationLeft( control );
		GrannySetControlLoopCount( control, loopsTotal );
		if( remaining > maxRemaining )
		{
			maxRemaining = remaining;
		}
	}

	return startTime + maxRemaining;
}

void Tr2GrannyAnimation::PostPhysicsAnimation( Be::Time time, const Matrix &modelTransform )
{
	return;
}

const std::string * Tr2GrannyAnimation::GetAnimationBoneList( unsigned int &numBones )
{
	numBones = (unsigned int)m_boneList.size();
	if( numBones )
	{
		return &m_boneList[0];
	}

	return NULL;
}

const Matrix* Tr2GrannyAnimation::GetAnimationTransforms()
{
	if( m_worldPose )
	{
		return reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4Array( m_worldPose ) );
	}

	return NULL;
}

void Tr2GrannyAnimation::Cleanup()
{
	if( m_modelInstance )
	{
		ClearAnimations();

		GrannyFreeModelInstance( m_modelInstance );
		m_modelInstance = nullptr;
	}

	GrannyFreeLocalPose( m_localPose );
	m_localPose = nullptr;

	GrannyFreeWorldPose( m_worldPose );
	m_worldPose = nullptr;

	GrannyFreeMeshBinding( m_meshBinding );
	m_meshBinding = nullptr;

	m_skeleton = nullptr;

	m_boneList.clear();

	CCP_ALIGNED_FREE( m_meshBoneMatrixList );
	m_meshBoneMatrixList = nullptr;
}

bool Tr2GrannyAnimation::FindBoneByName( const char* name, unsigned int& ix )
{
	if( m_skeleton )
	{
		granny_int32x boneIx;
		if( GrannyFindBoneByName( m_skeleton, name, &boneIx ) )
		{
			ix = boneIx;
			return true;
		}
	}

	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the number of bones used by the animated mesh
// --------------------------------------------------------------------------------------
int Tr2GrannyAnimation::GetMeshBoneCount() const
{
	return m_meshBoneCount;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a pointer to the internal list of 3x4 matrices, holding the transforms
//   of the current animation state
// --------------------------------------------------------------------------------------
const granny_matrix_3x4* Tr2GrannyAnimation::GetMeshBoneMatrixList() const
{
	return m_meshBoneMatrixList;
}

void Tr2GrannyAnimation::PlayAnimationOnce( const char* animName )
{
	PlayAnimation( animName, true, 1, 0.0f, 1.0f );
}

void Tr2GrannyAnimation::PlayAnimationEx( const char* animName, int loopCount, float delay, float speed )
{
	PlayAnimation( animName, true, loopCount, delay, speed );
}

void Tr2GrannyAnimation::ChainAnimation( const char* animName )
{
	PlayAnimation( animName, false, 1, 0.0f, 1.0f );
}

void Tr2GrannyAnimation::ChainAnimationEx( const char* animName, int loopCount, float delay, float speed )
{
	PlayAnimation( animName, false, loopCount, delay, speed );
}
