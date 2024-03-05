#pragma once

BLUE_DECLARE( Range );

BLUE_CLASS( Range ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	Range( IRoot* lockobj = nullptr );
	~Range();

	void SetCenterPoint( const float var );
	void SetMinRangePoint( float var );
	void Setup(float rangeCenterPoint, float rangeDeltaFromCenter, float sliderMin, float sliderMax);
	void SetMaxRangePoint( float var );
	float GetCenterPoint() const;
	float GetMinRangePoint() const;
	float GetMaxRangePoint() const;
	void ToggleIsUniform();
	void SetIsUniform( const bool var );
	bool GetIsUniform() const;
	void SetSliderMin( const float var );
	void SetSliderMax( const float var );
	float GetSliderMin() const;
	float GetSliderMax() const;


protected:
	void FixUniformity();

	bool m_isUniform;
	float m_centerPoint;
	float m_minRange;
	float m_maxRange;
	float m_sliderRangeMin;
	float m_sliderRangeMax;
};

TYPEDEF_BLUECLASS( Range );
 