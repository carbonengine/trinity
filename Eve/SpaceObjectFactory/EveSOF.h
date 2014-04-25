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
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

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

	// maintain the old style loading (with a bit of the new way...)
	IRootPtr Load( const char* resFile, const char* hullName, const char* raceName );


private:
	typedef std::map<std::string, EveSOFDataMgr::FactionAreaData> FactionAreaMap;

	// all setup functions for the to-be-created spaceship
	void SetupGeometry( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const;
	void SetupMeshArea( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const;
	void SetupSpriteSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const;
	void SetupSpotlightSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const;
	void SetupPlaneSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const;
	void SetupChildren( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::RaceData* raceData ) const;
	void SetupBoosters( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::RaceData* raceData ) const;
	void SetupHullDecals( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData ) const;

	// helper functions
	void ModifyTextureResPath( std::string& resPath, const char* name, const EveSOFDataMgr::FactionData* factionData ) const;
	void FillMeshAreaVector( const std::vector<EveSOFDataMgr::HullAreas>* hullAreas, const FactionAreaMap* factionAreas, const EveSOFDataMgr::FactionData* factionData, Tr2MeshAreaVector* meshAreaVector ) const;

	// all the source data
	PEveSOFDataMgr m_dataMgr;

	// shared
	Tr2EffectPtr m_spriteSetEffect, m_spriteSetEffectSkinned;
	Tr2EffectPtr m_shadowEffect, m_shadowEffectSkinned;
};

TYPEDEF_BLUECLASS( EveSOF );

#endif // EveSOF_H