// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SkinnedModel.h"
#include "Tr2Mesh.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2PerObjectData.h"
#include "Utilities/BoundingBox.h"


static const unsigned int NO_SKELETON = 0xffffffff;

Tr2SkinnedModel::Tr2SkinnedModel( IRoot* lockobj ) :
	Tr2Model( lockobj ),
	m_areAllMeshesBound( false ),
	m_skeletonIx( NO_SKELETON ),
	m_skinScale( 1.f, 1.f, 1.f ),
	m_pBoneList( NULL ),
	m_numBones( 0 )
{
}

Tr2SkinnedModel::~Tr2SkinnedModel()
{
	ReleaseCachedData( m_geometryRes );
	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
	}
}

void Tr2SkinnedModel::GetBatchesForArea( Tr2MeshAreaVector* areas, Tr2Mesh* mesh, ITriRenderBatchAccumulator* batches, const Matrix* pm, Tr2PerObjectDataSkinned* skinnedData )
{
	if( !mesh->GetDisplay() )
	{
		return;
	}

	TriGeometryRes* geomRes = mesh->GetGeometryResource();
	if( !geomRes || !geomRes->IsGood() )
	{
		return;
	}
	int meshIx = mesh->GetMeshIndex();

	TriGeometryResLodData* lod = geomRes->GetMeshLod( meshIx, 0 );
	if( !lod || !lod->m_allocationsValid )
	{
		return;
	}

	for( auto& area : *areas )
	{
		auto shader = area->GetMaterialInterface();
		if( !area->GetDisplay() || ( !shader ) )
		{
			continue;
		}
		Tr2PerAreaDataSkinned* areaData = batches->Allocate<Tr2PerAreaDataSkinned>();

		// Note that this can fail if the accumulator can't add more batches!
		if( areaData )
		{
			// if we have bone-remapping, get jointbindings from area
			TriGeometryResAreaData* areaRes = geomRes->GetAreaData( meshIx, area->GetIndex() );

			unsigned int n = area->GetJointCount();

			// Much more than this would trash the per-frame data
			if( n > TR2_MAX_BONES_PER_MESHAREA )
			{
				continue;
			}

			areaData->SetUserData( skinnedData->GetUserData() );
			unsigned int* animMapping = area->GetJointMappingAnimRig();
			if( ( m_skeletonIx != NO_SKELETON ) && ( animMapping != NULL ) )
			{
				areaData->SetJointCount( n );

				for( unsigned int joint = 0; joint < n; ++joint )
				{
					float* m = skinnedData->GetSkinningMatrix( animMapping[joint] );
					areaData->SetJointTransform( joint, m );
				}
			}
			else
			{
				areaData->SetJointCount( TR2_MAX_BONES_PER_MESHAREA );
				float idMatrix[4 * 3];
				memset( idMatrix, 0, 4 * 3 * sizeof( float ) );
				idMatrix[0] = idMatrix[5] = idMatrix[10] = 1.f;
				for( unsigned int joint = 0; joint < TR2_MAX_BONES_PER_MESHAREA; ++joint )
				{
					areaData->SetJointTransform( joint, idMatrix );
				}
			}
			areaData->SetPerObjectData( *skinnedData );

			auto batch = CreateGeometryBatch( lod, area, areaData );
			batches->Commit( batch );
		}
	}
}

void Tr2SkinnedModel::GetBatches( ITriRenderBatchAccumulator* batches,
								  TriBatchType batchType,
								  const Matrix& m,
								  const Tr2PerObjectData* data )
{
	Matrix* pm = batches->Allocate<Matrix>();

	CCP_ASSERT_M( pm, "No memory available for allocation of batches." );
	if( pm == NULL )
	{
		return;
	}
	*pm = m;

	auto skinnedData = static_cast<const Tr2PerObjectDataSkinned*>( data );

	for( PTr2MeshVector::iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		if( mesh )
		{
			Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );
			if( areas )
			{
				GetBatchesForArea( areas, mesh, batches, pm, const_cast<Tr2PerObjectDataSkinned*>( skinnedData ) );
			}
		}
	}
}

bool Tr2SkinnedModel::HasTransparency() const
{
	// check with each mesh and see if it has any transparent stuff
	for( PTr2MeshVector::const_iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		if( mesh )
		{
			Tr2MeshAreaVector* transparentAreas = mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT );
			if( transparentAreas )
			{
				if( !transparentAreas->empty() )
				{
					return true;
				}
			}
		}
	}
	// no, not a single transparent area found!
	return false;
}

bool Tr2SkinnedModel::GetBoundingBox( Vector3& min, Vector3& max )
{
	return Tr2Model::GetBoundingBox( min, max );
}

void Tr2SkinnedModel::BindToRig( const std::string* boneList, const int numBones, bool forceRebind )
{
	if( !forceRebind && ( boneList == m_pBoneList ) && m_areAllMeshesBound )
	{
		return;
	}

	if( m_skeletonIx == NO_SKELETON )
	{
		return;
	}

	if( !boneList )
	{
		m_pBoneList = NULL;
		m_areAllMeshesBound = false;
		return;
	}

	TriGeometryResSkeletonData* skel = m_geometryRes->GetSkeletonData( m_skeletonIx );

	forceRebind |= !m_areAllMeshesBound; // if a reset was requested, make sure every mesh honors that
	m_areAllMeshesBound = true;
	for( PTr2MeshVector::const_iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		if( !mesh->BindToRig( boneList, numBones, skel, forceRebind ) )
		{
			m_areAllMeshesBound = false;
		}
	}

	m_pBoneList = boneList;
}

void Tr2SkinnedModel::ResetBindings()
{
	m_areAllMeshesBound = false;
}

bool Tr2SkinnedModel::Initialize()
{
	m_skeletonIx = NO_SKELETON;
	m_areAllMeshesBound = false;

	if( m_geometryResPath.empty() )
	{
		return true;
	}

	if( m_geometryRes )
	{
		m_geometryRes->RemoveNotifyTarget( this );
		m_geometryRes.Unlock();
	}
	BeResMan->GetResource( m_geometryResPath.c_str(), "", m_geometryRes );
	if( m_geometryRes )
	{
		m_geometryRes->AddNotifyTarget( this );
	}
	return true;
}

// ---------------------------------------------------------------
bool Tr2SkinnedModel::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_geometryResPath ) )
	{
		Initialize();
	}
	else if( IsMatch( value, m_skeletonName ) )
	{
		Initialize();
	}

	return true;
}

void Tr2SkinnedModel::ReleaseCachedData( BlueAsyncRes* p )
{
	m_skeletonIx = NO_SKELETON;
}

void Tr2SkinnedModel::RebuildCachedData( BlueAsyncRes* p )
{
	m_skeletonIx = NO_SKELETON;
	if( m_geometryRes )
	{
		unsigned int n = m_geometryRes->GetSkeletonCount();
		for( unsigned int i = 0; i < n; ++i )
		{
			TriGeometryResSkeletonData* skel = m_geometryRes->GetSkeletonData( i );
			if( strcmp( skel->m_name.c_str(), m_skeletonName.c_str() ) == 0 )
			{
				m_skeletonIx = i;
				break;
			}
		}
	}
}

TriGeometryResSkeletonData* Tr2SkinnedModel::GetSkeleton() const
{
	if( m_skeletonIx == NO_SKELETON )
	{
		return NULL;
	}

	return m_geometryRes->GetSkeletonData( m_skeletonIx );
}

void Tr2SkinnedModel::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( event == BELIST_INSERTED )
	{
		// Trigger rebinding of meshes
		m_areAllMeshesBound = false;
	}
}

bool Tr2SkinnedModel::GetDynamicBoundingBox( const Matrix* boneTransforms, Vector3& minBounds, Vector3& maxBounds ) const
{
	if( m_meshes.empty() )
	{
		return false;
	}

	BoundingBoxInitialize( minBounds, maxBounds );

	for( auto it = begin( m_meshes ); it != end( m_meshes ); ++it )
	{
		if( auto aabb = ( *it )->GetBounds( boneTransforms ) )
		{
			BoundingBoxUpdate( minBounds, maxBounds, aabb.m_min, aabb.m_max );
		}
	}
	return true;
}