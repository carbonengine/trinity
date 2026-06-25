// Copyright © 2019 CCP ehf.

#pragma once

#include "Include/ITr2Updateable.h"
#include "ITr2ControllerAction.h"
#include "Controllers/Tr2ControllerExpression.h"

#include <ITriFunction.h>
#include <ITr2AudEmitter.h>


BLUE_DECLARE( Tr2ExpressionTermInfo );


BLUE_CLASS( Tr2ActionBindRTPC ) :
	public ITr2ControllerAction,
	public ITr2Updateable,
	public INotify
{
public:
	Tr2ActionBindRTPC( IRoot* = nullptr );

	EXPOSE_TO_BLUE();

	void Link( ITr2ActionController & controller ) override;
	void Unlink() override;
	void Start( ITr2ActionController & controller ) override;
	void StartWithController( ITr2ActionController * controller );
	void Stop( ITr2ActionController & controller ) override;
	void StopWithController( ITr2ActionController * controller );

	virtual void Update( Be::Time realTime, Be::Time simTime );

	virtual bool OnModified( Be::Var * value );

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
	const ITr2ActionController* m_controller;

	Be::Time m_startTime;
	Be::Time m_lastSimTime;
};

TYPEDEF_BLUECLASS( Tr2ActionBindRTPC );
