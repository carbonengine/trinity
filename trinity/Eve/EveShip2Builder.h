#pragma once
#ifndef EveShip2Builder_h
#define EveShip2Builder_h


#include "Resources/TriGrannyRes.h"

BLUE_DECLARE( EveShip2Builder);
BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE_VECTOR( TriGrannyRes );
BLUE_DECLARE( EveSOFDataHull );
BLUE_DECLARE_VECTOR( EveSOFDataHull );

class EveShip2Builder:
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

    EveShip2Builder( IRoot* lockobj = NULL );
	virtual ~EveShip2Builder();

protected:
	void Weld( granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count ) const;
	void InitializeGrannyFile();
	bool CombineGrannyGeometry(int grnResIdx, const Matrix& offsetTransform);
	bool CombineHullGeometry();
	bool AddGeometry(const TriGrannyResPtr grnRes, const granny_real32* affine, const granny_real32* matrix, const granny_real32* invMatrix);
	void FinalizeGrannyFile( const std::string& outputName, const bool& combineGroups = false );

	granny_file_info m_grannyFileInfo;
	granny_mesh m_finalGrannyMesh;
	int m_areaOffset;

	granny_vertex_data m_grannyVertexData;
	granny_tri_topology m_grannyTopology;
	std::vector<granny_tri_material_group> m_grannyGroups;
	std::vector<granny_material*> m_grannyMaterials;
	int m_vertexSize;

	float m_weldThreshold;
	
	// source hull files
	PTriGrannyResVector m_grannyResources;
	PEveSOFDataHullVector m_hulls;
	// dest granny path
	std::string m_outputFilename;
};

TYPEDEF_BLUECLASS( EveShip2Builder );
#endif //EveShip2Builder_h
