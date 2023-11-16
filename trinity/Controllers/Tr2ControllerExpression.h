////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include <ccpparser.h>


BLUE_DECLARE( Tr2StateMachine );
BLUE_DECLARE( Tr2Controller );
BLUE_DECLARE( Tr2ExpressionTermInfo );


class Tr2ControllerExpression
{
public:
	Tr2ControllerExpression();

	std::string SetExpr( const char* expression, const Tr2StateMachine& stateMachine );
	std::string SetExpr( const char* expression, const Tr2Controller& controller, const CcpParser::FunctionView& extraFunctions = {} );
	std::pair<bool, float> Eval( void* extraBuffer = nullptr ) const;
	void Clear();
	bool IsExpressionValid() const;
	uint64_t GetVariableMask() const;

	void GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const;

	static const uint32_t OWNER_BUFFER_INDEX = 1;
	static const uint32_t STATE_MACHINE_BUFFER_INDEX = 2;
	static const uint32_t EXTRA_BUFFER_INDEX = 3;

private:
	std::string CreateParser( const char* expression, const CcpParser::FunctionView& extraFunctions );

	CcpParser::Program m_program;

	const Tr2StateMachine* m_stateMachine;
	const Tr2Controller* m_controller;
	uint64_t m_variableMask;
};
