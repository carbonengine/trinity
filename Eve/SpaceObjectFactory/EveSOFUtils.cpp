////////////////////////////////////////////////////////////
//
//    Created:   May 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveSOFUtils.h"

#include "Utilities/StringUtils.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members and do all initial analyzing
// --------------------------------------------------------------------------------
EveSOFUtilsParameterName::EveSOFUtilsParameterName( const std::vector<std::string>& prefixes, const char* parameterName ) : m_fullname( parameterName ), m_shortname( parameterName ), m_materialIdx( -1 )
{
	// try to find the material prefix and with that indentify the index
	for( size_t i = 0; i < prefixes.size(); ++i )
	{
		if( StringStartsWithI( m_shortname.c_str(), prefixes[i].c_str() ) )
		{
			if(i < prefixes.size() )
			{
				m_materialIdx = int32_t(i);
				StringRemove( m_shortname, prefixes[i].c_str() );
				return;
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Is this a valid parameter name?
// --------------------------------------------------------------------------------
bool EveSOFUtilsParameterName::IsMaterialIdxValid() const
{
	return m_materialIdx != -1;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the material index
// --------------------------------------------------------------------------------
int32_t EveSOFUtilsParameterName::GetMaterialIdx() const
{
	return m_materialIdx;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the parameter name without the material prefix
// --------------------------------------------------------------------------------
const char* EveSOFUtilsParameterName::GetShortName() const
{
	return m_shortname.c_str();
}

// --------------------------------------------------------------------------------
const char* EveSOFUtilsParameterName::GetFullName() const
{
	return m_fullname.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Change parameter with a new material index
// --------------------------------------------------------------------------------
void EveSOFUtilsParameterName::ChangeMaterialIdx( const EveSOFDataMgr::GenericData* genericData, int32_t idx )
{
	if( IsMaterialIdxValid() )
	{
		// insert the prefix based on the index
		m_fullname = genericData->materialPrefixes[idx] + m_shortname;
		m_materialIdx = idx;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Fill structs from other structs
// --------------------------------------------------------------------------------
void EveSOFUtils::GeneratePatternProjectionData( EveSOFDataMgr::PatternProjectionData* ppd, const EveSOFDataPatternTransform* patternTransform )
{
	if( patternTransform )
	{
		ppd->enabled = true;
		ppd->position = patternTransform->m_position;
		ppd->scaling = patternTransform->m_scaling;
		ppd->rotation = patternTransform->m_rotation;
		ppd->isMirrored = patternTransform->m_isMirrored;
	}
	else
	{
		ppd->enabled = false;
		ppd->position = Vector3( 0.f, 0.f, 0.f );
		ppd->scaling = Vector3( 1.f, 1.f, 1.f );
		ppd->rotation = Quaternion( 0.f, 0.f, 0.f, 1.f );
		ppd->isMirrored = false;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Fill structs from other structs
// --------------------------------------------------------------------------------
void EveSOFUtils::GeneratePatternLayerData( EveSOFDataMgr::PatternLayerData* pld, const EveSOFDataPatternLayer* patternLayer )
{
	if( patternLayer )
	{
		// texture name
		pld->textureName = patternLayer->m_textureName;
		// texture res file
		pld->textureResFilePath = patternLayer->m_textureResFilePath;
		// projection types, translate to AL enums right here
		pld->projectionAddressModeU = GetTextureAddressMode( patternLayer->m_projectionTypeU );
		pld->projectionAddressModeV = GetTextureAddressMode( patternLayer->m_projectionTypeV );
		// material source id can be directly transltaed from enum
		pld->materialSourceID = (uint8_t)patternLayer->m_materialSource;
		// material targets are bools, but need to be stored as floats (for shader)
		pld->materialTargets = Vector4( patternLayer->m_isTargetMtl1 ? 1.f : 0.f, patternLayer->m_isTargetMtl2 ? 1.f : 0.f, patternLayer->m_isTargetMtl3 ? 1.f : 0.f, patternLayer->m_isTargetMtl4 ? 1.f : 0.f );
	}
	else
	{
		// defaults
		pld->textureName = BlueSharedString( "" );
		pld->textureResFilePath = "";
		pld->projectionAddressModeU = Tr2RenderContextEnum::TA_WRAP;
		pld->projectionAddressModeV = Tr2RenderContextEnum::TA_WRAP;
		pld->materialSourceID = 0;
		pld->materialTargets = Vector4( 0.f, 0.f, 0.f, 0.f );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Translate  SOF pattern projection enum into AL redner context enum
// --------------------------------------------------------------------------------
Tr2RenderContextEnum::TextureAddressMode EveSOFUtils::GetTextureAddressMode( EveSOFDataPatternLayer::ProjectionType projectionType )
{
	switch( projectionType )
	{
	case EveSOFDataPatternLayer::PROJECTION_BORDER:
		return Tr2RenderContextEnum::TA_BORDER;
	case EveSOFDataPatternLayer::PROJECTION_CLAMP:
		return Tr2RenderContextEnum::TA_CLAMP;
	default:
		return Tr2RenderContextEnum::TA_WRAP;
	}
}

// --------------------------------------------------------------------------------
const Vector4* EveSOFUtils::SearchForParameterData( const EveSOFDataMgr* dataMgr, const char* materialName, const EveSOFUtilsParameterName* parameterName )
{
	const EveSOFDataMgr::MaterialData* materialData = dataMgr->GetMaterialData( materialName );
	if( materialData )
	{
		BlueSharedString pn( parameterName->GetShortName() );
		auto parameterIt = materialData->parameters.find( pn );
		if( parameterIt != materialData->parameters.end() )
		{
			return &parameterIt->second;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
const Vector4* EveSOFUtils::SearchForParameterData( const EveSOFDataMgr* dataMgr, const Color* colorSet, const EveSOFDataMgr::AreaMaterialData* areaMaterialData, EveSOFDataArea::AreaType areaType, const EveSOFUtilsParameterName* parameterName )
{
	// what kind of parameter is it?
	if( parameterName->IsMaterialIdxValid() )
	{
		auto k = std::make_pair( areaType, parameterName->GetMaterialIdx() );
		auto finder = areaMaterialData->materialNames.find( k );
		if( finder != areaMaterialData->materialNames.end() )
		{
			return SearchForParameterData( dataMgr, finder->second.c_str(), parameterName );
		}
	}
	else
	{
		auto k = std::make_pair( areaType, parameterName->GetFullName() );
		auto finder = areaMaterialData->glowColor.find( k );
		if( finder != areaMaterialData->glowColor.end() )
		{
			EveSOFDataFactionColorSet::ColorType glowColorType = finder->second;
			return (const Vector4*)&colorSet[glowColorType];
		}
	}

	// if we can't find it, always try the PRIMARY
	if( areaType != EveSOFDataArea::TYPE_PRIMARY )
	{
		return SearchForParameterData( dataMgr, colorSet, areaMaterialData, EveSOFDataArea::TYPE_PRIMARY, parameterName );
	}

	return nullptr;
}


