#pragma once
#ifndef Tr2SkinnedModelBuilder_h
#define Tr2SkinnedModelBuilder_h

#include "ITr2Renderable.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2SkinnedModel );
BLUE_DECLARE( Tr2CpuSkinnedModel );
BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2DynamicMesh );
BLUE_DECLARE( TriFloatArrayParameter );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );
BLUE_DECLARE( Tr2SkinnedModelBuilderSource );
BLUE_DECLARE_VECTOR( Tr2SkinnedModelBuilderSource );
BLUE_DECLARE( Tr2SkinnedModelBuilderBlend );
BLUE_DECLARE_VECTOR( Tr2SkinnedModelBuilderBlend );

struct Tr2SkinnedModelBuilder_OutputData;

/////////////////////////////////////////////////////////////////////////////////////
// mesh assembler
/////////////////////////////////////////////////////////////////////////////////////
BLUE_DECLARE( Tr2SkinnedModelBuilder );
class Tr2SkinnedModelBuilder :
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

    Tr2SkinnedModelBuilder( IRoot* lockobj = NULL );
	~Tr2SkinnedModelBuilder();

	bool PrepareForBuild();
	bool Build();

	// query results
	Tr2SkinnedModel* GetSkinnedModel() const;	

protected:

	void ClearTargetData();

	void CollectBonesFromMesh( unsigned int ix );
	bool AddMeshToGrannyFile( unsigned int meshIx );
	void CopyAreas( Tr2MeshAreaVector* src, Tr2MeshAreaVector* dst, unsigned sourceIndex, unsigned batchType, Tr2MeshAreaPtr override0 = NULL, Tr2MeshAreaPtr override1 = NULL, bool enableMask = false );
	void AppendToEffectArrayParam( TriFloatArrayParameter* arrayParameter, const char* paramName, const ITr2ShaderMaterial *srcEffect, const Tr2MeshAreaPtr area, bool enableMask );
	void CollectAllBones();
	void ReIndexBoneMap();
	void CollectIndicesPerArea( const granny_mesh* mesh, const Tr2MeshAreaVector* srcAreas, unsigned int ixOffset, std::vector<int>& indices );
	void CollectIndicesPerMesh( const granny_mesh* mesh, unsigned int ixOffset, std::vector<int>& indices );
	void CollectMaterialsPerArea( const granny_mesh* mesh, const Tr2MeshAreaVector* srcAreas, int materialOffset, unsigned int* material, int areaIndex );
	void ReMapUVs( uint8_t* uv, granny_member_type type, const Vector2& scale, const Vector2& offset, size_t stride, unsigned count );
	void Weld( const granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count );
	void AddMaterialIndex();
	void FinalizeSingleAreas();
	void FinalizeGrannyFile();

	// created in PrepareBuild to make sure that the actual Build will see only effects
	// whose EffectRes is guaranteed IsPrepared()
	Tr2EffectPtr m_effect;
	bool CreateEffect( Tr2Effect ** effect, bool allowPrepare );

	int GetBoneIndexFromMap( const char* boneName ) const;

	// threaded assembly
	static void StaticBuildAsync( void* context );
	void BuildAsync( const BlueScriptCallback& doneCallback );
	static void StaticNotifyBuildDone( void* context );

	std::vector<std::string> m_extraArrayOf;

private:
	bool PrepareForBuildMesh( const Tr2SkinnedModelBuilderSource* source, Tr2MeshPtr mesh, const std::string& grannyPath );

	struct sSourceData
	{
		Tr2MeshPtr mesh;
		TriGrannyResPtr grnRes;
		Tr2MeshAreaPtr overrideMaterial0MeshArea;
		Tr2MeshAreaPtr overrideMaterial1MeshArea;
		Vector2 upperLeftTexCoord;
		Vector2 lowerRightTexCoord;
		bool enableCutMask;
		bool isError;

		// When trying to stay below the bone limit for GPU skinning, go over
		// the source data multiple times. Record here what's not yet processed,
		// under construction, or already done.
		enum State { New, InProcess, Done };
		State	m_state;
	};
	// source modules
	PTr2SkinnedModelBuilderSourceVector m_sourceMeshesInfo;
	typedef std::vector<sSourceData> SourceDataVector_t;
	SourceDataVector_t m_sourceData;

	// blendshape list
	PTr2SkinnedModelBuilderBlendVector m_blendshapeInfo;

	Tr2SkinnedModelPtr m_sourceSkinnedModel;

	// target gr2 name
	std::string m_outputName;	
	// target avatar
	Tr2SkinnedModelPtr m_targetSkinnedModel;
	// target areas

	Tr2SkinnedModelBuilder_OutputData*	m_outputData;	// output currently being built

	std::string	m_vertexSizeSource;	// which file was used to set the initial vertex size

	// cached values from the first valid vertex declaration we encounter
	int m_uvCoords0ByteOffset;
	int m_uvCoords1ByteOffset;
	granny_member_type m_uvCoords0Type;
	granny_member_type m_uvCoords1Type;
	int m_boneIndexOffset;

	// counter
	int m_areaOffset;

	// welding
	float m_weldThreshold;

	// threading
	CcpAtomic<uint32_t> m_notifyBuildDoneId;
	BlueScriptCallback m_doneCallbackToPython;
	bool m_buildSucceeded;

	// configuration options
	bool m_createGPUMesh;
	// false = old style "build a CPU optimized skin", very slow but always works
    // true  = new style "GPU skinned, but bonecount limit is your responsibility"

	bool m_enableVertexChopping;	// allow subsequent bigger vertices, throw away data
	bool m_enableVertexPadding;		// allow subsequent smaller vertices, pad with zero

	bool m_enableSubsetBuilding;	// process source data until we have too many bones, and then collapse just that subset.

	bool m_removeReversed;
	bool m_collapseToOpaque;	// if set, all areas end up being opaque, no matter their source (decal, transparent)
	bool m_collapseFromDepthNormal;		// if set, collapse the depthNormal areas, and only those.
	bool m_collapseTransparentAreas;	// if set, transparent areas are included in the build, else they're skipped

	// effect to apply to the created mesh
	std::string m_effectPath;

	void	DoPrepare();	// used when doing diskless collapse -- calls DoPrepare on the trigeometryres and unlocks self
	static void StaticDoPrepare( void* This );

	// blendshapes converted into map
	std::map<std::string, float>	blendWeights;

	// save some information so we can figure out after the collapse what went where.
	struct CollapseInfo
	{
		unsigned m_sourceIndex;
		unsigned m_batchType;
		unsigned m_areaIndex;
		unsigned m_permuteIndex;
	};
	std::vector<CollapseInfo>	m_collapseInfo;

	bool m_batchEnabled[TRIBATCHTYPE_COUNT_OF_BATCH_TYPES];

	// path adjusting callback
	BlueScriptCallback m_adjustPathMethod;

	// helper for PySet methods
	void SetAdjustPathMethod( const BlueScriptCallback& callback );

#if BLUE_WITH_PYTHON
	static PyObject * PyGetCollapsedInfo( PyObject* self, PyObject* args );
	static PyObject * PySetExtraArrayOf( PyObject* self, PyObject* args );
#endif
	void DoAdjustPathCallback( std::string & path );
};

TYPEDEF_BLUECLASS( Tr2SkinnedModelBuilder );
#endif //Tr2SkinnedModelBuilder_h
