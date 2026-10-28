// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2ProceduralGeometryProvider.h"
#include "../Tr2VoxelData.h"

BLUE_CLASS_IMPL( Tr2VoxelGeometry )
class Tr2VoxelGeometry : public Tr2ProceduralGeometryProvider, public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	enum class Method : uint32_t
	{
		Simple,
		SurfaceNets,
	};

	void Create( Be::Optional<Tr2VoxelData*> voxelData, Be::Optional<uint32_t> occupancyChannelIndex, Be::Optional<float> cellSize );

	bool Initialize() override;

	IRootPtr GetVoxelData() const;
	void SetVoxelData( Tr2VoxelData* voxelData );

	uint32_t GetOccupancyChannelIndex() const;
	void SetOccupancyChannelIndex( uint32_t channelIndex );

	float GetCellSize() const;
	void SetCellSize( float cellSize );

	uint32_t GetSubdivision() const;
	void SetSubdivision( uint32_t subdivision );

	uint32_t GetRelaxIterations() const;
	void SetRelaxIterations( uint32_t relaxIterations );

	uint32_t GetMethod() const;
	void SetMethod( Method method );

	BlueStdResult TestBuildFile( const char* resPath );

protected:
	std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const override;

private:
	void DataModified( Tr2VoxelData::ModifiedChannels modifiedChannels );

	Tr2VoxelDataPtr m_voxelData;
	uint32_t m_occupancyChannelIndex = 0;
	float m_cellSize = 1.f;
	uint32_t m_subdivision = 1;
	uint32_t m_relaxIterations = 0;
	Method m_method = Method::Simple;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2VoxelGeometry );
