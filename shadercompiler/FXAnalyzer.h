#pragma once

class ASTNode;
struct Sampler;
class ParserState;
struct Type;

union ExpressionValueElement
{
	long intValue;
	double floatValue;
};

typedef std::vector<ExpressionValueElement> ExpressionValue;

struct StateValue
{
	const char* name;
	unsigned value;
};

enum StateValueType
{
	SVT_BYTE,
	SVT_DWORD,
	SVT_FLOAT,
	SVT_BOOL,
	SVT_COLOR,
};

struct StateDescription
{
	const char* stateName;
	StateValueType type;
	StateValue* stateValues;
	unsigned offset;
};

extern StateValue g_renderStateNames[];
extern StateDescription g_samplerStates[];
extern StateDescription g_renderStates[];

bool ParseStateAssignment( ParserState& parserState, ASTNode* state, StateDescription* states, void* dataBlob );
bool GetSamplerState( ParserState& state, ASTNode* node, Sampler& sampler );

bool EvaluateExpression( ParserState& state, ASTNode* node, Type& type, ExpressionValue& value, StateValue* stateValues );
bool EvaluateInitializer( ParserState& state, ASTNode* node, Type& type, ExpressionValue& value, StateValue* stateValues );
bool CastExpressionValue( ExpressionValue& value, const Type& from, const Type& to );
int EvaluateIntegerExpression( ParserState& state, ASTNode* node, int defaultValue );