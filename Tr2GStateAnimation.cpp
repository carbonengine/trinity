#include "StdAfx.h"
#include "Tr2GStateAnimation.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/Tr2GrannyStateRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "gstate_character_instance.h"
#include "gstate_transition.h"
#include <algorithm>

using namespace gstate;

static const int MAX_JOINT_COUNT = 58;

Tr2GStateAnimation::Tr2GStateAnimation(IRoot* lockobj) :
	PARENTLOCK(m_boneOffset),
	m_skeleton( nullptr ),
	m_boneList("Tr2GStateAnimation/m_boneList"),
	m_worldPose( nullptr ),
	m_meshBinding( nullptr ),
	m_localPose( nullptr ),
	m_gStateCharacterInstance( nullptr ),
	m_state_machine( nullptr ),
	m_compositePose( nullptr ),
	m_gstate_pose_cache( nullptr ),
	m_meshBoneMatrixList( nullptr ),
	m_meshBoneCount( 0 ),
	m_debugRenderSkeleton( false ),
	m_useMeshBinding( false ),
	m_debugRenderJointNames( false ),
	m_modelInstance( nullptr ),
	m_modelIndex( 0 ),
	m_meshBindingIndex( -1 ),
	m_animationEnabled( true ),
	m_last_gstate_time( 0.0 ),
	m_boneBoundsInitialized( false ),
	m_paused( false ),
	m_pauseTime( 0.0 ),
	m_totalPauseOffset( 0.0 )
{
	m_callbackData.gsf_path = "";
	m_callbackData.anim_map_pointer = nullptr;
}

Tr2GStateAnimation::~Tr2GStateAnimation()
{
	Cleanup();

	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
	}

	if ( m_gStateRes )
	{
		m_gStateRes->RemoveNotifyTarget( this ); 
	}
}


void Tr2GStateAnimation::SetSharedGeometryRes( TriGeometryResPtr res )
{
	m_geometryRes = res;
	m_resPath = "<EveSpaceObject2>";
}


bool Tr2GStateAnimation::Initialize()
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


granny_file_info* GStateAnimationBindingCallback(gstate_character_info *BindingInfo,
															char const* SourceFilename,
															void* UserData)
{
	auto callbackData = *(static_cast<GStateBindingCallbackData *>(UserData));
	std::map<std::string, TriGrannyResPtr> *gStateAnimFilesPtr = callbackData.anim_map_pointer;

	// Animation granny files are relative to the parent directory of the .gsf file
	std::string SourceFilenameString = SourceFilename;
	std::string dir_path = callbackData.gsf_path;

	auto last_delimiter_pos = dir_path.find_last_of( "/\\" );
	dir_path.erase( last_delimiter_pos, dir_path.length() - last_delimiter_pos);

	while (SourceFilenameString.substr(0, 2) == "..")
	{
		SourceFilenameString.erase(0, 3);
		last_delimiter_pos = dir_path.find_last_of("/\\");
		dir_path.erase(last_delimiter_pos, dir_path.length() - last_delimiter_pos);
	}

	if (SourceFilenameString.substr(0, 1) == ".")
	{
		SourceFilenameString.erase(0, 2);
	}

	dir_path += "/";
	dir_path += SourceFilenameString;
	SourceFilenameString = dir_path;

	std::replace(SourceFilenameString.begin(), SourceFilenameString.end(), '\\', '/');

	TriGrannyResPtr result_granny_res = nullptr;

	for ( auto it = gStateAnimFilesPtr->begin(); it != gStateAnimFilesPtr->end(); it++ )
	{
		
		std::string iter_file_name = it->first;
		std::replace(iter_file_name.begin(), iter_file_name.end(), '\\', '/');

		if ( iter_file_name == SourceFilenameString )
		{
			result_granny_res = it->second;
		}
	}

	if ( result_granny_res != nullptr )
	{
		return result_granny_res->ValidateFileInfo();
	}

	CCP_LOGERR("GState Binding Step: '%s' is required by the GState file and is not loaded", SourceFilenameString.c_str());

	return nullptr;
}

void Tr2GStateAnimation::LoadGStateResPath( const std::string& val )
{
	if ( m_gStateRes )
	{
		m_gStateRes->RemoveNotifyTarget( this );
		m_gStateRes.Unlock();
	}

	BeResMan->GetResource( val.c_str(), "raw", BlueInterfaceIID<Tr2GrannyStateRes>(), (void**)&m_gStateRes );

	if ( m_gStateRes )
	{
		m_gStateRes->AddNotifyTarget( this );
	}

}

void Tr2GStateAnimation::LoadAnimResPath( const std::string& val )
{
	auto it = m_gStateAnimFiles.find(val);
	if ( it != m_gStateAnimFiles.end() )
	{
		it->second->RemoveNotifyTarget(this);
		it->second.Unlock();
	}

	TriGrannyResPtr granny_res;
	BeResMan->GetResource( val.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&granny_res );

	m_gStateAnimFiles[ val ] = granny_res;

	if ( granny_res )
	{
		granny_res->AddNotifyTarget(this);
	}
}

void Tr2GStateAnimation::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}

granny_file_info* Tr2GStateAnimation::GetFileInfo() const
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


void Tr2GStateAnimation::RebuildCachedData( BlueAsyncRes* p )
{
	if ( p != m_grannyRes && p != m_geometryRes )
	{
		for ( auto it = m_gStateAnimFiles.begin(); it != m_gStateAnimFiles.end(); it++ )
		{
			if ( p == it->second )
			{
				if ( it->second && !it->second->GetGrannyFile() )
				{
					CCP_LOGERR("'%s' not found or not a valid Granny file", it->first.c_str());
				}
				return;
			}
		}

		if ( p == m_gStateRes )
		{
			gstate_character_info *character_info = m_gStateRes->GetCharacterInfo();
			m_callbackData.gsf_path = m_gStateResPath;
			m_callbackData.anim_map_pointer = &m_gStateAnimFiles;
			if (!GStateBindCharacterFileReferences(character_info, static_cast<gstate_file_ref_callback*>(GStateAnimationBindingCallback), static_cast<void*>(&m_callbackData)))
			{
				CCP_LOGERR("'%s' Granny State file refers to invalid or unavailable animation.", m_gStateResPath.c_str());
			}

			m_gStateCharacterInstance = GStateInstantiateCharacter(character_info, GetAnimationTime(), 0, GetGrannyModel());
			m_state_machine = GStateGetStateMachine(m_gStateCharacterInstance);

			if ( !m_gstate_pose_cache )
			{
				m_gstate_pose_cache = GrannyNewPoseCache();
			}
		}
	}

	if( !m_grannyRes && !m_geometryRes )
	{
		return;
	}

	if( m_grannyRes && !m_grannyRes->GetGrannyFile() )
	{
		CCP_LOGERR( "'%s' not found or not a valid Granny file", m_resPath.c_str() );
		return;
	}

	if ( m_gStateRes && !m_gStateRes->GetCharacterFile() )
	{
		CCP_LOGERR("'%s' not found or not a valid GState file", m_gStateResPath.c_str());
		return;
	}


	const granny_file_info* const fi = GetFileInfo();
	if( !fi )
	{
		return;
	}


	if( fi->ModelCount > 0 )
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

		if (m_modelIndex != -1)
		{
			granny_model *model = GetGrannyModel();
			m_modelInstance = GrannyInstantiateModel(model);
			m_skeleton = GrannyGetSourceSkeleton(m_modelInstance);
			m_worldPose = GrannyNewWorldPose(m_skeleton->BoneCount);

			if (m_meshBindingIndex != -1)
			{
				m_meshBinding = GrannyNewMeshBinding(fi->Models[m_modelIndex]->MeshBindings[m_meshBindingIndex].Mesh, m_skeleton, m_skeleton);
			}
			else
			{
				m_meshBinding = nullptr;
			}

			for (int i = 0; i < m_skeleton->BoneCount; ++i)
			{
				m_boneList.push_back(m_skeleton->Bones[i].Name);
			}

			if (m_meshBinding)
			{
				m_meshBoneCount = GrannyGetMeshBindingBoneCount(m_meshBinding);
				if (m_meshBoneCount)
				{
					if (m_meshBoneCount >= MAX_JOINT_COUNT)
					{
						m_meshBoneCount = MAX_JOINT_COUNT;
					}
					m_meshBoneMatrixList = (granny_matrix_3x4*)CCP_ALIGNED_MALLOC("Tr2GrannyAnimation/m_boneMatrixList", m_meshBoneCount * sizeof(granny_matrix_3x4), 16);
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
}

bool Tr2GStateAnimation::InitializeBoundingInfo()
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
	return true;
}

tokenized* Tr2GStateAnimation::GetActiveMachineElement()
{
	tokenized *element = m_state_machine->GetActiveElement();

	return element;
}

const std::string Tr2GStateAnimation::GetActiveMachineElementName()
{
	tokenized *active = GetActiveMachineElement();
	assert(active);

	node *curr_node = GSTATE_DYNCAST(active, node);
	if (curr_node)
	{
		char const* curr_name = curr_node->GetName();
		return std::string(curr_name);
	}
	else
	{
		assert(GSTATE_DYNCAST(active, transition));
		transition *curr_transition = GSTATE_DYNCAST(active, transition);
		if (curr_transition)
		{
			char const* curr_transition_name = curr_transition->GetName();
			return std::string(curr_transition_name);
		}
	}

	return "";
}

int Tr2GStateAnimation::GetStartStateIdx()
{
	return m_state_machine->GetStartStateIdx();
}

void Tr2GStateAnimation::SetStartStateIdx(int StartState)
{
	m_state_machine->SetStartStateIdx(StartState);
}

bool Tr2GStateAnimation::RequestChangeToState( const std::string& name )
{
	return m_state_machine->RequestChangeToState(GStateInstanceTime(m_gStateCharacterInstance), 
												 GStateInstanceLastDeltaT(m_gStateCharacterInstance), name.c_str());
}

bool Tr2GStateAnimation::StartTransitionByName( const std::string& name )
{
	return m_state_machine->StartTransitionByName(GStateInstanceTime(m_gStateCharacterInstance), name.c_str());
}

float Tr2GStateAnimation::GetParameter( const std::string& param_node_name, granny_int32x param_idx )
{
	node *curr_param = m_state_machine->FindChildByName(param_node_name.c_str());
	parameters* param_set = GSTATE_DYNCAST(curr_param, parameters);
	assert(param_set);

	return param_set->GetRequestedParam( param_idx );
}

void Tr2GStateAnimation::SetParameter( const std::string& param_node_name, granny_int32x param_idx, float value )
{
	node *curr_param = m_state_machine->FindChildByName(param_node_name.c_str());
	parameters* param_set = GSTATE_DYNCAST(curr_param, parameters);
	assert(param_set);

	param_set->SetParameter( param_idx, value );
}

void Tr2GStateAnimation::RequestParameter( const std::string& param_node_name, granny_int32x param_idx, float value )
{
	node *curr_param = m_state_machine->FindChildByName(param_node_name.c_str());
	parameters* param_set = GSTATE_DYNCAST(curr_param, parameters);
	assert(param_set);

	param_set->RequestParameter( param_idx, value );
}


granny_int32x Tr2GStateAnimation::GetParameterIndexByName( const std::string& param_node_name, const std::string& param_name )
{
	node *curr_param = m_state_machine->FindChildByName(param_node_name.c_str());
	parameters* param_set = GSTATE_DYNCAST(curr_param, parameters);
	assert(param_set);

	int num_params = param_set->GetNumOutputs();
	for ( auto count = 0; count < num_params; count++ )
	{
		if ( !param_name.compare( param_set->GetOutputName( count )) )
		{
			return count;
		}
	}
	return -1;
}


bool Tr2GStateAnimation::GetDynamicBounds( Vector4& boundingSphere, Vector3 &aabbMin, Vector3 &aabbMax )
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
			transformed[point] = TransformCoord( m_boneBounds[i].m_corners[point], *mat );
			BoundingBoxUpdate( aabbMin, aabbMax, transformed[point] );
			BoundingSphereUpdate( transformed[point], boundingSphere );
		}
	}
	
	const granny_file_info* fi = GetFileInfo();
	if( fi )
	{
		aabbMin += *reinterpret_cast<Vector3*>( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
		aabbMax += *reinterpret_cast<Vector3*>( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
		boundingSphere.x += fi->Models[ m_modelIndex ]->InitialPlacement.Position[0];
		boundingSphere.y += fi->Models[ m_modelIndex ]->InitialPlacement.Position[1];
		boundingSphere.z += fi->Models[ m_modelIndex ]->InitialPlacement.Position[2];
	}
	return true;
}

void Tr2GStateAnimation::RenderBones( const Matrix& modelTransform )
{
	if( !m_meshBinding )
	{
		return;
	}

	Vector3 initialPlacement( 0, 0, 0 );
	const granny_file_info* fi = GetFileInfo();
	Matrix initialTranslation;
	if( fi )
	{
		initialPlacement = *reinterpret_cast<Vector3*>( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
	}
	initialTranslation = TranslationMatrix( initialPlacement );
	
	for( int boneIdx = 0; boneIdx < m_meshBoneCount; boneIdx++ )
	{
		const int* bi = GrannyGetMeshBindingFromBoneIndices( m_meshBinding );
		Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bi[boneIdx] ) ) * modelTransform * initialTranslation;
		Vector4 pos(0, 0, 0, 1);
		pos = Transform( pos, mat );
		pos.w = 2;
		Tr2Renderer::DrawSphere( pos, 1, 0xffffffff );
		Tr2Renderer::Printf( TRI_DBG_FONT_SMALL, Vector3( pos.x, pos.y, pos.z ), 0xffffffff, "  %s : %d", m_skeleton->Bones[bi[boneIdx]].Name, boneIdx );
	}
}

void Tr2GStateAnimation::RenderDynamicBounds( const Matrix& modelTransform )
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
		initialPlacement = *reinterpret_cast<Vector3*>( fi->Models[ m_modelIndex ]->InitialPlacement.Position );
	}
	initialTranslation = TranslationMatrix( initialPlacement );

	for( unsigned i = 0; i < m_boneBounds.size(); ++i )
	{
		
		Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, m_boneBounds[i].m_boneIndex ) ) * modelTransform * initialTranslation;

		for( unsigned point = 0; point < 8; point++ )
		{
			transformed[point] = TransformCoord( m_boneBounds[i].m_corners[point], mat );
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

const std::string& Tr2GStateAnimation::GetResPath() const
{
	return m_resPath;
}

void Tr2GStateAnimation::SetResPath( const std::string& val )
{
	m_resPath = val;
	Initialize();
}

const std::string& Tr2GStateAnimation::GetGStateResPath() const
{
	return m_gStateResPath;
}

void Tr2GStateAnimation::SetGStateResPath(const std::string& val)
{
	m_gStateResPath = val;
	LoadGStateResPath(val);
}


bool Tr2GStateAnimation::IsAnimationEnabled() const
{
	return m_animationEnabled;
}

void Tr2GStateAnimation::SetAnimationEnabled( bool enabled )
{
	m_animationEnabled = enabled;
}

const std::string& Tr2GStateAnimation::GetModel() const
{
	return m_model;
}

bool Tr2GStateAnimation::CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi )
{
	PrePhysicsAnimation( 0, IdentityMatrix() );

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

		pos = Transform( pos, transform );
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

Vector4 Tr2GStateAnimation::CalculateSkinnedBoundingSphere( granny_file_info* fi )
{
	PrePhysicsAnimation( 0, IdentityMatrix() );

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


granny_model* Tr2GStateAnimation::GetGrannyModel() const
{
	granny_file_info* fi = GetFileInfo();
	if( !fi )
	{
		return nullptr;
	}

	return fi->Models[m_modelIndex];
}


void Tr2GStateAnimation::SetModel( const std::string& val )
{
	m_model = val;
	Initialize();
}


void Tr2GStateAnimation::ApplyBoneOffsets(unsigned i)
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

void Tr2GStateAnimation::PrePhysicsAnimation( Be::Time time, const Matrix &modelTransform )
{
	if( IsInitialized() && m_animationEnabled )
	{
		float animationTime = GetAnimationTime();

		// TODO: Should this be done here? Seems wasteful to sample animations and build the pose
		// for objects that are off-screen.

		granny_real32 timeDelta = animationTime - m_last_gstate_time;
		m_last_gstate_time = animationTime;
		GStateAdvanceTime(m_gStateCharacterInstance, timeDelta);

		auto framenum = static_cast<int>(animationTime);
		if (!(framenum % 100))
		{
			CCP_LOGNOTICE("m_gstate_pose_cache size: %d", sizeof(m_gstate_pose_cache));
		}

		granny_local_pose *Pose = GStateSampleAnimation(m_gStateCharacterInstance, m_gstate_pose_cache);
		if ( !Pose )
		{
			return;
		}

		m_localPose = Pose;


		if( m_boneOffset.NeedRebind( m_skeleton->BoneCount ) && m_skeleton->BoneCount )
		{
			std::vector<std::string> bones( m_skeleton->BoneCount );			
			for( size_t i = 0; i < bones.size(); ++i )
				bones[ i ] = m_skeleton->Bones[ i ].Name;
			m_boneOffset.BindToRig( &bones[0], bones.size() );
		}

		if( !m_boneOffset.HaveTransforms() )
		{
			auto id = IdentityMatrix();
			// build the worldpos out of the localpose using identity matrix as base
			GrannyBuildWorldPose( m_skeleton, 0, m_skeleton->BoneCount, m_localPose, &id.m[0][0], m_worldPose );
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
				ApplyBoneOffsets(i);
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
						fromMat = fromMat * modelTransform;
						toMat = toMat * modelTransform;

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
					m = m * modelTransform;

					g_debugRenderer->Printf( m.GetTranslation(), 0xffffffff, name );
				}
			}
		}

		GrannyClearPoseCache( m_gstate_pose_cache );
		m_localPose = nullptr;
	}
}

void Tr2GStateAnimation::PostPhysicsAnimation( Be::Time time, const Matrix& modelTransform )
{
	return;
}

const std::string * Tr2GStateAnimation::GetAnimationBoneList( unsigned int& numBones ) const
{
	numBones = (unsigned int)m_boneList.size();
	if( numBones )
	{
		return &m_boneList[0];
	}

	return NULL;
}

const Matrix* Tr2GStateAnimation::GetAnimationTransforms()
{
	if( m_worldPose )
	{
		return reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4Array( m_worldPose ) );
	}

	return NULL;
}

void Tr2GStateAnimation::Cleanup()
{

	if ( m_gStateCharacterInstance )
	{
		GStateFreeCharacterInstance(m_gStateCharacterInstance);
		m_gStateCharacterInstance = nullptr;
		m_state_machine = nullptr;
	}


	if ( m_gstate_pose_cache )
	{
		GrannyFreePoseCache( m_gstate_pose_cache );
		m_gstate_pose_cache = nullptr;
		m_localPose = nullptr;
	}
	
	if ( m_compositePose )
	{
		GrannyFreeLocalPose( m_compositePose );
		m_compositePose = nullptr;
	}

	if ( m_worldPose )
	{
		GrannyFreeWorldPose( m_worldPose );
		m_worldPose = nullptr;
	}

	if ( m_meshBinding )
	{
		GrannyFreeMeshBinding( m_meshBinding );
		m_meshBinding = nullptr;
	}

	if ( m_modelInstance )
	{
		GrannyFreeModelInstance(m_modelInstance);
		m_modelInstance = nullptr;
	}

	m_skeleton = nullptr;

	m_boneList.clear();

	if ( m_meshBoneMatrixList )
	{
		CCP_ALIGNED_FREE( m_meshBoneMatrixList );
		m_meshBoneMatrixList = nullptr;
	}
}

bool Tr2GStateAnimation::FindBoneByName( const char* name, unsigned int& ix ) const
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
int Tr2GStateAnimation::GetMeshBoneCount() const
{
	return m_meshBoneCount;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a pointer to the internal list of 3x4 matrices, holding the transforms
//   of the current animation state
// --------------------------------------------------------------------------------------
const granny_matrix_3x4* Tr2GStateAnimation::GetMeshBoneMatrixList() const
{
	return m_meshBoneMatrixList;
}

float Tr2GStateAnimation::GetAnimationTime()
{
	if (m_paused)
	{
		return m_pauseTime;
	}
	return Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
}


bool Tr2GStateAnimation::IsInitialized() const
{
	return ( m_modelInstance != nullptr ) && ( m_gStateRes != nullptr ) && m_gStateRes->IsGood() && m_gStateCharacterInstance && m_gstate_pose_cache;
}



void Tr2GStateAnimation::TogglePauseAnimations(bool pause)
{
	if (m_paused && !pause)
	{
		m_paused = false;
		m_totalPauseOffset = Tr2Renderer::GetAnimationTime() - m_pauseTime;
		m_pauseTime = 0.0;
	}
	else if (!m_paused && pause)
	{
		m_paused = true;
		m_pauseTime = Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
	}
}