// Copyright © 2018 CCP ehf.

#pragma once

#include <ccpparser.h>
#include "Tr2BindingPoint.h"


BLUE_DECLARE( Tr2StateMachine );
BLUE_DECLARE_INTERFACE( ITr2ActionController );
BLUE_DECLARE( Tr2ExpressionTermInfo );


class Tr2ControllerExpression
{
public:
	Tr2ControllerExpression();

	std::string SetExpr( const char* expression, const Tr2StateMachine& stateMachine );
	std::string SetExpr( const char* expression, const ITr2ActionController& controller, const CcpParser::FunctionView& extraFunctions = {} );
	std::pair<bool, float> Eval( void* extraBuffer = nullptr ) const;
	void Clear();
	bool IsExpressionValid() const;
	uint64_t GetVariableMask() const;

	void GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const;

	static const uint32_t OWNER_BUFFER_INDEX = 1;
	static const uint32_t STATE_MACHINE_BUFFER_INDEX = 2;
	static const uint32_t EXTRA_BUFFER_INDEX = 3;
	static const uint32_t REFERENCE_BUFFER_INDEX = 4;

private:
	struct Reference
	{
		std::string path;
		std::string name;
		std::unique_ptr<Tr2BindingPoint> binding;
	};

	std::string CreateParser( const char* expression, const CcpParser::FunctionView& extraFunctions );
	std::string BindReferences( const char* expression, std::string& rewritten );
	std::string AddReference( const std::string& path );
	void ClearReferences();

	CcpParser::Program m_program;
	std::vector<Reference> m_references;
	std::vector<CcpParser::Variable> m_referenceVariables;
	mutable std::vector<float> m_referenceValues;

	const Tr2StateMachine* m_stateMachine;
	const ITr2ActionController* m_controller;
	uint64_t m_variableMask;
};
