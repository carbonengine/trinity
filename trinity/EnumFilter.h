// Copyright © 2026 CCP ehf.

#pragma once

#include <cstdint>


/**
 * @brief A filter for enum values stored as bits.
 * @tparam Enum The enum type to filter.
 * @tparam StorageType The underlying storage type for the filter bits. Needs to be large enough to hold all enum values. Defaults to uint8_t (up to 8 enum values).
 */
template <typename Enum, typename StorageType = uint8_t>
class EnumFilter
{
public:
	EnumFilter() = default;
	EnumFilter( const EnumFilter& other ) = default;
	EnumFilter( Enum value ) : m_filter( StorageType( StorageType( 1 ) << StorageType( value ) ) )
	{
		CCP_ASSERT_M( uint64_t( value ) < 8 * sizeof( StorageType ), "Enum value out of range for the chosen storage type" );
	}

	EnumFilter& operator|=( Enum value )
	{
		CCP_ASSERT_M( uint64_t( value ) < 8 * sizeof( StorageType ), "Enum value out of range for the chosen storage type" );
		m_filter |= StorageType( StorageType( 1 ) << StorageType( value ) );
		return *this;
	}

	EnumFilter operator|( const EnumFilter& other ) const
	{
		return EnumFilter( m_filter | other.m_filter );
	}

	EnumFilter operator|( Enum value ) const
	{
		EnumFilter result( *this );
		result |= value;
		return result;
	}

	bool HasBit( Enum value ) const
	{
		CCP_ASSERT_M( uint64_t( value ) < 8 * sizeof( StorageType ), "Enum value out of range for the chosen storage type" );
		return ( m_filter & StorageType( StorageType( 1 ) << StorageType( value ) ) ) != 0;
	}

	/// @brief Creates an EnumFilter with all bits set.
	static EnumFilter AllBits()
	{
		EnumFilter filter;
		filter.m_filter = ~StorageType( 0 );
		return filter;
	}

	// This variable should be private, but we need to access it for MAP_ATTRIBUTE macros
	StorageType m_filter = 0;
};
