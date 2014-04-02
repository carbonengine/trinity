#include "StdAfx.h"
#include "Tr2Model.h"
#include "Tr2Mesh.h"
#include "Utilities/BoundingBox.h"

Tr2Model::Tr2Model( IRoot* lockobj ):
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
	if ( pm == NULL )
	{
		return;
	}
    *pm = m;

	// Transparent batches are sorted by the mesh bounding box order
	if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		Tr2MeshItemList meshesToSort( "Tr2Model/GetBatches" );

		unsigned int meshCount = (unsigned int)m_meshes.size();
		for( unsigned int meshIx = 0; meshIx < meshCount; ++meshIx )
		{
			Tr2Mesh* pMesh = m_meshes[meshIx];
			if( pMesh->IsHidden() )
			{
				continue;
			}
			Tr2MeshItem item;
			item.m_mesh = pMesh;
			item.m_distance = pMesh->CalcMeshSortValue( m );
			meshesToSort.push_back( item );
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

    Vector3 myMin, myMax;

    BoundingBoxInitialize( myMin, myMax );

    for( Tr2MeshVector::iterator meshIt = m_meshes.begin(); meshIt != m_meshes.end(); ++meshIt )
    {
        Tr2Mesh* mesh = *meshIt;

        Vector3 meshMin, meshMax;

        if( !mesh->GetBoundingBox( meshMin, meshMax ) )
        {
            return false;
        }

        BoundingBoxUpdate( myMin, myMax, meshMin, meshMax );
    }

    min = myMin;
    max = myMax;

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
	if( mesh->IsHidden() )
	{
		return;
	}

	if( mesh->HasPendingLowLevelShaderBind() )
	{
		mesh->ExecutePendingLowLevelShaderBind();
	}

	Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );

	if( !areas )
	{
		return;
	}

	TriGeometryRes* geomRes = mesh->GetGeometryResource();
	int meshIx = mesh->GetMeshIndex();

	for( Tr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		ITr2ShaderMaterial* shader = area->GetMaterialInterface();

		if( area->IsHidden() )
		{
			continue;
		}
		if( !shader )
		{
			continue;
		}
		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetTransform( pm );
			batch->SetShaderMaterial( shader );
			batch->SetPerObjectData( data );
			batch->SetGeometryResource( geomRes );
			batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount(), area->GetReversed() );

			batches->Commit( batch );
		}
	}
}

void Tr2Model::AddMesh( Tr2Mesh* mesh )
{
	m_meshes.Insert( -1, mesh->GetRawRoot() );
}

Be::Result<std::string> Tr2Model::GetBoundingBoxInLocalSpace( std::pair<Vector3, Vector3>& result )
{
	bool bbFound = false;
	Vector3 min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vector3 max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	for( unsigned int i = 0, count = GetNumOfMeshes(); i < count; ++i )
	{
		Vector3 min2;
		Vector3 max2;
		bool found = GetMesh(i)->GetBoundingBox( min2, max2 );
		bbFound |= found;

		if( found )
		{
			BoundingBoxUpdate( min, max, min2, max2 );
		}
	}

	if( !bbFound )
	{
		return Be::Result<std::string>( "No meshes, or meshes aren't loaded yet." );
	}

	result = std::make_pair( min, max );
	return Be::Result<std::string>();
}
