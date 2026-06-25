// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "ITr2SoundEmitterOwner.h"
#include "Tr2ActionBindRTPC.h"
#include "Tr2ExpressionTermInfo.h"
#include "Controllers/Tr2Controller.h"
#include "Controllers/Tr2ControllerFloatVariable.h"


namespace
{
struct ExtraBuffer
{
	const Tr2ActionBindRTPC* action = nullptr;
	float* stateTime = nullptr;
};

float StateTime( float* stateTime )
{
	return *stateTime;
}

float Curve( Tr2ActionBindRTPC* action, float time )
{
	if( !action )
	{
		return 0;
	}
	return action->GetCurveValue( time );
}

CcpParser::Function s_extraFunctions[] = {
	CcpParser::Function( "StateTime", StateTime, Tr2ControllerExpression::EXTRA_BUFFER_INDEX, offsetof( ExtraBuffer, stateTime ) ),
	CcpParser::Function( "Curve", Curve, Tr2ControllerExpression::EXTRA_BUFFER_INDEX, offsetof( ExtraBuffer, action ) ),
};

}


Tr2ActionBindRTPC::Tr2ActionBindRTPC( IRoot* ) :
	m_controller( nullptr ),
	m_value( "" ),
	m_startTime( 0 ),
	m_lastSimTime( 0 )
{
}

void Tr2ActionBindRTPC::Link( ITr2ActionController& controller )
{
	m_controller = &controller;
	m_evaluator.SetExpr( m_value.c_str(), controller, s_extraFunctions );
}

void Tr2ActionBindRTPC::Unlink()
{
	m_controller = nullptr;
	m_evaluator.Clear();
}

void Tr2ActionBindRTPC::Start( ITr2ActionController& controller )
{
	m_startTime = BeOS->GetCurrentFrameTime();
	controller.RegisterUpdateable( *this );

	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			m_emitter = emitter;
		}
	}
}

void Tr2ActionBindRTPC::StartWithController( ITr2ActionController* controller )
{
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StartWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Start( *controller );
}

void Tr2ActionBindRTPC::Stop( ITr2ActionController& controller )
{
	controller.UnRegisterUpdateable( *this );
}

void Tr2ActionBindRTPC::StopWithController( ITr2ActionController* controller )
{
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StopWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Stop( *controller );
}

void Tr2ActionBindRTPC::Update( Be::Time realTime, Be::Time simTime )
{
	// Handle logic necessary in order to use simulated time in the expression.
	m_lastSimTime = simTime;
	float time = TimeAsFloat( simTime - m_startTime );
	ExtraBuffer buffer = { this, &time };
	auto value = m_evaluator.Eval( &buffer );

	if( value.first && m_emitter )
	{
		m_emitter->SetRTPC( m_rtpcName, value.second );
	}
}

bool Tr2ActionBindRTPC::OnModified( Be::Var* value )
{
	if( !m_controller )
	{
		return true;
	}
	if( IsMatch( value, m_value ) )
	{
		m_evaluator.SetExpr( m_value.c_str(), *m_controller, s_extraFunctions );
	}
	return true;
}

bool Tr2ActionBindRTPC::IsExpressionValid() const
{
	return m_evaluator.IsExpressionValid();
}

bool Tr2ActionBindRTPC::IsAttrExpressionValid( const char* ) const
{
	return IsExpressionValid();
}

std::vector<Tr2ExpressionTermInfoPtr> Tr2ActionBindRTPC::GetExpressionTermInfo() const
{
	std::vector<Tr2ExpressionTermInfoPtr> result;
	m_evaluator.GetExpressionTermInfo( result );

	if( m_controller )
	{
		m_controller->GetExpressionTermInfo( result );
	}
	return result;
}

BlueStdResult Tr2ActionBindRTPC::EvaluateExpression( const char* expression, float& value ) const
{
	if( !m_controller )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "controller needs to be running when evaluating expressions" );
	}
	Tr2ControllerExpression expr;
	auto error = expr.SetExpr( expression, *m_controller, s_extraFunctions );
	if( !error.empty() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, error.c_str() );
	}
	float time = TimeAsFloat( m_lastSimTime - m_startTime );
	ExtraBuffer buffer = { this, &time };
	auto result = expr.Eval( &buffer );
	if( !result.first )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "error evaluating expression" );
	}
	value = result.second;
	return BlueStdResult();
}

float Tr2ActionBindRTPC::GetCurveValue( float time ) const
{
	if( !m_curve )
	{
		return 0;
	}
	return m_curve->GetValueAt( time );
}
