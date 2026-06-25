// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveEulerRotationExpression.h"
#include "Tr2ExpressionTermInfo.h"
#include "include/TriMath.h"
#include <random>

extern bool g_expressionCurveFakeRandom;


namespace
{
// --------------------------------------------------------------------------------
float Fractal( const Tr2CurveEulerRotationExpression* curve, float x, float alpha, float beta, float n )
{
	return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
}

// --------------------------------------------------------------------------------
float Noise( const Tr2CurveEulerRotationExpression* curve, float x )
{
	return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
}

// --------------------------------------------------------------------------------
float RandomConstant( const Tr2CurveEulerRotationExpression* curve, float a, float b )
{
	return ( ( b - a ) * curve->GetRandomConstant() ) + a;
}

float RandomHash( const Tr2CurveEulerRotationExpression* curve, float a, float b, float x )
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
float Input( const Tr2CurveEulerRotationExpression* curve, float index )
{
	return curve->GetInputValue( int32_t( index + 0.5f ) );
}

// --------------------------------------------------------------------------------
float InputAt( const Tr2CurveEulerRotationExpression* curve, float index, float time )
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
	CcpParser::Function( "radians", &XMConvertToRadians, CcpParser::FunctionFlags::PURE_FUNC ),
};
CcpParser::Constant s_constants[] = {
	{ "pi", 3.1415926f },
	{ "pi2", 2.0f * 3.1415926f },
};
}

// --------------------------------------------------------------------------------
Tr2CurveEulerRotationExpression::Tr2CurveEulerRotationExpression( IRoot* lockobj ) :
	PARENTLOCK( m_inputs ),
	m_currentValue( 0, 0, 0, 1 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX )
{
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

	m_arguments.m_time = float( time / m_timeScale );
	auto self = this;
	void* buffers[] = { (void*)&m_arguments, (void*)&self };

	for( size_t i = 0; i < 3; ++i )
	{
		if( !m_programs[i] )
		{
			continue;
		}
		components[i] = m_programs[i].Eval( buffers, m_tempArena.get() );
	}
	return RotationQuaternion( result.x, result.y, result.z );
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
	auto result = CcpParser::Parse( expression.c_str(), externals, m_programs[index] );
	if( !result )
	{
		CCP_LOGERR( "Tr2CurveEulerRotationExpression::SetExpression invalid expression \"%s\": %s", expression.c_str(), ToString( result, expression.c_str() ).c_str() );
		return;
	}
	m_tempArena.reset( new uint8_t[std::max( m_programs[0].GetTempArenaSize(), std::max( m_programs[1].GetTempArenaSize(), m_programs[2].GetTempArenaSize() ) )] );
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
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_arguments.m_time );
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
	result.push_back( Tr2ExpressionTermInfo::Function( "Random", "randhash", "a", "b", "x", "random value in range [a, b) based on value x" ) );
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

BlueStdResult Tr2CurveEulerRotationExpression::EvaluateExpression( const char* expression, float& value ) const
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
