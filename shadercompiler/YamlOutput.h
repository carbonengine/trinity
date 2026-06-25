// Copyright © 2023 CCP ehf.

#pragma once

#include "InlineString.h"


class YamlOutput
{
public:
	explicit YamlOutput( bool enabled = true ) :
		m_expected( ROOT ),
		m_enabled( enabled )
	{
	}

	bool enabled() const
	{
		return m_enabled;
	}

	YamlOutput& dict()
	{
		if( !m_enabled )
		{
			return *this;
		}
		switch( m_expected )
		{
		case VALUE:
			m_os << std::endl;
			break;
		case ELEMENT:
			indent() << "-" << std::endl;
			break;
		default:
			break;
		}
		m_modeStack.push_back( DICT );
		m_indent = std::string( m_modeStack.size() * 2, ' ' );
		m_expected = KEY;
		return *this;
	}
	YamlOutput& list()
	{
		if( !m_enabled )
		{
			return *this;
		}
		switch( m_expected )
		{
		case VALUE:
			m_os << std::endl;
			break;
		case ELEMENT:
			indent() << "-" << std::endl;
			break;
		default:
			break;
		}
		m_modeStack.push_back( LIST );
		m_indent = std::string( m_modeStack.size() * 2, ' ' );
		m_expected = ELEMENT;
		return *this;
	}

	YamlOutput& end()
	{
		if( !m_enabled )
		{
			return *this;
		}
		m_modeStack.pop_back();
		if( !m_modeStack.empty() )
		{
			if( m_modeStack.back() == DICT )
			{
				m_expected = KEY;
			}
			else
			{
				m_expected = ELEMENT;
			}
		}
		m_indent = std::string( m_modeStack.size() * 2, ' ' );
		return *this;
	}

	template <typename T>
	YamlOutput& literal( const T& value )
	{
		if( !m_enabled )
		{
			return *this;
		}
		return _literal( value );
	}

	YamlOutput& literal( const char* str )
	{
		if( !m_enabled )
		{
			return *this;
		}
		if( strchr( str, '\n' ) )
		{
			return multiline( str );
		}
		else if( strchr( str, ':' ) || strchr( str, '-' ) || strchr( str, '#' ) )
		{
			return _literal( quote( str ) );
		}

		return _literal( str );
	}

	YamlOutput& literal( char* str )
	{
		if( !m_enabled )
		{
			return *this;
		}
		if( strchr( str, '\n' ) )
		{
			return multiline( str );
		}
		else if( strchr( str, ':' ) )
		{
			return _literal( quote( str ) );
		}

		return _literal( str );
	}

	YamlOutput& literal( const std::string& str )
	{
		if( !m_enabled )
		{
			return *this;
		}
		if( str.find( '\n' ) != std::string::npos )
		{
			return multiline( str );
		}
		else if( str.find( ':' ) != std::string::npos )
		{
			return _literal( quote( str ) );
		}
		return _literal( str );
	}

	YamlOutput& literal( bool value )
	{
		return literal( value ? "true" : "false" );
	}

	YamlOutput& literal( unsigned char value )
	{
		return literal( int( value ) );
	}

	YamlOutput& literal( const InlineString& str )
	{
		if( !m_enabled )
		{
			return *this;
		}
		return literal( ToString( str ) );
	}

	YamlOutput& literal( TextureType type )
	{
		switch( type )
		{
		case TEX_TYPE_1D:
			literal( "1D texture" );
			return *this;
		case TEX_TYPE_2D:
			literal( "2D texture" );
			return *this;
		case TEX_TYPE_3D:
			literal( "3D texture" );
			return *this;
		case TEX_TYPE_CUBE:
			literal( "CUBE texture" );
			return *this;
		case TEX_TYPE_TYPELESS:
			literal( "typeless texture" );
			return *this;
		case TEX_TYPE_BUFFER:
			literal( "buffer" );
			return *this;
		case TEX_TYPE_STRUCTURED_BUFFER:
			literal( "structured buffer" );
			return *this;
		case TEX_TYPE_TBUFFER:
			literal( "TBuffer" );
			return *this;
		case TEX_TYPE_BYTEADDRESS_BUFFER:
			literal( "byte address buffer" );
			return *this;

		case TEX_TYPE_UAV_RWTYPED:
			literal( "UAV typed" );
			return *this;
		case TEX_TYPE_UAV_RWSTRUCTURED:
			literal( "UAV structured" );
			return *this;
		case TEX_TYPE_UAV_RWBYTEADDRESS:
			literal( "UAV RW byte address" );
			return *this;
		case TEX_TYPE_UAV_APPEND_STRUCTURED:
			literal( "UAV append structured" );
			return *this;
		case TEX_TYPE_UAV_CONSUME_STRUCTURED:
			literal( "UAV consume structured" );
			return *this;
		case TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER:
			literal( "UAV structured with counter" );
			return *this;
		default:
			literal( "other" );
			return *this;
		}
	}

	std::string yaml() const
	{
		if( !m_enabled )
		{
			return "";
		}
		return m_os.str();
	}

private:
	static std::string quote( const std::string& str )
	{
		return '\"' + str + '\"';
	}

	template <typename T>
	YamlOutput& _literal( const T& value )
	{
		switch( m_expected )
		{
		case KEY:
			indent() << value << ": ";
			m_expected = VALUE;
			break;
		case VALUE:
			m_os << value << std::endl;
			m_expected = KEY;
			break;
		case ELEMENT:
			indent() << "-" << std::endl;
			indent() << "  " << value << std::endl;
			break;
		default:
			break;
		}
		return *this;
	}

	YamlOutput& multiline( const std::string& str )
	{
		std::string rawIndent( m_indent.length(), ' ' );
		auto p = str.find( '\n' );
		switch( m_expected )
		{
		case KEY:
			indent() << "? |" << std::endl;
			m_os << rawIndent << "  " << str.substr( 0, p );
			break;
		case VALUE:
			m_os << "|" << std::endl;
			m_os << rawIndent << "  " << str.substr( 0, p ) << std::endl;
			break;
		case ELEMENT:
			indent() << "- |" << std::endl;
			m_os << rawIndent << "  " << str.substr( 0, p ) << std::endl;
			break;
		default:
			break;
		}
		while( p != std::string::npos )
		{
			auto e = str.find( '\n', p + 1 );
			m_os << rawIndent << "  " << str.substr( p + 1, e - p - 1 ) << std::endl;
			p = e;
		}
		switch( m_expected )
		{
		case KEY:
			indent() << ": ";
			m_expected = VALUE;
			break;
		case VALUE:
			m_expected = KEY;
			break;
		case ELEMENT:
			break;
		default:
			break;
		}

		return *this;
	}

	std::ostream& indent()
	{
		m_os << m_indent;
		return m_os;
	}

	std::stringstream m_os;
	std::string m_indent;
	enum Expected
	{
		KEY,
		VALUE,
		ELEMENT,
		ROOT,
	} m_expected;
	enum Mode
	{
		NONE,
		DICT,
		LIST,
	};
	std::vector<Mode> m_modeStack;
	bool m_enabled;
};



extern bool g_generateListing;

class YamlListing : public YamlOutput
{
public:
	explicit YamlListing( bool enabled = true ) :
		YamlOutput( g_generateListing && enabled )
	{
	}

	~YamlListing()
	{
		if( !enabled() )
		{
			return;
		}
		extern std::string g_listing;
		extern std::mutex g_listingCS;

		std::lock_guard scope( g_listingCS );
		g_listing += yaml();
	}
};
