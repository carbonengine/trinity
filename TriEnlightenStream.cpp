#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "TriEnlightenStream.h"

GeoMemoryStream::GeoMemoryStream( )
{
	m_size = 0; 
	m_position = 0;
	m_buffer = NULL; 
	m_needsRelease = true;
}

GeoMemoryStream::GeoMemoryStream( size_t size, uint8_t* outpuffer )
{
	m_size = size; 
	m_position = 0;
	m_buffer = outpuffer; 
	m_needsRelease = false;
}

GeoMemoryStream::~GeoMemoryStream()
{
	if( m_needsRelease )
	{
		free( m_buffer );
	}
}

size_t GeoMemoryStream::Write(const void* pData, size_t size, Geo::s32 count)
{
	m_needsRelease = true;
	auto buffer = realloc(m_buffer, m_size+size*count);
	if( buffer )
	{
		m_buffer = static_cast<uint8_t*>( buffer );
		memcpy_s((uint8_t*)m_buffer+m_position, size*count, (uint8_t*)pData, size*count );
		m_size += size*count;
		m_position += size*count;
		return (size_t)count;
	}
	else
	{
		free( m_buffer );
		m_buffer = nullptr;
		return (size_t)0;
	}
}

size_t GeoMemoryStream::Read(void* pData, size_t size, Geo::s32 count)
{
	size_t maxReadSize = m_size - m_position;
	if( size <= maxReadSize )
	{
		memcpy_s((uint8_t*)pData, size*count, (uint8_t*)m_buffer+m_position, size*count );
		m_position += size*count;
		return (size_t)count;
	} else {
		return (size_t)0;
	}
}

size_t GeoMemoryStream::GetPosition() const 
{ 
	return m_position; 
}

bool GeoMemoryStream::SetPosition(size_t pos) 
{ 
	m_position = pos; 
	if( m_position > m_size ) 
	{ 
		return false; 
	} 
	else 
	{ 
		return true; 
	}
}

bool GeoMemoryStream::IsEof() const 
{ 
	return m_position >= m_size; 
}

unsigned int TriPackedGeometryData::GetVersion() const
{
	if( m_buffer == NULL )
	{
		return 0;
	}
	return *reinterpret_cast<unsigned int*>( m_buffer );
}

const unsigned int TriPackedGeometryData::s_versionNumber = 20110118;

#endif
