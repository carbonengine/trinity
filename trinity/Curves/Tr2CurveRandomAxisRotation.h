// Copyright © 2017 CCP ehf.

#pragma once

#include <ITriFunction.h>

BLUE_CLASS( Tr2CurveRandomAxisRotation ) :
	public ITriQuaternionFunction, public IInitialize
{
public:
	Tr2CurveRandomAxisRotation( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );

	virtual Quaternion* Update( Quaternion * in, Be::Time time );
	virtual Quaternion* Update( Quaternion * in, double time );
	virtual Quaternion* GetValueAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueAt( Quaternion * in, double time );
	virtual Quaternion* GetValueDotAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueDotAt( Quaternion * in, double time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion * in, Be::Time time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion * in, double time );

	virtual bool Initialize();

	Quaternion GetValue( double time ) const;

	uint32_t GetSeed() const;
	void SetSeed( uint32_t seed );

public:
	void SeedChanged();

	std::string m_name;

	Quaternion m_preRotation;
	Quaternion m_postRotation;
	Quaternion m_currentValue;
	float m_period;
	uint32_t m_seed;
};

TYPEDEF_BLUECLASS( Tr2CurveRandomAxisRotation );