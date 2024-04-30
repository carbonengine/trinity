////////////////////////////////////////////////////////////
//
//    Created:   February 2024
//    Copyright: CCP 2024
//

#pragma once

namespace EveSpaceObjectAttachmentUtils
{
	enum FadeType
	{
		FT_NONE,
		FT_BLINK,
		FT_FADEIN,
		FT_FADEOUT,
		FT_FADEINOUT
	};

	float Blink( float blinkRate, float blinkPhase, float minScale, float maxScale );
	float FadeIn( float blinkRate, float blinkPhase );
	float FadeOut( float blinkRate, float blinkPhase );
	float FadeInOut( float blinkRate, float blinkPhase );

	float Fade( FadeType type, float blinkRate, float blinkPhase );
}
