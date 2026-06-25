// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Model.h"
#include "Tr2Mesh.h"
#include "Utilities/BoundingBox.h"
#include "Tr2Renderer.h"
#include "Resources/TriGeometryRes.h"


Tr2Model::Tr2Model( IRoot* lockobj ) :
	PARENTLOCK( m_meshes, IRoot )
{
}

Tr2Model::~Tr2Model()
{
}

bool Tr2Model::HasTransparency() const
{
	for( PTr2MeshVector::const_iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		if( !mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() )
		{
			return true;
		}
	}
	return false;
}

void Tr2Model::GetBatches( ITriRenderBatchAccumulator* batches,
						   TriBatchType batchType,
						   const Matrix& m,
						   const Tr2PerObjectData* data )
{
	Matrix* pm = batches->Allocate<Matrix>();

	CCP_ASSERT_M( pm, "No memory available for opaque batches." );
	if( pm == NULL )
	{
		return;
	}
	*pm = m;

	// Transparent batches are sorted by the mesh bounding box order
	if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		Tr2MeshItemList meshesToSort( "Tr2Model/GetBatches" );

		const Vector3 viewPos = Tr2Renderer::GetViewPosition();

		unsigned int meshCount = (unsigned int)m_meshes.size();
		for( unsigned int meshIx = 0; meshIx < meshCount; ++meshIx )
		{
			Tr2Mesh* pMesh = m_meshes[meshIx];
			if( pMesh->GetDisplay() )
			{
				Tr2MeshItem item;
				item.m_mesh = pMesh;

				auto center = TransformCoord( pMesh->GetBounds().Center(), m );
				item.m_distance = LengthSq( center - viewPos );
				meshesToSort.push_back( item );
			}
		}

		// Sort the list back to front
		std::sort( meshesToSort.begin(), meshesToSort.end() );

		// Now get the batches in order
		for( Tr2MeshItemList::iterator meshIt = meshesToSort.begin();
			 meshIt != meshesToSort.end();
			 ++meshIt )
		{
			Tr2Mesh* mesh = meshIt->m_mesh;
			GetBatchesFromMesh( mesh, batchType, batches, pm, data );
		}
	}
	else
	{
		// In all other cases, we just get the batches in order
		for( Tr2MeshVector::iterator meshIt = m_meshes.begin();
			 meshIt != m_meshes.end();
			 ++meshIt )
		{
			Tr2Mesh* mesh = *meshIt;
			GetBatchesFromMesh( mesh, batchType, batches, pm, data );
		}
	}
}

bool Tr2Model::GetBoundingBox( Vector3& min, Vector3& max )
{
	// if we are still loading, then the bounding box is not yet there...
	if( IsLoading() )
	{
		return false;
	}

	// if we dont have any meshes, the bounding box is invalid...
	if( !m_meshes.size() )
	{
		return false;
	}

	CcpMath::AxisAlignedBox aabb;

	for( Tr2MeshVector::iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;

		auto meshAabb = mesh->GetBounds();

		if( !meshAabb )
		{
			return false;
		}

		aabb.Include( meshAabb );
	}

	min = aabb.m_min;
	max = aabb.m_max;

	return true;
}

bool Tr2Model::IsLoading() const
{
	for( Tr2MeshVector::const_iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		// if one mesh is still loading, this model is not yet ready...
		if( mesh->IsLoading() )
		{
			return true;
		}
	}
	return false;
}


// -------------------------------------------------------------
// Description:
//   This is a helper function for Tr2Model to separate the code for
//
// Arguments:
//	 areaType - the TriBatchType as enumerated in ITr2Renderable
// -------------------------------------------------------------
void Tr2Model::GetBatchesFromMesh( Tr2Mesh* mesh,
								   TriBatchType batchType,
								   ITriRenderBatchAccumulator* batches,
								   Matrix* pm,
								   const Tr2PerObjectData* data )
{
	if( !mesh->GetDisplay() )
	{
		return;
	}

	Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );

	if( !areas )
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

		if( !area->GetDisplay() )
		{
			continue;
		}
		if( !shader )
		{
			continue;
		}

		auto batch = CreateGeometryBatch( lod, area, data );
		batches->Commit( batch );
	}
}

void Tr2Model::AddMesh( Tr2Mesh* mesh )
{
	m_meshes.Insert( -1, mesh->GetRawRoot() );
}

Be::Result<std::string> Tr2Model::GetBoundingBoxInLocalSpace( std::pair<Vector3, Vector3>& result )
{
	bool bbFound = false;
	CcpMath::AxisAlignedBox aabb;
	for( unsigned int i = 0, count = GetNumOfMeshes(); i < count; ++i )
	{
		auto meshAabb = GetMesh( i )->GetBounds();

		if( meshAabb )
		{
			bbFound = true;
			aabb.Include( meshAabb );
		}
	}

	if( !bbFound )
	{
		return Be::Result<std::string>( "No meshes, or meshes aren't loaded yet." );
	}

	result = std::make_pair( aabb.m_min, aabb.m_max );
	return Be::Result<std::string>();
}
