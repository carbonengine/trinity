#pragma once
#ifndef TriEnlightenStream_H
#define TriEnlightenStream_H

// Load and Save packed geometry into a buffer
class GeoMemoryStream : public Geo::IGeoStream, Geo::NonCopyable
{
public:
	size_t				m_size;
	uint8_t*				m_buffer;	
	
	GeoMemoryStream( );
	GeoMemoryStream( size_t size, uint8_t* outpuffer );
	~GeoMemoryStream();

	size_t	Write(const void* pData, size_t size, Geo::s32 count);
	size_t	Read(void* pData, size_t size, Geo::s32 count);
	size_t	GetPosition() const;
	bool	SetPosition(size_t pos);
	bool	IsEof() const;
	bool	IsOk(void) const { return true; }
private:
	size_t				m_position;
	bool				m_needsRelease;
};


// Extended data for packed geometry
struct TriPackedGeometryData
{
	size_t				m_bufferSize;
	uint8_t*				m_buffer;
	float				m_pixelSize;
	unsigned int		m_geometryBuildHash1;
	unsigned int		m_geometryBuildHash2;

	unsigned int GetVersion() const;

	// Version number for Enlighten geometry stored in files
	static const unsigned int s_versionNumber;
};

#endif //TriEnlightenStream_H