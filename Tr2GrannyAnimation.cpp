#include "StdAfx.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2VertexDefinitionUtilities.h"

static const int MAX_JOINT_COUNT = 58;

Tr2GrannyAnimation::Tr2GrannyAnimation( IRoot* lockobj ) :
	PARENTLOCK( m_boneOffset ),
	m_boneList( "Tr2GrannyAnimation/m_boneList" ),
	m_skeleton( nullptr ),
	m_worldPose( nullptr ),
	m_localPose( nullptr ),
	m_compositePose( nullptr ),
	m_meshBinding( nullptr ),
	m_meshBoneMatrixList( nullptr ),
	m_meshBoneCount( 0 ),
	m_useMeshBinding( false ),
	m_debugRenderSkeleton( false ),
	m_debugRenderJointNames( false ),
	m_baseLayer( 1.f ),
	m_modelIndex( 0 ),
	m_meshBindingIndex( -1 ),
	m_animationEnabled( true ),
	m_boneBoundsInitialized( false )
{
}

Tr2GrannyAnimation::~Tr2GrannyAnimation()
{
	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
	}	
}

granny_animation* Tr2GrannyAnimation::FindAnimationByName( const char* name ) const
{
	const granny_file_info* fi = GetFileInfo();
	if( !fi )
	{
		return nullptr;
	}

	int animIx = fi->AnimationCount;

	for( int i = 0; i < fi->AnimationCount ; ++i )
	{
		if( strcmp( fi->Animations[i]->Name, name ) == 0 )
		{
			animIx = i;
			break;
		}

	}
	if( animIx == fi->AnimationCount )
	{
		return nullptr;
	}

	return fi->Animations[animIx];
}

void Tr2GrannyAnimation::SetSharedGeometryRes( TriGeometryResPtr res )
{
	m_geometryRes = res;
	m_resPath = "<EveSpaceObject2>";
}

Tr2GrannyAnimationLayer* Tr2GrannyAnimation::GetAnimationLayer( const char* name )
{
	if( !name )
	{
		return &m_baseLayer;
	}

	auto it = m_animationLayers.find( name );
	if( it != m_animationLayers.end() )
	{
		return &it->second;
	}

	return nullptr;
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

	m_boneBounds.clear();
	
	return true;
}

void Tr2GrannyAnimation::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}

granny_file_info* Tr2GrannyAnimation::GetFileInfo() const
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
		m_modelIndex = 0;
		m_meshBindingIndex = -1;

		if( m_useMeshBinding )
		{
			m_modelIndex = -1;
			for( int i = 0; i < fi->ModelCount ; ++i )
			{
				for( int j = 0; j < fi->Models[i]->MeshBindingCount ; ++j )
				{
					if( fi->Models[i]->MeshBindings[j].Mesh == fi->Meshes[0] )
					{
						m_modelIndex = i;
						m_meshBindingIndex = j;
						break;
					}
				}
				if( m_modelIndex != -1 )
				{
					break;
				}
			}
		}
		else if( !m_model.empty() )
		{
			// A named model is specified - look for its index
			m_modelIndex = -1;

			for( int i = 0; i < fi->ModelCount ; ++i )
			{
				if( m_model == fi->Models[i]->Name )
				{
					m_modelIndex = i;
					break;
				}
			}
		}
		
		if( m_modelIndex != -1 )
		{
			m_baseLayer.InitializeAnimationLayer( this );
			m_skeleton = GrannyGetSourceSkeleton( m_baseLayer.m_modelInstance );
			m_worldPose = GrannyNewWorldPose( m_skeleton->BoneCount );
			m_localPose = GrannyNewLocalPose( m_skeleton->BoneCount );
			if( m_animationLayers.size() > 0 )
			{
				m_compositePose = GrannyNewLocalPose( m_skeleton->BoneCount );
			}

			for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
			{
				it->second.InitializeAnimationLayer( this );
			}

			if( m_meshBindingIndex != -1 )
			{
				m_meshBinding = GrannyNewMeshBinding ( fi->Models[ m_modelIndex ]->MeshBindings[ m_meshBindingIndex ].Mesh, m_skeleton, m_skeleton );
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
		m_skeleton		= nullptr;
		m_worldPose		= nullptr;
		m_localPose		= nullptr;
		m_compositePose	= nullptr;

		m_baseLayer.Cleanup();
		for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
		{
			it->second.Cleanup();
		}

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

	m_baseLayer.ConsumeAnimationQueue( this );
	for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
	{
		it->second.ConsumeAnimationQueue( this );
	}
}

bool Tr2GrannyAnimation::InitializeBoundingInfo()
{
	const granny_file_info* const fi = GetFileInfo();
	if( !fi )
	{
		return false;
	}

	if( fi->ModelCount == 0 )
	{
		return false;
	}

	if( !m_useMeshBinding )
	{
		return false;
	}

	m_boneBounds.clear();
	for( int i = 0; i < fi->ModelCount ; ++i )
	{
		for( int j = 0; j < fi->Models[i]->MeshBindingCount ; ++j )
		{
			granny_model* model = fi->Models[i];
			granny_model_mesh_binding& meshBinding = model->MeshBindings[j];
			granny_bone_binding* bb = meshBinding.Mesh->BoneBindings;
			for( int boneIdx = 0; boneIdx < meshBinding.Mesh->BoneBindingCount; boneIdx++ )
			{
				GrannyBoneBindingBounds bounds;
				GrannyFindBoneByName( m_skeleton, bb[boneIdx].BoneName, &bounds.m_boneIndex );

				Vector3 minBounds( bb[boneIdx].OBBMin );
				Vector3 maxBounds( bb[boneIdx].OBBMax );
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
	return true;
}

bool Tr2GrannyAnimation::GetDynamicBounds( Vector4& boundingSphere, Vector3 &aabbMin, Vector3 &aabbMax )
{
	Vector3 transformed[8];
	if( m_boneBounds.empty() && !InitializeBoundingInfo() )
	{
		return false;
	}

	BoundingSphereInitialize( boundingSphere );
	BoundingBoxInitialize( aabbMin, aabbMax );

	for( unsigned i = 0; i < m_boneBounds.size(); ++i )
	{
		const Matrix* mat = reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, m_boneBounds[i].m_boneIndex ) );

		for( unsigned point = 0; point < 8; point++ )
		{
			D3DXVec3TransformCoord( &transformed[point], &m_boneBounds[i].m_corners[point], mat );
			BoundingBoxUpdate( aabbMin, aabbMax, transformed[point] );
			BoundingSphereUpdate( transformed[point], boundingSphere );
		}
	}
	
	const granny_file_info* fi = GetFileInfo();
	if( fi )
	{
		aabbMin += Vector3( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
		aabbMax += Vector3( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
		boundingSphere.x += fi->Models[ m_modelIndex ]->InitialPlacement.Position[0];
		boundingSphere.y += fi->Models[ m_modelIndex ]->InitialPlacement.Position[1];
		boundingSphere.z += fi->Models[ m_modelIndex ]->InitialPlacement.Position[2];
	}
	return true;
}

void Tr2GrannyAnimation::RenderBones( const Matrix& modelTransform )
{
	Vector3 initialPlacement( 0, 0, 0 );
	const granny_file_info* fi = GetFileInfo();
	Matrix initialTranslation;
	if( fi )
	{
		initialPlacement = Vector3( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
	}
	D3DXMatrixTranslation( &initialTranslation, initialPlacement.x, initialPlacement.y, initialPlacement.z );
	
	for( int boneIdx = 0; boneIdx < m_meshBoneCount; boneIdx++ )
	{
		const int* bi = GrannyGetMeshBindingFromBoneIndices( m_meshBinding );
		Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bi[boneIdx] ) ) * modelTransform * initialTranslation;
		Vector4 pos(0, 0, 0, 1);
		D3DXVec4Transform( &pos, &pos, &mat );
		pos.w = 2;
		Tr2Renderer::DrawSphere( pos, 1, 0xffffffff );
		Tr2Renderer::Printf( TRI_DBG_FONT_SMALL, Vector3( pos.x, pos.y, pos.z ), 0xffffffff, "  %s : %d", m_skeleton->Bones[bi[boneIdx]].Name, boneIdx );
	}
}

void Tr2GrannyAnimation::RenderDynamicBounds( const Matrix& modelTransform )
{
	Vector3 transformed[8];
	if( m_boneBounds.empty() && !InitializeBoundingInfo() )
	{
		return;
	}

	Vector4 boundingSphere;
	bool initialized = false;

	Vector3 aabbMin, aabbMax;
	BoundingBoxInitialize( aabbMin, aabbMax );
	Vector3 initialPlacement( 0, 0, 0 );
	const granny_file_info* fi = GetFileInfo();
	Matrix initialTranslation;
	if( fi )
	{
		initialPlacement = Vector3( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
	}
	D3DXMatrixTranslation( &initialTranslation, initialPlacement.x, initialPlacement.y, initialPlacement.z );

	for( unsigned i = 0; i < m_boneBounds.size(); ++i )
	{
		
		Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, m_boneBounds[i].m_boneIndex ) ) * modelTransform * initialTranslation;

		for( unsigned point = 0; point < 8; point++ )
		{
			D3DXVec3TransformCoord( &transformed[point], &m_boneBounds[i].m_corners[point], &mat );
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

		Tr2Renderer::DrawLine( transformed[0], 0xff7f0000, transformed[2], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[0], 0xff7f0000, transformed[5], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[5], 0xff7f0000, transformed[6], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[2], 0xff7f0000, transformed[6], 0xff7f0000 );
						
		Tr2Renderer::DrawLine( transformed[1], 0xff7f0000, transformed[7], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[1], 0xff7f0000, transformed[4], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[3], 0xff7f0000, transformed[4], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[3], 0xff7f0000, transformed[7], 0xff7f0000 );

		Tr2Renderer::DrawLine( transformed[1], 0xff7f0000, transformed[6], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[0], 0xff7f0000, transformed[3], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[7], 0xff7f0000, transformed[5], 0xff7f0000 );
		Tr2Renderer::DrawLine( transformed[4], 0xff7f0000, transformed[2], 0xff7f0000 );
	}

	Tr2Renderer::DrawSphere( boundingSphere, 8, 0xffff0000 );
	Tr2Renderer::DrawBox( aabbMin, aabbMax, 0xffff0000 );

	return;
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

bool Tr2GrannyAnimation::IsAnimationEnabled() const
{
	return m_animationEnabled;
}

void Tr2GrannyAnimation::SetAnimationEnabled( bool enabled )
{
	m_animationEnabled = enabled;
}

const std::string& Tr2GrannyAnimation::GetModel() const
{
	return m_model;
}

bool Tr2GrannyAnimation::CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi )
{
	Matrix m;
	PrePhysicsAnimation( 0, *D3DXMatrixIdentity( &m ) );

	if( !m_meshBinding )
	{
		return false;
	}

	if( !fi )
	{
		fi = GetFileInfo();
	}
	if( !fi )
	{
		return false;
	}

    // Known input and output vertex format
    granny_mesh* mesh = fi->Models[ m_modelIndex ]->MeshBindings[ m_meshBindingIndex ].Mesh;
    granny_int32x vertCount = GrannyGetMeshVertexCount( mesh );

    std::vector<granny_pwn313_vertex> sourceVerts( vertCount );
    std::vector<granny_pn33_vertex> deformedVerts( vertCount );

    GrannyCopyMeshVertices(mesh, GrannyPWN313VertexType, &sourceVerts[0]);

	granny_mesh_deformer* deformer = GrannyNewMeshDeformer(GrannyPWN313VertexType,
                                                               GrannyPN33VertexType,
                                                               GrannyDeformPositionNormal,
                                                               GrannyDontAllowUncopiedTail);
	if( !deformer )
	{
		return false;
	}

	GrannyDeformVertices(deformer, GrannyGetMeshBindingFromBoneIndices( m_meshBinding ),
                        (granny_real32*)GrannyGetWorldPoseComposite4x4Array( m_worldPose ), vertCount, &sourceVerts[0], &deformedVerts[0] );

	// Get the transformed bounding box
	bbMin = Vector3(  FLT_MAX,  FLT_MAX,  FLT_MAX );
	bbMax = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    for (int v = 0; v < vertCount; ++v)
    {
		Vector4 pos( 
			deformedVerts[v].Position[0] + fi->Models[ m_modelIndex ]->InitialPlacement.Position[0], 
			deformedVerts[v].Position[1] + fi->Models[ m_modelIndex ]->InitialPlacement.Position[1], 
			deformedVerts[v].Position[2] + fi->Models[ m_modelIndex ]->InitialPlacement.Position[2], 1 );

		D3DXVec4Transform( &pos, &pos, &transform );
		pos /= pos.w;

        bbMin.x = min(bbMin.x, pos.x);
        bbMax.x = max(bbMax.x, pos.x);
		
		bbMin.y = min(bbMin.y, pos.y);
        bbMax.y = max(bbMax.y, pos.y);
		
		bbMin.z = min(bbMin.z, pos.z);
        bbMax.z = max(bbMax.z, pos.z);
    }

	return true;
}

Vector4 Tr2GrannyAnimation::CalculateSkinnedBoundingSphere( granny_file_info* fi )
{
	Matrix m;
	PrePhysicsAnimation( 0, *D3DXMatrixIdentity( &m ) );

	if( !m_meshBinding )
	{
		return Vector4( 0.f, 0.f, 0.f, -1.f );
	}

	if( !fi )
	{
		fi = GetFileInfo();
	}
	if( !fi )
	{
		return Vector4( 0.f, 0.f, 0.f, -1.f );
	}

    // Known input and output vertex format
    granny_mesh* mesh = fi->Models[ m_modelIndex ]->MeshBindings[ m_meshBindingIndex ].Mesh;
    granny_int32x vertCount = GrannyGetMeshVertexCount( mesh );

    std::vector<granny_pwn313_vertex> sourceVerts( vertCount );
    std::vector<granny_pn33_vertex> deformedVerts( vertCount );

    GrannyCopyMeshVertices( mesh, GrannyPWN313VertexType, &sourceVerts[0] );

	granny_mesh_deformer* deformer = GrannyNewMeshDeformer(GrannyPWN313VertexType,
                                                               GrannyPN33VertexType,
                                                               GrannyDeformPositionNormal,
                                                               GrannyDontAllowUncopiedTail);
	if( !deformer )
	{
		return Vector4( 0.f, 0.f, 0.f, -1.f );
	}

	GrannyDeformVertices(deformer, GrannyGetMeshBindingFromBoneIndices( m_meshBinding ),
                        (granny_real32*)GrannyGetWorldPoseComposite4x4Array( m_worldPose ), vertCount, &sourceVerts[0], &deformedVerts[0] );

	// Gather a list of pointers to the positions
	std::vector<const Vector3*> points( vertCount );
	for( int32_t p = 0; p < vertCount; ++p )
	{
		points[p] = (const Vector3*)&deformedVerts[p].Position;
	}

	// Calculate the bounding sphere
	Vector4 boundingSphere;
	BoundingSphereFromPoints( boundingSphere, &points[0], points.size() );
	
	return boundingSphere;
}


granny_model* Tr2GrannyAnimation::GetGrannyModel() const
{
	granny_file_info* fi = GetFileInfo();
	if( !fi )
	{
		return nullptr;
	}

	return fi->Models[m_modelIndex];
}

void Tr2GrannyAnimation::SetModel( const std::string& val )
{
	m_model = val;
	Initialize();
}

bool Tr2GrannyAnimation::PlayAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone )
{
	return PlayLayerAnimationByName( nullptr, animName, replace, loopCount, delay, speed, clearWhenDone );
}

bool Tr2GrannyAnimation::PlayLayerAnimationByName( const char* layerName, const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone )
{
	Tr2GrannyAnimationLayer* layer = GetAnimationLayer( layerName );
	if( !layer )
	{
		return false;
	}

	if( ( !m_grannyRes  && !m_geometryRes )  ||		
		( m_grannyRes	&& !m_grannyRes->IsPrepared() ) ||
		( m_geometryRes	&& !m_geometryRes->IsPrepared() ) )
	{
		layer->QueueAnimation( animName, replace, loopCount, delay, speed, clearWhenDone );
		return true;
	}

	if( ( ( m_grannyRes && !m_grannyRes->IsGood() ) ||
		( m_geometryRes && !m_geometryRes->IsGood() ) ) )
	{
		CCP_LOGERR( "Animation resource failed to load!" );
		return false;
	}

	return layer->PlayAnimation( this, animName, replace, loopCount, delay, speed, clearWhenDone );
}

void Tr2GrannyAnimation::EndAnimation()
{
	m_baseLayer.EndAnimation();
}


void Tr2GrannyAnimation::ClearAnimations()
{
	m_baseLayer.ClearAnimations();
}

void Tr2GrannyAnimation::PrePhysicsAnimation( Be::Time time, const Matrix &modelTransform )
{
	if( IsInitialized() && m_animationEnabled )
	{
		float animationTime = Tr2Renderer::GetAnimationTime();

		// TODO: Should this be done here? Seems wasteful to sample animations and build the pose
		// for objects that are off-screen.
		m_baseLayer.SampleAnimation( animationTime, m_localPose, m_eventListener );
		for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
		{
			it->second.SampleAnimation( animationTime, m_compositePose, m_localPose, m_eventListener );
		}

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
	return m_baseLayer.GetAnimationChainCompleteTime();
}

float Tr2GrannyAnimation::GetAnimationChainCompleteTimeForLayer( const char* layerName )
{
	Tr2GrannyAnimationLayer* layer = GetAnimationLayer( layerName );
	if( !layer )
	{
		return 0;
	}
	return layer->GetAnimationChainCompleteTime();
}

void Tr2GrannyAnimation::PostPhysicsAnimation( Be::Time time, const Matrix& modelTransform )
{
	return;
}

const std::string * Tr2GrannyAnimation::GetAnimationBoneList( unsigned int& numBones ) const
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
	m_baseLayer.Cleanup();
	for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
	{
		it->second.Cleanup();
	}
	
	GrannyFreeLocalPose( m_localPose );
	m_localPose = nullptr;
	
	GrannyFreeLocalPose( m_compositePose );
	m_compositePose = nullptr;

	GrannyFreeWorldPose( m_worldPose );
	m_worldPose = nullptr;

	GrannyFreeMeshBinding( m_meshBinding );
	m_meshBinding = nullptr;

	m_skeleton = nullptr;

	m_boneList.clear();

	CCP_ALIGNED_FREE( m_meshBoneMatrixList );
	m_meshBoneMatrixList = nullptr;
}

bool Tr2GrannyAnimation::FindBoneByName( const char* name, unsigned int& ix ) const
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

bool Tr2GrannyAnimation::IsInitialized() const
{
	return m_baseLayer.m_modelInstance != nullptr;
}

void Tr2GrannyAnimation::AddAnimationLayer( const char* layerName, float layerWeight )
{
	if( GetAnimationLayer( layerName ) )
	{
		return;
	}

	Tr2GrannyAnimationLayer layer( 0.f, layerWeight );
	layer.m_name = layerName;
	m_animationLayers[layerName] = layer;

	if( !IsInitialized() )
	{
		return;
	}
	
	if( !m_compositePose )
	{
		m_compositePose = GrannyNewLocalPose( m_skeleton->BoneCount );
	}
	GetAnimationLayer( layerName )->InitializeAnimationLayer( this );
}

float Tr2GrannyAnimation::GetLayerWeight(const char* layerName)
{
	if ( GetAnimationLayer( layerName ) )
	{
		return GetAnimationLayer( layerName )->GetLayerWeight();
	}
	return 0.f;
}

void Tr2GrannyAnimation::SetLayerWeight( const char* layerName, float layerWeight )
{
	if ( GetAnimationLayer( layerName ) )
	{
		return GetAnimationLayer( layerName )->SetLayerWeight( layerWeight );
	}
}

void Tr2GrannyAnimation::AddAnimationLayerWithTrackMask( const char* layerName, const char* trackMask )
{
	if( GetAnimationLayer( layerName ) )
	{
		GetAnimationLayer( layerName )->ExtractTrackMask( this, trackMask );
		return;
	}

	Tr2GrannyAnimationLayer layer;
	layer.m_name = layerName;
	layer.ExtractTrackMask( this, trackMask );
	m_animationLayers[layerName] = layer;

	if( !IsInitialized() )
	{
		return;
	}
	
	if( !m_compositePose )
	{
		m_compositePose = GrannyNewLocalPose( m_skeleton->BoneCount );
	}
	GetAnimationLayer( layerName )->InitializeAnimationLayer( this );

	return;
}

void Tr2GrannyAnimation::AddAnimationLayerBone( const char* layerName, const char* boneName )
{
	Tr2GrannyAnimationLayer* layer = GetAnimationLayer( layerName );
	if( !layer )
	{
		return;
	}

	layer->AddBone( this, boneName );
}

void Tr2GrannyAnimation::RemoveAnimationLayerBone( const char* layerName, const char* boneName )
{
	Tr2GrannyAnimationLayer* layer = GetAnimationLayer( layerName );
	if( !layer )
	{
		return;
	}

	layer->RemoveBone( this, boneName );
}

namespace
{

template <typename T>
bool CopyIndices( Tr2IndexBufferAL& from, CcpMallocBuffer& to, uint32_t offset )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto ibOffset = to.size();
	to.resize( "ib", to.size() + from.GetNumIndices() * 4 );
	if( !to.get() )
	{
		return false;
	}

	T* src;
	if( FAILED( from.Lock( src, Tr2RenderContextEnum::LOCK_READONLY, renderContext ) ) )
	{
		return false;
	}

	uint32_t* dest = reinterpret_cast<uint32_t*>( to.get() + ibOffset );
	for( uint32_t j = 0; j < from.GetNumIndices(); ++j )
	{
		*dest++ = offset + *src++;
	}
	from.Unlock( renderContext );
	return true;
}

}

std::pair<TriGeometryRes*, std::map<std::pair<TriGeometryRes*, uint32_t>, uint32_t>> Tr2GrannyAnimation::CreateStaticGeometry( std::vector<TriGeometryRes*> grannies )
{
	PrePhysicsAnimation( 0, Tr2Renderer::GetIdentityTransform() );

	std::map<std::pair<TriGeometryRes*, uint32_t>, uint32_t> result;

	if( grannies.empty() )
	{
		return std::make_pair( nullptr, result );
	}

	TriGeometryResPtr newGeometry;
	newGeometry.CreateInstance();

	if( !newGeometry )
	{
		return std::make_pair( nullptr, result );
	}

	std::vector<TriGeometryResAreaData> areas;

	CcpMallocBuffer vbMirror;
	CcpMallocBuffer ibMirror;

	std::vector<granny_data_type_definition> layout;
	uint32_t vertexSize = 0;

	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto pose = GrannyGetWorldPoseComposite4x4Array( m_worldPose );

	for( auto it = grannies.begin(); it != grannies.end(); ++it )
	{
		auto geometry = *it;
		auto granny = geometry->m_sourceGranny;
		if( !granny )
		{
			result.clear();
			return std::make_pair( nullptr, result );
		}

		granny_file_info* fi = GrannyGetFileInfo( granny->GetGrannyFile() );
		if( !fi )
		{
			result.clear();
			return std::make_pair( nullptr, result );
		}

		auto count = geometry->GetMeshCount();
		for( unsigned i = 0; i < count; ++i )
		{
			if( layout.empty() )
			{
				auto grannyVertexDecl = fi->Meshes[i]->PrimaryVertexData->VertexType;
				while( grannyVertexDecl->Type != GrannyEndMember )
				{
					if( strcmp( grannyVertexDecl->Name, GrannyVertexBoneIndicesName ) && 
						strcmp( grannyVertexDecl->Name, GrannyVertexBoneWeightsName ) )
					{
						layout.push_back( *grannyVertexDecl );
					}
					++grannyVertexDecl;
				}
				layout.push_back( *grannyVertexDecl );
				vertexSize = GrannyGetTotalObjectSize( &layout[0] );
			}

			auto binding = GrannyNewMeshBinding( fi->Meshes[i], m_skeleton, m_skeleton );
			ON_BLOCK_EXIT( [&] { GrannyFreeMeshBinding( binding ); } );
			auto indices = GrannyGetMeshBindingFromBoneIndices( binding );

			auto deformer = GrannyNewMeshDeformer( 
				fi->Meshes[i]->PrimaryVertexData->VertexType, 
				&layout[0], 
				GrannyDeformPositionNormalTangentBinormal, 
				GrannyDontAllowUncopiedTail );
			ON_BLOCK_EXIT( [&] { GrannyFreeMeshDeformer( deformer ); } );

			auto data = geometry->GetMeshData( i );

			auto vbOffset = vbMirror.size();
			vbMirror.resize( "vbMirror", vbMirror.size() + vertexSize * data->m_vertexCount );

			void* locked;
			if( FAILED( data->m_vertexBuffer.Lock( 0, 0, &locked, Tr2RenderContextEnum::LOCK_READONLY, renderContext ) ) )
			{
				result.clear();
				return std::make_pair( nullptr, result );
			}
			GrannyDeformVertices( 
				deformer, 
				indices, 
				reinterpret_cast<const granny_real32*>( pose ), 
				data->m_vertexCount, 
				locked, 
				vbMirror.get() + vbOffset );
			data->m_vertexBuffer.Unlock( renderContext );

			auto ibOffset = ibMirror.size();

			if( data->m_indexBuffer.GetIBBitcount() == Tr2RenderContextEnum::IB_16BIT )
			{
				if( !CopyIndices<uint16_t>( data->m_indexBuffer, ibMirror, uint32_t( vbOffset ) / vertexSize ) )
				{
					result.clear();
					return std::make_pair( nullptr, result );
				}
			}
			else
			{
				if( !CopyIndices<uint32_t>( data->m_indexBuffer, ibMirror, uint32_t( vbOffset ) / vertexSize ) )
				{
					result.clear();
					return std::make_pair( nullptr, result );
				}
			}

			result[std::make_pair( *it, i )] = uint32_t( areas.size() );

			for( auto ai = data->m_areas.begin(); ai != data->m_areas.end(); ++ai )
			{
				TriGeometryResAreaData area( *ai );
				area.m_firstIndex += int( ibOffset ) / 4;
				areas.push_back( area );
			}
		}
	}

	Tr2VertexBufferAL vb;
	vb.Create( 
		uint32_t( vbMirror.size() ), 
		Tr2RenderContextEnum::USAGE_IMMUTABLE, 
		vbMirror.get(), 
		renderContext );

	Tr2IndexBufferAL ib;
	ib.Create( 
		uint32_t( ibMirror.size() / 4 ), 
		Tr2RenderContextEnum::USAGE_IMMUTABLE, 
		Tr2RenderContextEnum::IB_32BIT, 
		ibMirror.get(), 
		renderContext ); 

	auto declaration = Tr2EffectStateManager::GetVertexDeclarationHandle( 
		BuildFromGrannyVertexDecl( &layout[0] ) );

	newGeometry->PrepareFromBuffers( 
		std::move( vb ), 
		std::move( ib ), 
		declaration, 
		vertexSize, 
		&areas.front(), 
		areas.size() );

	return std::make_pair( newGeometry.Detach(), result );
}
