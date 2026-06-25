// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierExpressionBucket.h"
#include "include/TriMath.h"
#include "Tr2ExpressionTermInfo.h"
#include <random>


namespace
{

float Fractal( const EveSmartLightAttributeModifierExpressionBucket* bucket, float x, float alpha, float beta, float n )
{
	return float( ( PerlinNoise1D( x + bucket->GetRandomConstant(), alpha, beta, int( n + 0.5f ) ) + 1.0 ) / 2.0 );
}

float Noise( const EveSmartLightAttributeModifierExpressionBucket* bucket, float x )
{
	return float( ( PerlinNoise1D( x + bucket->GetRandomConstant(), 1.0, 1.0, 1 ) + 1.0 ) / 2.0 );
}

float RandomConstant( const EveSmartLightAttributeModifierExpressionBucket* bucket, float a, float b )
{
	return ( ( b - a ) * bucket->GetRandomConstant() ) + a;
}

float RandomHash( const EveSmartLightAttributeModifierExpressionBucket* bucket, float a, float b, float x )
{
	std::seed_seq::result_type seeds[] = {
		*reinterpret_cast<std::seed_seq::result_type*>( &x ),
		std::seed_seq::result_type( reinterpret_cast<uint64_t>( bucket ) )
	};
	std::seed_seq seq( std::begin( seeds ), std::end( seeds ) );
	std::minstd_rand0 e1( seq );
	std::uniform_real_distribution<float> d( a, b );
	return d( e1 );
}

float Random( float a, float b )
{
	return ( ( b - a ) * ( (float)rand() / RAND_MAX ) ) + a;
}

float Input( const EveSmartLightAttributeModifierExpressionBucket* bucket, float index )
{
	return bucket->GetInputValue( int32_t( index + 0.5f ) );
}

float InputAt( const EveSmartLightAttributeModifierExpressionBucket* bucket, float index, float time )
{
	return bucket->GetInputValue( int32_t( index + 0.5f ), time );
}

float ShipSpeed( IRoot* ctx )
{
	if( IEveSpaceObject2Ptr spaceObject = BlueCastPtr( ctx ) )
	{
		Vector3 velocity;
		spaceObject->GetWorldVelocity( velocity );
		return Length( velocity );
	}

	return 0;
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

EveSmartLightAttributeModifierExpressionBucket::EveSmartLightAttributeModifierExpressionBucket( IRoot* lockobj ) :
	PARENTLOCK( m_inputs ),
	m_randomConstant( float( rand() ) / RAND_MAX )
{
	m_name = "ExpressionBucket";
	m_expression = "";
}

bool EveSmartLightAttributeModifierExpressionBucket::Initialize()
{
	EveSmartLightAttributeModifierBucket::Initialize();
	if( !m_expression.empty() )
	{
		auto expression = m_expression;
		m_expression = "";
		SetExpression( expression );
	}
	return true;
}

float EveSmartLightAttributeModifierExpressionBucket::GetInputValue( int index ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( m_arguments.m_time );
}

float EveSmartLightAttributeModifierExpressionBucket::GetInputValue( int index, float time ) const
{
	if( index < 0 || index >= int( m_inputs.size() ) )
	{
		return 0;
	}
	return const_cast<ITriScalarFunction*>( m_inputs[index] )->GetValueAt( time );
}

float EveSmartLightAttributeModifierExpressionBucket::GetRandomConstant() const
{
	return m_randomConstant;
}

std::string EveSmartLightAttributeModifierExpressionBucket::GetExpression() const
{
	return m_expression;
}

// --------------------------------------------------------------------------------
void EveSmartLightAttributeModifierExpressionBucket::SetExpression( const std::string& expression )
{
	if( expression.empty() )
	{
		m_expression = expression;
		return;
	}

	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Arguments, m_time ) },
		{ "shipSpeed", 0, offsetof( Arguments, m_shipSpeed ) },
		{ "shipMaxSpeed", 0, offsetof( Arguments, m_shipMaxSpeed ) }
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
		CCP_LOGERR( "EveSmartLightAttributeModifierExpressionBucket::SetExpression invalid expression \"%s\": %s", expression.c_str(), ToString( result, expression.c_str() ).c_str() );
		return;
	}
	m_tempArena.reset( new uint8_t[m_program.GetTempArenaSize()] );
	m_expression = expression;
}

std::vector<Tr2ExpressionTermInfoPtr> EveSmartLightAttributeModifierExpressionBucket::GetExpressionTermInfo() const
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
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "time", "current time" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "shipSpeed", "current speed of ship" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Inputs", "shipMaxSpeed", "default maxSpeed for ship" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi", "Pi value" ) );
	result.push_back( Tr2ExpressionTermInfo::Variable( "Math", "pi2", "Pi x 2 value" ) );
	return result;
}

BlueStdResult EveSmartLightAttributeModifierExpressionBucket::EvaluateExpression( const char* expression, float& value ) const
{
	CcpParser::Variable s_variables[] = {
		{ "time", 0, offsetof( Arguments, m_time ) },
		{ "shipSpeed", 0, offsetof( Arguments, m_shipSpeed ) },
		{ "shipMaxSpeed", 0, offsetof( Arguments, m_shipMaxSpeed ) }
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

void EveSmartLightAttributeModifierExpressionBucket::SetControllerVariable( const char* name, float value )
{
	// todo: Maybe set a usable expression variable

	for( auto modifier : m_attributeModifiers )
	{
		modifier->SetControllerVariable( name, value );
	}
}

void EveSmartLightAttributeModifierExpressionBucket::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier )
{
	m_finalActivationStrength = activationMultiplier * m_activationStrength * m_activationValue;
	m_playTime += updateContext.GetDeltaT();

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->UpdateSyncronous( updateContext, params, activationMultiplier );
	}

	if( m_expression.empty() )
	{
		return;
	}

	m_arguments.m_time = m_playTime;
	m_arguments.m_shipSpeed = ShipSpeed( params.spaceObjectParent );
	m_arguments.m_shipMaxSpeed = params.ownerMaxSpeed;

	if( m_program )
	{
		auto self = this;
		void* buffers[] = { (void*)&m_arguments, (void*)&self };
		m_activationStrength = m_program.Eval( buffers, m_tempArena.get() );
	}
}
