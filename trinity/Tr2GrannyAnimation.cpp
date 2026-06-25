// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2VertexDefinitionUtilities.h"
#include <algorithm>
#include "TriSettingsRegistrar.h"

#include <cctype>
#include <TriMath.h>

int g_debugBoneLabelFont = TRI_DBG_FONT_MEDIUM;
TRI_REGISTER_SETTING( "debugBoneLabelFont", g_debugBoneLabelFont );


namespace Tr2GrannyAnimationUtils
{
bool GetBoneList( Tr2GrannyAnimation* animationUpdater, const Float4x3*& bones, size_t& boneCount )
{
	if( animationUpdater && animationUpdater->IsInitialized() )
	{
		boneCount = size_t( animationUpdater->GetMeshBoneCount() );
		if( boneCount )
		{
			bones = animationUpdater->GetMeshBoneMatrixList();
			return true;
		}
		else
		{
			bones = nullptr;
		}
	}
	else
	{
		boneCount = 0;
		bones = nullptr;
	}
	return false;
}

std::vector<int32_t> CreateMapping( const cmf::Skeleton& skeleton, cmf::Span<cmf::BoneBinding> boneBindings, uint32_t meshBoneCount )
{
	std::vector<int32_t> mapping( meshBoneCount, -1 );
	for( uint32_t meshBoneIndex = 0; meshBoneIndex < meshBoneCount; ++meshBoneIndex )
	{
		const auto boundBoneName = boneBindings[meshBoneIndex].name;

		auto foundBone = std::find_if( skeleton.bones.begin(), skeleton.bones.end(), [boundBoneName]( cmf::String boneName ) {
			return boundBoneName == boneName;
		} );
		if( foundBone != skeleton.bones.end() )
		{
			mapping[meshBoneIndex] = (int32_t)std::distance( skeleton.bones.begin(), foundBone );
		}
	}
	return mapping;
}
};

static const int MAX_JOINT_COUNT = 254;

Tr2GrannyAnimation::Tr2GrannyAnimation( IRoot* lockobj ) :
	PARENTLOCK( m_boneOffset ),
	m_boneList( "Tr2GrannyAnimation/m_boneList" ),
#if WITH_GRANNY
	m_skeleton( nullptr ),
	m_worldPose( nullptr ),
	m_localPose( nullptr ),
	m_compositePose( nullptr ),
	m_meshBinding( nullptr ),
	m_meshBindingIndex( -1 ),
#endif
	m_meshBoneMatrixList( nullptr ),
	m_meshBoneCount( 0 ),
	m_useMeshBinding( false ),
	m_debugRenderSkeleton( false ),
	m_debugRenderJointNames( false ),
	m_baseLayer( 1.f ),
	m_modelIndex( 0 ),
	m_animationEnabled( true ),
	m_boneBoundsInitialized( false ),
	m_additiveMode( false ),
	m_aimingBone( false ),
	m_aimBone( "" ),
	m_paused( false ),
	m_pauseTime( 0.f ),
	m_totalPauseOffset( 0.f )
{
}

Tr2GrannyAnimation::~Tr2GrannyAnimation()
{
	Cleanup();

	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}

	if( m_grannyRes )
	{
		m_grannyRes->RemoveNotifyTarget( this );
	}

	for( auto it = m_secondaryGrannyRes.begin(); it != m_secondaryGrannyRes.end(); it++ )
	{
		if( it->second )
		{
			it->second->RemoveNotifyTarget( this );
		}
	}
}

#if WITH_GRANNY
int AnimNameToIndex( const granny_file_info* fi, const char* name )
{
	int index = fi->AnimationCount;

	for( int i = 0; i < fi->AnimationCount; ++i )
	{
		if( strcmp( fi->Animations[i]->Name, name ) == 0 )
		{
			index = i;
			break;
		}
	}

	return index;
}
#endif

const cmf::Data* GetSecondaryCMFFileInfo( const std::string& cmfResPath, const TriGrannyResPtr grannyPtr )
{
	if( !grannyPtr || !grannyPtr->IsPrepared() )
	{
		return nullptr;
	}
	CCP_ASSERT( grannyPtr->IsUsingCMF() );
	auto cmfData = grannyPtr->GetCMFData();
	if( !cmfData )
	{
		CCP_LOGERR( "'%s' is not a valid CMF file", cmfResPath.c_str() );
	}
	return cmfData;
}

#if WITH_GRANNY
granny_file_info* GetSecondaryFileInfo( const std::string& grannyResPath, const TriGrannyResPtr grannyPtr )
{
	if( !grannyPtr || !grannyPtr->IsPrepared() )
	{
		return nullptr;
	}
	CCP_ASSERT( !grannyPtr->IsUsingCMF() );
	granny_file_info* const fi = GrannyGetFileInfo( grannyPtr->GetGrannyFile() );
	if( !fi )
	{
		CCP_LOGERR( "'%s' is not a valid Granny file", grannyResPath.c_str() );
	}
	return fi;
}
#endif

const cmf::Animation* Tr2GrannyAnimation::FindCMFAnimationByName( const char* name ) const
{
	auto cmfData = GetCMFData();
	if( !cmfData )
	{
		return nullptr;
	}

	auto animation = std::find_if( cmfData->animations.begin(), cmfData->animations.end(), [name]( const cmf::Animation& anim ) {
		return cmf::ToStdStringView( anim.name ) == name;
	} );
	if( animation != cmfData->animations.end() )
	{
		return animation;
	}

	for( auto& [resPath, res] : m_secondaryGrannyRes )
	{
		if( !res )
		{
			continue;
		}
		cmfData = GetSecondaryCMFFileInfo( resPath, res );
		if( !cmfData )
		{
			continue;
		}

		animation = std::find_if( cmfData->animations.begin(), cmfData->animations.end(), [name]( const cmf::Animation& anim ) {
			return cmf::ToStdStringView( anim.name ) == name;
		} );
		if( animation != cmfData->animations.end() )
		{
			return animation;
		}
	}

	return nullptr;
}

#if WITH_GRANNY
granny_animation* Tr2GrannyAnimation::FindGrannyAnimationByName( const char* name ) const
{
	const granny_file_info* fi = GetFileInfo();
	if( !fi )
	{
		return nullptr;
	}

	int animIx = AnimNameToIndex( fi, name );

	if( animIx != fi->AnimationCount )
	{
		return fi->Animations[animIx];
	}

	for( auto it = m_secondaryGrannyRes.begin(); it != m_secondaryGrannyRes.end(); ++it )
	{
		if( !it->second )
		{
			continue;
		}
		fi = GetSecondaryFileInfo( it->first, it->second );
		if( !fi )
		{
			continue;
		}

		animIx = AnimNameToIndex( fi, name );

		if( animIx != fi->AnimationCount )
		{
			return fi->Animations[animIx];
		}
	}

	return nullptr;
}
#endif

float Tr2GrannyAnimation::FindAnimationDurationByName( const char* name ) const
{
	if( IsUsingCMF() )
	{
		auto animation = FindCMFAnimationByName( name );
		if( !animation )
		{
			return 0;
		}
		return animation->duration;
	}
#if WITH_GRANNY
	else
	{
		auto animation = FindGrannyAnimationByName( name );
		if( !animation )
		{
			return 0;
		}
		return animation->Duration;
	}
#else
	else
	{
		return 0;
	}
#endif
}

void Tr2GrannyAnimation::SetSharedGeometryRes( TriGeometryResPtr res )
{
	if( res == m_geometryRes )
	{
		return;
	}
	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}
	Cleanup();
	m_geometryRes = res;
	if( m_geometryRes )
	{
		m_geometryRes->AddNotifyTarget( this );
	}
	m_resPath = "";
}

TriGeometryRes* Tr2GrannyAnimation::GetSharedGeometryRes() const
{
	return m_geometryRes;
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

void Tr2GrannyAnimation::LoadSecondaryResPath( const std::string& val )
{
	if( m_secondaryGrannyRes[val] )
	{
		m_secondaryGrannyRes[val]->RemoveNotifyTarget( this );
		m_secondaryGrannyRes[val].Unlock();
	}

	BeResMan->GetResource( val.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&m_secondaryGrannyRes[val] );

	if( m_secondaryGrannyRes[val] )
	{
		m_secondaryGrannyRes[val]->AddNotifyTarget( this );
	}
}

void Tr2GrannyAnimation::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}

#if WITH_GRANNY
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
#endif

bool Tr2GrannyAnimation::IsUsingCMF() const
{
	if( m_grannyRes )
	{
		return m_grannyRes->IsUsingCMF();
	}
	if( m_geometryRes )
	{
		return m_geometryRes->IsUsingCMF();
	}
	return false;
}

const cmf::Data* Tr2GrannyAnimation::GetCMFData() const
{
	if( m_grannyRes )
	{
		CCP_ASSERT( m_grannyRes->IsUsingCMF() );

		// when using a standalone cmf file, it's supposed to have an animation
		// track, so complain if it doesn't.
		const cmf::Data* data = m_grannyRes->GetCMFData();
		if( !data )
		{
			CCP_LOGERR( "'%s' is not a valid CMF file", m_resPath.c_str() );
		}
		return data;
	}

	// when using a shared geometryRes, there may not be an animation, or the
	// granny file isn't loaded yet.  Silently fail.
	if( m_geometryRes )
	{
		return m_geometryRes->GetCMFData();
	}
	return nullptr;
}

const std::vector<int32_t>& Tr2GrannyAnimation::GetSkeletonBoneIndices()
{
	CCP_ASSERT( IsUsingCMF() );

	return m_skeletonBoneIndices;
}

bool Tr2GrannyAnimation::HasMeshBinding() const
{
	if( IsUsingCMF() )
	{
		return !m_skeletonBoneIndices.empty();
	}
#if WITH_GRANNY
	else
	{
		return m_meshBinding != nullptr;
	}
#else
	else
	{
		return false;
	}
#endif
}

const std::pair<const int32_t*, size_t> Tr2GrannyAnimation::GetMeshBindingIndices() const
{
	if( IsUsingCMF() )
	{
		return std::make_pair( m_skeletonBoneIndices.data(), m_skeletonBoneIndices.size() );
	}
#if WITH_GRANNY
	else
	{
		auto boneCount = GrannyGetMeshBindingBoneCount( m_meshBinding );
		return std::make_pair( GrannyGetMeshBindingToBoneIndices( m_meshBinding ), boneCount );
	}
#else
	else
	{
		return std::make_pair( nullptr, 0 );
	}
#endif
}

bool Tr2GrannyAnimation::GetBoneWorldTransform( const char* boneName, Matrix& transform ) const
{
	if( IsUsingCMF() )
	{
		const cmf::Skeleton* skeleton = GetSkeleton();
		if( !m_worldTransforms.empty() && skeleton )
		{
			CCP_ASSERT( m_worldTransforms.size() == skeleton->bones.size() );

			for( int32_t bone = 0; bone < skeleton->bones.size(); bone++ )
			{
				if( cmf::ToStdStringView( skeleton->bones[bone] ) == boneName )
				{
					transform = m_worldTransforms[bone];
					return true;
				}
			}
		}
	}
#if WITH_GRANNY
	else
	{
		if( m_worldPose && m_skeleton )
		{
			granny_int32x bone;
			if( GrannyFindBoneByName( m_skeleton, boneName, &bone ) )
			{
				transform = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bone ) );
				return true;
			}
		}
	}
#endif
	return false;
}

void Tr2GrannyAnimation::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryRes && IsInitialized() )
	{
		return;
	}

	if( p != m_grannyRes && p != m_geometryRes )
	{
		for( auto it = m_secondaryGrannyRes.begin(); it != m_secondaryGrannyRes.end(); it++ )
		{
			if( p == it->second )
			{
				if( it->second )
				{
					if( it->second->IsUsingCMF() )
					{
						if( !it->second->GetCMFData() )
						{
							CCP_LOGERR( "'%s' not found or not a valid CMF file", it->first.c_str() );
						}
					}
#if WITH_GRANNY
					else
					{
						if( !it->second->GetGrannyFile() )
						{
							CCP_LOGERR( "'%s' not found or not a valid Granny file", it->first.c_str() );
						}
					}
#endif
				}
				return;
			}
		}
	}

	if( !m_grannyRes && !m_geometryRes )
	{
		return;
	}

	if( m_grannyRes )
	{
		if( m_grannyRes->IsUsingCMF() )
		{
			if( !m_grannyRes->GetCMFData() )
			{
				CCP_LOGERR( "'%s' not found or not a valid CMF file", m_resPath.c_str() );
				return;
			}
		}
#if WITH_GRANNY
		else
		{
			if( !m_grannyRes->GetGrannyFile() )
			{
				CCP_LOGERR( "'%s' not found or not a valid Granny file", m_resPath.c_str() );
				return;
			}
		}
#endif
	}

	if( IsUsingCMF() )
	{
		const cmf::Data* cmfData = GetCMFData();
		if( !cmfData )
		{
			return;
		}

		if( cmfData->skeletons.size() > 0 )
		{
			// By default we take the first model in the file
			m_modelIndex = 0;

			if( m_useMeshBinding )
			{
				m_modelIndex = -1;

				if( cmfData->meshes.size() > 0 )
				{
					m_modelIndex = cmfData->meshes[0].skeleton;
				}
			}
			else if( !m_model.empty() )
			{
				// A named model is specified - look for its index
				m_modelIndex = -1;

				auto skeleton = std::find_if( cmfData->skeletons.begin(), cmfData->skeletons.end(), [this]( const cmf::Skeleton& skel ) {
					return this->m_model == cmf::ToStdStringView( skel.name );
				} );
				if( skeleton != cmfData->skeletons.end() )
				{
					m_modelIndex = (int)std::distance( cmfData->skeletons.begin(), skeleton );
				}
			}

			if( m_modelIndex != -1 )
			{
				m_baseLayer.InitializeAnimationLayer( this );

				for( auto& [_, layer] : m_animationLayers )
				{
					layer.InitializeAnimationLayer( this );
				}

				auto& skeleton = cmfData->skeletons[m_modelIndex];

				cmf::RestPose( m_pose, skeleton );
				if( m_animationLayers.size() > 0 && !m_tmpPose.skeleton )
				{
					cmf::RestPose( m_tmpPose, skeleton );
				}

				for( const auto& bone : skeleton.bones )
				{
					m_boneList.push_back( cmf::ToStdString( bone ) );
				}

				if( m_useMeshBinding )
				{
					m_meshBoneCount = (int)cmfData->meshes[0].boneBindings.size();

					if( m_meshBoneCount )
					{
						if( m_meshBoneCount >= MAX_JOINT_COUNT )
						{
							m_meshBoneCount = MAX_JOINT_COUNT;
						}
						m_meshBoneMatrixList = (Float4x3*)CCP_ALIGNED_MALLOC( "Tr2GrannyAnimation/m_boneMatrixList", m_meshBoneCount * sizeof( Float4x3 ), 16 );
					}

					m_skeletonBoneIndices = Tr2GrannyAnimationUtils::CreateMapping( skeleton, cmfData->meshes[0].boneBindings, m_meshBoneCount );
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
			m_pose.boneTransforms.clear();
			m_pose.skeleton = nullptr;
			m_tmpPose.boneTransforms.clear();
			m_tmpPose.skeleton = nullptr;
			m_skeletonBoneIndices.clear();
			m_worldTransforms.clear();

			m_baseLayer.Cleanup();
			for( auto& [_, layer] : m_animationLayers )
			{
				layer.Cleanup();
			}
		}

		m_baseLayer.ConsumeAnimationQueue( this );
		for( auto& [_, layer] : m_animationLayers )
		{
			layer.ConsumeAnimationQueue( this );
		}

		if( m_geometryRes )
		{
			// Pump animation immediately, so that we have valid pose before the next update.
			PrePhysicsAnimation( 0, IdentityMatrix() );
		}

		for( auto& notify : m_notifyTargets )
		{
			notify->RebuildCachedData( p );
		}
	}
#if WITH_GRANNY
	else
	{
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

		if( m_geometryRes )
		{
			// Pump animation immediately, so that we have valid pose before the next update.
			PrePhysicsAnimation( 0, IdentityMatrix() );
		}

		for( auto& notify : m_notifyTargets )
		{
			notify->RebuildCachedData( p );
		}
	}
#endif
}

void Tr2GrannyAnimation::ClearAnimationLayers()
{
	for( auto& [_, layer] : m_animationLayers )
	{
		layer.Cleanup();
	}
	m_animationLayers.clear();
}

bool Tr2GrannyAnimation::InitializeBoundingInfo()
{
	if( IsUsingCMF() )
	{
		auto cmfData = GetCMFData();
		if( !cmfData )
		{
			return false;
		}

		if( cmfData->skeletons.size() == 0 )
		{
			return false;
		}

		if( cmfData->meshes.size() == 0 )
		{
			return false;
		}

		if( !m_useMeshBinding )
		{
			return false;
		}

		auto& skeleton = cmfData->skeletons[m_modelIndex];

		auto boneBindings = cmfData->meshes[0].boneBindings;
		m_boneBounds.clear();
		for( int boneIdx = 0; boneIdx < skeleton.bones.size(); boneIdx++ )
		{
			auto name = skeleton.bones[boneIdx];
			auto boneBinding = std::find_if( boneBindings.begin(), boneBindings.end(), [name]( const cmf::BoneBinding& bb ) {
				return name == bb.name;
			} );
			if( boneBinding != boneBindings.end() )
			{
				GrannyBoneBindingBounds bounds;
				bounds.m_boneIndex = boneIdx;
				auto minBounds = boneBinding->bounds.m_min;
				auto maxBounds = boneBinding->bounds.m_max;
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

		return true;
	}
#if WITH_GRANNY
	else
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
#else
	else
	{
		return false;
	}
#endif
}

bool Tr2GrannyAnimation::GetDynamicBounds( Vector4& boundingSphere, Vector3& aabbMin, Vector3& aabbMax )
{
	if( IsUsingCMF() )
	{
		Vector3 transformed[8];
		if( m_boneBounds.empty() && !InitializeBoundingInfo() )
		{
			return false;
		}

		BoundingSphereInitialize( boundingSphere );
		BoundingBoxInitialize( aabbMin, aabbMax );

		for( const auto& bb : m_boneBounds )
		{
			const Matrix& mat = m_worldTransforms[bb.m_boneIndex];

			for( unsigned point = 0; point < 8; point++ )
			{
				transformed[point] = TransformCoord( bb.m_corners[point], mat );
				BoundingBoxUpdate( aabbMin, aabbMax, transformed[point] );
				BoundingSphereUpdate( transformed[point], boundingSphere );
			}
		}

		return true;
	}
#if WITH_GRANNY
	else
	{
		Vector3 transformed[8];
		if( m_boneBounds.empty() && !InitializeBoundingInfo() )
		{
			return false;
		}

		BoundingSphereInitialize( boundingSphere );
		BoundingBoxInitialize( aabbMin, aabbMax );

		for( const auto& bb : m_boneBounds )
		{
			const Matrix* mat = reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bb.m_boneIndex ) );

			for( unsigned point = 0; point < 8; point++ )
			{
				transformed[point] = TransformCoord( bb.m_corners[point], *mat );
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
#else
	else
	{
		return false;
	}
#endif
}

void Tr2GrannyAnimation::RenderBones( const Matrix& modelTransform, const Tr2AnimationMeshBinding* meshBinding )
{
	if( IsUsingCMF() )
	{
		const int* bi = nullptr;
		size_t boneCount = 0;
		if( meshBinding )
		{
			bi = meshBinding->GetAnimBoneIndices().data();
			boneCount = meshBinding->GetAnimBoneIndices().size();
		}
		else
		{
			bi = m_skeletonBoneIndices.data();
			boneCount = m_skeletonBoneIndices.size();
		}

		if( !bi )
		{
			return;
		}

		auto& skeleton = GetCMFData()->skeletons[m_modelIndex];

		for( int boneIdx = 0; boneIdx < boneCount; boneIdx++ )
		{
			if( bi[boneIdx] < 0 )
			{
				continue;
			}

			Matrix mat = m_worldTransforms[bi[boneIdx]] * modelTransform;
			Vector4 pos( 0, 0, 0, 1 );
			pos = Transform( pos, mat );
			pos.w = 2;
			Tr2Renderer::DrawSphere( pos, 1, 0xffffffff );
			std::string tmp = cmf::ToStdString( skeleton.bones[bi[boneIdx]] );
			Tr2Renderer::Printf( static_cast<TriDebugFont>( g_debugBoneLabelFont ), Vector3( pos.x, pos.y, pos.z ), 0xffffffff, "  %s : %d", tmp.c_str(), boneIdx );

			int32_t parent = (int32_t)skeleton.parents[bi[boneIdx]];
			if( parent > 0 )
			{
				Matrix pmat = m_worldTransforms[parent] * modelTransform;
				Vector3 ppos = TransformCoord( Vector3( 0, 0, 0 ), pmat );
				Tr2Renderer::DrawLine( pos.GetXYZ(), ppos, 0x88888888 );
			}
		}
	}
#if WITH_GRANNY
	else
	{
		auto binding = meshBinding ? meshBinding->GetGrannyMeshBinding() : m_meshBinding;
		if( !binding )
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

		auto viewProj = Tr2Renderer::GetViewTransform() * Tr2Renderer::GetProjectionTransform();
		float screenRadius = 2.f / float( std::max( 1, std::min( Tr2Renderer::GetViewport().width, Tr2Renderer::GetViewport().height ) ) );

		auto boneCount = GrannyGetMeshBindingBoneCount( binding );
		for( int boneIdx = 0; boneIdx < boneCount; boneIdx++ )
		{
			const int* bi = GrannyGetMeshBindingToBoneIndices( binding );
			Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bi[boneIdx] ) ) * modelTransform * initialTranslation;
			Vector4 pos( 0, 0, 0, 1 );
			pos = Transform( pos, mat );
			pos.w = 2;
			Tr2Renderer::DrawSphere( pos, 1, 0xffffffff );
			Tr2Renderer::Printf( static_cast<TriDebugFont>( g_debugBoneLabelFont ), Vector3( pos.x, pos.y, pos.z ), 0xffffffff, "  %s : %d", m_skeleton->Bones[bi[boneIdx]].Name, boneIdx );

			auto parent = m_skeleton->Bones[bi[boneIdx]].ParentIndex;
			if( parent > 0 )
			{
				Matrix pmat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, parent ) ) * modelTransform * initialTranslation;
				Vector3 ppos = TransformCoord( Vector3( 0, 0, 0 ), pmat );
				Tr2Renderer::DrawLine( pos.GetXYZ(), ppos, 0x88888888 );
			}
		}
	}
#endif
}

void Tr2GrannyAnimation::RenderDynamicBounds( const Matrix& modelTransform )
{
	if( IsUsingCMF() )
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

		for( const auto& bb : m_boneBounds )
		{
			Matrix mat = m_worldTransforms[bb.m_boneIndex] * modelTransform;

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
#if WITH_GRANNY
	else
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

		for( const auto& bb : m_boneBounds )
		{

			Matrix mat = *reinterpret_cast<const Matrix*>( GrannyGetWorldPose4x4( m_worldPose, bb.m_boneIndex ) ) * modelTransform * initialTranslation;

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
#endif
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

void Tr2GrannyAnimation::AddSecondaryResPath( const std::string& val )
{
	auto fi = m_secondaryGrannyRes.find( val );
	if( fi == m_secondaryGrannyRes.end() )
	{
		m_secondaryGrannyRes[val] = nullptr;
		LoadSecondaryResPath( val );
	}
}

const std::string Tr2GrannyAnimation::GetSecondaryAnimationName( const std::string& resPath, int index ) const
{
	auto fi = m_secondaryGrannyRes.find( resPath );
	if( fi != m_secondaryGrannyRes.end() )
	{
		auto animName = fi->second->GetAnimationName( index );
		return animName;
	}
	return "";
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

#if WITH_GRANNY
bool Tr2GrannyAnimation::CalculateSkinnedBoundingBoxFromTransform( const Matrix& transform, Vector3& bbMin, Vector3& bbMax, granny_file_info* fi )
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

Vector4 Tr2GrannyAnimation::CalculateSkinnedBoundingSphere( granny_file_info* fi )
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
#endif

const cmf::Skeleton* Tr2GrannyAnimation::GetSkeleton() const
{
	CCP_ASSERT( IsUsingCMF() );

	auto cmfData = GetCMFData();
	if( cmfData && m_modelIndex >= 0 && m_modelIndex < cmfData->skeletons.size() )
	{
		return &cmfData->skeletons[m_modelIndex];
	}
	return nullptr;
}

const std::vector<Matrix>& Tr2GrannyAnimation::GetWorldTransforms() const
{
	return this->m_worldTransforms;
}

#if WITH_GRANNY
granny_model* Tr2GrannyAnimation::GetGrannyModel() const
{
	granny_file_info* fi = GetFileInfo();
	if( !fi )
	{
		return nullptr;
	}

	return fi->Models[m_modelIndex];
}
#endif

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

	bool secondaryGrannyResPrepared = true;
	for( auto it = m_secondaryGrannyRes.begin(); it != m_secondaryGrannyRes.end(); it++ )
	{
		secondaryGrannyResPrepared = secondaryGrannyResPrepared && it->second->IsPrepared();
	}

	if( ( !m_grannyRes && !m_geometryRes ) ||
		( m_grannyRes && !m_grannyRes->IsPrepared() ) ||
		( m_geometryRes && !m_geometryRes->IsPrepared() ) ||
		!secondaryGrannyResPrepared )
	{
		layer->QueueAnimation( animName, replace, loopCount, delay, speed, clearWhenDone );
		return true;
	}

	bool secondaryGrannyResGood = true;
	for( auto it = m_secondaryGrannyRes.begin(); it != m_secondaryGrannyRes.end(); it++ )
	{
		secondaryGrannyResGood = secondaryGrannyResGood && it->second->IsGood();
	}

	if( ( ( m_grannyRes && !m_grannyRes->IsGood() ) ||
		  ( m_geometryRes && !m_geometryRes->IsGood() ) ||
		  !secondaryGrannyResGood ) )
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

void Tr2GrannyAnimation::ApplyBoneOffsets( unsigned i )
{
	if( IsUsingCMF() )
	{
		m_boneOffset.ApplyToLocal( i, m_pose.boneTransforms[i].rotation, m_pose.boneTransforms[i].position );
	}
#if WITH_GRANNY
	else
	{
		granny_real32 localMatrix[16];
		GrannyBuildCompositeTransform4x4( GrannyGetLocalPoseTransform( m_localPose, i ), localMatrix );
		granny_real32* worldMatrix = GrannyGetWorldPose4x4( m_worldPose, i );

		const granny_int32 parentIndex = m_skeleton->Bones[i].ParentIndex;
		if( parentIndex != -1 )
		{
			const granny_real32* parentWorldMatrix = GrannyGetWorldPose4x4( m_worldPose, parentIndex );

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
#endif
}

#if WITH_GRANNY
void Tr2GrannyAnimation::UpdateAimingBone()
{
	if( m_aimingBone )
	{
		granny_int32x boneIndex;
		granny_real32 orientAxis[3] = { m_aimAxis[0], m_aimAxis[1], m_aimAxis[2] };
		granny_real32 target[3] = { m_aimBoneOrientation[0], m_aimBoneOrientation[1], m_aimBoneOrientation[2] };
		granny_real32 offset_matrix[16] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };

		if( GrannyFindBoneByNameLowercase( m_skeleton, m_aimBone.c_str(), &boneIndex ) )
		{
			auto id = IdentityMatrix();
			GrannyBuildWorldPose( m_skeleton, 0, m_skeleton->BoneCount, m_localPose, &id.m[0][0], m_worldPose );
			GrannyIKOrientTowards( boneIndex, orientAxis, target, m_skeleton, m_localPose, offset_matrix, m_worldPose );
		}
	}
}

void Tr2GrannyAnimation::UpdateDebugRenderer( const Matrix& modelTransform )
{
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
}
#endif

void Tr2GrannyAnimation::UpdateAimingBone( const cmf::Skeleton& skeleton )
{
	if( m_aimingBone )
	{
		int32_t boneIndex = -1;

		// find bone by name lowercase
		auto CaseInsensitiveEquals = []( const cmf::String& a, const std::string& b ) {
			return a.size() == b.size() && std::equal( a.begin(), a.end(), b.begin(), []( unsigned char ac, unsigned char bc ) {
					   return std::tolower( ac ) == std::tolower( bc );
				   } );
		};
		auto found = std::find_if( skeleton.bones.begin(), skeleton.bones.end(), [&]( const auto& boneName ) {
			return CaseInsensitiveEquals( boneName, m_aimBone );
		} );
		if( found != skeleton.bones.end() )
		{
			boneIndex = static_cast<int32_t>( std::distance( skeleton.bones.begin(), found ) );
		}

		if( boneIndex != -1 )
		{
			// TODO: intern, verify that this is the minimal rotation (by the "great circle on the hypersphere" metric)
			// single bone inverse kinematics
			cmf::ComputeWorldTransforms( m_worldTransforms, m_pose );

			uint32_t parentIdx = skeleton.parents[boneIndex];
			Matrix parentWorld = ( parentIdx != -1 ) ? m_worldTransforms[parentIdx] : IdentityMatrix();

			// careful: non-uniform scaling would require Inverse instead of Transpose!
			Matrix parentWorldInv = Transpose( parentWorld );
			Vector3 bonePos = TransformCoord( Vector3( 0, 0, 0 ), m_worldTransforms[boneIndex] );
			Vector3 worldDirection = Normalize( m_aimBoneOrientation - bonePos );
			Vector3 localTarget = Normalize( TransformNormal( worldDirection, parentWorldInv ) );

			Quaternion correction;
			TriQuaternionRotationArc( &correction, &m_aimAxis, &localTarget );

			m_pose.boneTransforms[boneIndex].rotation = correction;
		}
	}
}

void Tr2GrannyAnimation::UpdateDebugRenderer( const cmf::Skeleton& skeleton, const Matrix& modelTransform )
{
	extern ITr2DebugRendererPtr g_debugRenderer;
	if( g_debugRenderer )
	{
		if( m_debugRenderSkeleton )
		{
			for( int i = 0; i < skeleton.bones.size(); ++i )
			{
				int parentIx = skeleton.parents[i];
				if( parentIx != -1 )
				{
					Matrix fromMat = m_worldTransforms[parentIx];
					Matrix toMat = m_worldTransforms[i];

					// Transform to our world coordinates
					fromMat = fromMat * modelTransform;
					toMat = toMat * modelTransform;

					g_debugRenderer->DrawLine( fromMat.GetTranslation(), toMat.GetTranslation() );
				}
			}
		}
		if( m_debugRenderJointNames )
		{
			for( int i = 0; i < skeleton.bones.size(); ++i )
			{
				std::string name = cmf::ToStdString( skeleton.bones[i] );
				Matrix m = m_worldTransforms[i];

				// Transform to our world coordinates
				m = m * modelTransform;

				g_debugRenderer->Printf( m.GetTranslation(), 0xffffffff, name.c_str() );
			}
		}
	}
}

void Tr2GrannyAnimation::PrePhysicsAnimation( Be::Time time, const Matrix& modelTransform )
{
	if( IsUsingCMF() )
	{
		if( IsInitialized() && m_animationEnabled )
		{
			auto& skeleton = GetCMFData()->skeletons[m_modelIndex];
			float animationTime = GetAnimationTime();

			m_morphAnimations.clear();

			m_baseLayer.SampleAnimation( animationTime, &m_pose, m_eventListener, m_morphAnimations );
			for( auto& [_, layer] : m_animationLayers )
			{
				layer.SampleAnimation( animationTime, &m_tmpPose, &m_pose, m_eventListener, m_morphAnimations, m_additiveMode );
			}

			UpdateAimingBone( skeleton );

			if( m_boneOffset.NeedRebind( (uint32_t)skeleton.bones.size() ) && skeleton.bones.size() )
			{
				std::vector<std::string> bones( skeleton.bones.size() );
				for( size_t i = 0; i < bones.size(); ++i )
				{
					bones[i] = cmf::ToStdString( skeleton.bones[i] );
				}
				m_boneOffset.BindToRig( &bones[0], bones.size() );
			}

			if( m_boneOffset.HaveTransforms() )
			{
				for( int32_t i = 0; i < skeleton.bones.size(); i++ )
				{
					ApplyBoneOffsets( i );
				}
			}

			cmf::ComputeWorldTransforms( m_worldTransforms, m_pose );

			if( m_useMeshBinding && m_meshBoneMatrixList && m_meshBoneCount )
			{
				for( int32_t i = 0; i < m_meshBoneCount; i++ )
				{
					int32_t boneIdx = m_skeletonBoneIndices[i];
					if( boneIdx >= 0 )
					{
						Matrix skinMatrix = skeleton.invBindTransforms[boneIdx] * m_worldTransforms[boneIdx];
						m_meshBoneMatrixList[i] = Float4x3( skinMatrix );
					}
				}
			}

			UpdateDebugRenderer( skeleton, modelTransform );
		}
	}
#if WITH_GRANNY
	else
	{
		if( IsInitialized() && m_animationEnabled )
		{
			float animationTime = GetAnimationTime();

			m_morphAnimations.clear();

			m_baseLayer.SampleAnimation( animationTime, m_localPose, m_eventListener, m_morphAnimations );
			for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
			{
				it->second.SampleAnimation( animationTime, m_compositePose, m_localPose, m_eventListener, m_morphAnimations, m_additiveMode );
			}

			UpdateAimingBone();

			if( m_boneOffset.NeedRebind( m_skeleton->BoneCount ) && m_skeleton->BoneCount )
			{
				std::vector<std::string> bones( m_skeleton->BoneCount );
				for( size_t i = 0; i < bones.size(); ++i )
					bones[i] = m_skeleton->Bones[i].Name;
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
						GrannyBuildIndexedCompositeBufferTransposed( m_skeleton, m_worldPose, meshToBone, m_meshBoneCount, (granny_matrix_3x4*)m_meshBoneMatrixList );
					}
				}
			}
			else
			{
				for( unsigned i = 0; i != m_skeleton->BoneCount; ++i )
				{
					ApplyBoneOffsets( i );
				}
			}

			UpdateDebugRenderer( modelTransform );
		}
	}
#endif
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

const std::string* Tr2GrannyAnimation::GetAnimationBoneList( unsigned int& numBones ) const
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
	if( IsUsingCMF() )
	{
		if( !m_worldTransforms.empty() )
		{
			return m_worldTransforms.data();
		}
	}
#if WITH_GRANNY
	else
	{
		if( m_worldPose )
		{
			return reinterpret_cast<Matrix*>( GrannyGetWorldPose4x4Array( m_worldPose ) );
		}
	}
#endif

	return NULL;
}

void Tr2GrannyAnimation::Cleanup()
{
	if( m_grannyRes || m_geometryRes )
	{
		for( auto& notify : m_notifyTargets )
		{
			notify->ReleaseCachedData( m_grannyRes ? static_cast<BlueAsyncRes*>( m_grannyRes ) : m_geometryRes );
		}
	}

	m_baseLayer.Cleanup();
	for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
	{
		it->second.Cleanup();
	}

	m_pose.boneTransforms.clear();
	m_pose.skeleton = nullptr;
	m_skeletonBoneIndices.clear();
	m_worldTransforms.clear();

#if WITH_GRANNY
	if( m_localPose )
	{
		GrannyFreeLocalPose( m_localPose );
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

	m_skeleton = nullptr;
#endif

	m_boneList.clear();

	if( m_meshBoneMatrixList )
	{
		CCP_ALIGNED_FREE( m_meshBoneMatrixList );
		m_meshBoneMatrixList = nullptr;
	}
}

bool Tr2GrannyAnimation::FindBoneByName( const char* name, unsigned int& ix ) const
{
	if( IsUsingCMF() )
	{
		auto cmfData = GetCMFData();
		if( cmfData && m_modelIndex != -1 )
		{
			auto& skeleton = cmfData->skeletons[m_modelIndex];
			for( int32_t i = 0; i < skeleton.bones.size(); i++ )
			{
				if( cmf::ToStdStringView( skeleton.bones[i] ) == name )
				{
					ix = i;
					return true;
				}
			}
		}
	}
#if WITH_GRANNY
	else
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
	}
#endif

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
const Float4x3* Tr2GrannyAnimation::GetMeshBoneMatrixList() const
{
	return m_meshBoneMatrixList;
}

void Tr2GrannyAnimation::PlayAnimationOnce( const char* animName )
{
	PlayAnimation( animName, true, 1, 0.0f, 1.0f );
}

void Tr2GrannyAnimation::PlayAnimationEx( const char* animName, int loopCount, float delay, float speed, Be::OptionalWithDefaultValue<bool, true> clearWhenDone )
{
	PlayAnimation( animName, true, loopCount, delay, speed, clearWhenDone );
}

void Tr2GrannyAnimation::ChainAnimation( const char* animName )
{
	PlayAnimation( animName, false, 1, 0.0f, 1.0f );
}

void Tr2GrannyAnimation::ChainAnimationEx( const char* animName, int loopCount, float delay, float speed )
{
	PlayAnimation( animName, false, loopCount, delay, speed );
}

float Tr2GrannyAnimation::GetAnimationTime()
{
	if( m_paused )
	{
		return m_pauseTime;
	}
	return Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
}

void Tr2GrannyAnimation::TogglePauseAnimations( bool pause )
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
	for( auto it = m_animationLayers.begin(); it != m_animationLayers.end(); it++ )
	{
		it->second.TogglePauseAnimation( pause );
	}
}

bool Tr2GrannyAnimation::IsInitialized() const
{
	if( IsUsingCMF() )
	{
		return m_baseLayer.m_sequencer != nullptr;
	}
#if WITH_GRANNY
	else
	{
		return m_baseLayer.m_modelInstance != nullptr;
	}
#else
	else
	{
		return false;
	}
#endif
}

void Tr2GrannyAnimation::AddAnimationLayer( const char* layerName, float layerWeight )
{
	if( GetAnimationLayer( layerName ) )
	{
		return;
	}

	Tr2GrannyAnimationLayer layer( 0.f, layerWeight );
	layer.m_name = layerName;
	m_animationLayers[layerName] = std::move( layer );

	if( !IsInitialized() )
	{
		return;
	}

	if( IsUsingCMF() )
	{
		if( !m_tmpPose.skeleton )
		{
			auto& skeleton = GetCMFData()->skeletons[m_modelIndex];
			cmf::RestPose( m_tmpPose, skeleton );
		}
	}
#if WITH_GRANNY
	if( !IsUsingCMF() )
	{
		if( !m_compositePose )
		{
			m_compositePose = GrannyNewLocalPose( m_skeleton->BoneCount );
		}
	}

#endif

	GetAnimationLayer( layerName )->InitializeAnimationLayer( this );
}

float Tr2GrannyAnimation::GetLayerWeight( const char* layerName )
{
	if( GetAnimationLayer( layerName ) )
	{
		return GetAnimationLayer( layerName )->GetLayerWeight();
	}
	return 0.f;
}

void Tr2GrannyAnimation::SetLayerWeight( const char* layerName, float layerWeight )
{
	if( GetAnimationLayer( layerName ) )
	{
		return GetAnimationLayer( layerName )->SetLayerWeight( layerWeight );
	}
}

void Tr2GrannyAnimation::SetLayerControlParam( const char* layerName, float controlParam )
{
	const char* base_name_string = "";

	if( !layerName || !strcmp( layerName, base_name_string ) )
	{
		GetAnimationLayer( nullptr )->SetControlParam( controlParam );
	}
	else
	{
		auto animLayer = GetAnimationLayer( layerName );
		if( !animLayer )
		{
			CCP_LOGERR( "SetLayerControlParam: Layer name '%s' not found.", layerName );
			return;
		}
		animLayer->SetControlParam( controlParam );
	}
}

void Tr2GrannyAnimation::SetLayerControlParamSkewRate( const char* layerName, float skewRate )
{
	const char* base_name_string = "";
	Tr2GrannyAnimationLayer* layer;

	if( !strcmp( layerName, base_name_string ) )
	{
		layer = GetAnimationLayer( nullptr );
		if( !layer )
		{
			return;
		}
		layer->SetControlParamSkewRate( skewRate );
	}
	else
	{
		layer = GetAnimationLayer( layerName );
		if( !layer )
		{
			return;
		}
		layer->SetControlParamSkewRate( skewRate );
	}
}

void Tr2GrannyAnimation::AimBone( const char* boneName, float target_x, float target_y, float target_z, float axis_x, float axis_y, float axis_z )
{
	m_aimingBone = true;
	m_aimBone = boneName;

	m_aimBoneOrientation[0] = target_x;
	m_aimBoneOrientation[1] = target_y;
	m_aimBoneOrientation[2] = target_z;

	m_aimAxis[0] = axis_x;
	m_aimAxis[1] = axis_y;
	m_aimAxis[2] = axis_z;
}

void Tr2GrannyAnimation::DisableAimBone()
{
	m_aimingBone = false;
}

void Tr2GrannyAnimation::SetAdditiveBlendMode( bool additive )
{
	m_additiveMode = additive;
}

bool Tr2GrannyAnimation::GetAdditiveBlendMode()
{
	return m_additiveMode;
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
	m_animationLayers[layerName] = std::move( layer );

	if( !IsInitialized() )
	{
		return;
	}

	if( IsUsingCMF() )
	{
		if( !m_tmpPose.skeleton )
		{
			auto& skeleton = GetCMFData()->skeletons[m_modelIndex];
			cmf::RestPose( m_tmpPose, skeleton );
		}
	}
#if WITH_GRANNY
	if( !IsUsingCMF() )
	{
		if( !m_compositePose )
		{
			m_compositePose = GrannyNewLocalPose( m_skeleton->BoneCount );
		}
	}

#endif
	GetAnimationLayer( layerName )->InitializeAnimationLayer( this );

	return;
}

void Tr2GrannyAnimation::AddAnimationLayerBone( const char* layerName, const char* boneName )
{
	Tr2GrannyAnimationLayer* layer;
	if( !strcmp( layerName, "" ) )
	{
		layer = GetAnimationLayer( nullptr );
	}
	else
	{
		layer = GetAnimationLayer( layerName );
	}

	if( !layer )
	{
		return;
	}

	layer->AddBone( this, boneName );
}


void Tr2GrannyAnimation::AddAnimationLayerAllBones( const char* layerName )
{
	Tr2GrannyAnimationLayer* layer;
	if( !strcmp( layerName, "" ) )
	{
		layer = GetAnimationLayer( nullptr );
	}
	else
	{
		layer = GetAnimationLayer( layerName );
	}

	if( !layer )
	{
		return;
	}

	layer->AddAllBones( this );
}


void Tr2GrannyAnimation::RemoveAnimationLayerBone( const char* layerName, const char* boneName )
{
	Tr2GrannyAnimationLayer* layer;
	if( !strcmp( layerName, "" ) )
	{
		layer = GetAnimationLayer( nullptr );
	}
	else
	{
		layer = GetAnimationLayer( layerName );
	}

	if( !layer )
	{
		return;
	}

	layer->RemoveBone( this, boneName );
}

std::vector<std::string> Tr2GrannyAnimation::GetAnimationNames() const
{
	if( IsUsingCMF() )
	{
		std::vector<std::string> names;

		if( auto cmfData = GetCMFData() )
		{
			for( auto& animation : cmfData->animations )
			{
				names.push_back( cmf::ToStdString( animation.name ) );
			}
		}

		for( auto& [resPath, res] : m_secondaryGrannyRes )
		{
			if( !res )
			{
				continue;
			}

			CCP_ASSERT_M( res->IsUsingCMF(), "Tr2GrannyAnimation::GetAnimationNames: main data is cmf, secondary data is granny!" );

			if( auto cmfData = GetSecondaryCMFFileInfo( resPath, res ) )
			{
				for( auto& animation : cmfData->animations )
				{
					names.push_back( cmf::ToStdString( animation.name ) );
				}
			}
		}

		return names;
	}
#if WITH_GRANNY
	else
	{
		std::vector<std::string> names;

		auto CopyNames = [&]( granny_file_info* f ) {
			for( granny_int32 i = 0; i < f->AnimationCount; ++i )
			{
				names.push_back( f->Animations[i]->Name );
			}
		};

		if( auto fi = GetFileInfo() )
		{
			CopyNames( fi );
		}

		for( auto it = begin( m_secondaryGrannyRes ); it != end( m_secondaryGrannyRes ); ++it )
		{
			if( !it->second )
			{
				continue;
			}

			CCP_ASSERT_M( !it->second->IsUsingCMF(), "Tr2GrannyAnimation::GetAnimationNames: main data is granny, secondary data is cmf!" );

			if( auto fi = GetSecondaryFileInfo( it->first, it->second ) )
			{
				CopyNames( fi );
			}
		}

		return names;
	}
#else
	else
	{
		return {};
	}
#endif
}

void Tr2GrannyAnimation::AddNotifyTarget( IBlueAsyncResNotifyTarget* p )
{
	if( find( begin( m_notifyTargets ), end( m_notifyTargets ), p ) == end( m_notifyTargets ) )
	{
		m_notifyTargets.push_back( p );
	}
}

void Tr2GrannyAnimation::RemoveNotifyTarget( IBlueAsyncResNotifyTarget* p )
{
	m_notifyTargets.erase( remove( begin( m_notifyTargets ), end( m_notifyTargets ), p ), end( m_notifyTargets ) );
}

const std::unordered_map<std::string, float>& Tr2GrannyAnimation::GetMorphAnimations() const
{
	return m_morphAnimations;
}

Tr2AnimationMeshBinding::Tr2AnimationMeshBinding( Tr2GrannyAnimation* animationUpdater, TriGeometryRes* geometryRes, uint32_t meshIndex ) :
	m_animation( animationUpdater ),
	m_geometryRes( geometryRes ),
	m_meshIndex( meshIndex ),
	m_cmfSkeleton( nullptr )
{
	if( geometryRes )
	{
		geometryRes->AddNotifyTarget( this );
	}
	if( animationUpdater )
	{
		animationUpdater->AddNotifyTarget( this );
	}
}

Tr2AnimationMeshBinding::~Tr2AnimationMeshBinding()
{
	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}
	if( m_animation )
	{
		m_animation->RemoveNotifyTarget( this );
	}
}

std::pair<const Float4x3*, size_t> Tr2AnimationMeshBinding::GetBoneTransforms() const
{
	if( m_animation->IsUsingCMF() )
	{
		if( !m_cmfSkeleton || !m_boneTransforms )
		{
			return std::make_pair( nullptr, 0 );
		}

		auto boneCount = m_geometryRes->GetCMFData()->meshes[m_meshIndex].boneBindings.size();

		if( m_boneTransforms && m_bindBoneIndices.size() && boneCount )
		{
			auto& worldTransforms = m_animation->GetWorldTransforms();
			if( m_cmfSkeleton != m_animation->GetSkeleton() )
			{
				CCP_ASSERT( boneCount <= m_animBoneIndices.size() );
				for( int32_t i = 0; i < boneCount; ++i )
				{
					if( m_animBoneIndices[i] == -1 )
					{
						m_boneTransforms[i] = Float4x3( Matrix() );
						continue;
					}
					CCP_ASSERT( m_bindBoneIndices[i] < m_cmfSkeleton->invBindTransforms.size() );
					CCP_ASSERT( m_animBoneIndices[i] < worldTransforms.size() );
					Matrix skinMatrix = m_cmfSkeleton->invBindTransforms[m_bindBoneIndices[i]] * worldTransforms[m_animBoneIndices[i]];
					m_boneTransforms[i] = Float4x3( skinMatrix );
				}
			}
			else
			{
				for( int32_t i = 0; i < boneCount; i++ )
				{
					int32_t boneIdx = m_bindBoneIndices[i];
					if( boneIdx >= 0 )
					{
						Matrix skinMatrix = m_cmfSkeleton->invBindTransforms[boneIdx] * worldTransforms[boneIdx];
						m_boneTransforms[i] = Float4x3( skinMatrix );
					}
				}
			}
		}

		return { m_boneTransforms.get(), boneCount };
	}
#if WITH_GRANNY
	else
	{
		if( !m_meshBinding || !m_boneTransforms )
		{
			return std::make_pair( nullptr, 0 );
		}

		auto boneCount = GrannyGetMeshBindingBoneCount( m_meshBinding.get() );

		auto meshToBone = GrannyGetMeshBindingFromBoneIndices( m_meshBinding.get() );
		if( m_boneTransforms && meshToBone && boneCount )
		{
			if( m_meshSkeleton != m_animation->m_skeleton )
			{
				const auto animBones = GrannyGetMeshBindingToBoneIndices( m_meshBinding.get() );
				for( int32_t i = 0; i < boneCount; ++i )
				{
					GrannyColumnMatrixMultiply4x3Transpose(
						(granny_real32*)( (granny_matrix_3x4*)m_boneTransforms.get() )[i],
						(granny_real32*)m_meshSkeleton->Bones[meshToBone[i]].InverseWorld4x4,
						(granny_real32*)GrannyGetWorldPose4x4( m_animation->m_worldPose, animBones[i] ) );
				}
			}
			else
			{
				GrannyBuildIndexedCompositeBufferTransposed( m_animation->m_skeleton, m_animation->m_worldPose, meshToBone, boneCount, (granny_matrix_3x4*)m_boneTransforms.get() );
			}
		}

		return { m_boneTransforms.get(), boneCount };
	}
#else
	else
	{
		return std::make_pair( nullptr, 0 );
	}
#endif
}

TriGeometryRes* Tr2AnimationMeshBinding::GetGeometryRes() const
{
	return m_geometryRes;
}

uint32_t Tr2AnimationMeshBinding::GetMeshIndex() const
{
	return m_meshIndex;
}

Tr2GrannyAnimation* Tr2AnimationMeshBinding::GetAnimation() const
{
	return m_animation;
}

#if WITH_GRANNY
const granny_mesh_binding* Tr2AnimationMeshBinding::GetGrannyMeshBinding() const
{
	return m_meshBinding.get();
}
#endif

void Tr2AnimationMeshBinding::CreateBinding()
{
	if( m_geometryRes && m_geometryRes->IsGood() && m_animation && m_animation->IsInitialized() )
	{
		if( m_animation->IsUsingCMF() )
		{
			auto cmfData = m_geometryRes->GetCMFData();
			if( !cmfData )
			{
				return;
			}
			if( int( m_meshIndex ) >= cmfData->meshes.size() || int( m_meshIndex ) < 0 )
			{
				return;
			}
			auto& mesh = cmfData->meshes[m_meshIndex];

			// prefer m_geometryRes skeleton, use m_animation skeleton as fallback
			m_cmfSkeleton = mesh.skeleton < cmfData->skeletons.size() ? &cmfData->skeletons[mesh.skeleton] : m_animation->GetSkeleton();

			if( m_cmfSkeleton )
			{
				if( mesh.boneBindings.size() > 0 )
				{
					m_boneTransforms.reset( new Float4x3[mesh.boneBindings.size()] );
				}

				m_bindBoneIndices = Tr2GrannyAnimationUtils::CreateMapping( *m_cmfSkeleton, mesh.boneBindings, (uint32_t)mesh.boneBindings.size() );
				m_animBoneIndices = Tr2GrannyAnimationUtils::CreateMapping( *m_animation->GetSkeleton(), mesh.boneBindings, (uint32_t)mesh.boneBindings.size() );
			}
		}
#if WITH_GRANNY
		else
		{
			auto fi = m_geometryRes->GetGrannyInfo();
			if( !fi )
			{
				return;
			}
			if( int( m_meshIndex ) >= fi->MeshCount || int( m_meshIndex ) < 0 )
			{
				return;
			}
			auto mesh = fi->Meshes[m_meshIndex];
			auto meshSkeleton = m_animation->m_skeleton;
			for( int32_t i = 0; i < fi->ModelCount; ++i )
			{
				auto model = fi->Models[i];
				if( model && model->Skeleton )
				{
					auto found = std::find_if( model->MeshBindings, model->MeshBindings + model->MeshBindingCount, [mesh]( const auto& binding ) { return binding.Mesh == mesh; } );
					if( found != model->MeshBindings + model->MeshBindingCount )
					{
						meshSkeleton = model->Skeleton;
						break;
					}
				}
			}

			m_meshBinding.reset( GrannyNewMeshBinding( fi->Meshes[m_meshIndex], meshSkeleton, m_animation->m_skeleton ) );
			if( m_meshBinding )
			{
				auto count = GrannyGetMeshBindingBoneCount( m_meshBinding.get() );
				if( count > 0 )
				{
					m_boneTransforms.reset( new Float4x3[count] );
					m_meshSkeleton = meshSkeleton;
				}
			}
		}
#endif
	}
}

void Tr2AnimationMeshBinding::ReleaseCachedData( BlueAsyncRes* )
{
#if WITH_GRANNY
	m_meshBinding.reset();
	m_meshSkeleton = nullptr;
#endif
	m_boneTransforms.reset();
	m_bindBoneIndices.clear();
	m_animBoneIndices.clear();
	m_cmfSkeleton = nullptr;
}

void Tr2AnimationMeshBinding::RebuildCachedData( BlueAsyncRes* )
{
	CreateBinding();
}

const std::vector<int32_t>& Tr2AnimationMeshBinding::GetAnimBoneIndices() const
{
	CCP_ASSERT( m_animation->IsUsingCMF() );

	return m_animBoneIndices;
}

bool Tr2AnimationMeshBinding::HasMeshBinding() const
{
	if( m_animation->IsUsingCMF() )
	{
		return !m_animBoneIndices.empty();
	}
#if WITH_GRANNY
	else
	{
		return m_meshBinding != nullptr;
	}
#else
	else
	{
		return false;
	}
#endif
}

const std::pair<const int32_t*, size_t> Tr2AnimationMeshBinding::GetMeshBindingIndices() const
{
	if( m_animation->IsUsingCMF() )
	{
		return std::make_pair( m_animBoneIndices.data(), m_animBoneIndices.size() );
	}
#if WITH_GRANNY
	else
	{
		auto boneCount = GrannyGetMeshBindingBoneCount( m_meshBinding.get() );
		return std::make_pair( GrannyGetMeshBindingToBoneIndices( m_meshBinding.get() ), boneCount );
	}
#else
	else
	{
		return std::make_pair( nullptr, 0 );
	}
#endif
}
