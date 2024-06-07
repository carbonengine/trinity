#pragma once

#include "include/ITr2GpuBuffer.h"
#include "Shader/Tr2Effect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE( Tr2RaytracingGeometry );

struct TriGeometryResMeshData;
class Tr2RtMesh;
class Tr2RtMeshArea;

class Tr2RaytracingPipelineStateManager
{
public:
	Tr2RaytracingPipelineStateManager();

	bool AddLibrary( std::wstring& rayGenName, std::wstring& missName, Tr2Material* material, const BlueSharedString& techniqueName );
	void AddLibrary( std::wstring& rayGenName, std::wstring& missName, const Tr2EffectLibrary& library );
	std::wstring AddHitGroup( const Tr2EffectLibrary& library );
	Tr2RtPipelineStateAL GetPipelineState( Tr2RenderContext& renderContext );

private:
	std::wstring GetUniqueName();

	Tr2RtPipelineStateDescriptionAL m_pipelineDesc;
	Tr2RtPipelineStateAL m_pipelineState;
	std::unordered_map<uint32_t, std::wstring> m_hitGroups;
	std::unordered_map<uint32_t, std::pair<std::wstring, std::wstring>> m_libraries;
	uint32_t m_nextName;
	bool m_isDirty;
};


BLUE_CLASS( Tr2RuntimeGpuBuffer ) : public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	Tr2BufferAL* GetGpuBuffer( unsigned ) override
	{
		return &m_buffer;
	}
	void SetGpuBuffer( const Tr2BufferAL& buffer )
	{
		m_buffer = buffer;
	}

	Tr2BufferAL m_buffer;
};
TYPEDEF_BLUECLASS( Tr2RuntimeGpuBuffer );


class Tr2RaytracingMesh
{
public:
	Tr2RaytracingMesh();

	void UpdateRtMesh( TriGeometryRes* geometry, uint32_t meshIndex, float screenSize );
	bool SetBoneTransforms( size_t count, const granny_matrix_3x4* transforms, uint32_t offset );

	bool IsGood() const;
	bool GetAndResetDirtyFlag();

	TriGeometryResMeshData* GetMeshData() const;
	uint32_t GetTransformOffset() const;
	const Tr2BufferAL& GetSkinnedVertexBuffer( Tr2RenderContext& renderContext );
	const Tr2BufferAL& GetVertexBuffer() const;
	const Tr2BufferAL& GetIndexBuffer() const;

	void SetSkinnedVertices( const Tr2BufferAL& buffer, uint32_t offset );
	uint32_t GetSkinnedVertexOffset() const;

private:
	const Tr2BufferAL* m_skinnedVertices;
	TriGeometryResPtr m_geometry;
	uint32_t m_meshIndex;
	std::vector<float> m_transforms;
	uint32_t m_boneOffset;
	uint32_t m_skinnedVertexOffset;
	bool m_isDirty;
	float m_screenSize;
	int m_lodIndex;

	friend class Tr2RaytracingGeometry;
};

class Tr2RaytracingMeshArea
{
public:
	Tr2RaytracingMeshArea( uint32_t index );
	const Tr2RtBottomLevelAccelerationStructureAL& BuildBlas( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext );
	const Tr2ConstantBufferAL& GetGeometryConstants( Tr2RaytracingMesh& mesh, Tr2RenderContext& renderContext ) const;
	uint32_t GetAreaIndex(){ return m_areaIndex; }
	void MarkBlasOutdated() { m_blasOutdated = true; }

private:
	uint32_t m_areaIndex;

	Tr2RtBottomLevelAccelerationStructureAL m_blas;
	bool m_blasOutdated;
};

BLUE_CLASS( Tr2RaytracingGeometry ) : public ITr2GpuBuffer
{
public:
	EXPOSE_TO_BLUE();

	Tr2RaytracingGeometry();

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;
	void BeginSceneUpdate();
	void EndSceneUpdate( Tr2RenderContext& renderContext );
	void AddGeometry( Tr2RaytracingMesh& mesh, Tr2RaytracingMeshArea& area, Tr2Material* material, const Tr2ConstantBufferAL* perObjectData, const Matrix& worldTransform );
	bool HasGeometry() const;

	void ReleaseResources( TriStorage s );
    
    Tr2RtTopLevelAccelerationStructureAL GetTLAS() const;

	Tr2RaytracingPipelineStateManager* m_pipelineManager;
	Tr2RtShaderTableDescriptionAL m_shaderTableDesc;

	const BlueSharedString m_rtShadowTechniqueName = BlueSharedString( "RtShadow" );

private:
	struct VtxOffsets
	{
		uint32_t positionOffset;
		uint32_t boneOffset;
	};

	struct GeometryData
	{
		Tr2RaytracingMesh* mesh;
		Tr2RaytracingMeshArea* area;
		const Tr2Material* material;
		const Tr2ConstantBufferAL* perObjectData;
		Matrix worldTransform;
		uint32_t materialIndex;
		bool isTransparent;
	};
	
	const BlueSharedString m_inVertexBufferTechniqueName = BlueSharedString( "InVB" );
	const BlueSharedString m_outVertexBufferTechniqueName = BlueSharedString( "OutVB" );
	static const uint32_t INVALID_MATERIAL = 0xffffffff;

	void PrepareShaderTableDescription( Tr2RenderContext& renderContext );
	void TransformMeshes( Tr2RenderContext& renderContext );
	void BuildAccelerationStructures(Tr2RenderContext& renderContext);

	VtxOffsets FindOffsets( unsigned declHandle );
	
	std::vector<GeometryData> m_geometryData;
	Tr2RtTopLevelAccelerationStructureAL m_tlas;

	Tr2EffectPtr m_skinVerticesEffect;
	Tr2ConstantBufferAL m_skinVerticesData;
	std::unordered_map<unsigned, VtxOffsets> m_offsets;

	Tr2BufferAL m_skinnedVertices;
};

TYPEDEF_BLUECLASS( Tr2RaytracingGeometry );
