////////////////////////////////////////////////////////////
//
//    Created:   October 2020
//    Copyright: CCP 2020
//

#pragma once

BLUE_DECLARE( Tr2ImageRes );

BLUE_CLASS_IMPL( Tr2Sprite2dPickingMask );

class Tr2Sprite2dPickingMask: public IRoot
{
public:
	explicit Tr2Sprite2dPickingMask( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	const std::wstring GetMaskPath() const;
	void SetMaskPath( const wchar_t* path );

	bool SampleMask( const Vector2& point, const Vector2& topLeft, float width, float height ) const;
private:
	std::wstring m_maskPath;
	Tr2ImageResPtr m_mask;

	uint32_t m_channel;
	float m_threshold;

	uint32_t m_leftEdge;
	uint32_t m_topEdge;
	uint32_t m_rightEdge;
	uint32_t m_bottomEdge;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dPickingMask );
