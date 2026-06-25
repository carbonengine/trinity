// Copyright © 2016 CCP ehf.

#pragma once
#ifndef Tr2PrimitiveText_H
#define Tr2PrimitiveText_H

#include "TriDebugTextRenderer.h"

BLUE_CLASS( Tr2PrimitiveText ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PrimitiveText( IRoot* lockobj = NULL );
	~Tr2PrimitiveText();

	void Render();

private:
	std::string m_text;
	Color m_color;
	Vector3 m_position;
	TriDebugFont m_font;
	uint32_t m_style;
	bool m_display;
};

TYPEDEF_BLUECLASS( Tr2PrimitiveText );

#endif