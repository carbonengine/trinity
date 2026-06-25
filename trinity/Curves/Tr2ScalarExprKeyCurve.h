// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2ScalarExprKeyCurve_h
#define Tr2ScalarExprKeyCurve_h

#include "include/Tr2Curve.h"
#include "ccpparser.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ScalarExprKey is a key type for Tr2ScalarExprKeyCurve. The key allows setting
//   muParser expressions for time, value and tangents values.
// See Also:
//   Tr2ScalarExprKeyCurve
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ScalarExprKey ) :
	public IInitialize,
	public INotify,
	public Tr2Key<float>
{
public:
	Tr2ScalarExprKey( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );

	void RegenRandomConstant();
	void UpdateValues( Tr2ScalarExprKey * previousKey );

	// Tangent for non-linear interpolation modes
	float m_leftTangent;
	// Tangent for non-linear interpolation modes
	float m_rightTangent;

private:
	// Parser for time expression
	CcpParser::Program m_timeParser;
	// Time expression
	std::string m_timeExpression;

	// Parser for value expression
	CcpParser::Program m_valueParser;
	// Value expression
	std::string m_valueExpression;

	// Parser for left tangent expression
	CcpParser::Program m_leftTangentParser;
	// Left tangent expression
	std::string m_leftTangentExpression;

	// Parser for right tangent expression
	CcpParser::Program m_rightTangentParser;
	// Right tangent expression
	std::string m_rightTangentExpression;

	std::vector<uint8_t> m_tempArena;

	// Input variables for expressions
	float m_inputVar1;
	float m_inputVar2;
	float m_inputVar3;
	float m_inputVar4;

	// Random constant for expressions
	float m_randomConstant;
	float m_randomMin;
	float m_randomMax;

	// Time of the previous key (can be used in expressions)
	float m_prevKeyTime;
	// Value of the previous key (can be used in expressions)
	float m_prevKeyValue;

	void SetExpression( CcpParser::Program & parser, std::string & expression );
};
BLUE_DECLARE_VECTOR( Tr2ScalarExprKey );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ScalarExprKeyCurve is a scalar curves with key that can have muParser expressions
//   for their attributes.
// See Also:
//   Tr2ScalarExprKey
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ScalarExprKeyCurve ) :
	public ITriFunction,
	public IInitialize,
	public ITriCurveLength
{
public:
	Tr2ScalarExprKeyCurve( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITriCurveLength
	float Length();

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time );

	float GetValueAt( double time );
	float GetKeyValue( unsigned int idx );
	void SetKeyValue( unsigned int idx, float value );

	float GetKeyTime( unsigned int idx );
	void SetKeyTime( unsigned int idx, float time );

	unsigned int GetKeyInterpolation( unsigned int idx );
	void SetKeyInterpolation( unsigned int idx, unsigned int interp );

	unsigned int GetKeyCount() const
	{
		return (unsigned int)m_keys.size();
	}

	int AddKey( float time, float value, float leftTangent, float rightTangent, unsigned int interpolation );
	void RemoveKey( unsigned int idx );

	void Sort();

	float GetKeyLeftTangent( unsigned int idx );
	void SetKeyLeftTangent( unsigned int idx, float tangent );

	float GetKeyRightTangent( unsigned int idx );
	void SetKeyRightTangent( unsigned int idx, float tangent );

	// Starting tangent value
	float m_startTangent;
	// End tangent value`
	float m_endTangent;

private:
	void ReEvaluateKeys();
	float* Interpolate( float* out, float time, Tr2ScalarExprKey* lastKey, Tr2ScalarExprKey* nextKey );

	// Curve name
	std::string m_name;
	// Current value
	float m_currentValue;
	// Is curve played in reverse
	bool m_reversed;
	// Is curve played in a loop
	bool m_cycle;
	// internal time offset
	float m_timeOffset;
	// internal time scale
	float m_timeScale;
	// Default interpolation
	uint32_t m_interpolation;

	PTr2ScalarExprKeyVector m_keys;
};

TYPEDEF_BLUECLASS( Tr2ScalarExprKeyCurve );
TYPEDEF_BLUECLASS( Tr2ScalarExprKey );

#endif //Tr2ScalarExprKeyCurve_h
