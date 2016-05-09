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
EveSOFUtilsParameterName::EveSOFUtilsParameterName( const EveSOFDataMgr::GenericData* genericData, const char* parameterName ) : m_fullname( parameterName ), m_shortname( parameterName ), m_materialIdx( -1 )
{
	// try to find the material prefix and with that indentify the index
	for( size_t i = 0; i < genericData->materialPrefixes.size(); ++i )
	{
		if( StringStartsWithI( m_shortname.c_str(), genericData->materialPrefixes[i].c_str() ) )
		{
			if(i < genericData->materialPrefixes.size() )
			{
				m_materialIdx = int32_t(i);
				StringRemove( m_shortname, genericData->materialPrefixes[i].c_str() );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Is this a valid parameter name?
// --------------------------------------------------------------------------------
bool EveSOFUtilsParameterName::IsValid() const
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
// Description:
//   Return a changed parameter with a new material index
// --------------------------------------------------------------------------------
std::string EveSOFUtilsParameterName::ChangeMaterialIdx( const EveSOFDataMgr::GenericData* genericData, int32_t idx ) const
{
	// insert the prefix based on the index
	std::string result = m_shortname;
	result.insert( 0, genericData->materialPrefixes[ idx ] );
	return result;
}
