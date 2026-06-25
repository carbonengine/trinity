// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2BindingPoint.h"


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

bool IsAlpha( char ch )
{
	return ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' );
}

bool IsNum( char ch )
{
	return ch >= '0' && ch <= '9';
}

const char* MatchRoot( const char* path )
{
	if( !IsAlpha( *path ) && *path != '_' )
	{
		return path;
	}
	++path;
	while( true )
	{
		if( !IsAlpha( *path ) && !IsNum( *path ) && *path != '_' )
		{
			return path;
		}
		++path;
	}
}

const char* MatchProperty( const char* path )
{
	if( *path != '.' )
	{
		return path;
	}
	++path;
	return MatchRoot( path );
}

const char* MatchNumericIndex( const char* path )
{
	auto start = path;
	if( *path != '[' )
	{
		return path;
	}
	++path;
	if( *path == '-' )
	{
		++path;
	}
	while( true )
	{
		if( !IsNum( *path ) )
		{
			if( *path == ']' )
			{
				++path;
				return path;
			}
			else
			{
				return start;
			}
		}
		++path;
	}
}

const char* MatchNameIndex( const char* path )
{
	auto start = path;
	if( *path != '[' )
	{
		return path;
	}
	++path;
	if( *path != '\"' )
	{
		return start;
	}
	++path;
	while( true )
	{
		if( *path == 0 )
		{
			return start;
		}
		if( *path == '\"' )
		{
			if( path[1] == ']' )
			{
				return path + 2;
			}
			else
			{
				return start;
			}
		}
		++path;
	}
}

IRoot* ResolveReference( const std::string& reference, const std::vector<std::pair<std::string, IRoot*>>& roots )
{
	if( reference.empty() )
	{
		return nullptr;
	}

	auto rootEnd = MatchRoot( reference.c_str() );
	if( rootEnd == reference.c_str() )
	{
		return nullptr;
	}
	auto rootLength = rootEnd - reference.c_str();

	auto found = std::find_if( begin( roots ), end( roots ), [&]( const auto& x ) {
		return x.first.length() == rootLength && strncmp( x.first.c_str(), reference.c_str(), rootLength ) == 0;
	} );
	if( found == roots.end() )
	{
		return nullptr;
	}
	auto object = found->second;

	const char* start = rootEnd;
	while( *start )
	{
		const char* end;
		end = MatchProperty( start );
		if( end != start )
		{
			auto attrName = std::string( start + 1, end );
			object = GetIRootAttribute( object, attrName );
			start = end;
			continue;
		}
		end = MatchNumericIndex( start );
		if( end != start )
		{
			auto index = std::atoi( start + 1 );
			object = GetListElement( object, ssize_t( index ) );
			start = end;
			continue;
		}
		end = MatchNameIndex( start );
		if( end != start )
		{
			auto name = std::string( start + 2, end - 2 );
			object = GetListElement( object, name );
			start = end;
			continue;
		}
		return nullptr;
	}
	return object;
}
}




Tr2BindingPoint::Tr2BindingPoint() :
	m_entry( nullptr ),
	m_destination( nullptr ),
	m_entryOffset( -1 ),
	m_arraySize( 0 )
{
}

void Tr2BindingPoint::Link( const std::vector<std::pair<std::string, IRoot*>>& roots )
{
	Unlink();
	if( m_path.empty() )
	{
		SetDestination( m_object, m_attribute );
	}
	else
	{
		m_resolvedObject = ResolveReference( m_path, roots );
		SetDestination( m_resolvedObject, m_attribute );
	}
}

void Tr2BindingPoint::Unlink()
{
	m_entry = nullptr;
	m_destination = nullptr;
	m_resolvedObject = nullptr;
	m_notifyPtr = nullptr;
	m_entryOffset = -1;
	m_arraySize = 0;
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
	case Be::FLOATARRAY:
		if( m_entryOffset == -1 )
		{
			for( int32_t i = 0; i < m_arraySize; ++i )
			{
				reinterpret_cast<float*>( m_destination )[i] = value;
			}
		}
		else
		{
			reinterpret_cast<float*>( m_destination )[m_entryOffset] = value;
		}
		break;
	case Be::DOUBLEARRAY:
		if( m_entryOffset == -1 )
		{
			for( int32_t i = 0; i < m_arraySize; ++i )
			{
				reinterpret_cast<double*>( m_destination )[i] = value;
			}
		}
		else
		{
			reinterpret_cast<double*>( m_destination )[m_entryOffset] = value;
		}
		break;
	default:
		return;
	}
	if( !!m_notifyPtr )
	{
		m_notifyPtr->OnModified( m_destination );
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
	case Be::FLOATARRAY:
		value = reinterpret_cast<float*>( m_destination )[std::max( m_entryOffset, 0 )];
		break;
	case Be::DOUBLEARRAY:
		value = float( reinterpret_cast<double*>( m_destination )[std::max( m_entryOffset, 0 )] );
		break;
	default:
		return false;
	}
	return true;
}

bool Tr2BindingPoint::SetDestination( IRoot* object, const std::string& attribute )
{
	m_entry = nullptr;
	m_destination = nullptr;
	m_entryOffset = -1;
	m_notifyPtr = nullptr;
	m_arraySize = 0;

	std::string name;
	int32_t entryOffset = -1;
	int32_t arraySize = 0;
	auto dot = attribute.find( '.' );
	if( dot != std::string::npos )
	{
		name = attribute.substr( 0, dot );
		auto swizzle = attribute.substr( dot + 1 );
		if( swizzle.length() != 1 )
		{
			return false;
		}
		switch( swizzle[0] )
		{
		case 'x':
		case 'r':
			entryOffset = 0;
			break;
		case 'y':
		case 'g':
			entryOffset = 1;
			break;
		case 'z':
		case 'b':
			entryOffset = 2;
			break;
		case 'w':
		case 'a':
			entryOffset = 3;
			break;
		default:
			return false;
		}
	}
	else
	{
		name = attribute;
		entryOffset = -1;
	}

	auto entry = FindEntry( object, name.c_str() );
	if( !entry.second )
	{
		return false;
	}
	switch( entry.first->mType )
	{
	case Be::FLOAT:
	case Be::DOUBLE:
	case Be::BOOL:
		if( entryOffset != -1 )
		{
			return false;
		}
		break;
	case Be::FLOATARRAY:
		arraySize = int32_t( entry.first->GetFloatArraySize() );
		if( entryOffset >= arraySize )
		{
			return false;
		}
		break;
	case Be::DOUBLEARRAY:
		arraySize = int32_t( entry.first->GetDoubleArraySize() );
		if( entryOffset >= arraySize )
		{
			return false;
		}
		break;
	default:
		return false;
	}

	m_entry = entry.first;
	m_destination = entry.second;
	m_entryOffset = entryOffset;
	m_arraySize = arraySize;
	INotifyPtr notifyPtr = BlueCastPtr( object );
	m_notifyPtr = notifyPtr.p;
	return true;
}

IRoot* Tr2BindingPoint::GetBoundObject() const
{
	if( m_resolvedObject )
	{
		return m_resolvedObject;
	}
	return m_object;
}


IRootPtr ResolveObjectPath( const std::string& reference, const std::map<std::string, IRoot*>& roots )
{
	std::vector<std::pair<std::string, IRoot*>> unorderedRoots;
	for( auto it = begin( roots ); it != end( roots ); ++it )
	{
		unorderedRoots.push_back( *it );
	}
	return ResolveReference( reference, unorderedRoots );
}

MAP_FUNCTION_AND_WRAP(
	"ResolveObjectPath",
	ResolveObjectPath,
	"Resolve path to an objects (for controller actions, etc.).\n"
	":param path: path from one of the roots to an object\n"
	":param roots: named \"root\" objects - starting points for a path" );
