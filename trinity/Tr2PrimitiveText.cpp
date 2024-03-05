////////////////////////////////////////////////////////////
//
//    Created:   2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "Tr2PrimitiveText.h"
#include "Tr2Renderer.h"

Tr2PrimitiveText::Tr2PrimitiveText( IRoot* lockobj ) :
	m_font( TRI_DBG_FONT_SMALL ),
	m_style( TRI_DFS_CENTER | TRI_DFS_VCENTER ),
	m_position( 0, 0, 0 ),
	m_color( 1, 1, 1, 1 ),
	m_display( true )
{
}

Tr2PrimitiveText::~Tr2PrimitiveText()
{
}

void Tr2PrimitiveText::Render()
{
	if( m_display )
	{
		Tr2Renderer::Printf( m_font, m_style, m_position, (Vector4)m_color, m_text.c_str() );
	}
}
