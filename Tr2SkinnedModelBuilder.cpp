#include "StdAfx.h"
#include "Tr2SkinnedModelBuilder.h"
#include "Tr2SkinnedModelBuilderSource.h"
#include "Tr2SkinnedModelBuilderBlend.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/TriGeometryRes.h"
#include "EffectParameter/TriVector4Parameter.h"
#include "EffectParameter/Tr2Vector4Parameter.h"
#include "EffectParameter/TriFloatParameter.h"
#include "EffectParameter/Tr2FloatParameter.h"
#include "EffectParameter/TriFloatArrayParameter.h"
#include "EffectParameter/TriTexture2DParameter.h"
#include "Tr2SkinnedModel.h"
#include "Tr2PerObjectData.h"
#include "Tr2Effect.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2Mesh.h"

BLUE_DECLARE( BlueCallbackMan );
IBlueCallbackManPtr s_wodBackgroundWelder;

// this is the maximum allowed number of materials on a mesh. Carefull! If you
// change this, be sure to also change it in the shaders! See MULTIMATERIAL_MAX.
#define TR2SKINNEDMODELBUILDER_MAX_MATERIAL 32

// Description:
// Structure that saves all the intermediate data set up and processed by Build().
// By storing it in a separate struct, we can easily have a whole bunch of them at the same time,
// so we can invoke Build multiple times on the same source data (ie one PrepareForBuild call + N Build calls).
// This way we can collapse down to multiple GPU skinned meshes, each staying below the bone limit.
//
// (Note, most of the processing code is still in Tr2SkinnedModelBuilder itself, a case of data envy, but that's the setup for now.)
struct Tr2SkinnedModelBuilder_OutputData
{
	Tr2SkinnedModelBuilder*		m_owner;

	// target mesh
	Tr2MeshPtr m_targetMesh;

	std::vector<int> m_targetIndices[TRIBATCHTYPE_COUNT_OF_BATCH_TYPES];
	std::vector<int> m_targetIndicesAll;
	
	unsigned int m_targetMaterial[TRIBATCHTYPE_COUNT_OF_BATCH_TYPES];
	
	std::vector<uint8_t> m_targetVertices;

	
	int m_vertexSize;
	int m_numOfBones;
	int m_numOfIndices;
	int m_numOfVertices;

	granny_vertex_data			m_grannyVertexData;
	granny_tri_topology			m_grannyTopology;
	granny_mesh					m_finalGrannyMesh;
	granny_model				m_finalGrannyModel;
	granny_file_info			m_grannyFileInfo;
	granny_tri_material_group	m_grannySingleGroup;

	granny_mesh*				m_meshes[1];
	granny_tri_topology*		m_topologies[1];
	granny_vertex_data*			m_vertexDatas[1];
	granny_skeleton*			m_skeletons[1];
	granny_model*				m_models[1];
	granny_model_mesh_binding	m_meshBinding;

	granny_art_tool_info		m_artToolInfo;

	// skeleton info of target
	std::map<std::string, int>	m_boneMapping;

	// material info of target
	std::vector<granny_tri_material_group> m_grannyGroups;

	Tr2SkinnedModelBuilder_OutputData( Tr2SkinnedModelBuilder *owner )
	: m_owner( owner )
	, m_vertexSize( 0 )
	, m_numOfBones( 0 )
	, m_numOfIndices( 0 )
	, m_numOfVertices( 0 )
	{
		for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
		{
			m_targetMaterial[i] = 0;
		}

		memset( &m_grannyFileInfo, 0, sizeof( granny_file_info ) );
		memset( &m_finalGrannyMesh, 0, sizeof( granny_mesh ) );
		memset( &m_finalGrannyModel, 0, sizeof( granny_model ) );
		memset( &m_grannyVertexData, 0, sizeof( granny_vertex_data ) );
		memset( &m_grannyTopology, 0, sizeof( granny_tri_topology ) );
		memset( &m_grannySingleGroup, 0, sizeof( granny_tri_material_group ) );
		m_vertexSize = 0;

		m_meshes[0] = nullptr;
		m_topologies[0] = nullptr;
		m_vertexDatas[0] = nullptr;
		m_skeletons[0] = nullptr;
		m_models[0] = nullptr;

		//CCP_LOG( "Output struct ctor, 0x%x", this );
	}

	~Tr2SkinnedModelBuilder_OutputData()
	{
		CCP_FREE( m_finalGrannyMesh.MaterialBindings );
		CCP_FREE( m_finalGrannyMesh.BoneBindings );

		//CCP_LOG( "Output struct dtor, 0x%x", this );
	}

	void Finalize();

	void DoPrepare();
	static void StaticDoPrepare( void* This );

private:
	Tr2SkinnedModelBuilder_OutputData( const Tr2SkinnedModelBuilder_OutputData & );
	Tr2SkinnedModelBuilder_OutputData& operator= ( const Tr2SkinnedModelBuilder_OutputData & );
};

void Tr2SkinnedModelBuilder_OutputData::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_targetMesh->GetGeometryResource()->DoPrepareFromMemory();
	// If that worked, IsGood() and all that has been set

	m_targetMesh->RebuildCachedData( m_targetMesh->GetGeometryResource() );
	m_owner->Unlock();

	delete this;
}

void Tr2SkinnedModelBuilder_OutputData::StaticDoPrepare( void* This )
{
	static_cast<Tr2SkinnedModelBuilder_OutputData*>(This)->DoPrepare();
}


void Tr2SkinnedModelBuilder_OutputData::Finalize()
{
	m_finalGrannyMesh.PrimaryTopology = &m_grannyTopology;
	m_finalGrannyMesh.PrimaryVertexData = &m_grannyVertexData;

	m_meshes[0]		= &m_finalGrannyMesh;
	m_topologies[0]	= m_finalGrannyMesh.PrimaryTopology;
	m_vertexDatas[0] = m_finalGrannyMesh.PrimaryVertexData;
	m_skeletons[0]   = m_finalGrannyModel.Skeleton;
	m_models[0]      = &m_finalGrannyModel;

	m_meshBinding.Mesh = &m_finalGrannyMesh;

	GrannyMakeIdentity( &m_finalGrannyModel.InitialPlacement );
	m_finalGrannyModel.MeshBindingCount = 1;
	m_finalGrannyModel.MeshBindings = &m_meshBinding;

	granny_art_tool_info &artToolInfo = m_artToolInfo;
	memset( &artToolInfo, 0, sizeof( artToolInfo ) );
	artToolInfo.FromArtToolName = "Tr2SkinnedModelBuilder";
	artToolInfo.Origin[0] = 0.0f;
	artToolInfo.Origin[1] = 0.0f;
	artToolInfo.Origin[2] = 0.0f;
	artToolInfo.RightVector[0] = 1.0f;
	artToolInfo.RightVector[1] = 0.0f;
	artToolInfo.RightVector[2] = 0.0f;
	artToolInfo.UpVector[0] = 0.0f;
	artToolInfo.UpVector[1] = 1.0f;
	artToolInfo.UpVector[2] = 0.0f;
	artToolInfo.BackVector[0] = 0.0f;
	artToolInfo.BackVector[1] = 0.0f;
	artToolInfo.BackVector[2] = 1.0f;
	artToolInfo.UnitsPerMeter = 1.0f;

	m_grannyFileInfo.ArtToolInfo = &artToolInfo;

	m_grannyFileInfo.ModelCount = 1;
	m_grannyFileInfo.Models = m_models;
	m_grannyFileInfo.MeshCount = 1;
	m_grannyFileInfo.Meshes = m_meshes;
	m_grannyFileInfo.VertexDataCount = 1;
	m_grannyFileInfo.VertexDatas = m_vertexDatas;
	m_grannyFileInfo.TriTopologyCount = 1;
	m_grannyFileInfo.TriTopologies = m_topologies;
	m_grannyFileInfo.SkeletonCount = m_finalGrannyModel.Skeleton ? 1 : 0;
	m_grannyFileInfo.Skeletons = m_skeletons;

	// build a new bone list for this mesh from the bone map
	m_finalGrannyMesh.BoneBindingCount = (granny_int32)m_boneMapping.size();
	m_finalGrannyMesh.BoneBindings = (granny_bone_binding*)CCP_MALLOC("myMesh.BoneBindings", sizeof( granny_bone_binding ) * m_boneMapping.size() );
	int bindingCntr = 0;
	for( std::map<std::string, int>::const_iterator it = m_boneMapping.begin(); it != m_boneMapping.end(); ++it )
	{
		granny_bone_binding* boneBinding = &m_finalGrannyMesh.BoneBindings[bindingCntr];
		memset( boneBinding, 0, sizeof( granny_bone_binding ) );
		// insert name
		boneBinding->BoneName = it->first.c_str();

		// next in list
		++bindingCntr;
	}
}

// ------------------------------------------------------------------------------------------------------
Tr2SkinnedModelBuilder::Tr2SkinnedModelBuilder( IRoot* lockobj ) :
	PARENTLOCK( m_sourceMeshesInfo ),
	PARENTLOCK( m_blendshapeInfo ),
	m_weldThreshold( 0.0 ),
	m_buildSucceeded( false ),
	m_createGPUMesh( false ),
	m_enableVertexChopping( false ),
	m_enableVertexPadding( false ),
	m_enableSubsetBuilding( false ),
	m_removeReversed( false ),
	m_collapseToOpaque( false ),
	m_collapseFromDepthNormal( false ),
	m_collapseTransparentAreas( false ),
	m_notifyBuildDoneId( 0 ),
	m_outputData( NULL ),
	m_uvCoords0ByteOffset( 0 ),
	m_uvCoords1ByteOffset( 0 ),
	m_boneIndexOffset( 0 ),
	m_areaOffset( 0 ),
	m_effectPath( "res:/Graphics/Effect/Managed/Interior/Avatar/AvatarBRDFCombined.fx" )
{
	if( !s_wodBackgroundWelder )
	{
		// Create the background welder the first time we need to build
		s_wodBackgroundWelder = BeCallbackMan;
	}
	for( auto it = std::begin( m_batchEnabled ); it != std::end( m_batchEnabled ); ++it )
	{
		*it = false;
	}
}

// ------------------------------------------------------------------------------------------------------
Tr2SkinnedModelBuilder::~Tr2SkinnedModelBuilder()
{
	if( m_notifyBuildDoneId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_notifyBuildDoneId );
	}

	//CCP_LOG( "~Tr2SkinnedModelBuilder dtor, 0x%x", this );
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::SetAdjustPathMethod( const BlueScriptCallback& callback )
{
	m_adjustPathMethod = callback;
}

#if BLUE_WITH_PYTHON
// ------------------------------------------------------------------------------------------------------
PyObject* Tr2SkinnedModelBuilder::PyGetCollapsedInfo( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilder* pThis = BluePythonCast<Tr2SkinnedModelBuilder*>( self );

	PyObject* output = PyList_New( pThis->m_collapseInfo.size() );

	for( size_t i = 0; i != pThis->m_collapseInfo.size(); ++i )
	{
		const CollapseInfo &info = pThis->m_collapseInfo[ i ];
	
		PyObject* tuple = PyTuple_New(4);
		PyTuple_SetItem( tuple, 0, PyInt_FromLong( info.m_sourceIndex  ) );
		PyTuple_SetItem( tuple, 1, PyInt_FromLong( info.m_batchType    ) );
		PyTuple_SetItem( tuple, 2, PyInt_FromLong( info.m_areaIndex    ) );
		PyTuple_SetItem( tuple, 3, PyInt_FromLong( info.m_permuteIndex ) );

		PyList_SetItem( output, i, tuple );
	}

	return output;
}
#endif

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::DoAdjustPathCallback( std::string & path )
{
	if( !m_adjustPathMethod )
		return;

	//CCP_LOG( "Adjusting %s", path.c_str() );

	m_adjustPathMethod.Call( path, path );

	//CCP_LOG( "Adjusted: %s", path.c_str() );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2SkinnedModelBuilder::PrepareForBuildMesh( const Tr2SkinnedModelBuilderSource* source, Tr2MeshPtr mesh, const std::string& grannyPath )
{
	if( !mesh )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::PrepareForBuild: Invalid mesh'%s'\n", source->m_moduleResPath.c_str() );
		return false;
	}

	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		m_batchEnabled[i] = false;
	}
	if( m_collapseFromDepthNormal )
	{
		m_batchEnabled[TRIBATCHTYPE_DEPTHNORMAL] = true;
	}
	else
	{
		m_batchEnabled[TRIBATCHTYPE_OPAQUE]			= true;
		m_batchEnabled[TRIBATCHTYPE_DECAL]			= true;
		m_batchEnabled[TRIBATCHTYPE_TRANSPARENT]	= m_collapseTransparentAreas;
		m_batchEnabled[TRIBATCHTYPE_DEPTH]			= true;
		m_batchEnabled[TRIBATCHTYPE_ADDITIVE]		= true;
	}
	
	// this mesh might be explicitly excluded by user
	if( !source->isMeshExcluded( mesh->GetName() ) )
	{
		// collect also the granny resources of the specified LOD-mesh
		std::string grLodName = grannyPath;
		DoAdjustPathCallback( grLodName );

		TriGrannyResPtr sourceGrnRes;
		BeResMan->GetResource( grLodName.c_str(), "raw", sourceGrnRes );

		if( !sourceGrnRes )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::PrepareForBuild: Invalid granny file path '%s'\n", grLodName.c_str() );
			return true;
		}

		// put all data of this source mesh in array, so it can get assembled in the ::Build() process
		sSourceData sd;
		sd.isError = false;
		sd.mesh = mesh;
		sd.grnRes = sourceGrnRes;

		// additional override-material mesh area provided and specified?
		Tr2MeshAreaPtr overrideMaterial0 = source->getMeshOverrideMaterial0( mesh->GetName() );
		if( overrideMaterial0 )
		{
			sd.overrideMaterial0MeshArea = overrideMaterial0;
		}
		Tr2MeshAreaPtr overrideMaterial1 = source->getMeshOverrideMaterial1( mesh->GetName() );
		if( overrideMaterial1 )
		{
			sd.overrideMaterial1MeshArea = overrideMaterial1;
		}

		// more data...
		sd.upperLeftTexCoord = source->m_upperLeftTexCoord;
		sd.lowerRightTexCoord = source->m_lowerRightTexCoord;
		sd.enableCutMask = source->m_enableCutMask;

		sd.m_state = sd.New;

		m_sourceData.push_back( sd );
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2SkinnedModelBuilder::PrepareForBuild()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// clean arrays
	m_sourceData.clear();
	
#if BLUE_WITH_PYTHON
	if( !PyOS->CanYield() )
	{
		//this is a tasklet that cannot block
		PyErr_SetString( PyExc_RuntimeError, "Tr2SkinnedModelBuilder::PrepareForBuild: This tasklet cannot block" );
		return false;
	}
#endif

	if( !CreateEffect( &m_effect, true ) )
	{
		return false;
	}

	typedef std::map<std::string, const Tr2SkinnedModelBuilderSource * > MeshToSourceMap_t;
	MeshToSourceMap_t meshToSourceMap;

	for( unsigned int i = 0; i < m_sourceMeshesInfo.size(); ++i )
	{
		const Tr2SkinnedModelBuilderSource* source = m_sourceMeshesInfo[ i ];
		if( source->m_moduleResPath.empty() )
		{
			if( m_sourceSkinnedModel )
			{
				meshToSourceMap[ source->m_visualModelMeshName ] = source;
				continue;
			}
			else
			{
				CCP_LOGERR( "Tr2SkinnedModelBuilder::PrepareForBuild: Empty module res path in mesh %d\n", i );
				return false;
			}
		}
		// get new res path based on LOD
		IRoot* p = BeResMan->LoadObject( source->m_moduleResPath.c_str(), Be::LDOBJ_DONT_INITIALIZE );
		if( !p )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::PrepareForBuild: Invalid module res path '%s'\n", source->m_moduleResPath.c_str() );
			continue;
		}

		// input is Tr2SkinnedModel, containing the Tr2Meshes
		Tr2SkinnedModelPtr srcModel;
		BlueQIPtrAssign( (IRoot**)&srcModel, p, BlueInterfaceIID<Tr2SkinnedModel>() );

		p->Unlock();

		if( !srcModel )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::PrepareForBuild: source mesh is not a Tr2SkinnedModel '%s'\n", source->m_moduleResPath.c_str() );
			return false;
		}

		// collect all the meshes
		for( unsigned int m = 0; m < srcModel->GetNumOfMeshes(); ++m )
		{
			Tr2MeshPtr mesh = srcModel->GetMesh( m );
			if( !PrepareForBuildMesh( source, mesh, mesh->GetMeshResPath() ) )
			{
				return false;
			}
		}
	}

	if( m_sourceSkinnedModel )
	{
		for( unsigned m = 0; m != m_sourceSkinnedModel->GetNumOfMeshes(); ++m )
		{
			if( Tr2MeshPtr mesh = m_sourceSkinnedModel->GetMesh( m ) )
			{
				MeshToSourceMap_t::const_iterator it = meshToSourceMap.find( mesh->GetName() );
				if( it != meshToSourceMap.end() )
				{
					if( !PrepareForBuildMesh( it->second, mesh, it->second->m_visualModelMeshGrannyPath ) )
					{
						return false;
					}
				}
			}
		}
	}

	// Loop over all the resources gathered and ensure that load is finished
	// before we return from this function.
	for( unsigned int i = 0; i < m_sourceData.size(); ++i )
	{
		while( m_sourceData[i].grnRes->IsLoading() )
		{
#if BLUE_WITH_PYTHON
			if( !PyOS->Yield() )
			{
				return false;
			}
#endif
		}
	}

	for( unsigned i = 0; i != m_blendshapeInfo.size(); ++i )
	{
		blendWeights[ m_blendshapeInfo[i]->m_name ] = m_blendshapeInfo[i]->m_power;
	}

	return true;
}

bool Tr2SkinnedModelBuilder::CreateEffect( Tr2Effect ** effect, bool allowPrepare )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// create new effect
	if( !effect || !BeClasses->CreateInstance( GetTr2EffectClsid(), BlueInterfaceIID<Tr2Effect>(), (void**)effect ) )
		return false;
	
	// async load this one, we need it now!
	(*effect)->SetEffectPathName( 
			/*m_createGPUMesh	?	"res:/Graphics/Effect/Managed/Skinned.fx"
			:	"res:/Graphics/Effect/Managed/Interior/Avatar/AvatarBRDFCombined.fx"
			*/
			// the above are reasonable defaults, but let's just make it user driven:
			m_effectPath.c_str()
		);

	Tr2EffectRes* effectRes = (*effect)->GetEffectRes();
	if( !effectRes )
	{
		*effect = NULL;
		return false;
	}
	
	if( allowPrepare )
	{
		while( effectRes->IsLoading() )
		{
#if BLUE_WITH_PYTHON
			if( !PyOS->Yield() )
			{
				return false;
			}
#endif
		}
	}
	else
	{
		CCP_ASSERT( effectRes->IsPrepared() );
		if( !effectRes->IsPrepared() )
		{
			*effect = NULL;
			return false;
		}
	}

	// setup all parameters this target shader has
	(*effect)->PopulateParameters();

	for( size_t i = 0; i != m_extraArrayOf.size(); ++i )
	{
		bool found = false;
		for( size_t j = 0; j != (*effect)->m_parameters.size() && !found; ++j )
		{
			ITriEffectParameter *tep = (*effect)->m_parameters[j];
			found = m_extraArrayOf[i] == tep->GetParameterName();
		}
		if( found )
		{
			continue;
		}
		OTriFloatArrayParameter* newFloatArray = new OTriFloatArrayParameter();  // Creates object with 1 lock
		newFloatArray->m_name = BlueSharedString( m_extraArrayOf[i] );
		(*effect)->m_parameters.Insert( -1, (ITriEffectParameter*)newFloatArray );		
	}

	// make sure, all array parameters are cleared, because we will insert & fill them later
	for( Tr2Effect::EffectParameterList::const_iterator it = (*effect)->m_parameters.begin(); it != (*effect)->m_parameters.end(); ++it )
	{
		TriFloatArrayParameter* destVector4ArrayParamater = dynamic_cast<TriFloatArrayParameter*>( *it );
		if( destVector4ArrayParamater )
		{
			//	   destVector4ArrayParamater->m_value.clear();
			while( destVector4ArrayParamater->m_value.GetSize() > 0)
			{
				destVector4ArrayParamater->m_value.Remove( 0 );
			}
		}
	}

	return true;
}

void Tr2SkinnedModelBuilder::CollectAllBones()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::map<std::string, int> boneMapping;

	for( unsigned int i = 0; i < m_sourceData.size(); ++i )
	{
		if( m_sourceData[i].m_state != sSourceData::New )
		{
			continue;
		}

		// better check
		if( !m_sourceData[i].mesh )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: Invalid module res path '%s'\n", m_sourceMeshesInfo[i]->m_moduleResPath.c_str() );
			m_sourceData[i].isError = true;
			continue;
		}
		if( !m_sourceData[i].grnRes || !m_sourceData[i].grnRes->GetGrannyFile() )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: invalid gr2 file '%S'\n", m_sourceData[i].grnRes->GetPath() );
			m_sourceData[i].isError = true;
			continue;
		}

		CollectBonesFromMesh( i );

		if( m_createGPUMesh && m_outputData->m_boneMapping.size() > TR2_MAX_BONES_PER_MESHAREA && m_enableSubsetBuilding )
		{
			// back out
			m_outputData->m_boneMapping = boneMapping;
			return;
		}
		else
		if( m_enableSubsetBuilding )
		{
			boneMapping = m_outputData->m_boneMapping;
			m_sourceData[i].m_state = sSourceData::InProcess;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2SkinnedModelBuilder::Build()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_collapseInfo.clear();

	if( m_sourceData.empty() )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: No input!" );
		return false;
	}
	BeTimer time;

	m_outputData = new Tr2SkinnedModelBuilder_OutputData( this );

	// reset counters
	m_areaOffset = 0;

	// collect all bones used by all source meshes
	CollectAllBones();

	if( m_outputData->m_boneMapping.empty() )
	{
		if( !m_createGPUMesh || !m_enableSubsetBuilding )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: zero bones" );
		}
		// else this is a normal failure -- we've exhausted all source data, everything's processed
		delete m_outputData;
		m_outputData = NULL;
		return false;
	}

	if( m_createGPUMesh && m_outputData->m_boneMapping.size() > TR2_MAX_BONES_PER_MESHAREA )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: %d bones for GPU mesh, no can do.", m_outputData->m_boneMapping.size() );
		delete m_outputData;
		m_outputData = NULL;
		return false;
	}

	// create a target interior avatar and its single mesh
	if( m_createGPUMesh )
	{
		if( !m_targetSkinnedModel )
		{
			m_targetSkinnedModel.CreateInstance( GetTr2SkinnedModelClsid() );
		}
		m_outputData->m_targetMesh.CreateInstance( GetTr2MeshClsid() );
	}
	else
	{
		if( !m_targetSkinnedModel )
		{
			m_targetSkinnedModel.CreateInstance( GetTr2CpuSkinnedModelClsid() );
		}
		m_outputData->m_targetMesh.CreateInstance( GetTr2DynamicMeshClsid() );
	}
	
	// attach mesh to avatar
	m_targetSkinnedModel->m_meshes.Insert( -1, m_outputData->m_targetMesh->GetRawRoot() );

	// now that all bones are collected re-index them
	ReIndexBoneMap();

	// now we can add all the vertex/index data and do the re-map of the per-vertex bone-indices
	for( unsigned int i = 0; i < m_sourceData.size(); ++i )
	{
		// only do this if we haven't tracked an error before
		if( !m_sourceData[i].isError && m_sourceData[i].m_state == sSourceData::InProcess )
		{
			AddMeshToGrannyFile( i );
		}
	}

	// "pre-" finalize areas
	FinalizeSingleAreas();

	for( size_t i = 0; i != m_sourceData.size(); ++i )
	{
		if( m_sourceData[i].m_state == sSourceData::InProcess )
		{
			m_sourceData[i].m_state = sSourceData::Done;
		}
	}

	// do we have at least something? (is dangerous to write "nothing" to a gr2 file and return ok!)
	if( !m_outputData->m_grannyTopology.IndexCount || !m_outputData->m_grannyVertexData.VertexCount )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::Build: nothing collected for this avatar, aborting!\n" );
		return false;
	}

	m_targetSkinnedModel->SetSkeletonName( m_outputData->m_finalGrannyModel.Skeleton ? m_outputData->m_finalGrannyModel.Skeleton->Name : "None" );
	m_targetSkinnedModel->Initialize();

	// write out assembled granny
	FinalizeGrannyFile();

	if( !m_outputName.empty() )
	{
		// set resource paths (usually in the cache:/) and initialize avatar
		std::string filename = m_outputName;
		DoAdjustPathCallback( filename );
		m_outputData->m_targetMesh->SetMeshResPath( filename.c_str() );
		m_outputData->m_targetMesh->Initialize();
		m_targetSkinnedModel->SetGeometryResPath( filename.c_str() );		

		delete m_outputData;
		m_outputData = NULL;
	}

	// may want these numbers in Release for testing
	CCP_LOG( "Tr2SkinnedModelBuilder::Build took\t%g\tseconds", time.GetSeconds() );

	return true;
}

// ------------------------------------------------------------------------------------------------------
Tr2SkinnedModel* Tr2SkinnedModelBuilder::GetSkinnedModel() const
{
	return m_targetSkinnedModel;
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::Weld( const granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3* referencePositions = static_cast<Vector3*>( CCP_MALLOC( "Weld/referencePositions", referenceCount * sizeof( Vector3 ) ) );
	Vector3* positions = static_cast<Vector3*>( CCP_MALLOC( "Weld/referencePositions", count * sizeof( Vector3 ) ) );

	GrannyConvertVertexLayouts( referenceCount, m_outputData->m_grannyVertexData.VertexType, referenceVB, GrannyP3VertexType, referencePositions );
	GrannyConvertVertexLayouts( count, m_outputData->m_grannyVertexData.VertexType, vb, GrannyP3VertexType, positions );

	for( int outer = 0; outer < count; ++outer )
	{
		for( int inner = 0; inner < referenceCount; ++inner )
		{
			Vector3 d = positions[outer] - referencePositions[inner];
			float dSq = D3DXVec3LengthSq( &d );
			if( ( dSq > 0.0f ) && ( dSq < m_weldThreshold ) )
			{
				positions[outer] = referencePositions[inner];
			}
		}
	}

	granny_uint8* dst = vb;
	granny_uint8* src;
	int sz;
	D3DXFLOAT16* halfPositions = NULL;

	// Assume position is first element in vertex structure
	if( m_outputData->m_grannyVertexData.VertexType->Type == GrannyReal16Member )
	{
		halfPositions = (D3DXFLOAT16*)CCP_MALLOC( "Weld/halfPositions", 3 * count * sizeof( granny_uint16 ) );
		D3DXFloat32To16Array( halfPositions, (float*)positions, 3 * count );

		src = (granny_uint8*)halfPositions;
		sz = 6;
	}
	else
	{
		src = (granny_uint8*)positions;
		sz = 12;
	}

	for( int i = 0; i < count; ++i )
	{
		memcpy( dst, src, sz );
		dst += m_outputData->m_vertexSize;
		src += sz;
	}

	CCP_FREE( positions );
	CCP_FREE( referencePositions );
	if( halfPositions )
	{
		CCP_FREE( halfPositions );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::CollectBonesFromMesh( unsigned int ix )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2Mesh* mesh = m_sourceData[ix].mesh;
	int grannyMeshIx = mesh->GetMeshIndex();
	granny_file* grannyFile = m_sourceData[ix].grnRes->GetGrannyFile();
	if( !grannyFile )
	{
		CCP_LOGERR( "CollectBonesFromMesh: grannyfile not file: %S\n", m_sourceData[ix].grnRes->GetPath() );
		return;
	}

	granny_file_info* fi = GrannyGetFileInfo( grannyFile );

	if( fi->MeshCount <= grannyMeshIx )
	{
		CCP_LOGERR( "CollectBonesFromMesh: Granny file: %S does not have mesh index %i, only %i meshes.", m_sourceData[ix].grnRes->GetPath(), grannyMeshIx, fi->MeshCount );
		return;
	}

	granny_mesh* grannyMesh = fi->Meshes[grannyMeshIx];

	// collect bones (store each bone in a map by its name, thereby catching double!)
	for( int boneIx = 0; boneIx < grannyMesh->BoneBindingCount; ++boneIx )
	{
		const granny_bone_binding* boneBinding = &grannyMesh->BoneBindings[boneIx];
		m_outputData->m_boneMapping[boneBinding->BoneName] = 0;
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::ReIndexBoneMap()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// just give each bone in the map a seperate index
	int bindingCntr = 0;
	for( std::map<std::string, int>::iterator it = m_outputData->m_boneMapping.begin(); it != m_outputData->m_boneMapping.end(); ++it )
	{
		it->second = bindingCntr;
		// next in list
		++bindingCntr;
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2SkinnedModelBuilder::AddMeshToGrannyFile( unsigned int meshIx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2Mesh* mesh = m_sourceData[meshIx].mesh;
	TriGrannyResPtr grannyRes = m_sourceData[meshIx].grnRes;
	Tr2MeshAreaPtr override0MeshArea = m_sourceData[meshIx].overrideMaterial0MeshArea;
	Tr2MeshAreaPtr override1MeshArea = m_sourceData[meshIx].overrideMaterial1MeshArea;
	bool enableCutMask = m_sourceData[meshIx].enableCutMask;

	int grannyMeshIx = mesh->GetMeshIndex();

	if( !grannyRes || !grannyRes->IsGood() )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::AddMeshToGrannyFile: Invalid granny file, skipping!\n" );
		return false;
	}

	granny_file* grannyFile = grannyRes->GetGrannyFile();
	CW2A grannyPath( m_sourceData[meshIx].grnRes->GetPath() );

	if( !grannyFile )
	{
		CCP_LOGERR( "AddMeshToGrannyFile: grannyfile not file: %S\n", m_sourceData[meshIx].grnRes->GetPath() );
		return false;
	}

	granny_file_info* fi = GrannyGetFileInfo( grannyFile );

	if( fi->MeshCount <= grannyMeshIx )
	{
		CCP_LOGERR( "AddMeshToGrannyFile: Granny file: %S does not have mesh index %i, only %i meshes.", m_sourceData[meshIx].grnRes->GetPath(), grannyMeshIx, fi->MeshCount );
		return false;
	}

	granny_mesh* grannyMesh = fi->Meshes[grannyMeshIx];

	// first time init? all source meshes MUST have the same vertex format!
	int currentVertexSize;
	if( !m_outputData->m_grannyVertexData.VertexType )
	{
		m_outputData->m_grannyVertexData.VertexType = grannyRes->GetGrannyVertexType( grannyMeshIx );

		currentVertexSize = grannyRes->GetVertexSize( grannyMeshIx );

		m_outputData->m_vertexSize = currentVertexSize;
		m_vertexSizeSource = grannyPath;
		
		m_uvCoords0ByteOffset = grannyRes->GetVertexComponentOffset( grannyMeshIx, GrannyVertexTextureCoordinatesName "0" );
		m_uvCoords1ByteOffset = grannyRes->GetVertexComponentOffset( grannyMeshIx, GrannyVertexTextureCoordinatesName "1" );

		m_uvCoords0Type = grannyRes->GetVertexComponentType( grannyMeshIx, GrannyVertexTextureCoordinatesName "0" );
		if( m_uvCoords1ByteOffset )
		{
			m_uvCoords1Type = grannyRes->GetVertexComponentType( grannyMeshIx, GrannyVertexTextureCoordinatesName "1" );
		}

		m_boneIndexOffset = grannyRes->GetVertexComponentOffset( grannyMeshIx, GrannyVertexBoneIndicesName );
		if( m_boneIndexOffset == -1 )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder: cannot find any bone info in the vertices of mesh, skipping '%s'\n", grannyMesh->Name );
			return false;
		}
	}
	else
	{
		currentVertexSize = grannyRes->GetVertexSize( grannyMeshIx );
		if( currentVertexSize > m_outputData->m_vertexSize && !m_enableVertexChopping )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder: Mesh %s in %S is %d bytes per vertex, want %d (from %s), chopping not enabled", grannyMesh->Name, m_sourceData[meshIx].grnRes->GetPath(), currentVertexSize, m_outputData->m_vertexSize, m_vertexSizeSource.c_str() );
			return false;
		}
		else
		if( currentVertexSize < m_outputData->m_vertexSize && !m_enableVertexPadding )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder: Mesh %s in %S is %d bytes per vertex, want %d (from %s), padding not enabled", grannyMesh->Name, m_sourceData[meshIx].grnRes->GetPath(), currentVertexSize, m_outputData->m_vertexSize, m_vertexSizeSource.c_str() );
			return false;
		}
	}

	const int vertexSize = m_outputData->m_vertexSize;

	// first time init? all source meshes MUST have the same skeleton!
	if( !m_outputData->m_finalGrannyModel.Skeleton )
	{
		m_outputData->m_finalGrannyModel.Skeleton = grannyRes->GetGrannySkeleton( 0 );
		/*
		if( !m_outputData->m_finalGrannyModel.Skeleton )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder: missing skeleton for mesh, skipping: %s\n", grannyMesh->Name );
			return false;
		}
		*/
		// Allow it to still be null, we don't care, it's just used as a bonecount sanity check. If the meshes
		// don't have a skeleton, simply disable this feature and assume the TechArt guys got it right.
	}
	else
	{
		if( granny_skeleton* skl = grannyRes->GetGrannySkeleton( 0 ) )
		{
			if( m_outputData->m_finalGrannyModel.Skeleton->BoneCount != skl->BoneCount )
			{
				CCP_LOGERR( "Tr2SkinnedModelBuilder: meshes have different bone count, skipping: %s\n", grannyMesh->Name );
				return false;
			}
		}
		// else no skeleton = ok, optimized out of granny file
	}


	// copy all areas
	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_batchEnabled[i] )
		{
			CopyAreas( 
					mesh->GetAreas( TriBatchType(i) ), 
					m_outputData->m_targetMesh->GetAreas( m_collapseToOpaque ? TRIBATCHTYPE_OPAQUE : TriBatchType(i) ), 
					meshIx, TriBatchType(i),
					override0MeshArea, override1MeshArea, enableCutMask 
				);
		}
	}

	// collect vertices
	const unsigned previousVertexCount = m_outputData->m_numOfVertices;
	const unsigned addedVertexCount    = grannyMesh->PrimaryVertexData->VertexCount;
	m_outputData->m_targetVertices.resize( vertexSize * ( previousVertexCount + addedVertexCount ) );
	// straight copy or slices?
	if( currentVertexSize == vertexSize )
	{
		memcpy(	&m_outputData->m_targetVertices[ vertexSize * previousVertexCount ], grannyMesh->PrimaryVertexData->Vertices, addedVertexCount * vertexSize );
	}
	else
	{
		if( currentVertexSize < vertexSize )
		{
			for( unsigned i = 0; i != addedVertexCount; ++i )
			{
				memcpy( &m_outputData->m_targetVertices[vertexSize * ( previousVertexCount + i )], grannyMesh->PrimaryVertexData->Vertices + i * currentVertexSize, currentVertexSize );
				memset( &m_outputData->m_targetVertices[vertexSize * ( previousVertexCount + i )] + currentVertexSize, 0, vertexSize - currentVertexSize );
			}
		}
		else
		{
			for( unsigned i = 0; i != addedVertexCount; ++i )
			{
				memcpy( &m_outputData->m_targetVertices[vertexSize * ( previousVertexCount + i )], grannyMesh->PrimaryVertexData->Vertices + i * currentVertexSize, vertexSize );
			}
		}
	}

	granny_uint8* dstVb = &m_outputData->m_targetVertices[ vertexSize * previousVertexCount ];
	m_outputData->m_numOfVertices += addedVertexCount;

	if( !grannyRes->BakeBlendshape( grannyMeshIx, blendWeights, dstVb, addedVertexCount * vertexSize ) )
	{
		CCP_LOGERR( "BakeBlendshape failed for %s in %S", grannyMesh->Name, m_sourceData[meshIx].grnRes->GetPath() );
	}

	// all the new verts have "wrong" blend-indices, so correct them
	
	// 4 bones
	uint8_t* boneIndex0 = dstVb + m_boneIndexOffset + 0;
	uint8_t* boneIndex1 = dstVb + m_boneIndexOffset + 1;
	uint8_t* boneIndex2 = dstVb + m_boneIndexOffset + 2;
	uint8_t* boneIndex3 = dstVb + m_boneIndexOffset + 3;
	// remap uv-coords
	Vector2 scale = m_sourceData[meshIx].lowerRightTexCoord - m_sourceData[meshIx].upperLeftTexCoord;
	
	uint8_t* uvCoords0 = dstVb + m_uvCoords0ByteOffset;	
	uint8_t* uvCoords1 = m_uvCoords1ByteOffset != -1 ? dstVb + m_uvCoords1ByteOffset : NULL;
	
	// re-position uv's
	ReMapUVs( uvCoords0, m_uvCoords0Type, scale, m_sourceData[meshIx].upperLeftTexCoord, vertexSize, addedVertexCount );
	if( uvCoords1 )
	{
		ReMapUVs( uvCoords1, m_uvCoords1Type, scale, m_sourceData[meshIx].upperLeftTexCoord, vertexSize, addedVertexCount );
	}

	for( unsigned vertIx = 0; vertIx != addedVertexCount; ++vertIx )
	{
		// get bone names, re-index and store again
		const char* boneName0 = grannyMesh->BoneBindings[ *boneIndex0 ].BoneName;
		*boneIndex0 = (uint8_t)GetBoneIndexFromMap( boneName0 );
		const char* boneName1 = grannyMesh->BoneBindings[ *boneIndex1 ].BoneName;
		*boneIndex1 = (uint8_t)GetBoneIndexFromMap( boneName1 );
		const char* boneName2 = grannyMesh->BoneBindings[ *boneIndex2 ].BoneName;
		*boneIndex2 = (uint8_t)GetBoneIndexFromMap( boneName2 );
		const char* boneName3 = grannyMesh->BoneBindings[ *boneIndex3 ].BoneName;
		*boneIndex3 = (uint8_t)GetBoneIndexFromMap( boneName3 );


		// next vert
		boneIndex0 += vertexSize;
		boneIndex1 += vertexSize;
		boneIndex2 += vertexSize;
		boneIndex3 += vertexSize;
	}

	// do the welding
	if( previousVertexCount && m_weldThreshold )
	{
		//BeTimer time2;
		Weld( &m_outputData->m_targetVertices[0], previousVertexCount, dstVb, addedVertexCount );
		//CCP_LOG( "Tr2SkinnedModelBuilder -- Welding \t%g\tseconds", time2.GetSeconds() );
	}

	// remember
	m_outputData->m_grannyVertexData.Vertices = &m_outputData->m_targetVertices[0];
	m_outputData->m_grannyVertexData.VertexCount = m_outputData->m_numOfVertices;



	// collect index buffers
	// store indices (=triangles) per areatype throughout the assembly process, so we can later put these into one single area pre areatype
	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_batchEnabled[i] )
		{
			CollectIndicesPerArea( grannyMesh, mesh->GetAreas( TriBatchType(i)  ), previousVertexCount, m_outputData->m_targetIndices[i] );
		}
	}
	// also collect materials, so we have the correct id to write into each vertex
	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_batchEnabled[i] )
		{
			int materialOffset = ( override1MeshArea && !mesh->GetAreas( TriBatchType(i) )->empty() ) ? 2 : 1;
			CollectMaterialsPerArea( grannyMesh, mesh->GetAreas( TriBatchType(i) ), materialOffset, &m_outputData->m_targetMaterial[i], i );
		}
	}
	// just store all indices from mesh
	CollectIndicesPerMesh( grannyMesh, previousVertexCount, m_outputData->m_targetIndicesAll );

	// update counters
	m_outputData->m_numOfIndices = (int)m_outputData->m_targetIndicesAll.size();

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::CollectIndicesPerArea( const granny_mesh* mesh, const Tr2MeshAreaVector* srcAreas, unsigned int ixOffset, std::vector<int>& indices )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( srcAreas )
	{
		// 16 vs 32 bit indices
		bool is16Bit = ( mesh->PrimaryTopology->Index16Count > 0 );

		// all areas
		for( Tr2MeshAreaVector::const_iterator it = srcAreas->begin(); it != srcAreas->end(); ++it )
		{
			Tr2MeshAreaPtr area = *it;
			if( area->IsReversed() && m_removeReversed )
			{
				continue;
			}
			for( int a = 0; a < area->GetCount(); ++a )
			{
				// check if this area exists in granny model
				if( a + area->GetIndex() >= mesh->PrimaryTopology->GroupCount )
				{
					continue;
				}
				granny_tri_material_group grp = mesh->PrimaryTopology->Groups[a + area->GetIndex()];
				// do a realloc of indices vector here
				indices.reserve( indices.size() + 3 * grp.TriCount );
				// collect all indices from this group (and convert to 32bit)
				for( int i = 0; i < 3 * grp.TriCount; ++i )
				{
					unsigned int ix;
					if( is16Bit )
					{
						ix = mesh->PrimaryTopology->Indices16[i + 3 * grp.TriFirst];
					}
					else
					{
						ix = mesh->PrimaryTopology->Indices[i + 3 * grp.TriFirst];
					}
					// offset by earlier vertex buffers
					ix += ixOffset;
					indices.push_back( ix );
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::CollectIndicesPerMesh( const granny_mesh* mesh, unsigned int ixOffset, std::vector<int>& indices )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// 16 vs 32 bit indices
	const bool is16Bit = ( mesh->PrimaryTopology->Index16Count > 0 );
	// total
	const int idxCount = is16Bit ? mesh->PrimaryTopology->Index16Count : mesh->PrimaryTopology->IndexCount;
	if( !idxCount )
	{
		return;
	}
	// do a realloc of indices vector here
	const size_t oldSize = indices.size();
	indices.resize( oldSize + idxCount );
	int* out = &indices[ oldSize ];
	if( is16Bit )
	{
		granny_uint16 * in = mesh->PrimaryTopology->Indices16;
		for( unsigned i = 0; i != idxCount; ++i )
		{
			*out++ = *in++ + ixOffset;
		}
	}
	else
	{
		granny_int32 * in = mesh->PrimaryTopology->Indices;
		for( unsigned i = 0; i != idxCount; ++i )
		{
			*out++ = *in++ + ixOffset;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::CollectMaterialsPerArea( const granny_mesh* mesh, const Tr2MeshAreaVector* srcAreas, int materialOffset, unsigned int* material, int areaIndex )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( srcAreas )
	{
		// all areas
		for( Tr2MeshAreaVector::const_iterator it = srcAreas->begin(); it != srcAreas->end(); ++it )
		{
			Tr2MeshAreaPtr area = *it;
			if( area->IsReversed() && m_removeReversed )
			{
				continue;
			}
			for( int a = 0; a < area->GetCount(); ++a )
			{
				// check if this area exists in granny model
				if( a + area->GetIndex() >= mesh->PrimaryTopology->GroupCount )
				{
					continue;
				}
				granny_tri_material_group grp = mesh->PrimaryTopology->Groups[a + area->GetIndex()];
				// this group is used, so add to group-list
				grp.TriFirst += m_outputData->m_numOfIndices / 3;
				grp.MaterialIndex = areaIndex * 1024 + *material;
				m_outputData->m_grannyGroups.push_back( grp );
				(*material) += materialOffset;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
//void Tr2SkinnedModelBuilder::ReMapUV( BYTE* uv, granny_member_type type, const Vector2* scale, const Vector2* offset )
void Tr2SkinnedModelBuilder::ReMapUVs( uint8_t* uv, granny_member_type type, const Vector2& scale, const Vector2& offset, size_t stride, unsigned count )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( type == GrannyReal32Member )
	{
		// full float
		while( count-- )
		{
			Vector2& uvVec2 = *reinterpret_cast<Vector2*>( uv );
			uvVec2.x = uvVec2.x * scale.x + offset.x;
			uvVec2.y = uvVec2.y * scale.y + offset.y;
			uv += stride;
		}
	}
	else if( type == GrannyReal16Member )
	{
		// half float, use D3DX help
		while( count-- )
		{
			// 16 -> 32
			Vector2 uvVec2;
			D3DXFloat16To32Array( (float*)&uvVec2, (D3DXFLOAT16*)uv, 2 );
			uvVec2.x = uvVec2.x * scale.x + offset.x;
			uvVec2.y = uvVec2.y * scale.y + offset.y;
			// 32 -> 16
			D3DXFloat32To16Array( (D3DXFLOAT16*)uv, (float*)&uvVec2, 2 );

			uv += stride;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::AddMaterialIndex()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// vertex buffer gets bigger by two additional floats per vertex -- a material index and an area index / type ( 0 = opaque, 1 = decal, etc )
	int newVertexSize = m_outputData->m_vertexSize + sizeof( float ) * 2;
	m_outputData->m_targetVertices.resize( newVertexSize * m_outputData->m_numOfVertices );
	// expand verts by putting on float between them
	for( int v = m_outputData->m_numOfVertices - 1; v >= 0; --v )
	{
		const uint8_t* const src = &m_outputData->m_targetVertices[v * m_outputData->m_vertexSize];
		uint8_t* const dst = &m_outputData->m_targetVertices[v * newVertexSize];
		memcpy( dst, src, m_outputData->m_vertexSize );
		float* f = reinterpret_cast<float*>( dst + m_outputData->m_vertexSize );
		f[0] = 0;
		f[1] = 0;
	}

	// store new index by cycling through the group list
	for( unsigned int g = 0; g < m_outputData->m_grannyGroups.size(); ++g )
	{
		const granny_int32 code = m_outputData->m_grannyGroups[g].MaterialIndex;
		const float material = (float)( code & 1023 );
		const float type     = (float)( code / 1024 );

		const int* indices = &m_outputData->m_targetIndicesAll[3 * m_outputData->m_grannyGroups[g].TriFirst];
		for( int i = 0; i < 3 * m_outputData->m_grannyGroups[g].TriCount; i++ )
		{
			float* newData = (float*)&m_outputData->m_targetVertices[*indices * newVertexSize + m_outputData->m_vertexSize];
			newData[0] = material;
			newData[1] = type;
			++indices;
		}
	}

	/*
	for( unsigned i = 0; i != m_outputData->m_numOfVertices; ++i )
	{
		float* newData = (float*)&m_outputData->m_targetVertices[ i * newVertexSize + m_outputData->m_vertexSize ];
		if( newData[0] == 999.0f )
		{
			CCP_LOGERR( "Tr2SkinnedModelBuilder::AddMaterialIndex: some vertices are not in any group\n" );
			return;
		}
	}
	*/

	// now change the granny vertex format description
	granny_data_type_definition* vertexFormat = m_outputData->m_grannyVertexData.VertexType;
	if( !vertexFormat )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::AddMaterialIndex: couldn't find any vertex format information!\n" );
		return;
	}
	int componentIx = 0;
	while( vertexFormat[componentIx].Type != GrannyEndMember )
	{
		++componentIx;
	}

	// need to alloc new memory for this granny vertex type array
	granny_data_type_definition* newVertexFormat = (granny_data_type_definition*)CCP_MALLOC( "Tr2SkinnedModelBuilder/vertexFormat", ( componentIx + 2 ) * sizeof( granny_data_type_definition ) );
	// copy old vertex types
	memcpy( newVertexFormat, vertexFormat, ( componentIx + 1 ) * sizeof( granny_data_type_definition ) );
	// move end marker to new end of array
	memcpy( newVertexFormat + componentIx + 1, newVertexFormat + componentIx, sizeof( granny_data_type_definition ) );
	// setup new last real vertex component
	newVertexFormat[componentIx].Type = GrannyReal32Member;
	newVertexFormat[componentIx].ArrayWidth = 2;
	newVertexFormat[componentIx].Name = "TextureCoordinates2";

	// store
	m_outputData->m_grannyVertexData.VertexType = newVertexFormat;
	m_outputData->m_vertexSize = newVertexSize;
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::CopyAreas( Tr2MeshAreaVector* src, Tr2MeshAreaVector* dst, unsigned sourceIndex, unsigned batchType, Tr2MeshAreaPtr override0, Tr2MeshAreaPtr override1, bool enableMask )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( src );
	CCP_ASSERT( dst );

	// there must be at least one source area
	if( src->size() )
	{
		// do we already have a dest area from a previous mesh?
		if( dst->empty() )
		{
			// create new one
			Tr2MeshAreaPtr area;
			if( area.CreateInstance() )
			{
				// fill it with content from first source area to get all important indices, etc.
				*area = *(*src)[0];
				area->SetIndex( /*area->GetIndex() +*/ m_areaOffset );
				// areaOffset really is a counter for meshes in the target model
				++m_areaOffset;
				
				// put effect "onto" the right mesh-type
				Tr2EffectPtr effect;
				if( CreateEffect( &effect, false ) )
				{
					area->SetMaterial( effect );
				}
			}
			// and put into target
			dst->Insert( -1, area );
		}

		// now check all areas in source mesh and put their data into the single dest mesh
		unsigned areaCounter = 0;
		for( Tr2MeshAreaVector::const_iterator itArea = src->begin(); itArea != src->end(); ++itArea, ++areaCounter )
		{
			// source area...
			Tr2MeshAreaPtr srcArea = *itArea;

			if( srcArea->IsHidden() )
			{
				continue;
			}

			if( srcArea->IsReversed() && m_removeReversed )
			{
				continue;
			}

			CollapseInfo info;
			info.m_sourceIndex = sourceIndex;
			info.m_batchType   = batchType;
			info.m_areaIndex   = areaCounter;

			// ...might be overriden
			if( override0 )
			{
				srcArea = override0;
			}
			// it's all in the effect
			ITr2ShaderMaterial* srcEffect  = srcArea->GetMaterialInterface();
			Tr2Effect* destEffect = dynamic_cast<Tr2Effect*>( ( *dst )[0]->GetMaterialInterface() );

			if( Tr2ShaderMaterial * material = dynamic_cast<Tr2ShaderMaterial*>( srcEffect ) )
			{
				info.m_permuteIndex = material->GetLowLevelPermuteIndex();
			}
			else
			{
				info.m_permuteIndex = 0;
			}

			m_collapseInfo.push_back( info );

			if( srcEffect && destEffect )
			{
				// cycle through all parameters of the dest effect, they are the ones that need data
				for( Tr2Effect::EffectParameterList::const_iterator it = destEffect->m_parameters.begin(); it != destEffect->m_parameters.end(); ++it )
				{
					const char* paramName = (*it)->GetParameterName();
					// if this is an array paramater, it must start with "ArrayOf"
					if( strncmp( paramName, "ArrayOf", 7 ) == 0 )
					{
						// so this is a float array, cast it!
						TriFloatArrayParameter* dstFloatArrayParameter = dynamic_cast<TriFloatArrayParameter*>( *it );
						if( dstFloatArrayParameter )
						{
							// "ArrayOf..." so "..." is source parameter name (non-array)
							AppendToEffectArrayParam( dstFloatArrayParameter, &paramName[7], srcEffect, srcArea, enableMask );
							// second override appends right here
							if( override1 )
							{
								AppendToEffectArrayParam( dstFloatArrayParameter, &paramName[7], dynamic_cast<Tr2Effect*>( override1->GetMaterialInterface()), srcArea, enableMask );
							}
						}
					}
				}

				// handle resources: for now just enforce the brdf lookup texture
				for( Tr2Effect::EffectResourceList::const_iterator it = destEffect->m_resources.begin(); it != destEffect->m_resources.end(); ++it )
				{
					// name: check for 'FresnelLookupMap'
					const char* resourceName = (*it)->GetParameterName();
					if( resourceName && ( strcmp( resourceName, "FresnelLookupMap" ) == 0 ) )
					{
						// is resource we ant to change
						TriTexture2DParameter* destTextureParameter = dynamic_cast<TriTexture2DParameter*>( *it );
						if( destTextureParameter )
						{
							destTextureParameter->SetResourcePath( "res:/Texture/Global/brdfLibrary.dds" );
						}
					}
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::AppendToEffectArrayParam( TriFloatArrayParameter* arrayParameter, const char* paramName, const ITr2ShaderMaterial *srcEffect, const Tr2MeshAreaPtr area, bool enableMask )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// check if there is still some room for another material
	if( arrayParameter->m_value.size() >= TR2SKINNEDMODELBUILDER_MAX_MATERIAL )
	{
		CCP_LOGERR( "Tr2SkinnedModelBuilder::CopyAreas: too many materials! mesharea: '%s'\n", area->GetName().c_str() );
		return;
	}
	// need value to copy
	Vector4 parameterValue( 0.f, 0.f, 0.f, 0.f );
	// find source parameter
	ITriEffectParameter* srcParameter = srcEffect->GetParameterByName( paramName );
	if( srcParameter )
	{
		if( TriVector4Parameter* src = dynamic_cast<TriVector4Parameter*>( srcParameter ) )
		{
			parameterValue = src->m_value;
		}
		else if( Tr2Vector4Parameter* src = dynamic_cast<Tr2Vector4Parameter*>( srcParameter ) )
		{
			parameterValue = src->m_value;
		}
		else if( TriFloatParameter* src = dynamic_cast<TriFloatParameter*>( srcParameter ) )
		{
			parameterValue.x = src->m_value;
		}
		else if( Tr2FloatParameter* src = dynamic_cast<Tr2FloatParameter*>( srcParameter ) )
		{
			parameterValue.x = src->m_value;
		}
	}
	else
	{
		// this parameter is not found, this can be dangerous! at least we must insert a default value
		CCP_LOGWARN( "Tr2SkinnedModelBuilder::CopyAreas: material attribute '%s' missing in source mesh '%s', using 0\n", paramName, area->GetName().c_str() );
	}

	// modify value?
	if( enableMask )
	{
		if( strcmp( paramName, "MaterialLibraryID" ) == 0 )
		{
			parameterValue.y = 1.f;
		}
	}

	// put this parameter into array of dest parameter
	TriVector4Ptr newVector4;
	if( newVector4.CreateInstance() )
	{
		newVector4->m_data = parameterValue;
		arrayParameter->m_value.Insert( -1, newVector4 );
	}
}

// ------------------------------------------------------------------------------------------------------
int Tr2SkinnedModelBuilder::GetBoneIndexFromMap( const char* boneName ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::map<std::string, int>::const_iterator finder = m_outputData->m_boneMapping.find( boneName );
	if( finder != m_outputData->m_boneMapping.end() )
		return finder->second;
	return 0;
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::FinalizeSingleAreas()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// add a material index to each vertex based on its original group
	AddMaterialIndex();

	// it is not really a single area, it is a single area per renderbatch state,
	// like OPAQUE, DECAL, TRANSPARENT, etc...
	// so we have to re-order all the collected triangles in order to create these few batches
	unsigned totalCount = 0;
	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_batchEnabled[i] )
		{
			totalCount += (unsigned int)m_outputData->m_targetIndices[i].size();
		}
	}
	m_outputData->m_targetIndicesAll.resize( totalCount );

	unsigned indicesCounter = 0;
	granny_tri_material_group grp;
	std::vector<granny_tri_material_group> singleAreaGroups;

	grp.TriCount = 0;
	grp.TriFirst = 0;
	
	for( unsigned i = 0; i != TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++i )
	{
		if( m_batchEnabled[i] && !m_outputData->m_targetIndices[i].empty() )
		{
			// group
			grp.MaterialIndex = 0;
			if( m_collapseToOpaque )
			{
				grp.TriCount += (unsigned int)m_outputData->m_targetIndices[i].size() / 3;
			}
			else
			{
				grp.TriFirst = indicesCounter / 3;
				grp.TriCount = (unsigned int)m_outputData->m_targetIndices[i].size() / 3;
				singleAreaGroups.push_back( grp );
			}
			// indices of that group
			memcpy( &m_outputData->m_targetIndicesAll[ indicesCounter ], &m_outputData->m_targetIndices[i][0], m_outputData->m_targetIndices[i].size() * sizeof( int ) );
			indicesCounter += (unsigned int)m_outputData->m_targetIndices[i].size();
		}
	}

	if( m_collapseToOpaque && grp.TriCount != 0 )
	{
		singleAreaGroups.push_back( grp );
	}
	
	// group info is now obsolete: replace with new one
	m_outputData->m_grannyGroups = singleAreaGroups;

	// assign to granny struct for write-out
	m_outputData->m_grannyTopology.GroupCount = (unsigned int)m_outputData->m_grannyGroups.size();
	m_outputData->m_grannyTopology.Groups = m_outputData->m_grannyGroups.empty() ? NULL : &m_outputData->m_grannyGroups[0];

	m_outputData->m_grannyTopology.Indices = &m_outputData->m_targetIndicesAll[0];
	m_outputData->m_grannyTopology.IndexCount = (unsigned int)m_outputData->m_targetIndicesAll.size();

	m_outputData->m_grannyVertexData.Vertices = &m_outputData->m_targetVertices[0];
	m_outputData->m_grannyVertexData.VertexCount = m_outputData->m_numOfVertices;
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::FinalizeGrannyFile()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_outputData->Finalize();
	
	if( !m_outputName.empty() )
	{
		// save it to temp gr2 file
		granny_file_builder* Builder = GrannyBeginFileInMemory( 1, GrannyCurrentGRNStandardTag, GrannyGRNFileMV_ThisPlatform, 256 << 10 );
		granny_file_data_tree_writer* Writer = GrannyBeginFileDataTreeWriting( GrannyFileInfoType, &m_outputData->m_grannyFileInfo, 0, 0 );
		GrannyWriteDataTreeToFileBuilder( Writer, Builder );
		GrannyEndFileDataTreeWriting( Writer );

		// finalize it to provided path
		std::string fullName = m_outputName;
		DoAdjustPathCallback( fullName );
		std::wstring fullNameW = (const wchar_t*)CA2W( fullName.c_str() );
		std::wstring fullPathW = BePaths->ResolvePathForWritingW( fullNameW );
		{
			//BeTimer time2;

			granny_file_writer* MemWriter = GrannyCreateMemoryFileWriter( 256 << 10 );
			if( MemWriter )
			{
				GrannyEndFileToWriter( Builder, MemWriter );

				granny_uint8 *FileBuffer = 0;
				granny_int32 FileBufferSize = 0;
				GrannyStealMemoryWriterBuffer( MemWriter, &FileBuffer, &FileBufferSize );
				if( FileBuffer && FileBufferSize )
				{
					FILE* output;
					fopen_s( &output, CW2A( fullPathW.c_str() ), "wb" );
					if( output )
					{
						fwrite( FileBuffer, FileBufferSize, 1, output );
						fclose( output );
					}
					else
					{		
						CCP_LOG("Tr2SkinnedModelBuilder: Error opening %S for writing", fullPathW.c_str() );
					}
					GrannyFreeMemoryWriterBuffer( FileBuffer );
					GrannyCloseFileWriter( MemWriter );
				}
			}

			//CCP_LOG( "Tr2SkinnedModelBuilder -- GrannyEndFile:\t%g\tseconds", time2.GetSeconds() );		
		}
	}
	else
	{
		// Not interested in caching, so avoid the extra step of writing the .gr2 out to disk, only to read
		// it back right away.
		// Shortcut this by creating a TriGeometryRes, and initializing it directly from a granny_file_info pointer.
		// This replaces the DoLoad part, but we need DoPrepare as well, which needs to happen on the main thread.
		// So schedule a call on BeResMan; we need to make sure both 'this' and the file info are valid until that
		// has happened, so this->Lock, plus put all needed temporary data in a member (m_fileInfoData).

		// DoLoad equivalent
		TriGeometryResPtr res;
		res.CreateInstance();
		res->m_name = "Collapsed mesh";
		
		res->InitializeFromMemory( &m_outputData->m_grannyFileInfo );
		m_outputData->m_targetMesh->SetGeometryRes( res );

		// DoPrepare setup
		this->Lock();
		BeResMan->AddToQueue( BRMQ_MAIN, &Tr2SkinnedModelBuilder_OutputData::StaticDoPrepare, m_outputData, 0, NULL );
		m_outputData = NULL;
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::StaticBuildAsync( void* context )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2SkinnedModelBuilder* pThis = static_cast<Tr2SkinnedModelBuilder*>( context );
	pThis->m_buildSucceeded = pThis->Build();
	BeResMan->AddToQueue( BRMQ_MAIN, StaticNotifyBuildDone, context, 0, &pThis->m_notifyBuildDoneId );
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::BuildAsync( const BlueScriptCallback& doneCallback )
{
	m_doneCallbackToPython = doneCallback;

	// Make sure we survive should the python reference get dropped while we're in the async build
	this->Lock();
	s_wodBackgroundWelder->Add( StaticBuildAsync, (void*)this, 0, NULL );
}

// ------------------------------------------------------------------------------------------------------
void Tr2SkinnedModelBuilder::StaticNotifyBuildDone( void* context )
{
	Tr2SkinnedModelBuilder* pThis = static_cast<Tr2SkinnedModelBuilder*>( context );

	if( pThis->m_doneCallbackToPython )
	{
		pThis->m_doneCallbackToPython.CallVoid( pThis->m_buildSucceeded );
	}

	// Make sure we don't accidentally cancel some random thread if this builder goes out of scope
	// and the notify slot is already reused by someone.
	pThis->m_notifyBuildDoneId = 0;


	pThis->Unlock();
}

// ------------------------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
PyObject* Tr2SkinnedModelBuilder::PySetExtraArrayOf( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilder* pThis = BluePythonCast<Tr2SkinnedModelBuilder*>( self );
	
	PyObject* v = PyTuple_GetItem( args, 0 );
	// Setter
	pThis->m_extraArrayOf.clear();
	if( !PyList_Check( v ) )
	{
		PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
		return NULL;
	}
	for( Py_ssize_t i = 0; i < PyList_GET_SIZE(v); ++i )
	{
		PyObject* item = PyList_GetItem( v, i );
		if( !PyString_Check( item ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
			return NULL;
		}

		pThis->m_extraArrayOf.push_back( std::string( PyString_AsString( item ), PyString_Size( item ) ) );
	}
	Py_RETURN_NONE;
}
#endif

