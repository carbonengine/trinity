////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2BindingPoint.h"
#include <regex>


namespace
{
	std::pair<const Be::VarEntry*, Be::Var*> FindEntry( IRoot* object, const char* name )
	{
		if( !object )
		{
			return std::make_pair( nullptr, nullptr );
		}
		auto type = object->ClassType();
		ssize_t offs = 0;
		// Loop over all entries - this double loop covers chaining
		for( ; type; offs += type->mOffsetToParent, type = type->mParentClassInfo )
		{
			for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; entry++ )
			{
				if( !entry->mGetProperty && strcmp( entry->mName, name ) == 0 )
				{
					return std::make_pair( entry, BLUEMAPMEMBEROFFSET( object, entry, type, offs ) );
				}
			}
		}
		return std::make_pair( nullptr, nullptr );
	}

	IRoot* GetIRootAttribute( IRoot* object, const std::string& name )
	{
		auto entry = FindEntry( object, name.c_str() );
		if( !entry.second )
		{
			return nullptr;
		}
		if( entry.first->mSize && ( entry.first->mType == Be::IROOT || entry.first->mType == Be::IROOTPTR ) )
		{
			if( entry.first->mType == Be::IROOTPTR )
			{
				return entry.second->mIRootPtr;
			}
			else
			{
				return reinterpret_cast<IRoot*>( entry.second );
			}
		}
		return nullptr;
	}

	bool GetStringAttribute( IRoot* object, const std::string& name, std::string& value )
	{
		auto entry = FindEntry( object, name.c_str() );
		if( !entry.second )
		{
			return nullptr;
		}
		switch( entry.first->mType )
		{
		case Be::CHARARRAY:
			value = reinterpret_cast<const char*>( entry.second );
			return true;
		case Be::CSTRING:
			value = reinterpret_cast<const char*>( entry.second->mCharPtr );
			return true;
		case Be::STDSTRING:
			value = *reinterpret_cast<const std::string*>( entry.second );
			return true;
		default:
			return false;
		}
	}

	IRoot* GetListElement( IRoot* object, ssize_t index )
	{
		if( IListPtr list = BlueCastPtr( object ) )
		{
			if( index < 0 )
			{
				index += int( list->GetSize() );
			}
			if( index >= 0 && index < list->GetSize() )
			{
				return list->GetAt( index );
			}
		}
		return nullptr;
	}

	IRoot* GetListElement( IRoot* object, const std::string& name )
	{
		if( IListPtr list = BlueCastPtr( object ) )
		{
			auto size = list->GetSize();
			for( ssize_t i = 0; i < size; ++i )
			{
				auto element = list->GetAt( i );
				std::string elementName;
				if( GetStringAttribute( element, "name", elementName ) && elementName == name )
				{
					return element;
				}
			}

		}
		return nullptr;
	}

	IRoot* ResolveReference( const std::string& reference, const std::unordered_map<std::string, IRoot*>& roots )
	{
		if( reference.empty() )
		{
			return nullptr;
		}

		static const std::regex tokens( "(\\.[a-zA-Z_][a-zA-Z_0-9]*)|(\\[-?[0-9]+\\])|(\\[\"[^\"]*\"\\]).*" );
		static const std::regex root( "([a-zA-Z_][a-zA-Z_0-9]*).*" );

		auto start = begin( reference );
		auto finish = end( reference );
		std::smatch match;
		if( !std::regex_match( start, finish, match, root ) )
		{
			return nullptr;
		}
		auto found = roots.find( match[1].str() );
		if( found == roots.end() )
		{
			return nullptr;
		}
		auto object = found->second;
		start += match.length();

		while( object && start != finish )
		{
			if( !std::regex_match( start, finish, match, tokens ) )
			{
				return nullptr;
			}
			if( match[1].length() )
			{
				auto attrName = match[1].str().substr( 1 );
				object = GetIRootAttribute( object, attrName );
			}
			else if( match[2].length() )
			{
				auto index = std::atoi( match[2].str().c_str() + 1 );
				object = GetListElement( object, ssize_t( index ) );
			}
			else if( match[3].length() )
			{
				auto name = match[3].str();
				name = name.substr( 2, name.length() - 4 );
				object = GetListElement( object, name );
			}
			else
			{
				object = nullptr;
			}
			start += match.length();
		}
		return object;
	}
}




Tr2BindingPoint::Tr2BindingPoint()
	:m_entry( nullptr ),
	m_destination( nullptr )
{
}

void Tr2BindingPoint::Link( const std::unordered_map<std::string, IRoot*>& roots )
{
	Unlink();
	if( m_path.empty() )
	{
		SetDestination( m_object, m_attribute.c_str() );
	}
	else
	{
		m_resolvedObject = ResolveReference( m_path, roots );
		SetDestination( m_resolvedObject, m_attribute.c_str() );
	}
}

void Tr2BindingPoint::Unlink()
{
	m_entry = nullptr;
	m_destination = nullptr;
	m_resolvedObject = nullptr;
}

bool Tr2BindingPoint::IsValid() const
{
	return m_destination != nullptr;
}

void Tr2BindingPoint::SetValue( float value ) const
{
	if( !IsValid() )
	{
		return;
	}
	switch( m_entry->mType )
	{
	case Be::FLOAT:
		*reinterpret_cast<float*>( m_destination ) = value;
		break;
	case Be::DOUBLE:
		*reinterpret_cast<double*>( m_destination ) = value;
		break;
	case Be::BOOL:
		*reinterpret_cast<bool*>( m_destination ) = value != 0;
		break;
	default:
		return;
	}
}

bool Tr2BindingPoint::GetValue( float& value ) const
{
	if( !IsValid() )
	{
		return false;
	}
	switch( m_entry->mType )
	{
	case Be::FLOAT:
		value = *reinterpret_cast<float*>( m_destination );
		break;
	case Be::DOUBLE:
		value = float( *reinterpret_cast<double*>( m_destination ) );
		break;
	case Be::BOOL:
		value = float( *reinterpret_cast<bool*>( m_destination ) ? 1.f : 0.f );
		break;
	default:
		return false;
	}
	return true;
}

bool Tr2BindingPoint::SetDestination( IRoot* object, const char* attribute )
{
	auto entry = FindEntry( object, attribute );
	if( !entry.second )
	{
		m_entry = nullptr;
		m_destination = nullptr;
		return false;
	}
	m_entry = entry.first;
	m_destination = entry.second;
	return true;
}
