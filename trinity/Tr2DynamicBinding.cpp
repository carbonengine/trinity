////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2DynamicBinding.h"
#include "ITr2DynamicBindingOwner.h"
#include "TriValueBinding.h"
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
			return false;
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
		case Be::SHAREDSTRING:
			value = reinterpret_cast<BlueSharedString*>( entry.second )->c_str();
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

		static const std::regex tokens( "((\\.[a-zA-Z_][a-zA-Z_0-9]*)|(\\[-?[0-9]+\\])|(\\[\"[^\"]*\"\\])).*" );
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
		start += match[1].length();

		while( object && start != finish )
		{
			if( !std::regex_match( start, finish, match, tokens ) )
			{
				return nullptr;
			}
			if( match[2].length() )
			{
				auto attrName = match[2].str().substr( 1 );
				object = GetIRootAttribute( object, attrName );
			}
			else if( match[3].length() )
			{
				auto index = std::atoi( match[3].str().c_str() + 1 );
				object = GetListElement( object, ssize_t( index ) );
			}
			else if( match[4].length() )
			{
				auto name = match[4].str();
				name = name.substr( 2, name.length() - 4 );
				object = GetListElement( object, name );
			}
			else
			{
				object = nullptr;
			}
			start += match[1].length();
		}
		return object;
	}
}


Tr2DynamicBinding::Tr2DynamicBinding( IRoot* lockobj ) :
	m_destination( nullptr ),
	m_source( nullptr ),
	m_owner( nullptr ),
	m_scale( 1.0f ),
	m_bindingDelay( 0L ),
	m_bindingTime( 0 )
{
	m_binding = nullptr;
	BeOS->RegisterForSimTimeRebase( this );
}

Tr2DynamicBinding::~Tr2DynamicBinding()
{
	BeOS->UnregisterForSimTimeRebase( this );
	m_owner = nullptr;
	Unlink();
}

bool Tr2DynamicBinding::OnModified( Be::Var* value )
{
	if( m_owner != nullptr )
	{
		Link();
	}
	else
	{
		Unlink();
	}

	return true;
}

void Tr2DynamicBinding::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	m_bindingTime += ( newTime - oldTime );
}

bool Tr2DynamicBinding::IsSourceValid() const
{
	return m_source != nullptr;
}

bool Tr2DynamicBinding::IsDestinationValid() const
{
	return m_destination != nullptr;
}

void Tr2DynamicBinding::Update( Be::Time time )
{
	if( m_binding != nullptr && m_bindingTime <= time )
	{
		m_binding->CopyValue();
	}
}

void Tr2DynamicBinding::Link()
{
	Unlink();
	if( m_owner == nullptr )
	{
		return;
	}

	auto parameters = m_owner->GetParameterMap();

	m_destination = ResolveReference( m_destinationObjectPath.c_str(), parameters );
	m_source = ResolveReference( m_sourceObjectPath.c_str(), parameters );

	if( IsSourceValid() && IsDestinationValid() )
	{
		m_binding.CreateInstance();
		m_binding->SetDestination( m_destinationObjectAttribute.c_str(), static_cast<IRoot*>( m_destination ) );
		m_binding->SetSource( m_sourceObjectAttribute.c_str(), static_cast<IRoot*>( m_source ) );
		m_binding->SetScale( m_scale );
		m_binding->Initialize();
		if( m_bindingTime == 0 )
		{
			m_bindingTime = BeOS->GetCurrentFrameTime() + TimeFromMS( m_bindingDelay );
		}
	}	
}

void Tr2DynamicBinding::Unlink()
{
	m_binding = nullptr;
	m_source = nullptr;
	m_destination = nullptr;
	m_bindingTime = 0;
}

void Tr2DynamicBinding::SetOwner( ITr2DynamicBindingOwner* owner)
{
	m_owner = owner;
}
