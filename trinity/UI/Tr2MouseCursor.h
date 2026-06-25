// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2MouseCursor_H
#define Tr2MouseCursor_H

BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( Tr2MouseCursor );
BLUE_DECLARE_VECTOR( Tr2MouseCursor );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MouseCursor encapsulates mouse cursor image management. There is a considerable
//   difference in how mouse cursors are handled in DX9 and DX11/GL.
// --------------------------------------------------------------------------------------
class Tr2MouseCursor : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2MouseCursor( IRoot* lockobj = nullptr );
	~Tr2MouseCursor();

	void py__init__( Tr2HostBitmap* bitmap, unsigned hotspotX, unsigned hotspotY, const std::vector<Tr2HostBitmap*>& representations );

	bool IsValid() const;
	bool Create( Tr2HostBitmap* bitmap, int hotspotX, int hotspotY, const std::vector<Tr2HostBitmap*>& representations );
	void Apply();

#if _WIN32
	HCURSOR m_cursor;
#elif __APPLE__
	struct Representation
	{
		uint32_t width;
		uint32_t height;
		std::unique_ptr<char[]> pixelData;
	};

	bool Create_MacOS( const std::vector<Representation>& representations, uint32_t width, uint32_t height, int hotspotX, int hotspotY );
	void Apply_MacOS();

	id m_cursor; // NSCursor
#endif
};

TYPEDEF_BLUECLASS( Tr2MouseCursor );

#endif // Tr2MouseCursor_H
