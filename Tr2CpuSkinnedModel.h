#ifndef TR2CPUSKINNEDMODEL_H
#define TR2CPUSKINNEDMODEL_H

#include "Tr2SkinnedModel.h"

BLUE_DECLARE( Tr2DynamicMesh );

// ------------------------------------------------------------------------------------------------------
BLUE_DECLARE( Tr2CpuSkinnedModel );
class Tr2CpuSkinnedModel:
	public Tr2SkinnedModel
{
public:
	EXPOSE_TO_BLUE();

	Tr2CpuSkinnedModel( IRoot* lockobj = NULL );
	~Tr2CpuSkinnedModel();

	using Tr2SkinnedModel::Lock;
	using Tr2SkinnedModel::Unlock;

	//////////////////////////////////////////////////////////////////////////
	// Tr2SkinnedModel overrides
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Matrix& m, const Tr2PerObjectData* data );

	// update dynamic buffers / animate
	void deform( const float* deformMatrices, unsigned int numOfMatrices );

private:
	// helper for get batches
	void GetBatchesForAreaDynamic( Tr2MeshAreaVector* areas, Tr2DynamicMesh* mesh, ITriRenderBatchAccumulator* batches, const Matrix* pm, const Tr2PerObjectData* data );
};
TYPEDEF_BLUECLASS( Tr2CpuSkinnedModel );

#endif