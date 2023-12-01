#pragma once
#ifndef TriGrannyRes_H
#define TriGrannyRes_H


#if BLUE_WITH_PYTHON
class GrannyMaterialWrapper
{
public:
	GrannyMaterialWrapper(granny_material * wrapee, unsigned int meshIndex, unsigned int areaIndex, PyObject * flatStringDictionary);
	~GrannyMaterialWrapper();
	PyObject * m_dictionary;
	std::string m_name;
	unsigned int m_meshIndex;
	unsigned int m_areaIndex;
};
#endif


BLUE_CLASS( Tr2GrannyIntersectionResult ): public IRoot
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
BLUE_CLASS( TriGrannyRes ):
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

    granny_file* GetGrannyFile() const { return m_grannyFile; }
	granny_skeleton* GetGrannySkeleton( int skeletonIx ) const;

	const granny_mesh* GetGrannyMesh( int meshIx ) const;

	// access the main vertices
	granny_data_type_definition* GetGrannyVertexType( int meshIx ) const;
	int GetVertexSize( int meshIx ) const;
	int GetVertexComponentOffset( int meshIx, const char* componentName ) const;

	// Bake by mapping every morphtarget name to a mesh
	typedef std::map<std::string, float> NameToWeightMap;
	bool BakeBlendshape( unsigned int meshIx, const NameToWeightMap& nameToWeight, void* pVertexData, unsigned int vertexDataSize );

	// Bake by providing a vector of weights that exactly matches the layout of morph targets
	bool BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights  , void* pVertexData, unsigned int vertexDataSize );

	int GetModelCount();
	std::string GetModelName( unsigned int ix );

	granny_file_info* ValidateFileInfo();
	int GetMeshCount();
	Be::Result<std::string> GetMeshAreaCount( unsigned int meshIx, int& count );
	Be::Result<std::string> GetMeshName( unsigned int meshIx, std::string& name );
	Be::Result<std::string> GetMeshMorphCount( unsigned int meshIx, int& count );
	Be::Result<std::string> GetMeshMorphName( unsigned int meshIx, unsigned int morphIx, std::string& name );
	Be::Result<std::string> GetAllMeshMorphNamesNoDigits( unsigned int meshIx, std::vector<std::string>& names );

	int GetAnimationIndex( const std::string& name );
	int GetAnimationCount();
	std::string GetAnimationName( int ix );

	float GetAnimationDuration( int ix );

	int GetTransformTrackCount( int groupIdx );
	std::string GetTransformTrackName( int groupIdx, int ix );
	int GetVectorTrackCount( int groupIdx );
	std::string GetVectorTrackName( int groupIdx, int ix );
	std::string GetTrackGroupName( int groupIdx );
	int GetTrackGroupCount();
	int GetEventTrackCount( int groupIdx );
	std::string GetEventTrackName( int groupIdx, int ix );

#if BLUE_WITH_PYTHON
	PyObject * GetMaterialDictionaryForArea( int mesh, int area );
	PyObject * GetMaterialDictionaryStringsForAllAreas();
#endif

	Be::Result<std::string> CreateGeometryRes( TriGeometryRes** result );
	Be::Result<std::string> BakeBlendshapeFromScript( unsigned int meshIx, const std::vector<float>& weights, TriGeometryRes* geom );

protected:
	// Gets the file info if available, reporting errors if not.
	// checks animation index, returns null if out of bounds
	granny_file_info* ValidateAnimationIx( int ix );

	// Provide the functions that do the actual work of loading and preparing.
    // The async management itself is done in TriAsyncLoadedResource.
    virtual LoadingResult DoLoad();
    virtual bool DoPrepare();

	void CollectGrannyMaterials();

	bool BakeBlendshape( unsigned int meshIx, const std::vector<float>& weights  , void* pVertexData, unsigned int vertexDataSize, const NameToWeightMap* nameToWeight, bool deltaOnly );
protected:
	size_t m_dataSize;
	size_t m_memoryUsage;
	void* m_data;
    granny_file* m_grannyFile;
	granny_memory_arena *m_grannyArena;

#if BLUE_WITH_PYTHON
	std::unordered_map<unsigned int, GrannyMaterialWrapper*> m_meshAreaMaterials;
	PyObject * m_allMaterialStringsDictionary; // a flat list of strings, so the pipeline doesn't have to make and query a mesh.
#endif

};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( TriGrannyRes );



#endif // TriGrannyRes_H