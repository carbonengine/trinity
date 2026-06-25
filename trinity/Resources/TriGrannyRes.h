// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriGrannyRes_H
#define TriGrannyRes_H

#include "TriGeometryRes.h"


struct CmfVertexReader
{
	cmf::VertexElement* posElem;
	cmf::VertexElement* normElem;
	cmf::VertexElement* tanElem;
	cmf::VertexElement* binormElem;
	cmf::VertexElement* pkdTanElem;
	cmf::VertexElement* pkdLegElem;

	CmfVertexReader( cmf::Span<cmf::VertexElement> decl );
};

void ReadCmfVertexAttributes( const CmfVertexReader& reader, const uint8_t* data, uint32_t vertexCount, uint32_t stride, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<Vector3>* tangents, std::vector<Vector3>* binormals );


BLUE_CLASS( Tr2GrannyIntersectionResult ) :
	public IRoot
{
public:
	Tr2GrannyIntersectionResult( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	struct Result
	{
		Result();

		Vector3 position;
		Vector3 normal;
		Vector2 uv;
		int32_t meshIndex;
		int32_t areaIndex;
		int32_t boneIndex;
		bool hasPosition;
		bool hasNormal;
		bool hasUv;
		bool hasBoneIndex;
	};

	Result m_result;
};

TYPEDEF_BLUECLASS( Tr2GrannyIntersectionResult );

BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( TriGrannyRes );

// TriGrannyRes is used to load Granny files 'raw' - i.e. they are loaded without creating
// any D3D resources. This is used, for example, when final geometry is constructed from
// blendshapes.
BLUE_CLASS( TriGrannyRes ) :
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();

	TriGrannyRes( IRoot* lockobj = NULL );
	~TriGrannyRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	bool Load( const std::string& path );

#if WITH_GRANNY
	granny_file* GetGrannyFile() const
	{
		return m_grannyFile;
	}
	granny_skeleton* GetGrannySkeleton( int skeletonIx ) const;

	const granny_mesh* GetGrannyMesh( int meshIx ) const;

	// access the main vertices
	granny_data_type_definition* GetGrannyVertexType( int meshIx ) const;
#endif

	int GetVertexSize( int meshIx ) const;

#if WITH_GRANNY
	int GetVertexComponentOffset( int meshIx, const char* componentName ) const;
#endif

	// Bake by mapping every morphtarget name to a mesh
	typedef std::map<std::string, float> NameToWeightMap;
	bool BakeBlendshape( unsigned int meshIx, const NameToWeightMap& nameToWeight, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize );

	// Bake by providing a vector of weights that exactly matches the layout of morph targets
	bool BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize );

	int GetModelCount();
	std::string GetModelName( unsigned int ix );

#if WITH_GRANNY
	granny_file_info* ValidateFileInfo();
#endif
	int GetMeshCount();
	Be::Result<std::string> GetMeshAreaCount( unsigned int meshIx, int& count );
	Be::Result<std::string> GetMeshName( unsigned int meshIx, std::string& name );
	Be::Result<std::string> GetMeshMorphCount( unsigned int meshIx, int& count );
	Be::Result<std::string> GetMeshMorphName( unsigned int meshIx, unsigned int morphIx, std::string& name );
	Be::Result<std::string> GetAllMeshMorphNamesNoDigits( unsigned int meshIx, std::vector<std::string>& names );
	Be::Result<std::string> GetMeshBoneBindings( unsigned int meshIx, std::vector<std::string>& boneBindings );
	Be::Result<std::string> GetMeshVertexElements( unsigned int meshIx, std::vector<std::pair<uint32_t, uint32_t>>& vertexElements );
	Be::Result<std::string> GetMeshTriangleCount( unsigned int meshIx, uint32_t& count );

	int GetAnimationCount();
	std::string GetAnimationName( int ix );

	float GetAnimationDuration( int ix );

#if WITH_GRANNY
	int GetVectorTrackCount( int groupIdx );
	std::string GetVectorTrackName( int groupIdx, int ix );
	std::string GetTrackGroupName( int groupIdx );
	int GetTrackGroupCount();
	int GetEventTrackCount( int groupIdx );
	std::string GetEventTrackName( int groupIdx, int ix );
#endif

	Tr2GrannyIntersectionResultPtr RayIntersection( const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex );

	Be::Result<std::string> CreateGeometryRes( TriGeometryRes * *result );
	Be::Result<std::string> BakeBlendshapeFromScript( unsigned int meshIx, const std::vector<float>& weights, TriGeometryRes* geom );

	bool IsUsingCMF() const;
	const cmf::Data* GetCMFData() const;
	const void* GetCMFViewData( const cmf::BufferView& view );
	Tr2CmfContents& GetCMFContents();

protected:
	// Gets the file info if available, reporting errors if not.
	// checks animation index, returns null if out of bounds
#if WITH_GRANNY
	granny_file_info* ValidateAnimationIx( int ix );
#endif

	Be::Result<std::string> ValidateMeshIndex( unsigned int meshIx );

	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in TriAsyncLoadedResource.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

	bool BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights, Tr2SuballocatedBuffer::Allocation& pVertexData, Tr2RenderContextAL& renderContext, unsigned int vertexDataSize, const NameToWeightMap* nameToWeight, bool deltaOnly );

protected:
	size_t m_memoryUsage;
#if WITH_GRANNY
	size_t m_dataSize;
	void* m_data;
	granny_file* m_grannyFile;
	granny_memory_arena* m_grannyArena;
#endif
	Tr2CmfContents m_cmfContents;
	bool m_useCMF;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( TriGrannyRes );

#if WITH_GRANNY
Tr2GrannyIntersectionResultPtr GrannyRayIntersectionWrap( uintptr_t fileinfo, const Vector3& pos, const Vector3& dir, int32_t meshIndex, int32_t areaIndex );
#endif

#endif // TriGrannyRes_H
