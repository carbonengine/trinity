#include "StdAfx.h"
#include "Miniball3.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"

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
	m_meshBindingIndex( -1 )
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
		const Matrix* mat = (const Matrix*)GrannyGetWorldPose4x4( m_worldPose, m_boneBounds[i].m_boneIndex );

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
		
		Matrix mat = *(const Matrix*)GrannyGetWorldPose4x4( m_worldPose, m_boneBounds[i].m_boneIndex ) * modelTransform * initialTranslation;

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
		return Vector4(0,0,0,-1);
	}

	if( !fi )
	{
		fi = GetFileInfo();
	}
	if( !fi )
	{
		return Vector4(0,0,0,-1);
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
		return Vector4(0,0,0,-1);
	}

	GrannyDeformVertices(deformer, GrannyGetMeshBindingFromBoneIndices( m_meshBinding ),
                        (granny_real32*)GrannyGetWorldPoseComposite4x4Array( m_worldPose ), vertCount, &sourceVerts[0], &deformedVerts[0] );

	// Calculate the bounding sphere
	Vector4 boundingSphere;

	typedef double* const* PointIterator; 
	typedef const double* CoordIterator;
	typedef Miniball::Miniball <Miniball::CoordAccessor<PointIterator, CoordIterator>> MB;

	// Figure out how many points
	double** samplePoints = new double*[vertCount];		

	// Add points to the bounding object. We'll get duplicates, but this is ok.
	for (int v = 0; v < vertCount; ++v)
    {
        double* point = new double[3];
		point[0] = deformedVerts[v].Position[0];
		point[1] = deformedVerts[v].Position[1];
		point[2] = deformedVerts[v].Position[2];
		samplePoints[v] = point;
    }

	MB mb( 3, samplePoints, samplePoints + vertCount );	
	const double* boundingSphereCenter = mb.center();

	boundingSphere.x = float( boundingSphereCenter[0] ) + fi->Models[ m_modelIndex ]->InitialPlacement.Position[0];
	boundingSphere.y = float( boundingSphereCenter[1] ) + fi->Models[ m_modelIndex ]->InitialPlacement.Position[1];
	boundingSphere.z = float( boundingSphereCenter[2] ) + fi->Models[ m_modelIndex ]->InitialPlacement.Position[2];
	boundingSphere.w = float( sqrt( mb.squared_radius() ) );

	for( int j=0; j< vertCount; ++j )
	{
		delete[] samplePoints[j];
	}
	delete[] samplePoints;
	
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
	if( IsInitialized() )
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

void Tr2GrannyAnimation::AddAnimationLayer( const char* layerName )
{
	if( GetAnimationLayer( layerName ) )
	{
		return;
	}

	Tr2GrannyAnimationLayer layer;
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