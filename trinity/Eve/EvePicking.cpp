// Copyright © 2026 CCP ehf.

#include "EvePicking.h"

#include "PixelFormat.h"




EvePendingPickingReadback::EvePendingPickingReadback( uint32_t pickedX, uint32_t pickedY ) :
	m_pickedX( pickedX ),
	m_pickedY( pickedY )
{
}

// ------------------------------------------------------------------------------------------------------
void EvePendingPickingReadback::ReleaseResources( TriStorage s )
{
}

// ------------------------------------------------------------------------------------------------------
bool EvePendingPickingReadback::OnPrepareResources()
{
	return true;
}

EvePickingContext::EvePickingContext( IRoot* lockobj ) :
	m_lastPickedX( 0 ),
	m_lastPickedY( 0 ),
	m_lastPickedObject( NULL ),
	m_lastPickedExtraData1( 0 ),
	m_lastPickedExtraData2( 0 )
{
}



void EvePickingContext::UpdateResult( uint32_t x, uint32_t y, IRootPtr object, uint32_t extraData1, uint32_t extraData2 )
{
	m_lastPickedX = x;
	m_lastPickedY = y;
	m_lastPickedObject = object;
	m_lastPickedExtraData1 = extraData1;
	m_lastPickedExtraData2 = extraData2;
}

IRootPtr EvePickingContext::GetObject() const
{
	return m_lastPickedObject;
}

uint32_t EvePickingContext::GetExtraData1() const
{
	return m_lastPickedExtraData1;
}


uint32_t EvePickingContext::GetExtraData2() const
{
	return m_lastPickedExtraData2;
}
