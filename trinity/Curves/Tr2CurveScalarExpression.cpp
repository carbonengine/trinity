// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveScalarExpression.h"
#include "include/TriMath.h"
#include "TbbStub.h"
#include "TriSettingsRegistrar.h"
#include "Tr2ExpressionTermInfo.h"
#include <random>

bool g_expressionCurveFakeRandom = false;
TRI_REGISTER_SETTING( "expressionCurveFakeRandom", g_expressionCurveFakeRandom );


namespace
{

// --------------------------------------------------------------------------------
float Fractal( const Tr2CurveScalarExpression* curve, float x, float alpha, float beta, float n )
{
	return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
}

// --------------------------------------------------------------------------------
float Noise( const Tr2CurveScalarExpression* curve, float x )
{
	return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
}

// --------------------------------------------------------------------------------
float RandomConstant( const Tr2CurveScalarExpression* curve, float a, float b )
{
	return ( ( b - a ) * curve->GetRandomConstant() ) + a;
}

float RandomHash( const Tr2CurveScalarExpression* curve, float a, float b, float x )
{
	std::seed_seq::result_type seeds[] = {
		*reinterpret_cast<std::seed_seq::result_type*>( &x ),
		std::seed_seq::result_type( reinterpret_cast<uint64_t>( curve ) )
	};
	std::seed_seq seq( std::begin( seeds ), std::end( seeds ) );
	std::minstd_rand0 e1( seq );
	std::uniform_real_distribution<float> d( a, b );
	return d( e1 );
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
float Input( const Tr2CurveScalarExpression* curve, float index )
{
	return curve->GetInputValue( int32_t( index + 0.5f ) );
}

// --------------------------------------------------------------------------------
float InputAt( const Tr2CurveScalarExpression* curve, float index, float time )
{
	return curve->GetInputValue( int32_t( index + 0.5f ), time );
}

CcpParser::Function s_functions[] = {
	CcpParser::Function( "fractal", &Fractal, 1, 0 ),
	CcpParser::Function( "noise", &Noise, 1, 0 ),
	CcpParser::Function( "randomConstant", &RandomConstant, 1, 0 ),
	CcpParser::Function( "randconst", &RandomConstant, 1, 0 ),
	CcpParser::Function( "random", &Random ),
	CcpParser::Function( "randhash", &RandomHash, 1, 0 ),
	CcpParser::Function( "input", &Input, 1, 0 ),
	CcpParser::Function( "inputAt", &InputAt, 1, 0 ),
	CcpParser::Function( "clamp", &TriClamp, CcpParser::FunctionFlags::PURE_FUNC ),
};
CcpParser::Constant s_constants[] = {
	{ "pi", 3.1415926f },
	{ "pi2", 2.0f * 3.1415926f },
};
}

// --------------------------------------------------------------------------------
Tr2CurveScalarExpression::Tr2CurveScalarExpression( IRoot* lockobj ) :
	PARENTLOCK( m_inputs ),
	m_currentValue( 0 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX )
{
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

	m_arguments.m_time = float( time / m_timeScale );
	if( m_program )
	{
		auto self = this;
		void* buffers[] = { (void*)&m_arguments, (void*)&self };
		return m_program.Eval( buffers, m_tempArena.get() );
	}
	return 0;
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

	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Arguments, m_time ) },
		{ "input1", 0, offsetof( Arguments, m_input1 ) },
		{ "input2", 0, offsetof( Arguments, m_input2 ) },
		{ "input3", 0, offsetof( Arguments, m_input3 ) },
		{ "input4", 0, offsetof( Arguments, m_input4 ) },
	};

	CcpParser::FunctionView functionView[] = { s_functions };
	CcpParser::ConstantView constantView[] = { s_constants };
	CcpParser::VariableView variableView[] = { s_variables };

	CcpParser::Externals externals;
	externals.functions = functionView;
	externals.variables = variableView;
	externals.constants = constantView;
	auto result = CcpParser::Parse( expression.c_str(), externals, m_program );
	if( !result )
	{
		CCP_LOGERR( "Tr2CurveScalarExpression::SetExpression invalid expression \"%s\": %s", expression.c_str(), ToString( result, expression.c_str() ).c_str() );
		return;
	}
	m_tempArena.reset( new uint8_t[m_program.GetTempArenaSize()] );
	m_expression = expression;
}

// --------------------------------------------------------------------------------
float Tr2CurveScalarExpression::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_arguments.m_time );
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
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "randhash", "a", "b", "x", "random value in range [a, b) based on value x" ) );
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

BlueStdResult Tr2CurveScalarExpression::EvaluateExpression( const char* expression, float& value ) const
{
	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Arguments, m_time ) },
		{ "input1", 0, offsetof( Arguments, m_input1 ) },
		{ "input2", 0, offsetof( Arguments, m_input2 ) },
		{ "input3", 0, offsetof( Arguments, m_input3 ) },
		{ "input4", 0, offsetof( Arguments, m_input4 ) },
	};

	CcpParser::FunctionView functionView[] = { s_functions };
	CcpParser::ConstantView constantView[] = { s_constants };
	CcpParser::VariableView variableView[] = { s_variables };

	CcpParser::Externals externals;
	externals.functions = functionView;
	externals.variables = variableView;
	externals.constants = constantView;
	CcpParser::Program program;
	auto result = CcpParser::Parse( expression, externals, program );
	if( !result )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, ToString( result, expression ).c_str() );
	}
	std::unique_ptr<uint8_t[]> tempArena( new uint8_t[program.GetTempArenaSize()] );
	auto self = this;
	void* buffers[] = { (void*)&m_arguments, (void*)&self };
	value = program.Eval( buffers, tempArena.get() );
	return BlueStdResult();
}
