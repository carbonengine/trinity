#pragma once
#ifndef Tr2SkinnedModelBuilderSource_h
#define Tr2SkinnedModelBuilderSource_h


BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

/////////////////////////////////////////////////////////////////////////////////////
// source info on a mesh for the assembler
/////////////////////////////////////////////////////////////////////////////////////
BLUE_DECLARE( Tr2SkinnedModelBuilderSource );
BLUE_DECLARE_VECTOR( Tr2SkinnedModelBuilderSource );
class Tr2SkinnedModelBuilderSource :
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

	Tr2SkinnedModelBuilderSource( IRoot* lockobj = NULL );
	~Tr2SkinnedModelBuilderSource() {}

	// search helper
	bool isMeshExcluded( const char* meshName ) const;
	Tr2MeshAreaPtr getMeshOverrideMaterial ( const std::vector<std::string> & overrideMaterialMeshes, const PTr2MeshAreaVector & overrideMaterial, const char* const meshName, const unsigned digit ) const;
	Tr2MeshAreaPtr getMeshOverrideMaterial0( const char* meshName ) const;
	Tr2MeshAreaPtr getMeshOverrideMaterial1( const char* meshName ) const;

	// resource name
	std::string m_moduleResPath;
	// upperleft texcoord
	Vector2 m_upperLeftTexCoord;
	// lowerright texcoord
	Vector2 m_lowerRightTexCoord;
	// list of mesh-names to exclude
	std::vector<std::string> m_excludeMeshes;
	// standard override of material0 (meshareas)
	std::vector<std::string> m_overrideMaterial0Meshes;
	PTr2MeshAreaVector m_overrideMaterial0;
	// 2nd override of material1 (meshareas)
	std::vector<std::string> m_overrideMaterial1Meshes;
	PTr2MeshAreaVector m_overrideMaterial1;
	// uses cutmask information
	bool m_enableCutMask;

	// when not using a moduleResPath, map a Tr2Mesh.name to its granny path, in case
	// geometryResPath is empty due to blendshapes.
	std::string m_visualModelMeshName;
	std::string m_visualModelMeshGrannyPath;
};
TYPEDEF_BLUECLASS( Tr2SkinnedModelBuilderSource );

#endif //Tr2SkinnedModelBuilderSource_h
