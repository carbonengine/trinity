////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2CairoScriptSourceRes.h"

#include <regex>


namespace
{
	template<class BidirIt, class Traits, class CharT, class UnaryFunction>
	std::basic_string<CharT> regex_replace( BidirIt first, BidirIt last,
		const std::basic_regex<CharT, Traits>& re, UnaryFunction f )
	{
		std::basic_string<CharT> s;

		typename std::match_results<BidirIt>::difference_type
			positionOfLastMatch = 0;
		auto endOfLastMatch = first;

		auto callback = [&]( const std::match_results<BidirIt>& match )
		{
			auto positionOfThisMatch = match.position( 0 );
			auto diff = positionOfThisMatch - positionOfLastMatch;

			auto startOfThisMatch = endOfLastMatch;
			std::advance( startOfThisMatch, diff );

			s.append( endOfLastMatch, startOfThisMatch );
			s.append( f( match ) );

			auto lengthOfMatch = match.length( 0 );

			positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

			endOfLastMatch = startOfThisMatch;
			std::advance( endOfLastMatch, lengthOfMatch );
		};

		std::regex_iterator<BidirIt> begin( first, last, re ), end;
		std::for_each( begin, end, callback );

		s.append( endOfLastMatch, last );

		return s;
	}

	template<class Traits, class CharT, class UnaryFunction>
	std::string regex_replace( const CcpMallocBuffer& s,
		const std::basic_regex<CharT, Traits>& re, UnaryFunction f )
	{
		return regex_replace( s.get(), s.get() + s.size(), re, f );
	}

	static const std::regex s_colorCommands(
		"[+\\-]?[0-9]\\.?[0-9]*([eE][+\\-]?[0-9]+)?[ ]+"
		"("
		"(g )|"
		"("
		"[+\\-]?[0-9]\\.?[0-9]*([eE][+\\-]?[0-9]+)?[ ]+"
		"[+\\-]?[0-9]\\.?[0-9]*([eE][+\\-]?[0-9]+)?[ ]+"
		"("
		"(rgb )|"
		"(([+\\-]?[0-9]\\.?[0-9]*([eE][+\\-]?[0-9]+)?)[ ]+(rgba |add-color-stop))"
		")"
		")"
		")"
	);
}


Tr2CairoScriptSourceRes::Tr2CairoScriptSourceRes( IRoot* )
{
}

bool Tr2CairoScriptSourceRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2CairoScriptSourceRes::GetMemoryUsage()
{
	return m_contents.size();
}

BlueAsyncRes::LoadingResult Tr2CairoScriptSourceRes::DoLoad()
{
	m_contents.clear();
	m_width = 0;
	m_height = 0;

	if( !m_dataStream )
	{
		return LR_FAILED;
	}

	auto size = m_dataStream->GetSize();
	if( size <= 0 )
	{
		return LR_FAILED;
	}

	m_contents.resize( "Tr2CairoScriptSourceRes::m_contents", size_t( size ) );
	if( m_contents.empty() )
	{
		return LR_FAILED;
	}

	if( m_dataStream->Read( m_contents.get(), size ) != size )
	{
		m_contents.clear();
		return LR_FAILED;
	}

	std::regex sizeR( "/width (\\d+) /height (\\d+)" );
	std::match_results<const char*> match;
	if( std::regex_search( m_contents.get(), match, sizeR ) )
	{
		m_width = uint32_t( atoi( match[1].str().c_str() ) );
		m_height = uint32_t( atoi( match[2].str().c_str() ) );
	}

	return LR_SUCCESS;
}

bool Tr2CairoScriptSourceRes::DoPrepare()
{
	return true;
}

uint32_t Tr2CairoScriptSourceRes::GetWidth() const
{
	return m_width;
}

uint32_t Tr2CairoScriptSourceRes::GetHeight() const
{
	return m_height;
}

const CcpMallocBuffer& Tr2CairoScriptSourceRes::GetContents() const
{
	return m_contents;
}

bool Tr2CairoScriptSourceRes::ApplyColor( std::string& dest, const Color& color ) const
{
	if( !IsGood() )
	{
		return false;
	}

	if( m_contents.empty() )
	{
		dest = "";
		return true;
	}

	char buffer[256];
	sprintf_s( buffer, "%.3f %.3f %.3f", color.r, color.g, color.b );

	std::string newRgb = buffer;

	auto replaceColors = [&]( const std::match_results<char*>& match ) -> std::string
	{
		if( match[3].length() )
		{
			return newRgb + " rgb ";
		}
		else if( match[8].length() )
		{
			return newRgb + " rgb ";
		}
		else if( match[9].length() )
		{
			return newRgb + " " + match[9].str();
		}
		return "";
	};

	dest = regex_replace( m_contents, s_colorCommands, replaceColors );
	return true;
}