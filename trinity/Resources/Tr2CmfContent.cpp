// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2CmfContent.h"

#include <cmf/utils.h>
#include <cmf/compression.h>


void Tr2CmfContents::Initialize( const void* data, size_t size, const char* name, cmf::ValidationOptions options )
{
	auto validated = cmf::ValidateFile( data, size, options );
	if( !validated )
	{
		CCP_LOGERR( "Invalid CMF data %s: %s", name, validated.error.c_str() );
		return;
	}

	auto header = static_cast<const cmf::Header*>( data );
	m_sections.reserve( header->sections.size() );
	for( const auto& section : header->sections )
	{
		m_sections.emplace_back();
		auto& dest = m_sections.back();
		dest.section = section;
		dest.data = std::make_unique<uint8_t[]>( section.compressedSize );
		memcpy( dest.data.get(), (uint8_t*)data + section.offset, section.compressedSize );
	}

	cmf::OffsetsToPointers( *reinterpret_cast<cmf::Data*>( m_sections[0].data.get() ) );
}

Tr2CmfContents::Tr2CmfContents( IBlueStream& stream, const char* filename )
{
	void* data;
	if( !stream.LockData( &data, 0 ) )
	{
		CCP_LOGERR( "Failed to read CMF file %s", filename );
		return;
	}
	auto size = stream.GetSize();
	Initialize( data, size, filename, { true, true } );
}

Tr2CmfContents::Tr2CmfContents( const void* data, size_t size, const char* name )
{
	Initialize( data, size, name, { false, true } );
}

Tr2CmfContents::operator bool() const
{
	return !m_sections.empty();
}

const cmf::Data* Tr2CmfContents::GetData() const
{
	if( m_sections.empty() )
	{
		return nullptr;
	}
	return reinterpret_cast<cmf::Data*>( m_sections[0].data.get() );
}

const void* Tr2CmfContents::GetSection( uint32_t index )
{
	if( index >= m_sections.size() )
	{
		CCP_ASSERT_M( false, "Tr2CmfContents::GetSection: index out of range!" );
		return nullptr;
	}
	auto& section = m_sections[index];
	if( section.section.compression == cmf::SectionCompression::None )
	{
		return m_sections[index].data.get();
	}
	// May happen if the buffer has been unloaded from memory
	if( !section.data )
	{
		return nullptr;
	}

	TRINITY_STATS_ZONE( __FUNCTION__ );

	auto decompressed = std::make_unique<uint8_t[]>( section.section.uncompressedSize );
	cmf::Decompress( decompressed.get(), section.section, section.data.get() );
	std::swap( decompressed, section.data );
	section.section.compression = cmf::SectionCompression::None;
	return section.data.get();
}

const void* Tr2CmfContents::GetViewData( const cmf::BufferView& view )
{
	const auto* base = static_cast<const uint8_t*>( GetSection( view.index ) );
	return base ? base + view.offset : nullptr;
}

void Tr2CmfContents::UnloadGpuBuffers()
{
	for( auto& section : m_sections )
	{
		if( section.section.type == cmf::SectionType::GpuBuffer )
		{
			section.data.reset();
		}
	}
}
