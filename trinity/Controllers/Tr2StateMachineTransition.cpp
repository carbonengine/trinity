////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2StateMachineTransition.h"
#include "Tr2StateMachineState.h"
#include "Tr2StateMachine.h"
#include "Tr2ExpressionTermInfo.h"
#include "Tr2Controller.h"
#include "Tr2ControllerFloatVariable.h"


Tr2StateMachineTransition::Tr2StateMachineTransition( IRoot* lockobj )
	:m_source( nullptr )
{
}

bool Tr2StateMachineTransition::OnModified( Be::Var* value )
{
	if( !m_source )
	{
		return true;
	}

	if( IsMatch( value, m_condition ) )
	{
		m_evaluator.SetExpr( m_condition.c_str(), *m_source->GetStateMachine() );
		if( m_source )
		{
			m_source->UpdateVariableMask();
		}
	}
	else if( IsMatch( value, m_destinationName ) )
	{
		UpdateDestination();
	}
	return true;
}

void Tr2StateMachineTransition::Link( const Tr2StateMachineState& state )
{
	Unlink();

	m_source = &state;
	if( m_source->GetStateMachine() )
	{
		UpdateDestination();
	}
	m_evaluator.SetExpr( m_condition.c_str(), *m_source->GetStateMachine() );
}

void Tr2StateMachineTransition::Unlink()
{
	m_source = nullptr;
	m_destination = nullptr;
	m_evaluator.Clear();
}

Tr2StateMachineState* Tr2StateMachineTransition::GetDestination() const
{
	return m_destination;
}

void Tr2StateMachineTransition::UpdateDestination()
{
	m_destination = m_source->GetStateMachine()->GetStateByName( m_destinationName.c_str() );
	if( !m_destination )
	{
		CCP_LOGERR( "Invalid destination state name %s for state machine transition", m_destinationName.c_str() );
	}
}

bool Tr2StateMachineTransition::CanActivate( uint64_t variableDirtyMask ) const
{
	if( !m_source )
	{
		return false;
	}
	auto variableMask = m_evaluator.GetVariableMask();
	if( variableMask != 0 && ( ( variableMask & variableDirtyMask ) == 0 ) )
	{
		return false;
	}
	auto result = m_evaluator.Eval();
	if( !result.first )
	{
		return false;
	}
	return result.second != 0;
}

uint64_t Tr2StateMachineTransition::GetVariableMask() const
{
	return m_evaluator.GetVariableMask();
}

IRoot* Tr2StateMachineTransition::GetSource() const
{
	return m_source->GetRawRoot();
}

bool Tr2StateMachineTransition::IsConditionValid() const
{
	return m_evaluator.IsExpressionValid();
}

bool Tr2StateMachineTransition::IsExpressionValid( const char* ) const
{
	return IsConditionValid();
}

std::vector<Tr2ExpressionTermInfoPtr> Tr2StateMachineTransition::GetExpressionTermInfo() const
{
	std::vector<Tr2ExpressionTermInfoPtr> result;
	m_evaluator.GetExpressionTermInfo( result );

	if( m_source )
	{
		auto controller = m_source->GetStateMachine()->GetController();
		auto& variables = controller->GetVariables();
		for( auto it = begin( variables ); it != end( variables ); ++it )
		{
			result.push_back( Tr2ExpressionTermInfo::Variable( "Variables", ( *it )->GetName().c_str(), "controller variable" ) );
		}
	}
	return result;
}

BlueStdResult Tr2StateMachineTransition::EvaluateExpression( const char* expression, float& value ) const
{
	if( !m_source )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "controller needs to be running when evaluating expressions" );
	}
	Tr2ControllerExpression expr;
	auto error = expr.SetExpr( expression, *m_source->GetStateMachine() );
	if( !error.empty() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, error.c_str() );
	}
	auto result = expr.Eval();
	if( !result.first )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "error evaluating expression" );
	}
	value = result.second;
	return BlueStdResult();
}