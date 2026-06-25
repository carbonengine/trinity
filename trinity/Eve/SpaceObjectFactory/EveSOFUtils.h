// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveSOFUtils_H
#define EveSOFUtils_H

#include "EveSOFDataMgr.h"

// --------------------------------------------------------------------------------
// Description:
//   This class holds a material parameter name and has helper functions to fiddle
//   with it. Mainly identity the material index and change it
// SeeAlso:
//   EveSOF
// --------------------------------------------------------------------------------
class EveSOFUtilsParameterName
{
public:
	EveSOFUtilsParameterName( const std::vector<std::string>& prefixes, const char* parameterName );

	// queries
	bool IsMaterialIdxValid() const;
	int32_t GetMaterialIdx() const;
	const char* GetFullName() const;
	const char* GetShortName() const;

	// substitute
	void ChangeMaterialIdx( const EveSOFDataMgr::GenericData* genericData, int32_t idx );

private:
	// direct copy of the original name
	std::string m_fullname;
	// name without the material prefix
	std::string m_shortname;
	// original material index
	int32_t m_materialIdx;
};

// --------------------------------------------------------------------------------
// Description:
//   This class holds various static helper functions used in various places, but
//   stored here to save some space
// SeeAlso:
//   EveSOF
// --------------------------------------------------------------------------------
class EveSOFUtils
{
public:
	static void GeneratePatternProjectionData( EveSOFDataMgr::PatternProjectionData* ppd, const EveSOFDataPatternTransform* patternTransform );
	static void GeneratePatternLayerData( EveSOFDataMgr::PatternLayerData* pld, const EveSOFDataPatternLayer* patternLayer, const EveSOFDataPatternLayerProperties* patternProperties );
	static Tr2RenderContextEnum::TextureAddressMode GetTextureAddressMode( EveSOFDataPatternLayer::ProjectionType projectionType );
	static const Vector4* SearchForParameterData( const EveSOFDataMgr* dataMgr, const char* materialName, const EveSOFUtilsParameterName* parameterName );
	static const Vector4* SearchForParameterData( const EveSOFDataMgr* dataMgr, const Color* colorSet, const EveSOFDataMgr::AreaMaterialData* areaMaterialData, EveSOFDataArea::AreaType areaType, const EveSOFUtilsParameterName* parameterName );
	static const Vector4 CreateMaterialApplicationVector( bool material1, bool material2, bool material3, bool material4 );
};



#endif // EveSOFUtils_H