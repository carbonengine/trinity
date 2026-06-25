// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "EveSmartLightBaseAttributeModifier.h"
#include "IEveSmartLightGroupAttributeModifier.h"
#include "EveSmartLightAttributeModifierBucket.h"

#include "ccpparser.h"

BLUE_DECLARE_IVECTOR( ITriScalarFunction );
BLUE_DECLARE( Tr2ExpressionTermInfo );

BLUE_CLASS( EveSmartLightAttributeModifierExpressionBucket ) :
	public EveSmartLightAttributeModifierBucket
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightAttributeModifierExpressionBucket( IRoot* lockobj = nullptr );

	void SetControllerVariable( const char* name, float value ) override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, float activationMultiplier ) override;

	// IInitialize
	bool Initialize() override;

	std::string GetExpression() const;
	void SetExpression( const std::string& expression );
	float GetRandomConstant() const;
	float GetInputValue( int index ) const;
	float GetInputValue( int index, float time ) const;
	void ResetRandomConstant();

	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
	BlueStdResult EvaluateExpression( const char* expression, float& value ) const;

private:
	std::string m_name;
	std::string m_expression;

	CcpParser::Program m_program;
	std::unique_ptr<uint8_t[]> m_tempArena;

	PITriScalarFunctionVector m_inputs;

	float m_randomConstant;

	struct Arguments
	{
		mutable float m_time = 0;
		mutable float m_shipSpeed = 0;
		mutable float m_shipMaxSpeed = 0;
	} m_arguments;
};

TYPEDEF_BLUECLASS( EveSmartLightAttributeModifierExpressionBucket );
