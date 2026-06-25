// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetValue.h"
#include "Controllers/Tr2Controller.h"
#include "Tr2ExpressionTermInfo.h"
#include "Controllers/Tr2ControllerFloatVariable.h"


Tr2ActionSetValue::Tr2ActionSetValue( IRoot* ) :
	m_controller( nullptr ),
	m_delayBinding( false )
{
}

void Tr2ActionSetValue::Link( ITr2ActionController& controller )
{
	m_controller = &controller;
	if( !HasDelayedBinding() )
	{
		LinkDestination( controller );
	}
	m_evaluator.SetExpr( m_value.c_str(), controller );
}

void Tr2ActionSetValue::Unlink()
{
	m_controller = nullptr;
	m_destination.Unlink();
	m_evaluator.Clear();
}

void Tr2ActionSetValue::Start( ITr2ActionController& controller )
{
	if( HasDelayedBinding() )
	{
		LinkDestination( controller );
	}
	if( !m_destination.IsValid() )
	{
		return;
	}
	auto value = m_evaluator.Eval();
	if( !value.first )
	{
		return;
	}
	m_destination.SetValue( value.second );
}

bool Tr2ActionSetValue::OnModified( Be::Var* value )
{
	if( !m_controller )
	{
		return true;
	}
	if( IsMatch( value, m_destination.m_path ) || IsMatch( value, m_destination.m_attribute ) || IsMatch( value, m_destination.m_object ) || IsMatch( value, m_delayBinding ) )
	{
		if( !HasDelayedBinding() )
		{
			LinkDestination( *m_controller );
		}
	}
	else if( IsMatch( value, m_value ) )
	{
		m_evaluator.SetExpr( m_value.c_str(), *m_controller );
	}
	return true;
}

bool Tr2ActionSetValue::IsBindingValid() const
{
	return m_destination.IsValid();
}

bool Tr2ActionSetValue::IsExpressionValid() const
{
	return m_evaluator.IsExpressionValid();
}

IRootPtr Tr2ActionSetValue::GetDestination() const
{
	return m_destination.GetBoundObject();
}

bool Tr2ActionSetValue::IsAttrExpressionValid( const char* ) const
{
	return IsExpressionValid();
}

std::vector<Tr2ExpressionTermInfoPtr> Tr2ActionSetValue::GetExpressionTermInfo() const
{
	std::vector<Tr2ExpressionTermInfoPtr> result;
	m_evaluator.GetExpressionTermInfo( result );

	if( m_controller )
	{
		m_controller->GetExpressionTermInfo( result );
	}
	return result;
}

void Tr2ActionSetValue::LinkDestination( const ITr2ActionController& controller )
{
	m_destination.Link( controller.GetBindingPathRoots() );
}

bool Tr2ActionSetValue::HasDelayedBinding() const
{
	return m_delayBinding && !m_destination.m_path.empty();
}

BlueStdResult Tr2ActionSetValue::EvaluateExpression( const char* expression, float& value ) const
{
	if( !m_controller )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "controller needs to be running when evaluating expressions" );
	}
	Tr2ControllerExpression expr;
	auto error = expr.SetExpr( expression, *m_controller );
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