// Copyright © 2026 CCP ehf.

#pragma once


BLUE_CLASS( EveThrottleable ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveThrottleable( IRoot* lockobj = nullptr );

	bool ShouldSkipUpdate( float normalizedUpdateFrequency = 0.5f );

private:
	bool m_updateThrottle;
	uint32_t m_minUpdateFrequency;
	uint32_t m_maxUpdateFrequency;
	float m_currentUpdateFrequency;
	Be::Time m_nextUpdateTS;
};

TYPEDEF_BLUECLASS( EveThrottleable );
