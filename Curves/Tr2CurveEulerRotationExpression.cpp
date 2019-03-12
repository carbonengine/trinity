////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveEulerRotationExpression.h"
#include "Tr2ExpressionTermInfo.h"
#include "include/TriMath.h"

extern bool g_expressionCurveFakeRandom;


namespace
{
	CcpMutex s_mutex( "Tr2CurveScalarExpression", "s_mutex", 1000 );

	std::vector<const Tr2CurveEulerRotationExpression*> s_currentCurve;

	// --------------------------------------------------------------------------------
	float Fractal( float x, float alpha, float beta, float n )
	{
		return float( ( PerlinNoise1D( x + s_currentCurve.back()->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float Noise( float x )
	{
		return float( ( PerlinNoise1D( x + s_currentCurve.back()->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float RandomConstant( float a, float b )
	{
		return ( ( b - a ) * s_currentCurve.back()->GetRandomConstant() ) + a;
	}

	// --------------------------------------------------------------------------------
	float Random( float a, float b )
	{
		if( g_expressionCurveFakeRandom )
		{
			return ( ( b - a ) * 0.41f ) + a;
		}
		return ( ( b - a ) * ( (float)rand() / RAND_MAX ) ) + a;
	}

	// --------------------------------------------------------------------------------
	float Input( float index )
	{
		return s_currentCurve.back()->GetInputValue( int32_t( index + 0.5f ) );
	}

	// --------------------------------------------------------------------------------
	float InputAt( float index, float time )
	{
		return s_currentCurve.back()->GetInputValue( int32_t( index + 0.5f ), time );
	}
}

// --------------------------------------------------------------------------------
Tr2CurveEulerRotationExpression::Tr2CurveEulerRotationExpression( IRoot* lockobj )
	:PARENTLOCK( m_inputs ),
	m_currentValue( 0, 0, 0, 1 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX ),
	m_input1( 0 ),
	m_input2( 0 ),
	m_input3( 0 ),
	m_input4( 0 )
{
	for( size_t i = 0; i < 3; ++i )
	{
		auto& parser = m_expressionParsers[i];

		parser.DefineFun( "fractal", &Fractal, false );
		parser.DefineFun( "noise", &Noise, false );
		parser.DefineFun( "randomConstant", &RandomConstant, false );
		parser.DefineFun( "randconst", &RandomConstant, false );
		parser.DefineFun( "random", &Random, false );
		parser.DefineFun( "input", &Input, false );
		parser.DefineFun( "inputAt", &InputAt, false );
		parser.DefineFun( "clamp", &TriClamp, false );
		parser.DefineFun( "radians", &XMConvertToRadians, true );

		parser.DefineVar( "input1", &m_input1 );
		parser.DefineVar( "input2", &m_input2 );
		parser.DefineVar( "input3", &m_input3 );
		parser.DefineVar( "input4", &m_input4 );

		parser.DefineVar( "time", &m_time );

		parser.DefineConst( "pi", 3.1415926f );
		parser.DefineConst( "pi2", 2.0f * 3.1415926f );
	}
}

// --------------------------------------------------------------------------------
bool Tr2CurveEulerRotationExpression::Initialize()
{
	for( size_t i = 0; i < 3; ++i )
	{
		if( !m_expressions[i].empty() )
		{
			auto expression = m_expressions[i];
			m_expressions[i] = "";
			SetExpression( i, expression );
		}
	}
	return true;

}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Quaternion Tr2CurveEulerRotationExpression::GetValue( double time ) const
{
	Vector3 result( 0, 0, 0 );
	float* components = &result.x;

	m_time = float( time / m_timeScale );

	for( size_t i = 0; i < 3; ++i )
	{
		if( m_expressions[i].empty() )
		{
			continue;
		}

		CcpAutoMutex lock( s_mutex );
		s_currentCurve.push_back( this );

		try
		{
			components[i] = m_expressionParsers[i].Eval();
		}
		catch( const mu::Parser::exception_type& )
		{
			components[i] = 0;
		}
		s_currentCurve.pop_back();
	}
	return Quaternion( XMQuaternionRotationRollPitchYaw( result.y, result.x, result.z ) );

}

// --------------------------------------------------------------------------------
std::string Tr2CurveEulerRotationExpression::GetExpression( size_t index ) const
{
	return m_expressions[index];
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::SetExpression( size_t index, const std::string& expression )
{
	if( expression.empty() )
	{
		m_expressions[index] = expression;
		return;
	}
	m_expressionParsers[index].SetExpr( expression );

	CcpAutoMutex lock( s_mutex );
	s_currentCurve.push_back( this );

	try
	{
		m_expressionParsers[index].Eval();
	}
	catch( const mu::Parser::exception_type& e )
	{
		s_currentCurve.pop_back();
		CCP_LOGERR( "Tr2CurveEulerRotationExpression::SetExpression invalid expression \"%s\": %s", expression.c_str(), e.GetMsg().c_str() );
		m_expressionParsers[index].SetExpr( m_expressions[index] );
		return;
	}
	s_currentCurve.pop_back();
	m_expressions[index] = expression;
}

// --------------------------------------------------------------------------------
std::string Tr2CurveEulerRotationExpression::GetExpressionYaw() const
{
	return GetExpression( 0 );
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::SetExpressionYaw( const std::string& expression )
{
	SetExpression( 0, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveEulerRotationExpression::GetExpressionPitch() const
{
	return GetExpression( 1 );
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::SetExpressionPitch( const std::string& expression )
{
	SetExpression( 1, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveEulerRotationExpression::GetExpressionRoll() const
{
	return GetExpression( 2 );
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::SetExpressionRoll( const std::string& expression )
{
	SetExpression( 2, expression );
}

// --------------------------------------------------------------------------------
float Tr2CurveEulerRotationExpression::GetRandomConstant() const
{
	if( g_expressionCurveFakeRandom )
	{
		return 0.21f;
	}
	else
	{
		return m_randomConstant;
	}
}

// --------------------------------------------------------------------------------
float Tr2CurveEulerRotationExpression::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_time );
}

// --------------------------------------------------------------------------------
float Tr2CurveEulerRotationExpression::GetInputValue( int index, float time ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( time );
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::Update( Quaternion* in, Be::Time time )
{
	*in = m_currentValue = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::Update( Quaternion* in, double time )
{
	*in = m_currentValue = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueAt( Quaternion* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueAt( Quaternion* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueDoubleDotAt( Quaternion* in, Be::Time time )
{
	return in;
}

// --------------------------------------------------------------------------------
Quaternion* Tr2CurveEulerRotationExpression::GetValueDoubleDotAt( Quaternion* in, double time )
{
	return in;
}

// --------------------------------------------------------------------------------
void Tr2CurveEulerRotationExpression::ResetRandomConstant()
{
	m_randomConstant = float( rand() ) / RAND_MAX;
}

// --------------------------------------------------------------------------------
std::vector<Tr2ExpressionTermInfoPtr> Tr2CurveEulerRotationExpression::GetExpressionTermInfo() const
{
	std::vector<Tr2ExpressionTermInfoPtr> result;
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "fractal", "x", "alpha", "beta", "n", "fractal noise" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "noise", "x", "simple one-octave noise" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "randomConstant", "a", "b", "random per-curve constant in range [a, b)" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "randconst", "a", "b", "random per-curve constant in range [a, b)" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "random", "a", "b", "random value in range [a, b)" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Inputs", "input", "n", "n-th input curve value at current time" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Inputs", "inputAt", "n", "t", "input curve value at time t" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Math", "clamp", "x", "min", "max", "value x clamped to [min, max] range" ) );
	result.push_back( Tr2ExpressionTermInfo::Function( "Math", "radians", "x", "convert x degrees to radians" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input1", "input1 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input2", "input2 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input3", "input3 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input4", "input4 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "time", "current time" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi", "Pi value" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi2", "Pi x 2 value" ) );
	return result;
}