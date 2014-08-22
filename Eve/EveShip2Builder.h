#pragma once
#ifndef EveShip2Builder_h
#define EveShip2Builder_h


#include "SpaceObject/EveShip2.h"
#include "Resources/TriGrannyRes.h"

BLUE_DECLARE( EveShip2Builder);
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE( EveSpotlightSet );
BLUE_DECLARE( Tr2Mesh );

class EveShip2Builder:
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

    EveShip2Builder( IRoot* lockobj = NULL );
	~EveShip2Builder();

	bool PrepareForBuild();
	bool Build();

	EveShip2* GetShip();

protected:
	void CopyLocators( EveLocator2Vector& locators, const Vector3& offset );
#if BLUE_WITH_PYTHON
	void CopyDamageLocators( EveDamageLocatorStructureList& locators, const Vector3& offset );
#endif

	void CopyAreas( Tr2MeshAreaVector* src, Tr2MeshAreaVector* dst );
	void CopySpriteSets( EveSpriteSetVector* src, EveSpriteSetVector* dst, const Vector3& offset );
	void CopySpotlightSets( const EveSpotlightSetVector* src, EveSpotlightSetVector* dst, const Vector3& offset );
	void CopyDecals( const EveSpaceObjectDecalVector* src, EveSpaceObjectDecalVector* dst, const Vector3& offset );
	void Weld( granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count );
	void InitializeGrannyFile();
	bool AddMeshToGrannyFile( TriGrannyResPtr* grannies, int ix, Tr2Mesh* mesh, const Vector3& offset, Tr2Mesh* dstMesh );
	void CalculateBoundingSphere( const Vector3* offsets );
	void CalculateAudioBooster();
	void FinalizeGrannyFile( const std::string& outputName );

	static void StaticBuildAsync( void* context );
	void BuildAsync( const BlueScriptCallback& pyCallback );

	static void StaticNotifyBuildDone( void* context );

protected:
	static const int MODULE_MAX_COUNT = 5;

	std::string m_moduleResPath[MODULE_MAX_COUNT];
	EveShip2Ptr m_module[MODULE_MAX_COUNT];
	TriGrannyResPtr m_grannies[MODULE_MAX_COUNT];
	EveShip2Ptr m_ship;
	std::string m_highDetailOutputName;

	int m_turretLocatorCount;
	int m_boosterLocatorCount;
	std::vector<Vector3> m_allAudioBoosterPositions;

	granny_file_info m_grannyFileInfo;
	granny_mesh m_finalGrannyMesh;
	int m_areaOffset;

	granny_vertex_data m_grannyVertexData;
	granny_tri_topology m_grannyTopology;
	std::vector<granny_tri_material_group> m_grannyGroups;
	std::vector<granny_material*> m_grannyMaterials;
	int m_vertexSize;

	float m_weldThreshold;

	Vector4 m_boundingSphere;

	CcpAtomic<uint32_t> m_buildQueueID;
	CcpAtomic<uint32_t> m_notifyBuildDoneId;
	BlueScriptCallback m_doneCallbackToPython;
	bool m_buildSucceeded;
};

TYPEDEF_BLUECLASS( EveShip2Builder );
#endif //EveShip2Builder_h
