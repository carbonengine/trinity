#include "StdAfx.h"
#include "Tr2CurveVector3Expression.h"
#include "Tr2ExpressionTermInfo.h"
#include "include/TriMath.h"
#include <random>

extern bool g_expressionCurveFakeRandom;


namespace
{
	// --------------------------------------------------------------------------------
	float Fractal( const Tr2CurveVector3Expression* curve, float x, float alpha, float beta, float n )
	{
		return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float Noise( const Tr2CurveVector3Expression* curve, float x )
	{
		return float( ( PerlinNoise1D( x + curve->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
	}

	// --------------------------------------------------------------------------------
	float RandomConstant( const Tr2CurveVector3Expression* curve, float a, float b )
	{
		return ( ( b - a ) * curve->GetRandomConstant() ) + a;
	}

	float RandomHash( const Tr2CurveVector3Expression* curve, float a, float b, float x )
	{
		std::seed_seq::result_type seeds[] = {
			*reinterpret_cast<std::seed_seq::result_type*>( &x ),
			std::seed_seq::result_type( reinterpret_cast<uint64_t>( curve ) )
		};
		std::seed_seq seq( std::begin( seeds ), std::end( seeds ) );
		std::default_random_engine e1( seq );
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
	float Input( const Tr2CurveVector3Expression* curve, float index )
	{
		return curve->GetInputValue( int32_t( index + 0.5f ) );
	}

	// --------------------------------------------------------------------------------
	float InputAt( const Tr2CurveVector3Expression* curve, float index, float time )
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
Tr2CurveVector3Expression::Tr2CurveVector3Expression( IRoot* lockobj )
	:PARENTLOCK( m_inputs ),
	m_currentValue( 0, 0, 0 ),
	m_timeScale( 1 ),
	m_randomConstant( float( rand() ) / RAND_MAX ),
	m_time( 0 ),
	m_input1( 0 ),
	m_input2( 0 ),
	m_input3( 0 ),
	m_input4( 0 )
{
}

// --------------------------------------------------------------------------------
bool Tr2CurveVector3Expression::Initialize()
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
void Tr2CurveVector3Expression::UpdateValue( double time )
{
	m_currentValue = GetValue( time );
}

// --------------------------------------------------------------------------------
Vector3 Tr2CurveVector3Expression::GetValue( double time ) const
{
	Vector3 result( 0, 0, 0 );
	float* components = &result.x;

	m_time = float( time / m_timeScale );
	auto self = this;
	void* buffers[] = { (void*)this, (void*)&self };

	for( size_t i = 0; i < 3; ++i )
	{
		if( !m_programs[i] )
		{
			continue;
		}
		components[i] = m_programs[i].Eval( buffers, m_tempArena.get() );
	}
	return result;
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpression( size_t index ) const
{
	return m_expressions[index];
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpression( size_t index, const std::string& expression )
{
	if( expression.empty() )
	{
		m_expressions[index] = expression;
		return;
	}

	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Tr2CurveVector3Expression, m_time ) },
		{ "input1", 0, offsetof( Tr2CurveVector3Expression, m_input1 ) },
		{ "input2", 0, offsetof( Tr2CurveVector3Expression, m_input2 ) },
		{ "input3", 0, offsetof( Tr2CurveVector3Expression, m_input3 ) },
		{ "input4", 0, offsetof( Tr2CurveVector3Expression, m_input4 ) },
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
		CCP_LOGERR( "Tr2CurveVector3Expression::SetExpression invalid expression \"%s\": %s", expression.c_str(), ToString( result, expression.c_str() ).c_str() );
		return;
	}
	m_tempArena.reset( new uint8_t[std::max( m_programs[0].GetTempArenaSize(), std::max( m_programs[1].GetTempArenaSize(), m_programs[2].GetTempArenaSize() ) )] );
	m_expressions[index] = expression;
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionX() const
{
	return GetExpression( 0 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionX( const std::string& expression )
{
	SetExpression( 0, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionY() const
{
	return GetExpression( 1 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionY( const std::string& expression )
{
	SetExpression( 1, expression );
}

// --------------------------------------------------------------------------------
std::string Tr2CurveVector3Expression::GetExpressionZ() const
{
	return GetExpression( 2 );
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::SetExpressionZ( const std::string& expression )
{
	SetExpression( 2, expression );
}

// --------------------------------------------------------------------------------
float Tr2CurveVector3Expression::GetRandomConstant() const
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
float Tr2CurveVector3Expression::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_time );
}

// --------------------------------------------------------------------------------
float Tr2CurveVector3Expression::GetInputValue( int index, float time ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( time );
}


// --------------------------------------------------------------------------------
Color* Tr2CurveVector3Expression::Update( Color* in, Be::Time time )
{
	m_currentValue = GetValue( TimeAsDouble( time ) );
	in->r = m_currentValue.x;
	in->g = m_currentValue.y;
	in->b = m_currentValue.z;
	in->a = 0;
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveVector3Expression::Update( Color* in, double time )
{
	m_currentValue = GetValue( time );
	in->r = m_currentValue.x;
	in->g = m_currentValue.y;
	in->b = m_currentValue.z;
	in->a = 0;
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveVector3Expression::GetValueAt( Color* in, Be::Time time )
{
	auto value = GetValue( TimeAsDouble( time ) );
	in->r = value.x;
	in->g = value.y;
	in->b = value.z;
	in->a = 0;
	return in;
}

// --------------------------------------------------------------------------------
Color* Tr2CurveVector3Expression::GetValueAt( Color* in, double time )
{
	auto value = GetValue( time );
	in->r = value.x;
	in->g = value.y;
	in->b = value.z;
	in->a = 0;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::Update( Vector3* in, Be::Time time )
{
	m_currentValue = GetValue( TimeAsDouble( time ) );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::Update( Vector3* in, double time )
{
	m_currentValue = GetValue( time );
	*in = m_currentValue;
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueAt( Vector3* in, Be::Time time )
{
	*in = GetValue( TimeAsDouble( time ) );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueAt( Vector3* in, double time )
{
	*in = GetValue( time );
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueDoubleDotAt( Vector3* in, Be::Time )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3* Tr2CurveVector3Expression::GetValueDoubleDotAt( Vector3* in, double )
{
	return in;
}

// --------------------------------------------------------------------------------
Vector3d* Tr2CurveVector3Expression::InterpolatedPosition( Vector3d* out, Be::Time )
{
	return out;
}

// --------------------------------------------------------------------------------
void Tr2CurveVector3Expression::ResetRandomConstant()
{
	m_randomConstant = float( rand() ) / RAND_MAX;
}

// --------------------------------------------------------------------------------
std::vector<Tr2ExpressionTermInfoPtr> Tr2CurveVector3Expression::GetExpressionTermInfo() const
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

BlueStdResult Tr2CurveVector3Expression::EvaluateExpression( const char* expression, float& value ) const
{
	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Tr2CurveVector3Expression, m_time ) },
		{ "input1", 0, offsetof( Tr2CurveVector3Expression, m_input1 ) },
		{ "input2", 0, offsetof( Tr2CurveVector3Expression, m_input2 ) },
		{ "input3", 0, offsetof( Tr2CurveVector3Expression, m_input3 ) },
		{ "input4", 0, offsetof( Tr2CurveVector3Expression, m_input4 ) },
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
	void* buffers[] = { (void*)this, (void*)&self };
	value = program.Eval( buffers, tempArena.get() );
	return BlueStdResult();
}
