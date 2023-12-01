////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"

namespace Tr2CurveInterpolation
{
	enum Type
	{
		// Constant (L0) interpolation
		CONSTANT = 0,
		// Linear (L1) interpolation
		LINEAR = 1,
		// Hermite/cubic (L2) interpolation
		HERMITE = 2,
	};
}


namespace Tr2CurveTangentType
{
	enum Type
	{
		// Auto-adjusted tangents (without overshoots)
		AUTO_CLAMP = 0,
		// Auto-adjusted tangents
		AUTO = 1,
		// Joined hand-edited tangents
		FREE_JOINED = 2,
		// Split hand-edited tangents
		FREE_SPLIT = 3,
	};
}


namespace Tr2CurveExtrapolation
{
	enum Type
	{
		// Use first/last values when outside curve time range
		CLAMP = 0,
		// Cycle the curve
		CYCLE = 1,
		// Cycle the curve with mirroring
		MIRROR = 2,
		// Linear exprapolation based on first/last key tangent
		LINEAR = 3,
	};
}


struct Tr2CurveScalarKey
{
	// Key time
	float m_time;
	// Key value
	float m_value;
	// Left (arriving) tangent
	float m_leftTangent;
	// Right (departing) tangent
	float m_rightTangent;
	// Key ID, used by the editor
	uint16_t m_id;
	// Curve segment after the key interpolation (Tr2CurveInterpolation::Type)
	uint8_t m_interpolation;
	// Key tangent type (Tr2CurveTangentType::Type)
	uint8_t m_tangentType;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2CurveScalarKey );


BLUE_CLASS( Tr2CurveScalar ): public ITriScalarFunction, public ITriCurveLength
{
public:
	Tr2CurveScalar( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void UpdateValue( double time );
	virtual float Update( Be::Time time );
	virtual float Update( double time );
	virtual float GetValueAt( Be::Time time );
	virtual float GetValueAt( double time );
	virtual void ScaleTime( float s );

	virtual float Length();

	std::string GetName() const;
	void SetName( const char* name );

	float GetValue( double time ) const;
	float GetTangent( double time ) const;
	float GetCurrentValue() const;

	float GetTimeOffset() const;
	void SetTimeOffset( float timeOffset );
	float GetTimeScale() const;
	void SetTimeScale( float timeScale );

	bool IsEmpty() const;

	void OnKeysChanged();

	void AddKey(
		float time,
		float value,
		Be::OptionalWithDefaultValue<Tr2CurveInterpolation::Type, Tr2CurveInterpolation::HERMITE> interpolation,
		float leftTangent,
		float rightTangent,
		Be::OptionalWithDefaultValue<Tr2CurveTangentType::Type, Tr2CurveTangentType::AUTO_CLAMP> tangentType );

	void SetExtrapolation( Tr2CurveExtrapolation::Type extrapolation );

	Tr2CurveScalarKeyStructureList& GetKeys();
private:
	float GetLocalTime( double time ) const;
	float GetSegmentValue( float time, const Tr2CurveScalarKey& k0, const Tr2CurveScalarKey& k1 ) const;
	float GetSegmentTangent( float time, const Tr2CurveScalarKey& k0, const Tr2CurveScalarKey& k1 ) const;

	PTr2CurveScalarKeyStructureList m_keys;
	std::string m_name;
	float m_timeOffset;
	float m_timeScale;
	float m_currentValue;
	mutable size_t m_lastSegment;
	Tr2CurveExtrapolation::Type m_extrapolationBefore;
	Tr2CurveExtrapolation::Type m_extrapolationAfter;
};

TYPEDEF_BLUECLASS( Tr2CurveScalar );