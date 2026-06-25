// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if GSTATE_ENABLED

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
#define GSTATE_INTERNAL_HEADER 1
#include "gstate_character_internal.h"
#include <algorithm>

using namespace gstate;
using namespace std;

static const int MAX_JOINT_COUNT = 254;
BLUE_DECLARE( Tr2GStateParameter );
BLUE_DECLARE_VECTOR( Tr2GStateParameter );

extern int g_debugBoneLabelFont;


Tr2GStateAnimation::Tr2GStateAnimation( IRoot* lockobj ) :
	m_skeleton( nullptr ),
	m_boneList( "Tr2GStateAnimation/m_boneList" ),
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
	m_animationBound( false ),
	m_boneBoundsInitialized( false ),
	m_paused( false ),
	m_pauseTime( 0.0 ),
	m_totalPauseOffset( 0.0 ),
	m_scrubbing( false ),
	m_scrub_time( 0.0 ),
	m_scrub_offset( 0.0 ),
	m_scrub_play_offset( 0.0 ),
	m_scrub_playing( false ),
	PARENTLOCK( m_gStateParameterList ),
	PARENTLOCK( m_gStateParameterDefaultList )
{
}

Tr2GStateAnimation::~Tr2GStateAnimation()
{
	Cleanup();

	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
	}

	if( m_gStateRes )
	{
		m_gStateRes->RemoveNotifyTarget( this );
	}
}


void Tr2GStateAnimation::SetSharedGeometryRes( TriGeometryResPtr res )
{
	m_geometryRes = res;
	m_resPath = "<EveSpaceObject2>";
}


void Tr2GStateAnimation::LoadGrannyRes()
{

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
}


bool Tr2GStateAnimation::IsFullyLoaded()
{
	return m_gStateRes && m_gStateRes->IsFullyLoaded() && m_grannyRes && m_grannyRes->IsGood() && m_modelInstance != nullptr;
}


Tr2GStateParameterPtr Tr2GStateAnimation::GetParameterObjectByName( const std::string nodeName, const std::string name )
{
	for( auto param = m_gStateParameterList.begin(); param != m_gStateParameterList.end(); param++ )
	{
		if( nodeName == ( *param )->GetNodeName() && name == ( *param )->GetName() )
		{
			return *param;
		}
	}
	return nullptr;
}

void Tr2GStateAnimation::ResetParamsToDefault()
{
	auto defaultParameterInstance = m_gStateParameterDefaultList.cbegin();
	for( auto parameterInstance = m_gStateParameterList.cbegin(); parameterInstance != m_gStateParameterList.cend(); parameterInstance++ )
	{
		auto parameterNodeName = ( *parameterInstance )->GetNodeName();
		auto name = ( *parameterInstance )->GetName();
		auto value = ( *defaultParameterInstance )->GetValue();
		( *parameterInstance )->SetValue( value );
		if( ( parameterNodeName.length() != 0 ) && ( name.length() != 0 ) )
		{
			auto iterator = m_gStateParameterCachedList.find( { parameterNodeName, name } );
			if( iterator != m_gStateParameterCachedList.end() )
			{
				SetParameter( parameterNodeName, iterator->second, value );
			}
		}
		defaultParameterInstance++;
	}
}

void Tr2GStateAnimation::InitializeParametersFromGState()
{
	// Get Top level nodes
	auto top_node_names = GetTopLevelParameterNodeNames();

	// Loop all the parameters nodes(parents) in the GState
	for( auto node_name = top_node_names.cbegin(); node_name != top_node_names.cend(); node_name++ )
	{
		// Get list of parameters in there
		auto params = GetParameters( *node_name );

		// Loop all the parameters in the parameter node(parent)
		for( auto param_name = params.cbegin(); param_name != params.cend(); param_name++ )
		{
			auto param = GetParameterObjectByName( *node_name, *param_name );
			auto value = GetParameterByName( *node_name, *param_name );
			if( !param )
			{
				// creating the parameter and storing it in the list
				Tr2GStateParameterPtr mytest;
				mytest.CreateInstance();
				mytest->SetValue( value );
				mytest->SetName( *param_name );
				mytest->SetNodeName( *node_name );
				m_gStateParameterList.Append( mytest );

				Tr2GStateParameterPtr default_param;
				default_param.CreateInstance();
				default_param->SetValue( value );
				default_param->SetName( *param_name );
				default_param->SetNodeName( *node_name );
				m_gStateParameterDefaultList.Append( default_param );
			}

			// cache index
			auto index = GetParameterIndexByName( *node_name, *param_name );
			if( index > -1 )
			{
				m_gStateParameterCachedList[{ *node_name, *param_name }] = index;
			}
		}
	}
}


bool Tr2GStateAnimation::Initialize()
{
	Cleanup();

	if( m_gStateRes )
	{
		m_gStateRes->RemoveNotifyTarget( this );
		m_gStateRes.Unlock();
	}

	BeResMan->GetResource( m_gStateResPath.c_str(), "raw", BlueInterfaceIID<Tr2GrannyStateRes>(), (void**)&m_gStateRes );

	if( m_gStateRes )
	{
		m_gStateRes->AddNotifyTarget( this );
	}

	return true;
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
	if( p != m_grannyRes && p != m_geometryRes )
	{
		if( p == m_gStateRes )
		{
			if( m_gStateRes && !m_gStateRes->GetCharacterInfo() )
			{
				CCP_LOGERR( "'%s' not found or not a valid GState file", m_gStateResPath.c_str() );
				return;
			}

			LoadModelFromGstate();

			return;
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
			for( int i = 0; i < fi->ModelCount; ++i )
			{
				for( int j = 0; j < fi->Models[i]->MeshBindingCount; ++j )
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

			for( int i = 0; i < fi->ModelCount; ++i )
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
			granny_model* model = GetGrannyModel();
			m_modelInstance = GrannyInstantiateModel( model );
			m_skeleton = GrannyGetSourceSkeleton( m_modelInstance );
			m_worldPose = GrannyNewWorldPose( m_skeleton->BoneCount );

			if( m_meshBindingIndex != -1 )
			{
				m_meshBinding = GrannyNewMeshBinding( fi->Models[m_modelIndex]->MeshBindings[m_meshBindingIndex].Mesh, m_skeleton, m_skeleton );
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
					m_meshBoneMatrixList = (Float4x3*)CCP_ALIGNED_MALLOC( "Tr2GrannyAnimation/m_boneMatrixList", m_meshBoneCount * sizeof( Float4x3 ), 16 );
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
		m_skeleton = nullptr;
		m_worldPose = nullptr;
		m_localPose = nullptr;
		m_compositePose = nullptr;

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

void Tr2GStateAnimation::LoadModelFromGstate()
{
	if( m_resPath.empty() )
	{
		gstate_character_info* info = m_gStateRes->GetCharacterInfo();

		std::string model_name_hint = info->ModelNameHint;
		if( model_name_hint[model_name_hint.length() - 1] == ';' )
		{
			model_name_hint = model_name_hint.substr( 0, model_name_hint.length() - 1 );
		}
		m_resPath = GetFullAnimPath( model_name_hint, m_gStateResPath );
		LoadGrannyRes();
	}
}


void Tr2GStateAnimation::InstantiateCharacter()
{
	auto character_info = m_gStateRes->GetCharacterInfo();
	m_gStateCharacterInstance = GStateInstantiateCharacter( character_info, GetAnimationTime(), 0, GetGrannyModel() );
	m_state_machine = GStateGetStateMachine( m_gStateCharacterInstance );
	InitializeParametersFromGState();
	if( !m_gstate_pose_cache )
	{
		m_gstate_pose_cache = GrannyNewPoseCache();
	}
	m_animationBound = true;
}

const std::vector<std::string> Tr2GStateAnimation::GetGStateAnimFileRefPaths() const
{
	return m_gStateRes->GetGStateAnimFileRefPaths();
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
	for( int i = 0; i < fi->ModelCount; ++i )
	{
		for( int j = 0; j < fi->Models[i]->MeshBindingCount; ++j )
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

std::vector<std::string> Tr2GStateAnimation::GetTopLevelNodeNames()
{
	std::vector<std::string> name_list;
	auto num_children = m_state_machine->GetNumChildren();
	for( auto child = 0; child < num_children; child++ )
	{
		auto child_node_name = m_state_machine->GetChild( child )->GetName();
		name_list.push_back( child_node_name );
	}

	return name_list;
}


std::vector<std::string> Tr2GStateAnimation::GetTopLevelParameterNodeNames()
{
	std::vector<std::string> name_list;
	auto num_children = m_state_machine->GetNumChildren();
	for( auto child = 0; child < num_children; child++ )
	{
		auto curr_node = m_state_machine->GetChild( child );
		parameters* curr_params = GSTATE_DYNCAST( curr_node, parameters );
		if( curr_params )
		{
			auto child_node_name = curr_node->GetName();
			name_list.push_back( child_node_name );
		}
	}

	return name_list;
}


std::vector<std::string> Tr2GStateAnimation::GetTopLevelStateNodeNames()
{
	std::vector<std::string> name_list;
	auto num_children = m_state_machine->GetNumChildren();
	for( auto child = 0; child < num_children; child++ )
	{
		auto curr_node = m_state_machine->GetChild( child );
		if( m_state_machine->IsStateNode( curr_node ) )
		{
			auto child_node_name = curr_node->GetName();
			name_list.push_back( child_node_name );
		}
	}

	return name_list;
}



std::vector<std::string> Tr2GStateAnimation::GetParameters( const std::string& param_node )
{
	std::vector<std::string> name_list;
	node* curr_param = m_state_machine->FindChildByName( param_node.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );

	auto num_params = param_set->GetNumOutputs();
	for( auto curr_idx = 0; curr_idx < num_params; curr_idx++ )
	{
		name_list.push_back( param_set->GetOutputName( curr_idx ) );
	}

	return name_list;
}



std::vector<float> Tr2GStateAnimation::GetParameterRange( const std::string& param_node, const std::string& param_name )
{
	std::vector<float> return_list;
	node* curr_param = m_state_machine->FindChildByName( param_node.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );

	auto num_params = param_set->GetNumOutputs();
	for( auto curr_idx = 0; curr_idx < num_params; curr_idx++ )
	{
		std::string curr_name = param_set->GetOutputName( curr_idx );
		if( curr_name == param_name )
		{
			granny_real32 out_min;
			granny_real32 out_max;
			auto it_worked = param_set->GetScalarOutputRange( curr_idx, &out_min, &out_max );
			if( it_worked )
			{
				return_list.push_back( out_min );
				return_list.push_back( out_max );
			}
		}
	}

	return return_list;
}


tokenized* Tr2GStateAnimation::GetActiveMachineElement()
{
	tokenized* element = m_state_machine->GetActiveElement();

	return element;
}

const std::string Tr2GStateAnimation::GetActiveMachineElementName()
{
	tokenized* active = GetActiveMachineElement();
	assert( active );

	node* curr_node = GSTATE_DYNCAST( active, node );
	if( curr_node )
	{
		char const* curr_name = curr_node->GetName();
		return std::string( curr_name );
	}
	else
	{
		assert( GSTATE_DYNCAST( active, transition ) );
		transition* curr_transition = GSTATE_DYNCAST( active, transition );
		if( curr_transition )
		{
			char const* curr_transition_name = curr_transition->GetName();
			return std::string( curr_transition_name );
		}
	}

	return "";
}

int Tr2GStateAnimation::GetStartStateIdx()
{
	return m_state_machine->GetStartStateIdx();
}

void Tr2GStateAnimation::SetStartStateIdx( int StartState )
{
	m_state_machine->SetStartStateIdx( StartState );
}


void Tr2GStateAnimation::SetStartStateByName( const std::string& name )
{
#if GrannyProductMajorVersion > 2 || ( GrannyProductMajorVersion == 2 && GrannyProductMinorVersion >= 11 )
	m_state_machine->SetStartState( m_state_machine->GetStateByName( name.c_str() ) );
#else
	CCP_LOGERR( "SetStartStateByName: outdated Granny version" );
#endif
}

bool Tr2GStateAnimation::ForceChangeToState( const std::string& name )
{
	return m_state_machine->ForceChangeToState( GStateInstanceTime( m_gStateCharacterInstance ), name.c_str() );
}

bool Tr2GStateAnimation::RequestChangeToState( const std::string& name )
{
	return m_state_machine->RequestChangeToState( GStateInstanceTime( m_gStateCharacterInstance ),
												  GStateInstanceLastDeltaT( m_gStateCharacterInstance ),
												  name.c_str() );
}

bool Tr2GStateAnimation::StartTransitionByName( const std::string& name )
{
	return m_state_machine->StartTransitionByName( GStateInstanceTime( m_gStateCharacterInstance ), name.c_str() );
}

float Tr2GStateAnimation::GetParameter( const std::string& param_node_name, granny_int32x param_idx )
{
#if GrannyProductMajorVersion > 2 || ( GrannyProductMajorVersion == 2 && GrannyProductMinorVersion >= 11 )
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	return param_set->GetRequestedParam( param_idx );
#else
	CCP_LOGERR( "GetParameter: outdated Granny version" );
	return 0;
#endif
}

void Tr2GStateAnimation::SetParameter( const std::string& param_node_name, granny_int32x param_idx, float value )
{
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	param_set->SetParameter( param_idx, value );
}

void Tr2GStateAnimation::RequestParameter( const std::string& param_node_name, granny_int32x param_idx, float value )
{
#if GrannyProductMajorVersion > 2 || ( GrannyProductMajorVersion == 2 && GrannyProductMinorVersion >= 11 )
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	param_set->RequestParameter( param_idx, value );
#else
	CCP_LOGERR( "RequestParameter: outdated Granny version" );
#endif
}


void Tr2GStateAnimation::RequestParameterByName( const std::string& param_node_name, const std::string& param_name, float value )
{
#if GrannyProductMajorVersion > 2 || ( GrannyProductMajorVersion == 2 && GrannyProductMinorVersion >= 11 )
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	granny_int32x param_idx = -1;

	int num_params = param_set->GetNumOutputs();
	for( auto count = 0; count < num_params; count++ )
	{
		if( !param_name.compare( param_set->GetOutputName( count ) ) )
		{
			param_idx = count;
		}
	}

	if( param_idx != -1 )
		param_set->RequestParameter( param_idx, value );
#else
	CCP_LOGERR( "RequestParameterByName: outdated Granny version" );
#endif
}


float Tr2GStateAnimation::GetParameterByName( const std::string& param_node_name, const std::string& param_name )
{
#if GrannyProductMajorVersion > 2 || ( GrannyProductMajorVersion == 2 && GrannyProductMinorVersion >= 11 )
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	granny_int32x param_idx = -1;

	int num_params = param_set->GetNumOutputs();
	for( auto count = 0; count < num_params; count++ )
	{
		if( !param_name.compare( param_set->GetOutputName( count ) ) )
		{
			param_idx = count;
		}
	}

	return param_set->GetRequestedParam( param_idx );
#else
	CCP_LOGERR( "GetParameterByName: outdated Granny version" );
	return 0;
#endif
}


granny_int32x Tr2GStateAnimation::GetParameterIndexByName( const std::string& param_node_name, const std::string& param_name )
{
	node* curr_param = m_state_machine->FindChildByName( param_node_name.c_str() );
	parameters* param_set = GSTATE_DYNCAST( curr_param, parameters );
	assert( param_set );

	int num_params = param_set->GetNumOutputs();
	for( auto count = 0; count < num_params; count++ )
	{
		if( !param_name.compare( param_set->GetOutputName( count ) ) )
		{
			return count;
		}
	}
	return -1;
}


bool Tr2GStateAnimation::GetDynamicBounds( Vector4& boundingSphere, Vector3& aabbMin, Vector3& aabbMax )
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
		aabbMin += *reinterpret_cast<Vector3*>( fi->Models[m_modelIndex]->InitialPlacement.Position );
		aabbMax += *reinterpret_cast<Vector3*>( fi->Models[m_modelIndex]->InitialPlacement.Position );
		boundingSphere.x += fi->Models[m_modelIndex]->InitialPlacement.Position[0];
		boundingSphere.y += fi->Models[m_modelIndex]->InitialPlacement.Position[1];
		boundingSphere.z += fi->Models[m_modelIndex]->InitialPlacement.Position[2];
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
		initialPlacement = *reinterpret_cast<Vector3*>( fi->Models[m_modelIndex]->InitialPlacement.Position );
	}
	initialTranslation = TranslationMatrix( initialPlacement );

	for( int boneIdx = 0; boneIdx < m_meshBoneCount; boneIdx++ )
	{
		const int* bi = GrannyGetMeshBindingFromBoneIndices( m_meshBinding );
		Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bi[boneIdx] ) ) * modelTransform * initialTranslation;
		Vector4 pos( 0, 0, 0, 1 );
		pos = Transform( pos, mat );
		pos.w = 2;
		Tr2Renderer::DrawSphere( pos, 1, 0xffffffff );
		Tr2Renderer::Printf( static_cast<TriDebugFont>( g_debugBoneLabelFont ), Vector3( pos.x, pos.y, pos.z ), 0xffffffff, "  %s : %d", m_skeleton->Bones[bi[boneIdx]].Name, boneIdx );
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
		initialPlacement = *reinterpret_cast<Vector3*>( fi->Models[m_modelIndex]->InitialPlacement.Position );
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
	LoadGrannyRes();
}

const std::string& Tr2GStateAnimation::GetGStateResPath() const
{
	return m_gStateResPath;
}

void Tr2GStateAnimation::SetGStateResPath( const std::string& val )
{
	m_gStateResPath = val;
	Initialize();
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
	granny_mesh* mesh = fi->Models[m_modelIndex]->MeshBindings[m_meshBindingIndex].Mesh;
	granny_int32x vertCount = GrannyGetMeshVertexCount( mesh );

	std::vector<granny_pwn313_vertex> sourceVerts( vertCount );
	std::vector<granny_pn33_vertex> deformedVerts( vertCount );

	GrannyCopyMeshVertices( mesh, GrannyPWN313VertexType, &sourceVerts[0] );

	granny_mesh_deformer* deformer = GrannyNewMeshDeformer( GrannyPWN313VertexType,
															GrannyPN33VertexType,
															GrannyDeformPositionNormal,
															GrannyDontAllowUncopiedTail );
	if( !deformer )
	{
		return false;
	}

	GrannyDeformVertices( deformer, GrannyGetMeshBindingFromBoneIndices( m_meshBinding ), (granny_real32*)GrannyGetWorldPoseComposite4x4Array( m_worldPose ), vertCount, &sourceVerts[0], &deformedVerts[0] );

	// Get the transformed bounding box
	bbMin = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	bbMax = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	for( int v = 0; v < vertCount; ++v )
	{
		Vector4 pos(
			deformedVerts[v].Position[0] + fi->Models[m_modelIndex]->InitialPlacement.Position[0],
			deformedVerts[v].Position[1] + fi->Models[m_modelIndex]->InitialPlacement.Position[1],
			deformedVerts[v].Position[2] + fi->Models[m_modelIndex]->InitialPlacement.Position[2],
			1 );

		pos = Transform( pos, transform );
		pos /= pos.w;

		bbMin.x = min( bbMin.x, pos.x );
		bbMax.x = max( bbMax.x, pos.x );

		bbMin.y = min( bbMin.y, pos.y );
		bbMax.y = max( bbMax.y, pos.y );

		bbMin.z = min( bbMin.z, pos.z );
		bbMax.z = max( bbMax.z, pos.z );
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
	granny_mesh* mesh = fi->Models[m_modelIndex]->MeshBindings[m_meshBindingIndex].Mesh;
	granny_int32x vertCount = GrannyGetMeshVertexCount( mesh );

	std::vector<granny_pwn313_vertex> sourceVerts( vertCount );
	std::vector<granny_pn33_vertex> deformedVerts( vertCount );

	GrannyCopyMeshVertices( mesh, GrannyPWN313VertexType, &sourceVerts[0] );

	granny_mesh_deformer* deformer = GrannyNewMeshDeformer( GrannyPWN313VertexType,
															GrannyPN33VertexType,
															GrannyDeformPositionNormal,
															GrannyDontAllowUncopiedTail );
	if( !deformer )
	{
		return Vector4( 0.f, 0.f, 0.f, -1.f );
	}

	GrannyDeformVertices( deformer, GrannyGetMeshBindingFromBoneIndices( m_meshBinding ), (granny_real32*)GrannyGetWorldPoseComposite4x4Array( m_worldPose ), vertCount, &sourceVerts[0], &deformedVerts[0] );

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
	LoadGrannyRes();
}


void Tr2GStateAnimation::PrePhysicsAnimation( Be::Time time, const Matrix& modelTransform )
{
	if( IsInitialized() && m_animationEnabled )
	{
		// Update gstate with parameters if fully loaded
		if( IsFullyLoaded() )
		{
			for( auto parameterInstance = m_gStateParameterList.cbegin(); parameterInstance != m_gStateParameterList.cend(); parameterInstance++ )
			{
				auto parameterNodeName = ( *parameterInstance )->GetNodeName();
				auto name = ( *parameterInstance )->GetName();
				auto value = ( *parameterInstance )->GetValue();
				if( ( parameterNodeName.length() != 0 ) && ( name.length() != 0 ) )
				{
					auto iterator = m_gStateParameterCachedList.find( { parameterNodeName, name } );
					if( iterator != m_gStateParameterCachedList.end() )
					{
						SetParameter( parameterNodeName, iterator->second, value );
					}
				}
			}
		}

		float animationTime = GetAnimationTime();

		// TODO: Should this be done here? Seems wasteful to sample animations and build the pose
		// for objects that are off-screen.

		granny_real32 timeDelta = animationTime - m_last_gstate_time;
		m_last_gstate_time = animationTime;
		GStateAdvanceTime( m_gStateCharacterInstance, timeDelta );

		auto framenum = static_cast<int>( animationTime );

		granny_local_pose* Pose = GStateSampleAnimation( m_gStateCharacterInstance, m_gstate_pose_cache );
		if( !Pose )
		{
			return;
		}

		m_localPose = Pose;

		auto id = IdentityMatrix();
		// build the worldpos out of the localpose using identity matrix as base
		GrannyBuildWorldPose( m_skeleton, 0, m_skeleton->BoneCount, m_localPose, &id.m[0][0], m_worldPose );
		// construct the 3x4 matrix list, that will be passed to the shader, if we have a meshbinding at all
		if( m_meshBinding )
		{
			int const* meshToBone = GrannyGetMeshBindingToBoneIndices( m_meshBinding );
			if( m_meshBoneMatrixList && meshToBone && m_meshBoneCount )
			{
				GrannyBuildIndexedCompositeBufferTransposed( m_skeleton, m_worldPose, meshToBone, m_meshBoneCount, reinterpret_cast<granny_matrix_3x4*>( m_meshBoneMatrixList ) );
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

const std::string* Tr2GStateAnimation::GetAnimationBoneList( unsigned int& numBones ) const
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
	if( m_gStateCharacterInstance )
	{
		GStateFreeCharacterInstance( m_gStateCharacterInstance );
		m_gStateCharacterInstance = nullptr;
		m_state_machine = nullptr;
	}


	if( m_gstate_pose_cache )
	{
		GrannyFreePoseCache( m_gstate_pose_cache );
		m_gstate_pose_cache = nullptr;
		m_localPose = nullptr;
	}

	if( m_compositePose )
	{
		GrannyFreeLocalPose( m_compositePose );
		m_compositePose = nullptr;
	}

	if( m_worldPose )
	{
		GrannyFreeWorldPose( m_worldPose );
		m_worldPose = nullptr;
	}

	if( m_meshBinding )
	{
		GrannyFreeMeshBinding( m_meshBinding );
		m_meshBinding = nullptr;
	}

	if( m_modelInstance )
	{
		GrannyFreeModelInstance( m_modelInstance );
		m_modelInstance = nullptr;
	}

	m_skeleton = nullptr;

	m_boneList.clear();

	if( m_meshBoneMatrixList )
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
const Float4x3* Tr2GStateAnimation::GetMeshBoneMatrixList() const
{
	return m_meshBoneMatrixList;
}


float Tr2GStateAnimation::GetAnimationTime()
{
	if( m_paused )
	{
		return m_pauseTime;
	}

	if( m_scrubbing )
	{
		if( m_scrub_playing )
		{
			return Tr2Renderer::GetAnimationTime() - m_totalPauseOffset + m_scrub_play_offset;
		}

		return m_scrub_time + m_scrub_offset;
	}

	return Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
}


void Tr2GStateAnimation::StartScrub()
{
	if( m_scrubbing )
	{
		return;
	}
	m_scrub_time = GetAnimationTime();
	m_scrub_offset = 0.0;
	m_scrubbing = true;
}


void Tr2GStateAnimation::ClearScrub()
{
	m_scrubbing = false;
	m_scrub_time = 0.0;
	m_scrub_offset = 0.0;
	m_scrub_playing = false;
	m_scrub_play_offset = 0.0;
}


void Tr2GStateAnimation::PlayFromScrub()
{
	if( !m_scrubbing )
	{
		return;
	}

	m_scrub_play_offset = m_scrub_time + m_scrub_offset - ( Tr2Renderer::GetAnimationTime() + m_totalPauseOffset );
	m_scrub_playing = true;
}


void Tr2GStateAnimation::StopPlayFromScrub()
{
	if( !m_scrubbing )
	{
		return;
	}

	if( !m_scrub_playing )
	{
		return;
	}

	m_scrub_offset = GetAnimationTime() - m_scrub_time;
	m_scrub_playing = false;
	m_scrub_play_offset = 0.0;
}


void Tr2GStateAnimation::SetScrubOffset( float scrub_offset )
{
	if( !m_scrubbing )
	{
		return;
	}

	if( m_scrub_playing )
	{
		return;
	}

	m_scrub_offset = scrub_offset;
}


float Tr2GStateAnimation::GetScrubOffset()
{
	if( !m_scrubbing )
	{
		return 0.0;
	}

	return m_scrub_offset;
}


bool Tr2GStateAnimation::IsInitialized() const
{
	return ( m_modelInstance != nullptr ) && ( m_gStateRes != nullptr ) && m_gStateRes->IsFullyLoaded() && m_gStateCharacterInstance && m_gstate_pose_cache && m_animationBound;
}



void Tr2GStateAnimation::TogglePauseAnimations( bool pause )
{
	if( m_paused && !pause )
	{
		m_paused = false;
		m_totalPauseOffset = Tr2Renderer::GetAnimationTime() - m_pauseTime;
		m_pauseTime = 0.0;
	}
	else if( !m_paused && pause )
	{
		m_paused = true;
		m_pauseTime = Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
	}
}

#endif