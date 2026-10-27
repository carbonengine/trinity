// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2ProceduralGeometryProvider.h"
#include "../Tr2VoxelData.h"

BLUE_CLASS_IMPL( Tr2VoxelGeometry )
class Tr2VoxelGeometry : public Tr2ProceduralGeometryProvider
{
public:
	EXPOSE_TO_BLUE();


	IRootPtr GetVoxelData() const;
	void SetVoxelData( Tr2VoxelData* voxelData );

	float GetCellSize() const;
	void SetCellSize( float cellSize );

protected:
	std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const override;

private:
	Tr2VoxelDataPtr m_voxelData;

	float m_cellSize = 1.f;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2VoxelGeometry );
