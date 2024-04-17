////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionAnimateCurveSet.h"
#include "Controllers/Tr2Controller.h"
#include "Controllers/Tr2ControllerFloatVariable.h"
#include "Curves/TriCurveSet.h"
#include "Tr2ExpressionTermInfo.h"


namespace
{
	float StateTime( float* stateTime )
	{
		return *stateTime;
	}

	CcpParser::Function s_extraFunctions[] = {
		CcpParser::Function( "StateTime", StateTime, Tr2ControllerExpression::EXTRA_BUFFER_INDEX, 0 ),
	};
	}


Tr2ActionAnimateCurveSet::Tr2ActionAnimateCurveSet( IRoot* )
	:m_controller( nullptr ),
	m_value( "StateTime()" ),
	m_startTime( 0 ),
	m_lastSimTime( 0 )
{
}

void Tr2ActionAnimateCurveSet::Link( Tr2Controller& controller )
{
	m_controller = &controller;
	m_evaluator.SetExpr( m_value.c_str(), controller, s_extraFunctions );
}

void Tr2ActionAnimateCurveSet::Unlink()
{
	m_controller = nullptr;
	m_evaluator.Clear();
}

void Tr2ActionAnimateCurveSet::Start( Tr2Controller& controller )
{
	if( !m_curveSet )
	{
		return;
	}
	m_startTime = BeOS->GetCurrentFrameTime();
	controller.RegisterUpdateable( *this );
}

void Tr2ActionAnimateCurveSet::Stop( Tr2Controller& controller )
{
	controller.UnRegisterUpdateable( *this );
}

void Tr2ActionAnimateCurveSet::RebaseSimTime( Be::Time diff )
{
	m_startTime += diff;
	m_lastSimTime += diff;
}

void Tr2ActionAnimateCurveSet::Update( Be::Time realTime, Be::Time simTime )
{
	m_lastSimTime = simTime;
	if( !m_curveSet )
	{
		return;
	}
	float stateTime = TimeAsFloat( simTime - m_startTime );
	auto timePtr = &stateTime;
	auto value = m_evaluator.Eval( &timePtr );
	if( value.first )
	{
		m_curveSet->ApplyTime( value.second );
	}
}

bool Tr2ActionAnimateCurveSet::OnModified( Be::Var* value )
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

bool Tr2ActionAnimateCurveSet::IsExpressionValid() const
{
	return m_evaluator.IsExpressionValid();
}

bool Tr2ActionAnimateCurveSet::IsAttrExpressionValid( const char* ) const
{
	return IsExpressionValid();
}

std::vector<Tr2ExpressionTermInfoPtr> Tr2ActionAnimateCurveSet::GetExpressionTermInfo() const
{
	std::vector<Tr2ExpressionTermInfoPtr> result;
	m_evaluator.GetExpressionTermInfo( result );

	if( m_controller )
	{
		auto& variables = m_controller->GetVariables();
		for( auto it = begin( variables ); it != end( variables ); ++it )
		{
			result.push_back( Tr2ExpressionTermInfo::Variable( "Variables", ( *it )->GetName().c_str(), "controller variable" ) );
		}
	}
	return result;
}

BlueStdResult Tr2ActionAnimateCurveSet::EvaluateExpression( const char* expression, float& value ) const
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
	float stateTime = TimeAsFloat( m_lastSimTime - m_startTime );
	auto timePtr = &stateTime;
	auto result = expr.Eval( &timePtr );
	if( !result.first )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "error evaluating expression" );
	}
	value = result.second;
	return BlueStdResult();
}