////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#pragma once

#include "ITr2ControllerAction.h"
#include "Controllers/Tr2BindingPoint.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Controllers/Tr2ControllerExpression.h"


BLUE_DECLARE( Tr2ExpressionTermInfo );


BLUE_CLASS( Tr2ActionSetExternalControllerVariable ) :
	public ITr2ControllerAction,
	public INotify
{
public:
	Tr2ActionSetExternalControllerVariable( IRoot* = nullptr );

	EXPOSE_TO_BLUE();

	virtual void Link( Tr2Controller& controller );
	virtual void Unlink();
	virtual void Start( Tr2Controller& controller );

	virtual bool OnModified( Be::Var* value );

	bool IsDestinationValid() const;
	bool IsVariableValid() const;
private:
	void LinkToDestinationOwner();

	BlueSharedString m_destinationOwner;
	BlueWeakRef<IRoot> m_destination;
	BlueSharedString m_sourceVariable;
	BlueSharedString m_variable;
	float m_value;
	bool m_startControllers;

	const Tr2Controller* m_controller;
	Tr2ControllerExpression m_evaluator;
};
TYPEDEF_BLUECLASS( Tr2ActionSetExternalControllerVariable );
