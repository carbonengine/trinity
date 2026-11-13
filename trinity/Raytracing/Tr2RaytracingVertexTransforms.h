// Copyright © 2026 CCP ehf.

#pragma once

#include "Shader/Tr2Effect.h"

class Tr2RaytracingMesh;
class Tr2RaytracingMeshArea;
struct TriGeometryResLodData;

// One job of the ray tracing vertex transform shaders: the vertices of one mesh LOD, transformed by one shader.
// Mirrors TransformJob in shaders/include/Carbon/SpaceScene/Raytracing/TransformVertices.fxh, field for field.
// Offsets, strides and sizes are in bytes unless stated otherwise.
struct Tr2TransformVerticesJob
{
	// Marks an offset as unused (TRANSFORM_VERTICES_INVALID_OFFSET).
	static constexpr uint32_t INVALID_OFFSET = 0xffffffff;
	// Number of threads in a thread group (TRANSFORM_VERTICES_GROUP_SIZE).
	static constexpr uint32_t GROUP_SIZE = 64;

	uint32_t vertexCount;
	uint32_t vertexStride;
	// Index of the input vertex buffer in the buffer heap (DX12 only).
	uint32_t inVBIndex;
	// Index of the first output vertex in the output buffer, in vertices. Each output vertex is 3 floats.
	uint32_t outVBVertexOffset;

	// POSITION0 (FLOAT32_3). Includes the offset of the vertex allocation.
	uint32_t positionOffset;
	// NORMAL0 and its Tr2VertexDefinition::DataType, or INVALID_OFFSET.
	uint32_t normalOffset;
	uint32_t normalType;
	// TEXCOORD0 and its Tr2VertexDefinition::DataType, or INVALID_OFFSET.
	uint32_t texCoord0Offset;
	uint32_t texCoord0Type;

	// BLENDINDICES0, or INVALID_OFFSET if the vertices aren't skinned.
	uint32_t boneOffset;
	// BLENDWEIGHTS0, or INVALID_OFFSET for one bone per vertex.
	uint32_t boneWeightsOffset;
	// Index of the mesh's first bone in BoneTransforms.
	uint32_t transformOffset;

	// The mesh's entries in MorphTargetAnimations.
	uint32_t morphAnimationDataOffset;
	uint32_t morphAnimationDataCount;
	// Position of the first vertex of morph target 0, in the input vertex buffer.
	uint32_t morphTargetPositionOffset;
	uint32_t morphTargetStride;
	// Size of one morph target (vertexCount * morphTargetStride).
	uint32_t morphTargetSize;
	// Position of the first vertex in BakedMorphTargetBuffer, or INVALID_OFFSET.
	uint32_t bakedMorphTargetPositionOffset;

	// Index of the job's first thread group in TransformJobGroups, relative to the frame's first thread group.
	uint32_t firstGroup;
	uint32_t padding;
};

// Catches a field added or removed on the C++ side only. The field order must still match TransformJob in
// TransformVertices.fxh by hand.
static_assert( sizeof( Tr2TransformVerticesJob ) == 20 * sizeof( uint32_t ), "Tr2TransformVerticesJob must be 20 uints, like TransformJob in TransformVertices.fxh" );

// One entry per thread group of the ray tracing vertex transform dispatches: the index of the group's job, relative to
// the frame's first job. A struct rather than a plain uint32_t so that Tr2RingBuffer::GetInstance gives it its own buffer.
struct Tr2TransformVerticesJobGroup
{
	uint32_t job;
};

// The shader reads the table as StructuredBuffer<uint> (TransformJobGroups in TransformVertices.fxh).
static_assert( sizeof( Tr2TransformVerticesJobGroup ) == sizeof( uint32_t ), "Tr2TransformVerticesJobGroup must be one uint, like TransformJobGroups in TransformVertices.fxh" );

// A mesh area in the ray tracing scene, see Tr2RaytracingVertexTransforms::Update.
struct Tr2RaytracingVertexTransformInput
{
	Tr2RaytracingMesh* mesh;
	Tr2RaytracingMeshArea* area;
	const Tr2Material* material;
	uint32_t bakedMorphOffset;
};

// Writes the vertex positions that the BLASes of skinned, morphed and displaced mesh areas are built from.
//
// An area's vertices are written by the TransformVertices technique of its material, if it has one (see
// TransformVertices.fxh), or otherwise, if the area is skinned or morphed, by SkinVertices.fx. Other areas use the
// LOD's shared static BLAS.
//
// An area's vertices are written again when its mesh is dirty (bones, morph targets or LOD changed), when its BLAS is
// outdated (e.g. the owner called MarkBlasOutdated() after changing a displacement parameter), when its shader changes,
// or, with a TransformVertices technique, every frame while the area's update flag is set
// (Tr2RaytracingMeshArea::SetUpdateCustomRtDisplacement). Areas that need the same mesh transformed by the same shader
// share one job, so all skinned areas of a mesh share the SkinVertices.fx output.
//
// The jobs are batched into as few dispatches as possible: jobs whose shader and material data are the same share a
// dispatch.
class Tr2RaytracingVertexTransforms
{
public:
	Tr2RaytracingVertexTransforms();

	// Queues and dispatches the transforms that the areas need this frame, and gives each of those areas the range of
	// the output that its BLAS is built from (Tr2RaytracingMeshArea::SetTransformedVertices). Clears the dirty flag of
	// the meshes.
	void Update( const std::vector<Tr2RaytracingVertexTransformInput>& inputs, Tr2RenderContext& renderContext );

	void ReleaseResources();

private:
	// Byte offsets of vertex elements within a vertex, Tr2TransformVerticesJob::INVALID_OFFSET if missing.
	struct VtxOffsets
	{
		uint32_t positionOffset = 0;
		uint32_t normalOffset = Tr2TransformVerticesJob::INVALID_OFFSET;
		uint32_t normalType = 0;
		uint32_t texCoord0Offset = Tr2TransformVerticesJob::INVALID_OFFSET;
		uint32_t texCoord0Type = 0;
		uint32_t boneOffset = Tr2TransformVerticesJob::INVALID_OFFSET;
		uint32_t boneWeightsOffset = Tr2TransformVerticesJob::INVALID_OFFSET;
	};

	// Defined in the .cpp file.
	struct TransformShader;
	struct TransformRequest;
	struct TransformDispatch;
	struct TransformBatch;

	// The shader that writes the area's vertices, or no shader for a static area.
	TransformShader ResolveShader( const Tr2RaytracingVertexTransformInput& input, const TriGeometryResLodData& lod ) const;
	// Updates each area's shader, and returns the areas whose vertices are written this frame.
	std::vector<TransformRequest> CollectRequests( const std::vector<Tr2RaytracingVertexTransformInput>& inputs );
	// Creates the jobs and the dispatches, and gives each requesting area its output range. Sorts the requests; the
	// dispatches point into them.
	TransformBatch PlanBatch( std::vector<TransformRequest>& requests );
	// The job for the current vertices of a mesh LOD.
	Tr2TransformVerticesJob CreateJob( const Tr2RaytracingMesh& mesh, const TriGeometryResLodData& lod, uint32_t bakedMorphOffset, uint32_t outVBVertexOffset, uint32_t firstGroup );
	void Dispatch( const TransformBatch& batch, uint32_t jobBase, uint32_t groupBase, Tr2RenderContext& renderContext );

	VtxOffsets FindOffsets( unsigned declHandle );

	const BlueSharedString m_customTransformTechniqueName = BlueSharedString( "TransformVertices" );

	// SkinVertices.fx: the shader for skinned and morphed areas whose material has no transform technique.
	Tr2EffectPtr m_fallbackEffect;
	// The per-dispatch constants (cbuffer TransformVerticesDispatch).
	Tr2ConstantBufferAL m_dispatchConstants;
	// The output of all jobs of the frame, 3 floats per vertex. Repacked every frame.
	Tr2BufferAL m_outputVertices;
	std::unordered_map<unsigned, VtxOffsets> m_offsets;
};
