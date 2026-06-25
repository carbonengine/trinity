// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2VertexDefinition.h"


Tr2VertexDefinition::Tr2VertexDefinition()
{
	for( unsigned i = 0; i != OFFSET_COUNT; ++i )
	{
		m_nextOffset[i] = 0;
	}
}

Tr2VertexDefinition::Tr2VertexDefinition( const Tr2VertexDefinition& other )
{
	*this = other;
}

Tr2VertexDefinition::Item::Item() :
	m_usage( Tr2VertexDefinition::POSITION ), m_usageIndex( 0 ), m_dataType( FLOAT32_1 ), m_offset( 0 ), m_stream( 0 ), m_instanceStepRate( 0 )
{
}

bool Tr2VertexDefinition::Item::operator==( const Tr2VertexDefinition::Item& other ) const
{
	return m_usage == other.m_usage &&
		m_usageIndex == other.m_usageIndex &&
		m_dataType == other.m_dataType &&
		m_offset == other.m_offset &&
		m_stream == other.m_stream &&
		m_instanceStepRate == other.m_instanceStepRate;
}

bool Tr2VertexDefinition::operator==( const Tr2VertexDefinition& other ) const
{
	return m_nextOffset[0] == other.m_nextOffset[0] &&
		m_nextOffset[1] == other.m_nextOffset[1] &&
		m_nextOffset[2] == other.m_nextOffset[2] &&
		m_nextOffset[3] == other.m_nextOffset[3] &&
		m_items == other.m_items;
}

Tr2VertexDefinition& Tr2VertexDefinition::operator=( const Tr2VertexDefinition& other )
{
	for( unsigned i = 0; i != OFFSET_COUNT; ++i )
	{
		m_nextOffset[i] = other.m_nextOffset[i];
	}
	m_items = other.m_items;
	return *this;
}

Tr2VertexDefinition& Tr2VertexDefinition::operator=( Tr2VertexDefinition&& other )
{
	for( unsigned i = 0; i != OFFSET_COUNT; ++i )
	{
		m_nextOffset[i] = other.m_nextOffset[i];
	}
	m_items = std::move( other.m_items );
	return *this;
}

Tr2VertexDefinition::Item& Tr2VertexDefinition::Add( Tr2VertexDefinition::DataType type, Tr2VertexDefinition::UsageCode usage, unsigned usageIndex, unsigned stream, unsigned stepRate )
{
	Item item;
	item.m_dataType = type;
	item.m_offset = stream < OFFSET_COUNT ? m_nextOffset[stream] : 0;
	item.m_stream = stream;
	item.m_usage = usage;
	item.m_usageIndex = usageIndex;
	item.m_instanceStepRate = stepRate;
	m_items.push_back( item );
	if( stream < OFFSET_COUNT )
	{
		m_nextOffset[stream] += GetDataTypeSizeInBytes( type );
	}
	return m_items.back();
}

Tr2VertexDefinition::Item* Tr2VertexDefinition::Find( UsageCode usage )
{
	auto it = std::find_if( begin( m_items ), end( m_items ), [=]( const Item& v ) { return v.m_usage == usage; } );

	return it == m_items.end() ? nullptr : &*it;
}

Tr2VertexDefinition::Item* Tr2VertexDefinition::Find( UsageCode usage, unsigned usageIndex )
{
	auto it = std::find_if( begin( m_items ), end( m_items ), [=]( const Item& v ) { return v.m_usage == usage && v.m_usageIndex == usageIndex; } );

	return it == m_items.end() ? nullptr : &*it;
}
