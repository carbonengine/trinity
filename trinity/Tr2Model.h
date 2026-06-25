// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2Model_H
#define Tr2Model_H

#include "ITr2Renderable.h"

BLUE_DECLARE( Tr2Model );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;

class Tr2MeshArea;

class Tr2Model : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2Model( IRoot* lockobj = NULL );
	~Tr2Model();

	virtual void GetBatches( ITriRenderBatchAccumulator* batches,
							 TriBatchType batchType,
							 const Matrix& m,
							 const Tr2PerObjectData* data );


	virtual bool HasTransparency() const;
	virtual bool GetBoundingBox( Vector3& min, Vector3& max );

	unsigned int GetNumOfMeshes()
	{
		return (unsigned int)m_meshes.size();
	}
	Tr2MeshPtr GetMesh( unsigned int ix )
	{
		return m_meshes[ix];
	}
	const PTr2MeshVector& GetMeshes( void ) const
	{
		return m_meshes;
	}

	// Allows model to be constructed by hand from C++ code.
	void AddMesh( Tr2Mesh* mesh );

	bool IsLoading() const;

	Be::Result<std::string> GetBoundingBoxInLocalSpace( std::pair<Vector3, Vector3>& result );

protected:
	friend class Tr2SkinnedModelBuilder;

	std::string m_name;
	PTr2MeshVector m_meshes;

private:
	// This is a helper function for GetBatches to separate it from the mesh sorting required for transparent areas
	void GetBatchesFromMesh( Tr2Mesh* mesh,
							 TriBatchType batchType,
							 ITriRenderBatchAccumulator* batches,
							 Matrix* pm,
							 const Tr2PerObjectData* data );
};

TYPEDEF_BLUECLASS( Tr2Model );

#endif