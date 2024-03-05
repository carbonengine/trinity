////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2ActionSetExternalControllerVariable.h"
#include "Controllers/Tr2Controller.h"
#include "Tr2ExpressionTermInfo.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Controllers/Tr2ControllerFloatVariable.h"

BLUE_DECLARE_INTERFACE( ITr2ControllerOwner );



Tr2ActionSetExternalControllerVariable::Tr2ActionSetExternalControllerVariable( IRoot* ) :
	m_destination( nullptr ),
	m_controller( nullptr ),
	m_value( 0.0 ),
	m_startControllers( false )
{
}

void Tr2ActionSetExternalControllerVariable::Link( Tr2Controller& controller )
{
	m_controller = &controller;
	LinkToDestinationOwner();
}

void Tr2ActionSetExternalControllerVariable::Unlink()
{
	m_controller = nullptr;
	m_destination = nullptr;
}

void Tr2ActionSetExternalControllerVariable::Start( Tr2Controller& controller )
{
	if( !IsDestinationValid() )
	{
		LinkToDestinationOwner();
	}

	if( IsDestinationValid() )
	{
		auto target = static_cast<IRoot*>( m_destination );
		ITr2ControllerOwnerPtr dest = BlueCastPtr( target );
		if( !dest )
		{
			return;
		}
		if( m_startControllers )
		{
			dest->StartControllers();
		}
		float value = m_value;
		if( !m_sourceVariable.empty() )
		{
			if( auto var = m_controller->GetVariableByName( m_sourceVariable.c_str() ) )
			{
				value = var->GetValue();
			}
		}

		dest->SetControllerVariable( m_variable.c_str(), value );
	}
}

bool Tr2ActionSetExternalControllerVariable::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_destinationOwner ) )
	{
		LinkToDestinationOwner();
	}
	return true;
}

bool Tr2ActionSetExternalControllerVariable::IsDestinationValid() const
{
	return m_destination != nullptr;
}

void Tr2ActionSetExternalControllerVariable::LinkToDestinationOwner()
{
	m_destination = nullptr;

	if( m_controller == nullptr )
	{
		return;
	}
	std::unordered_map<std::string, IRoot*> bindingPathRoots;
	ITr2ControllerOwnerPtr owner = BlueCastPtr( m_controller->GetOwner() );
	if( !owner )
	{
		return;
	}

	ITr2ControllerOwnerPtr pappy = BlueCastPtr( owner->GetRootObject() );
	if( !pappy )
	{
		return;
	}
	owner->GetBindingRoots( bindingPathRoots );
	std::string destOwner = m_destinationOwner.c_str();
	std::transform( destOwner.begin(), destOwner.end(), destOwner.begin(), ::tolower );

	for( auto& it : bindingPathRoots )
	{
		auto key = it.first;
		std::transform( key.begin(), key.end(), key.begin(), ::tolower );
		if( key == destOwner )
		{
			if( ITr2ControllerOwnerPtr target = BlueCastPtr( it.second ) )
			{
				m_destination = target.p;
				return;
			}
		}
	}
}
