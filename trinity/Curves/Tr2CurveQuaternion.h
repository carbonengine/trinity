// Copyright © 2017 CCP ehf.

#pragma once

#include "Tr2CurveScalar.h"



struct Tr2CurveQuaternionKey
{
	// Key time
	float m_time;
	// Key value
	Quaternion m_value;
	// Key ID, used by the editor
	uint16_t m_id;
	// Curve segment after the key interpolation (Tr2CurveInterpolation::Type)
	uint16_t m_interpolation;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2CurveQuaternionKey );


BLUE_CLASS( Tr2CurveQuaternion ) :
	public ITriQuaternionFunction, public ITriCurveLength
{
public:
	Tr2CurveQuaternion( IRoot* lockobj = nullptr );

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

	virtual float Length();

	Quaternion GetValue( double time ) const;
	Quaternion GetCurrentValue() const;

	void OnKeysChanged();

	void AddKey(
		float time,
		Quaternion value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::LINEAR> interpolation );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );

private:
	float GetLocalTime( double time ) const;
	Quaternion GetSegmentValue( float time, const Tr2CurveQuaternionKey& k0, const Tr2CurveQuaternionKey& k1 ) const;

	PTr2CurveQuaternionKeyStructureList m_keys;
	std::string m_name;
	Quaternion m_currentValue;
	mutable size_t m_lastSegment;
	Tr2CurveExtrapolation::Type m_extrapolationBefore;
	Tr2CurveExtrapolation::Type m_extrapolationAfter;
};

TYPEDEF_BLUECLASS( Tr2CurveQuaternion );