////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOF_H
#define EveSOF_H

#include "EveSOFDataMgr.h"

// forwards
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( EveShip2 );
BLUE_DECLARE( EveSOF );
BLUE_DECLARE( EveSOFDNA );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );
enum TriBatchType;

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOF ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSOF( IRoot* lockobj = NULL );
	~EveSOF();

	// build a spaceship and return a EveShip2 object
	IRootPtr Build( const char* hullName, const char* factionName, const char* raceName );
	// build a spaceship from a dns string and return a EveShip2 object
	IRootPtr BuildFromDNA( const char* dnaString );

	// maintain the old style loading (with a bit of the new way...)
	IRootPtr Load( const char* resFile, const char* hullName, const char* factionName, const char* raceName );


private:
	// all setup functions for the to-be-created spaceship
	void SetupMesh( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupSpriteSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupSpotlightSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupPlaneSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupChildrenAndAnimations( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupDecals( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupModelCurves( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;
	void SetupLocators( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;

	// helper functions
	void FillMeshAreaVector( Tr2MeshPtr mesh, Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna ) const;
	void ModifyResourcePathsForLOD( const Tr2MeshAreaVector* areas, const char* lodInsert ) const;
	Tr2MeshPtr CreateMeshLOD( const Tr2Mesh* base, const char* lodInsert ) const;
	bool GenerateLodResourcePaths( std::string& mediumResPath, std::string& lowResPath, const char* resPath, const char* usage ) const;

	// all the source data
	PEveSOFDataMgr m_dataMgr;

	// shared
	Tr2EffectPtr m_spriteSetEffect, m_spriteSetEffectSkinned;
	Tr2EffectPtr m_shadowEffect, m_shadowEffectSkinned;
};

TYPEDEF_BLUECLASS( EveSOF );

#endif // EveSOF_H