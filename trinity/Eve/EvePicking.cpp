// Copyright © 2026 CCP ehf.

#include "EvePicking.h"

#include "PixelFormat.h"




EvePendingPickingReadback::EvePendingPickingReadback( uint32_t pickedX, uint32_t pickedY ) :
	m_pickedX( pickedX ),
	m_pickedY( pickedY ),
	m_debugPickBuffer( NULL, ImageIO::PIXEL_FORMAT_R32G32B32A32_FLOAT ),
	m_debugPickData( nullptr ),
	m_mainPickBuffer( NULL, ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM ),
	m_mainPickData( nullptr )
{
}

// ------------------------------------------------------------------------------------------------------
void EvePendingPickingReadback::ReleaseResources( TriStorage s )
{
	m_debugPickData = nullptr;
	m_mainPickData = nullptr;
}

// ------------------------------------------------------------------------------------------------------
bool EvePendingPickingReadback::OnPrepareResources()
{
	return true;
}


void EvePendingPickingReadback::MapDebug( bool synchronize, Tr2RenderContext& renderContext )
{
	uint32_t pitch;
	m_debugPickBuffer.MapForReading( synchronize, m_debugPickData, pitch, renderContext );
}


void EvePendingPickingReadback::MapMain( bool synchronize, Tr2RenderContext& renderContext )
{
	uint32_t pitch;
	m_mainPickBuffer.MapForReading( synchronize, m_mainPickData, pitch, renderContext );
}


void EvePendingPickingReadback::Unmap( Tr2RenderContext& renderContext )
{
	if( m_debugPickData )
	{
		m_debugPickBuffer.UnmapForReading( renderContext );
	}
	if( m_mainPickData )
	{
		m_mainPickBuffer.UnmapForReading( renderContext );
	}
}

EvePickingContext::EvePickingContext( IRoot* lockobj ) :
	m_lastPickedX( 0 ),
	m_lastPickedY( 0 ),
	m_lastPickedObject( NULL ),
	m_lastPickedArea( 0 )
{
}



void EvePickingContext::UpdateResult( uint32_t x, uint32_t y, IRootPtr object, uint32_t area )
{
	m_lastPickedX = x;
	m_lastPickedY = y;
	m_lastPickedObject = object;
	m_lastPickedArea = area;
}

IRoot* EvePickingContext::GetObject()
{
	return m_lastPickedObject;
}

uint32_t EvePickingContext::GetArea()
{
	return m_lastPickedArea;
}
