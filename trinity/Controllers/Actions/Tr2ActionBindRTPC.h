#pragma once
////////////////////////////////////////////////////////////
//
//    Created:   September 2019
//    Copyright: CCP 2019
//

#include "Include/ITr2Updateable.h"
#include "Include/ITriFunction.h"
#include "ITr2ControllerAction.h"
#include "Controllers/Tr2ControllerExpression.h"
#include "Audio/ITr2AudEmitter.h"


BLUE_DECLARE( Tr2ExpressionTermInfo );


BLUE_CLASS( Tr2ActionBindRTPC ) :
	public ITr2ControllerAction,
	public ITr2Updateable,
	public INotify
{
public:
	Tr2ActionBindRTPC( IRoot* = nullptr );

	EXPOSE_TO_BLUE();

	virtual void Link( Tr2Controller& controller );
	virtual void Unlink();
	virtual void Start( Tr2Controller& controller );
	void StartWithController( PyObject * controller );
	virtual void Stop( Tr2Controller& controller );
	void StopWithController( PyObject * controller );

	virtual void Update( Be::Time realTime, Be::Time simTime );

	virtual bool OnModified( Be::Var* value );

	bool IsExpressionValid() const;

	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
	BlueStdResult EvaluateExpression( const char* expression, float& value ) const;
	float GetCurveValue( float time ) const;
private:
	bool IsAttrExpressionValid( const char* attributeName ) const;

	BlueSharedString m_value;
	BlueSharedString m_emitterName;
	std::wstring m_rtpcName;
	ITriScalarFunctionPtr m_curve;

	Tr2ControllerExpression m_evaluator;
	ITr2AudEmitterPtr m_emitter;
	const Tr2Controller* m_controller;

	Be::Time m_startTime;
	Be::Time m_lastSimTime;
};

TYPEDEF_BLUECLASS( Tr2ActionBindRTPC );
