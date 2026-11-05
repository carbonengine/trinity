// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2ProceduralGeometryProvider.h"
#include "../Tr2VoxelData.h"
#include "../Tr2SignedDistanceField.h"
#include "../EnumFilter.h"

enum class Tr2VoxelGeometryComponents : int32_t
{
	SmoothNormals,
	SplitNormals,
	Curvature,
	SmoothedCurvature,
	DataTags,
};


BLUE_CLASS_IMPL( Tr2VoxelGeometry )
class Tr2VoxelGeometry : public Tr2ProceduralGeometryProvider, public IInitialize, public INotify
{
public:
	EXPOSE_TO_BLUE();

	enum class Method : uint32_t
	{
		Simple,
		SurfaceNets,
		DualContouring,
	};

	BlueStdResult Create( Be::Optional<Tr2VoxelData*> voxelData, Be::Optional<int32_t> shapeChannelIndex, Be::Optional<int32_t> occupancyChannelIndex );

	bool Initialize() override;
	bool OnModified( Be::Var* value ) override;

	IRootPtr GetVoxelData() const;
	void SetVoxelData( Tr2VoxelData* voxelData );

	int32_t GetShapeChannelIndex() const;
	void SetShapeChannelIndex( int32_t channelIndex );

	float GetShapeChannelScale() const;
	void SetShapeChannelScale( float scale );

	float GetShapeChannelOffset() const;
	void SetShapeChannelOffset( float offset );

	int32_t GetOccupancyChannelIndex() const;
	void SetOccupancyChannelIndex( int32_t channelIndex );

	int32_t GetDataTagChannelIndex() const;
	void SetDataTagChannelIndex( int32_t channelIndex );

	uint32_t GetSubdivision() const;
	void SetSubdivision( uint32_t subdivision );

	uint32_t GetRelaxIterations() const;
	void SetRelaxIterations( uint32_t relaxIterations );

	uint32_t GetMethod() const;
	void SetMethod( Method method );

	uint32_t GetCutSeed() const;
	void SetCutSeed( uint32_t cutSeed );

	float GetFacetSpacing() const;
	void SetFacetSpacing( float facetSpacing );

	float GetFacetDepth() const;
	void SetFacetDepth( float facetDepth );

	float GetFacetMaxCut() const;
	void SetFacetMaxCut( float facetMaxCut );

	float GetCleaveDepth() const;
	void SetCleaveDepth( float cleaveDepth );

	void LockBuild();
	void UnlockBuild();

	BlueStdResult TestBuildFile( const char* resPath );

protected:
	std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const override;

private:
	void DataModified( Tr2VoxelData::ModifiedChannels modifiedChannels );
	void TryRebuild();

	Tr2VoxelDataPtr m_voxelData;
	int32_t m_shapeChannelIndex = 0;
	Tr2SignedDistanceField::LinearTransform m_shapeTransform;
	int32_t m_occupancyChannelIndex = -1;
	int32_t m_dataTagChannelIndex = -1;
	uint32_t m_subdivision = 1;
	uint32_t m_relaxIterations = 0;
	Method m_method = Method::Simple;
	uint32_t m_cutSeed = 0;
	float m_facetSpacing = 3.f;
	float m_facetDepth = 0.f;
	float m_facetMaxCut = 0.3f;
	float m_cleaveDepth = 0.f;
	EnumFilter<Tr2VoxelGeometryComponents> m_components;
	int32_t m_buildLock = 0;
	bool m_rebuildPending = false;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2VoxelGeometry );
