////////////////////////////////////////////////////////////
//
//    Created:   May 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveScalarExpression.h"
#include "include/TriMath.h"
#include "TbbStub.h"
#include "TriSettingsRegistrar.h"
#include "Tr2ExpressionTermInfo.h"

bool g_expressionCurveFakeRandom = false;
TRI_REGISTER_SETTING( "expressionCurveFakeRandom", g_expressionCurveFakeRandom );


namespace
{
	CcpMutex s_mutex( "Tr2CurveScalarExpression", "s_mutex", 1000 );

	std::vector<const Tr2CurveScalarExpression*> s_currentCurve;

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
Tr2CurveScalarExpression::Tr2CurveScalarExpression( IRoot* lockobj )
	:PARENTLOCK( m_inputs ),
	m_currentValue( 0 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX ),
	m_input1( 0 ),
	m_input2( 0 ),
	m_input3( 0 ),
	m_input4( 0 )
{
	m_expressionParser.DefineFun( "fractal", &Fractal, false );
	m_expressionParser.DefineFun( "noise", &Noise, false );
	m_expressionParser.DefineFun( "randomConstant", &RandomConstant, false );
	m_expressionParser.DefineFun( "randconst", &RandomConstant, false );
	m_expressionParser.DefineFun( "random", &Random, false );
	m_expressionParser.DefineFun( "input", &Input, false );
	m_expressionParser.DefineFun( "inputAt", &InputAt, false );
	m_expressionParser.DefineFun( "clamp", &TriClamp, false );

	m_expressionParser.DefineVar( "input1", &m_input1 );
	m_expressionParser.DefineVar( "input2", &m_input2 );
	m_expressionParser.DefineVar( "input3", &m_input3 );
	m_expressionParser.DefineVar( "input4", &m_input4 );

	m_expressionParser.DefineVar( "time", &m_time );

	m_expressionParser.DefineConst( "pi", 3.1415926f );
	m_expressionParser.DefineConst( "pi2", 2.0f * 3.1415926f );

}

// --------------------------------------------------------------------------------
bool Tr2CurveScalarExpression::Initialize()
{
	if( !m_expression.empty() )
	{
		auto expression = m_expression;
		m_expression = "";
		SetExpression( expression );
	}
	return true;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalarExpression::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::Update( Be::Time time )
{
	return m_currentValue = GetValue( TimeAsDouble( time ) );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::Update( double time )
{
	return m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetValueAt( Be::Time time )
{
	return GetValue( TimeAsDouble( time ) );
}

// --------------------------------------------------------------------------------
void Tr2CurveScalarExpression::ScaleTime( float s )
{
	m_timeScale = s;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetValueAt( double time )
{
	return GetValue( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetValue( double time ) const
{
	if( m_expression.empty() )
	{
		return 0;
	}

	m_time = float( time / m_timeScale );

	CcpAutoMutex lock( s_mutex );
	s_currentCurve.push_back( this );
	float value;
	try
	{
		value = m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& )
	{
		value = 0;
	}
	s_currentCurve.pop_back();
	return value;
}

// --------------------------------------------------------------------------------
std::string Tr2CurveScalarExpression::GetExpression() const
{
	return m_expression;
}

// --------------------------------------------------------------------------------
void Tr2CurveScalarExpression::SetExpression( const std::string& expression )
{
	if( expression.empty() )
	{
		m_expression = expression;
		return;
	}
	m_expressionParser.SetExpr( expression );

	CcpAutoMutex lock( s_mutex );
	s_currentCurve.push_back( this );

	try
	{
		m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& e )
	{
		s_currentCurve.pop_back();
		CCP_LOGERR( "Tr2CurveScalarExpression::SetExpression invalid expression \"%s\": %s", expression.c_str(), e.GetMsg().c_str() );
		m_expressionParser.SetExpr( m_expression );
		return;
	}
	s_currentCurve.pop_back();
	m_expression = expression;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetInputValue( int index, float time ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( time );
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetRandomConstant() const
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
void Tr2CurveScalarExpression::ResetRandomConstant()
{
	m_randomConstant = float( rand() ) / RAND_MAX;
}

// --------------------------------------------------------------------------------
std::vector<Tr2ExpressionTermInfoPtr> Tr2CurveScalarExpression::GetExpressionTermInfo() const
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
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input1", "input1 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input2", "input2 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input3", "input3 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "input4", "input4 attribute" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "time", "current time" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi", "Pi value" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi2", "Pi x 2 value" ) );
	return result;
}