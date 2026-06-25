// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2FontMeasurer.h"
#include "Tr2AtlasTexture.h"
#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"

BLUE_DEFINE( Tr2FontMeasurer );

const Be::ClassInfo* Tr2FontMeasurer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FontMeasurer, "Tr2FontMeasurer is used for measuring and rendering fonts." )
		MAP_INTERFACE( Tr2FontMeasurer )
		MAP_ATTRIBUTE(
			"cursorX",
			m_cursorX,
			"Current x-position of cursor.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"cursorY",
			m_cursorY,
			"Current y-position of cursor.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"letterSpace",
			m_letterSpace,
			"Letter spacing, in pixels",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"ascender",
			m_ascender,
			"Ascender value, in pixels",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"asc",
			m_ascender,
			"Same as ascender (for backwards compatibility)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"descender",
			m_descender,
			"Descender value, in pixels",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"des",
			m_descender,
			"Same as descender (for backwards compatibility)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"limit",
			m_limit,
			"Maximum width allowed when adding text",
			Be::READWRITE )

		MAP_PROPERTY(
			"color",
			GetColor,
			SetColor,
			"Color of the text when drawn to a texture. Can be set explicitly\n"
			"or passed in via params to AddText" )

		MAP_ATTRIBUTE(
			"underline",
			m_underline,
			"Should text be underlined when drawn to a texture? Can be set explicitly\n"
			"or passed in via params to AddText",
			Be::READWRITE )

		MAP_PROPERTY(
			"font",
			GetFont,
			SetFont,
			"Name of font file to use" )

		MAP_PROPERTY(
			"fontSize",
			GetFontSize,
			SetFontSize,
			"Size of font, in pixels" )

		MAP_ATTRIBUTE(
			"letterSpace",
			m_letterSpace,
			"Spacing between letters, in pixels",
			Be::READWRITE )

		MAP_PROPERTY(
			"fadeLeftStart",
			GetFadeLeftStart,
			SetFadeLeftStart,
			"Distance (in pixels) from the left edge where letters begin to fade in."
			"\nAny text to the left of this value will be invisible." )

		MAP_PROPERTY(
			"fadeLeftEnd",
			GetFadeLeftEnd,
			SetFadeLeftEnd,
			"Distance (in pixels) from the left edge where letters finish fading in."
			"\nAny text to the right of this value will be fully opaque, until fadeRightStart"
			"\nis reached." )

		MAP_PROPERTY(
			"fadeRightStart",
			GetFadeRightStart,
			SetFadeRightStart,
			"Distance (in pixels) from the left edge where letters begin to fade out." )

		MAP_PROPERTY(
			"fadeRightEnd",
			GetFadeRightEnd,
			SetFadeRightEnd,
			"Distance (in pixels) from the left edge where letters finish fading out."
			"\nAny text to the right of this value will be invisible." )

		MAP_PROPERTY(
			"fadeTopStart",
			GetFadeTopStart,
			SetFadeTopStart,
			"Distance (in pixels) from the top edge where letters begin to fade in."
			"\nAny text above this value will be invisible." )

		MAP_PROPERTY(
			"fadeTopEnd",
			GetFadeTopEnd,
			SetFadeTopEnd,
			"Distance (in pixels) from the top edge where letters finish fading in."
			"\nAny text below this value will be fully opaque, until fadeBottomStart"
			"\nis reached." )

		MAP_PROPERTY(
			"fadeBottomStart",
			GetFadeBottomStart,
			SetFadeBottomStart,
			"Distance (in pixels) from the top edge where letters begin to fade out." )

		MAP_PROPERTY(
			"fadeBottomEnd",
			GetFadeBottomEnd,
			SetFadeBottomEnd,
			"Distance (in pixels) from the top edge where letters finish fading out."
			"\nAny text below this value will be invisible." )

		MAP_METHOD_AND_WRAP(
			"AddText",
			AddText,
			"n = AddText(text)"
			"\nAdds text to the measurer object, advancing the cursor horizontally."
			"\nNew line characters are not handled here, they are expected to be"
			"\nhandled outside the measurer."
			"\n:param text: unicode string to be added"
			"\n\n:returns:"
			"\n  the number of characters added. May be less the length of the given"
			"\n  text if a limit is set on the measurer." )

		MAP_METHOD_AND_WRAP(
			"CommitText",
			CommitText,
			"CommitText(shiftX, shiftY)"
			"\nCommits text previously added with AddText, shifting it by the"
			"\ngiven x,y coordinates."
			"\nText can also be canceled with CancelLastText."
			"\n:param shiftX: amount in pixels that text to commit is shifted by horizontally"
			"\n:param shiftY: amount in pixels that text to commit is shifted by vertically" )

		MAP_METHOD_AND_WRAP(
			"CancelLastText",
			CancelLastText,
			"CancelLastText()"
			"\nCancels the last added text. Can be called repeatedly to undo"
			"\nmultiple AddText calls. Text that has been committed cannot be canceled." )

		MAP_METHOD_AND_WRAP(
			"HasCommittedText",
			HasCommittedText,
			"HasCommittedText()"
			"\nReturns true if text has been added and committed." )

		MAP_METHOD_AND_WRAP(
			"HasUncommittedText",
			HasUncommittedText,
			"HasUncommittedText()"
			"\nReturns true if text has been added and not yet committed." )

		MAP_METHOD_AND_WRAP(
			"Reset",
			Reset,
			"Reset()"
			"\nClears any text added and resets the cursor." )

		MAP_METHOD_AND_WRAP(
			"DrawToAtlasTexture",
			DrawToAtlasTexture,
			"DrawToAtlasTexture(atlasTexture)"
			"\nDraws all committed text to the atlas texture."
			"\n:param atlasTexture: a Tr2AtlasTexture object" )

		MAP_METHOD_AND_WRAP(
			"DrawToTexture",
			DrawToTexture,
			"DrawToTexture(texture)"
			"\nDraws all committed text to the texture."
			"\n:param texture: a TriTextureRes object" )

		MAP_METHOD_AND_WRAP(
			"DrawToHostBitmap",
			DrawToHostBitmap,
			"DrawToHostBitmap(hostBitmap)"
			"\nDraws all committed text to the hostBitmap."
			"\n:param hostBitmap: a Tr2HostBitmap object" )

		MAP_METHOD_AND_WRAP(
			"GetIndexAtPos",
			GetIndexAtPos,
			"ix = GetIndexAtPos(x)"
			"\nReturns the character index of the given x position. This only works"
			"\nfor single line text."
			"\n:param x: position being queried"
			"\n\n:returns:"
			"\n  index into the string that corresponds to the x position." )

		MAP_METHOD_AND_WRAP(
			"GetWidthAtIndex",
			GetWidthAtIndex,
			"x = GetWidthAtIndex(ix)"
			"\nReturns the width of the string at the given index."
			"\n:param ix: index into the string"
			"\n\n:returns: the width of the string at the given index." )
	EXPOSURE_END()
}