////////////////////////////////////////////////////////////
//
//    Created:   March 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "Tr2ExternalParameter.h"
#include <BlueVariable.h>
#include "TriValueBinding.h"

namespace
{

const Be::VarEntry* FindEntry( const char* name, const Be::ClassInfo* type, ssize_t& offs )
{
	offs = 0;
	for( ; type; offs += type->mOffsetToParent, type = type->mParentClassInfo )
	{
		for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; entry++ )
		{
			if( !entry->mGetProperty && strcmp( entry->mName, name ) == 0 )
			{
				return entry;
			}
		}
	}

	return NULL;
}

}

Tr2ExternalParameter::Tr2ExternalParameter( IRoot* lockobj )
	:m_destination( nullptr ),
	m_destinationEntry( nullptr ),
	m_destItemOffset( 0 ),
	m_valid( false ),
	m_useOffset( false )
{
}

Tr2ExternalParameter::~Tr2ExternalParameter()
{
}

bool Tr2ExternalParameter::Initialize()
{
	m_destination = nullptr;
	m_destinationEntry = nullptr;
	m_destItemOffset = 0;
	m_notifyPtr.Unlock();
	m_valid = false;
	m_useOffset = false;

	if( !m_destinationObject || m_destinationAttribute.empty() )
	{
		return true;
	}

	const Be::ClassInfo* dstClassInfo = m_destinationObject->ClassType();

	std::string destAttr = m_destinationAttribute;
	size_t destDot = destAttr.find( '.' );
	if( destDot != std::string::npos )
	{
		std::string destItem = destAttr.substr( destDot + 1 );
		if( destItem == "x" || destItem == "r" )
		{
			m_destItemOffset = 0;
		}
		else if( destItem == "y" || destItem == "g" )
		{
			m_destItemOffset = 1;
		}
		else if( destItem == "z" || destItem == "b" )
		{
			m_destItemOffset = 2;
		}
		else if( destItem == "w" || destItem == "a" )
		{
			m_destItemOffset = 3;
		}
		else
		{
			return true;
		}
		destAttr = destAttr.substr( 0, destDot );
		m_useOffset = true;
	}

	ssize_t dstOffset;
	m_destinationEntry = FindEntry( destAttr.c_str(), dstClassInfo, dstOffset );
	if( !m_destinationEntry )
	{
		m_destItemOffset = 0;
		return true;
	}
	if( m_useOffset )
	{
		if( m_destinationEntry->mType != Be::FLOATARRAY )
		{
			m_destinationEntry = nullptr;
			m_destItemOffset = 0;
			m_useOffset = false;
			return true;
		}
	}
	m_destination = static_cast<Be::Var*>( BLUEMAPMEMBEROFFSET( m_destinationObject.p, m_destinationEntry, dstClassInfo, dstOffset ) );
	m_valid = true;
	m_notifyPtr = BlueCastPtr( m_destinationObject );
	return true;
}

bool Tr2ExternalParameter::OnModified( Be::Var* )
{
	Initialize();
	return true;
}

const char* Tr2ExternalParameter::GetName() const
{
	return m_name.c_str();
}

void Tr2ExternalParameter::SetName( const std::string& name )
{
	m_name = name;
}

void Tr2ExternalParameter::SetDestinationObject( IRoot* destinationObject )
{
	m_destinationObject = destinationObject;
}

void Tr2ExternalParameter::SetDestinationAttribute( const std::string& attributeName )
{
	m_destinationAttribute = attributeName;
}

BlueStdResult Tr2ExternalParameter::SetValue( BlueScriptValue value )
{
#if BLUE_WITH_PYTHON
	if( !m_valid )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "invalid binding" );
	}

	if( m_useOffset )
	{
		float element;
		if( !BlueExtractFloat( value, element ) )
		{
			return BlueStdResult( BLUE_STD_RESULT_TYPE_ERROR, "float value expected" );
		}
		( &m_destination->mFloat )[m_destItemOffset] = element;
	}
	else
	{
		if( !BlueConvertValueFromPython( m_destinationEntry, m_destination, value ) )
		{
			return BlueStdResult( BLUE_STD_RESULT_TYPE_ERROR, "incompatible type" );
		}
	}
	if( m_notifyPtr )
	{
		m_notifyPtr->OnModified( m_destination );
	}
	return BLUE_STD_RESULT_OK;
#else
	return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "not implemented in this language" );
#endif
}

BlueStdResult Tr2ExternalParameter::GetValue( BlueScriptValue& value ) const
{
#if BLUE_WITH_PYTHON
	if( m_valid )
	{
		if( m_useOffset )
		{
			value = Py_BuildValue( "f", ( &m_destination->mFloat )[m_destItemOffset] );
		}
		else
		{
			value = BlueConvertValueToPython( m_destinationEntry, m_destination );
		}
		return BLUE_STD_RESULT_OK;
	}
	else
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "invalid binding" );
	}
#else
	return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "not implemented in this language" );
#endif
}

Be::Var* Tr2ExternalParameter::GetDestination() const
{
	return m_destination;
}

const Be::VarEntry* Tr2ExternalParameter::GetDestinationEntry() const
{
	return m_destinationEntry;
}

bool Tr2ExternalParameter::IsValid() const
{
	return m_valid;
}

TriValueBindingPtr Tr2ExternalParameter::CreateBinding() const
{
	TriValueBindingPtr ptr;
	ptr.CreateInstance();
	ptr->SetDestination( m_destinationAttribute, m_destinationObject );
	return ptr;
}
