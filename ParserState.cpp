#include "stdafx.h"
#include "ParserState.h"
#include "CompileMessageQueue.h"
#include "SymbolTable.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "CachingIncludeHandler.h"
#include "ParserUtils.h"
#include "Preprocessor.h"
#include "IntrinsicTypes.h"

extern CompileMessageQueue g_messages;

void *ParseAlloc( void *(*mallocProc)(size_t) );
void ParseFree( void *p, void (*freeProc)(void*) );
void Parse( void *yyp, int yymajor, ScannerToken token, ParserState* state );
void ParseTrace(FILE *TraceFILE, char *zTracePrompt);

void *PreprocessorParseAlloc( void *(*mallocProc)(size_t) );
void PreprocessorParseFree( void *p, void (*freeProc)(void*) );
void PreprocessorParse( void *yyp, int yymajor, ScannerToken token, ParserState* state );
void PreprocessorParseTrace(FILE *TraceFILE, char *zTracePrompt);

void ParserState::AddIntrinsics()
{
	SymbolTable& table = *m_symbols;
	m_dx9Functions["abs"] = AddFunction( table, "abs", &FunctionDescription1<TypeSameAsArg<0>, DimSameAsArg<0>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["acos"] = AddFunction( table, "acos", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["all"] = AddFunction( table, "all", &FunctionDescription1<TypeIs<OP_BOOL>, DimIs<1, 1>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["AllMemoryBarrier"] = AddFunction( table, "AllMemoryBarrier", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["AllMemoryBarrierWithGroupSync"] = AddFunction( table, "AllMemoryBarrierWithGroupSync", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["any"] = AddFunction( table, "any", &FunctionDescription1<TypeIs<OP_BOOL>, DimIs<1, 1>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["asdouble"] = AddFunction( table, "asdouble", &FunctionDescription2<TypeIs<OP_DOUBLE>, DimIs<1, 1>, TypeIs<OP_UINT>, DimIs<1, 1>, TypeIs<OP_UINT>, DimIs<1, 1>> );
	m_dx9Functions["asfloat"] = AddFunction( table, "asfloat", &FunctionDescription1<TypeIs<OP_FLOAT>, DimIs<2, 1>, TypeIs<OP_DOUBLE>, DimIs<1, 1>> );
	m_dx9Functions["asin"] = AddFunction( table, "asin", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["asint"] = AddFunction( table, "asint", &FunctionDescription1<TypeIs<OP_INT>, DimIs<2, 1>, TypeIs<OP_DOUBLE>, DimIs<1, 1>> );
	m_dx9Functions["asuint"] = AddFunction( table, "asuint", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_DOUBLE>, DimIs<1, 1>, TypeIs<OP_UINT>, DimIs<1, 1>, TypeIs<OP_UINT>, DimIs<1, 1>> );
	m_dx9Functions["atan"] = AddFunction( table, "atan", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["atan2"] = AddFunction( table, "atan2", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["ceil"] = AddFunction( table, "ceil", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["clamp"] = AddFunction( table, "clamp", &FunctionDescription3<CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim> );
	m_dx9Functions["clip"] = AddFunction( table, "clip", &FunctionDescription1<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["cos"] = AddFunction( table, "cos", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );

	m_dx9Functions["cosh"] = AddFunction( table, "cosh", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["countbits"] = AddFunction( table, "countbits", &FunctionDescription1<TypeIs<OP_UINT>, DimSameAsArg<0>, TypeIs<OP_UINT>, DimSameAsArg<0>> );
	m_dx9Functions["cross"] = AddFunction( table, "cross", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>> );
	m_dx9Functions["D3DCOLORtoUBYTE4"] = AddFunction( table, "D3DCOLORtoUBYTE4", &FunctionDescription1<TypeIs<OP_INT>, DimIs<4, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> );
	m_dx9Functions["ddx"] = AddFunction( table, "ddx", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ddx_coarse"] = AddFunction( table, "ddx_coarse", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ddx_fine"] = AddFunction( table, "ddx_fine", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ddy"] = AddFunction( table, "ddy", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ddy_coarse"] = AddFunction( table, "ddy_coarse", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ddy_fine"] = AddFunction( table, "ddy_fine", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["degrees"] = AddFunction( table, "degrees", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["determinant"] = AddFunction( table, "determinant", &FunctionDescription1<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["DeviceMemoryBarrier"] = AddFunction( table, "DeviceMemoryBarrier", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["DeviceMemoryBarrierWithGroupSync"] = AddFunction( table, "DeviceMemoryBarrierWithGroupSync", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["distance"] = AddFunction( table, "distance", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["dot"] = AddFunction( table, "dot", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["dst"] = AddFunction( table, "dst", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["EvaluateAttributeAtCentroid"] = AddFunction( table, "EvaluateAttributeAtCentroid", &FunctionDescription1<TypeSameAsArg<0>, DimSameAsArg<0>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["EvaluateAttributeAtSample"] = AddFunction( table, "EvaluateAttributeAtSample", &FunctionDescription1<TypeSameAsArg<0>, DimSameAsArg<0>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["EvaluateAttributeSnapped"] = AddFunction( table, "EvaluateAttributeSnapped", &FunctionDescription1<TypeSameAsArg<0>, DimSameAsArg<0>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["exp"] = AddFunction( table, "exp", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["exp2"] = AddFunction( table, "exp2", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["f16tof32"] = AddFunction( table, "f16tof32", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_UINT>, DimSameAsArg<0>> );
	m_dx9Functions["f32tof16"] = AddFunction( table, "f32tof16", &FunctionDescription1<TypeIs<OP_UINT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["faceforward"] = AddFunction( table, "faceforward", &FunctionDescription3<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );

	m_dx9Functions["firstbithigh"] = AddFunction( table, "firstbithigh", &FunctionDescription1<TypeIs<OP_INT>, DimSameAsArg<0>, TypeIs<OP_INT>, DimSameAsArg<0>> );
	m_dx9Functions["firstbitlow"] = AddFunction( table, "firstbitlow", &FunctionDescription1<TypeIs<OP_INT>, DimSameAsArg<0>, TypeIs<OP_INT>, DimSameAsArg<0>> );
	m_dx9Functions["floor"] = AddFunction( table, "floor", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["fmod"] = AddFunction( table, "fmod", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["frac"] = AddFunction( table, "frac", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["frexp"] = AddFunction( table, "frexp", &FunctionDescription2<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["fwidth"] = AddFunction( table, "fwidth", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["GetRenderTargetSampleCount"] = AddFunction( table, "GetRenderTargetSampleCount", &FunctionDescription0<TypeIs<OP_UINT>, DimIs<1, 1>> );
	m_dx9Functions["GetRenderTargetSamplePosition"] = AddFunction( table, "GetRenderTargetSamplePosition", &FunctionDescription1<TypeIs<OP_UINT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["GroupMemoryBarrier"] = AddFunction( table, "GroupMemoryBarrier", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["GroupMemoryBarrierWithGroupSync"] = AddFunction( table, "GroupMemoryBarrierWithGroupSync", &FunctionDescription0<TypeIs<OP_VOID>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedAdd"] = AddFunction( table, "InterlockedAdd", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedAnd"] = AddFunction( table, "InterlockedAnd", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedCompareExchange"] = AddFunction( table, "InterlockedCompareExchange", &FunctionDescription4<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedCompareStore"] = AddFunction( table, "InterlockedCompareStore", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedExchange"] = AddFunction( table, "InterlockedExchange", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedMax"] = AddFunction( table, "InterlockedMax", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedMin"] = AddFunction( table, "InterlockedMin", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedOr"] = AddFunction( table, "InterlockedOr", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["InterlockedXor"] = AddFunction( table, "InterlockedXor", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>, TypeIs<OP_INT>, DimIs<1, 1>> );
	m_dx9Functions["isfinite"] = AddFunction( table, "isfinite", &FunctionDescription1<TypeIs<OP_BOOL>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["isinf"] = AddFunction( table, "isinf", &FunctionDescription1<TypeIs<OP_BOOL>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["isnan"] = AddFunction( table, "isnan", &FunctionDescription1<TypeIs<OP_BOOL>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["ldexp"] = AddFunction( table, "ldexp", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["length"] = AddFunction( table, "length", &FunctionDescription1<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["lerp"] = AddFunction( table, "lerp", &FunctionDescription3<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["lit"] = AddFunction( table, "lit", &FunctionDescription3<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> );
	m_dx9Functions["log"] = AddFunction( table, "log", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );

	m_dx9Functions["log10"] = AddFunction( table, "log10", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["log2"] = AddFunction( table, "log2", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["mad"] = AddFunction( table, "mad", &FunctionDescription3<CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim> );
	m_dx9Functions["max"] = AddFunction( table, "max", &FunctionDescription2<CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim> );
	m_dx9Functions["min"] = AddFunction( table, "min", &FunctionDescription2<CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, CommonArgType, CommonArgDim> );
	m_dx9Functions["modf"] = AddFunction( table, "modf", &FunctionDescription2Ex<CommonArgType, CommonArgDim, CommonArgType, CommonArgDim, 0, CommonArgType, CommonArgDim, OP_OUT> );
	m_dx9Functions["mul"] = AddFunction( table, "mul", &MulIntrinsicType );
	m_dx9Functions["noise"] = AddFunction( table, "noise", &FunctionDescription1<TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["normalize"] = AddFunction( table, "normalize", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["pow"] = AddFunction( table, "pow", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	//m_dx9Functions["Process2DQuadTessFactorsAvg"] = AddFunction( table, "Process2DQuadTessFactorsAvg", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["Process2DQuadTessFactorsMax"] = AddFunction( table, "Process2DQuadTessFactorsMax", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["Process2DQuadTessFactorsMin"] = AddFunction( table, "Process2DQuadTessFactorsMin", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessIsolineTessFactors"] = AddFunction( table, "ProcessIsolineTessFactors", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessQuadTessFactorsAvg"] = AddFunction( table, "ProcessQuadTessFactorsAvg", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessQuadTessFactorsMax"] = AddFunction( table, "ProcessQuadTessFactorsMax", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessQuadTessFactorsMin"] = AddFunction( table, "ProcessQuadTessFactorsMin", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessTriTessFactorsAvg"] = AddFunction( table, "ProcessTriTessFactorsAvg", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessTriTessFactorsMax"] = AddFunction( table, "ProcessTriTessFactorsMax", &ConstType<OP_VOID, 1, 1> );
	//m_dx9Functions["ProcessTriTessFactorsMin"] = AddFunction( table, "ProcessTriTessFactorsMin", &ConstType<OP_VOID, 1, 1> );
	m_dx9Functions["radians"] = AddFunction( table, "radians", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["rcp"] = AddFunction( table, "rcp", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["reflect"] = AddFunction( table, "reflect", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["refract"] = AddFunction( table, "refract", &FunctionDescription3<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, DimIs<1, 1>> );

	m_dx9Functions["reversebits"] = AddFunction( table, "reversebits", &FunctionDescription1<TypeIs<OP_UINT>, DimSameAsArg<0>, TypeIs<OP_UINT>, DimSameAsArg<0>> );
	m_dx9Functions["round"] = AddFunction( table, "round", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["rsqrt"] = AddFunction( table, "rsqrt", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["saturate"] = AddFunction( table, "saturate", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["sign"] = AddFunction( table, "sign", &FunctionDescription1<TypeIs<OP_INT>, DimSameAsArg<0>, TypeSameAsArg<0>, DimSameAsArg<0>> );
	m_dx9Functions["sin"] = AddFunction( table, "sin", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["sincos"] = AddFunction( table, "sincos", &FunctionDescription3<TypeIs<OP_VOID>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["sinh"] = AddFunction( table, "sinh", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["smoothstep"] = AddFunction( table, "smoothstep", &FunctionDescription3<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["sqrt"] = AddFunction( table, "sqrt", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["step"] = AddFunction( table, "step", &FunctionDescription2<TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim, TypeIs<OP_FLOAT>, CommonArgDim> );
	m_dx9Functions["tan"] = AddFunction( table, "tan", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );
	m_dx9Functions["tanh"] = AddFunction( table, "tanh", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );

	m_dx9Functions["transpose"] = AddFunction( table, "transpose", &TransposeIntrinsicType );
	m_dx9Functions["trunc"] = AddFunction( table, "trunc", &FunctionDescription1<TypeIs<OP_FLOAT>, DimSameAsArg<0>, TypeIs<OP_FLOAT>, DimSameAsArg<0>> );

	m_dx9TextureFunctions.insert( AddFunction( table, "tex1D", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex1Dbias", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex1Dgrad", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex1Dlod", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex1Dproj", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex2D", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<2, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex2Dbias", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex2Dgrad", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<2, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex2Dlod", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex2Dproj", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex3D", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex3Dbias", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex3Dgrad", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex3Dlod", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "tex3Dproj", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "texCUBE", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "texCUBEbias", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "texCUBEgrad", &FunctionDescription4<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<3, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<1, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "texCUBElod", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );
	m_dx9TextureFunctions.insert( AddFunction( table, "texCUBEproj", &FunctionDescription2<TypeIs<OP_FLOAT>, DimIs<4, 1>, TypeIs<OP_SAMPLER>, DimIs<1, 1>, TypeIs<OP_FLOAT>, DimIs<4, 1>> ) );

	for( auto it = m_dx9TextureFunctions.begin(); it != m_dx9TextureFunctions.end(); ++it )
	{
		m_dx9Functions[( *it )->name.start] = *it;
	}
}


ParserState::ParserState( const InlineString& code )
	:m_newLine( true ),
	m_mode( HLSL ),
	m_symbols( nullptr ),
	m_root( nullptr ),
	m_offlineStatements( nullptr ),
	m_hasErrors( false ),
	m_expandMacros( true ),
	m_inPreprocessorCondition( false )
{
	FileContents fileContents;
	fileContents.location.fileName = MakeInlineString( "\\memory" );
	fileContents.location.lineNumber = 0;
	fileContents.code = code;
	fileContents.code.start--;
	fileContents.position = code.start - 1;
	fileContents.isMacro = false;
	m_fileStack.push_back( fileContents );

	m_newLine = true;

	m_symbols = new SymbolTable;
	m_mode = HLSL;

	m_offlineStatements = new ASTNode( NT_BLOCK, fileContents.location, nullptr, nullptr );

	AddIntrinsics();
}

ParserState::~ParserState()
{
	delete m_symbols;
	delete m_root;
	delete m_offlineStatements;
	for( auto it = m_strings.begin(); it != m_strings.end(); ++it )
	{
		delete[] *it;
	}
}

extern PreprocessorScanResult GetPreprocessorToken( ParserState &state, PreprocessorToken& token );
extern bool ConvertToScannerToken( ParserState &state, const PreprocessorToken& ppToken, ScannerToken& token );

bool ParserState::ParseMacroArguments( std::vector<InlineString>& arguments )
{
	std::vector<PreprocessorToken> argument;
	int parenDepth = 0;
	size_t depth = m_fileStack.size();
	while( true )
	{
		PreprocessorToken token;
		PreprocessorScanResult code = GetPreprocessorToken( token );
		if( code == PPSR_ERROR )
		{
			return false;
		}
		else if( code == PPSR_EOF )
		{
			if( depth < m_fileStack.size() )
			{
				m_fileStack.pop_back();
				continue;
			}
			ShowMessage( EC_UNEXPECTED_EOF );
			return false;
		}
		if( token.type == PPT_OPERATOR && token.string.end - token.string.start == 1 )
		{
			if( *token.string.start == ',' )
			{
				if( parenDepth == 0 )
				{
					arguments.push_back( MakeInlineString( argument.front().string.start, argument.back().string.end ) );
					argument.clear();
				}
				continue;
			}
			else if( *token.string.start == '(' )
			{
				parenDepth++;
			}
			else if( *token.string.start == ')' )
			{
				if( parenDepth == 0 )
				{
					arguments.push_back( MakeInlineString( argument.front().string.start, argument.back().string.end ) );
					return true;
				}
				else
				{
					parenDepth--;
				}
			}
		}
		argument.push_back( token );
	}
}

bool ParserState::InSkipMode() const
{
	if( m_inPreprocessorCondition )
	{
		return false;
	}

	if( m_prepocessorConditions.empty() )
	{
		return false;
	}
	return m_prepocessorConditions.back().inheridedSkipMode || m_prepocessorConditions.back().skipMode;
}

bool ParserState::FindConcatOperator( size_t depth )
{
	while( true )
	{
		const char* backup = GetStreamPosition();
		for( ; GetStreamPosition() < GetStreamEnd(); ++GetStreamPosition() )
		{
			switch( *GetStreamPosition() )
			{
			case ' ':
				continue;
			case '#':
				if( GetStreamPosition() + 1 < GetStreamEnd() && GetStreamPosition()[1] == '#' )
				{
					GetStreamPosition() += 2;
					return true;
				}
			default:
				GetStreamPosition() = backup;
				return false;
			}
		}
		if( m_fileStack.size() > depth )
		{
			m_fileStack.pop_back();
		}
		else
		{
			break;
		}
	}
	return false;
}

PreprocessorScanResult ParserState::GetPreprocessorToken( PreprocessorToken& token )
{
	do
	{
		FileContents& curFile = m_fileStack.back();
		PreprocessorScanResult code = ::GetPreprocessorToken( *this, token );
		if( code != PPSR_OK )
		{
			return code;
		}
	}
	while( InSkipMode() );

	FileContents& curFile = m_fileStack.back();

	if( token.type == PPT_ID && m_expandMacros )
	{
		PreprocessorDefine* found = FindDefine( token.string );
		if( found )
		{
			std::vector<InlineString> arguments;
			if( !found->parameters.empty() )
			{
				PreprocessorToken paren;
				const char* backup = GetStreamPosition();
				if( GetPreprocessorToken( paren ) != PPSR_OK || paren.type != PPT_OPERATOR || paren.string.end - paren.string.start != 1 || *paren.string.start != '(' )
				{
					GetStreamPosition() = backup;
					return PPSR_OK;
				}
				if( !ParseMacroArguments( arguments ) )
				{
					return PPSR_ERROR;
				}
				if( arguments.size() != found->parameters.size() )
				{
					ShowMessage( EC_INCORRECT_MACRO_PARAMETER_COUNT, ToString( token.string ).c_str() );
					return PPSR_ERROR;
				}
			}
			ParseDefine( *found, token.fileLocation, arguments );
			PreprocessorScanResult code = GetPreprocessorToken( token );
			if( code != PPSR_OK )
			{
				return code;
			}
		}
	}

	if( InMacro() )
	{
		size_t depth = m_fileStack.size();
		if( FindConcatOperator( depth ) )
		{
			std::string concatToken( token.string.start, token.string.end );
			do
			{
				PreprocessorToken nextToken;
				PreprocessorScanResult code = GetPreprocessorToken( nextToken );
				if( code != PPSR_OK )
				{
					return code;
				}
				concatToken.append( nextToken.string.start, nextToken.string.end );
			}
			while( FindConcatOperator( depth ) );
			char* string = AllocateString( concatToken.length() );
			memcpy( string, concatToken.c_str(), concatToken.length() );
			token.string = MakeInlineString( string, string + concatToken.length() );
		}
	}

	return PPSR_OK;
}

bool ParserState::Parse()
{
	//PreprocessorParseTrace( stdout, "parser: " );
	void* parser = ParseAlloc( malloc );

	m_symbols->EnterScope();
	
	ScannerToken token;

	bool done = false;
	while( !done )
	{
		PreprocessorToken ppToken;
		PreprocessorScanResult scanCode = GetPreprocessorToken( ppToken );
		switch( scanCode )
		{
		case PPSR_ERROR:
			ParseFree( parser, free );
			return false;
		case PPSR_EOF:
			{
				if( m_fileStack.size() > 1 )
				{
					m_fileStack.pop_back();
				}
				else
				{
					done = true;
					break;
				}
			}
			break;
		case PPSR_OK:
			if( !ConvertToScannerToken( *this, ppToken, token ) )
			{
				ShowMessage( ppToken.fileLocation, EC_SYNTAX_ERROR, ToString( ppToken.string ).c_str() );
				ParseFree( parser, free );
				return false;
			}
			//printf( "token: %04i %s\n", token.type, ToString( ppToken.string ).c_str() );
			::Parse( parser, token.type, token, this );
		}
	}

	token.type = 0;
	::Parse( parser, 0, token, this );
	ParseFree( parser, free );

	m_symbols->LeaveScope();

	return m_root && !m_hasErrors;
}

bool ConvertToPreprocessorToken( int& type )
{
	static int typeMap[256] = { 0 };
	static bool initialized = false;
	if( !initialized )
	{
		typeMap[OP_INT_CONST] = PP_INT_CONST;
		typeMap[OP_ID] = PP_ID;
		typeMap[OP_LEFT_PAREN] = PP_LEFT_PAREN;
		typeMap[OP_RIGHT_PAREN] = PP_RIGHT_PAREN;
		typeMap[OP_PLUS] = PP_PLUS;
		typeMap[OP_DASH] = PP_DASH;
		typeMap[OP_BANG] = PP_BANG;
		typeMap[OP_TILDE] = PP_TILDE;
		typeMap[OP_STAR] = PP_STAR;
		typeMap[OP_SLASH] = PP_SLASH;
		typeMap[OP_PERCENT] = PP_PERCENT;
		typeMap[OP_LEFT_OP] = PP_LEFT_OP;
		typeMap[OP_RIGHT_OP] = PP_RIGHT_OP;
		typeMap[OP_LESS] = PP_LESS;
		typeMap[OP_MORE] = PP_MORE;
		typeMap[OP_LE_OP] = PP_LE_OP;
		typeMap[OP_GE_OP] = PP_GE_OP;
		typeMap[OP_EQ_OP] = PP_EQ_OP;
		typeMap[OP_NE_OP] = PP_NE_OP;
		typeMap[OP_AMPERSAND] = PP_AMPERSAND;
		typeMap[OP_CARET] = PP_CARET;
		typeMap[OP_VERTICAL_BAR] = PP_VERTICAL_BAR;
		typeMap[OP_AND_OP] = PP_AND_OP;
		typeMap[OP_OR_OP] = PP_OR_OP;
		typeMap[OP_QUESTION] = PP_QUESTION;
		typeMap[OP_COLON] = PP_COLON;
		initialized = true;
	}
	if( typeMap[type] == 0 )
	{
		return false;
	}
	type = typeMap[type];
	return true;
}

PreprocessorScanResult ParserState::EvaluatePreprocessorCondition( const InlineString& string, bool& result )
{
	//PreprocessorParseTrace( stdout, "parser: " );
	void* preprocessor = PreprocessorParseAlloc( malloc );

	FileContents fileContents;
	fileContents.location = GetCurrentLocation();
	fileContents.code = string;
	fileContents.position = string.start;
	fileContents.isMacro = false;

	size_t depth = m_fileStack.size();
	m_fileStack.push_back( fileContents );

	m_preprocessorConditionResult = false;
	m_expandMacros = true;
	m_inPreprocessorCondition = true;
	ScannerToken token;
	bool done = false;
	while( !done )
	{
		PreprocessorToken ppToken;
		PreprocessorScanResult scanCode = GetPreprocessorToken( ppToken );
		switch( scanCode )
		{
		case PPSR_ERROR:
			return PPSR_ERROR;
		case PPSR_EOF:
			{
				m_fileStack.pop_back();
				if( m_fileStack.size() == depth )
				{
					done = true;
					break;
				}
			}
			break;
		case PPSR_OK:
			if( !ConvertToScannerToken( *this, ppToken, token ) )
			{
				ShowMessage( EC_SYNTAX_ERROR, ToString( ppToken.string ).c_str() );
				return PPSR_ERROR;
			}

			if( !ConvertToPreprocessorToken( token.type ) )
			{
				ShowMessage( EC_SYNTAX_ERROR, ToString( ppToken.string ).c_str() );
				return PPSR_ERROR;
			}
			PreprocessorParse( preprocessor, token.type, token, this );
		}
	}
	token.type = 0;
	PreprocessorParse( preprocessor, 0, token, this );
	PreprocessorParseFree( preprocessor, free );
	m_expandMacros = true;
	m_inPreprocessorCondition = false;

	result = m_preprocessorConditionResult != 0;

	return PPSR_OK;
}

bool ParserState::HasErrors() const
{
	return m_hasErrors;
}

ASTNode* ParserState::GetTree()
{
	return m_root;
}

SymbolTable& ParserState::GetSymbolTable()
{
	return *m_symbols;
}

FileLocation& ParserState::GetCurrentLocation()
{
	return m_fileStack.back().location;
}

InlineString ParserState::AllocateName()
{
	const char *format = "_new_symbol_%04i";
	size_t len = strlen( format ) + 1;
	char* newName = new char[len];
	sprintf_s( newName, len, format, m_strings.size() );
	m_strings.push_back( newName );
	return MakeInlineString( newName, newName + len - 1 );
}

char* ParserState::AllocateString( size_t size )
{
	char* string = new char[size];
	m_strings.push_back( string );
	return string;
}

void ParserState::ShowMessage( const FileLocation& location, ErrorCode errorCode, ... )
{
	va_list args;
	va_start( args, errorCode );
	ShowMessageImpl( location, errorCode, args );
}

void ParserState::ShowMessage( const ScannerToken& token, ErrorCode errorCode, ... )
{
	va_list args;
	va_start( args, errorCode );
	ShowMessageImpl( token.fileLocation, errorCode, args );
}

void ParserState::ShowMessage( ErrorCode errorCode, ... )
{
	va_list args;
	va_start( args, errorCode );
	ShowMessageImpl( GetCurrentLocation(), errorCode, args );
}

void ParserState::AddOfflineStatement( ASTNode* node )
{
	m_offlineStatements->AddChild( node );
}

void ParserState::MoveOfflineStatements( ASTNode* to )
{
	m_offlineStatements->MoveChildren( to );
}

void ParserState::SetTree( ASTNode* root )
{
	m_root = root;
}

const char*& ParserState::GetStreamPosition()
{
	return m_fileStack.back().position;
}

const char* ParserState::GetStreamEnd()
{
	return m_fileStack.back().code.end;
}

bool ParserState::InMacro() const
{
	return m_fileStack.back().isMacro;
}

void ParserState::IncludeFile( const InlineString& fileName )
{
	extern CachingIncludeHandler g_includeHandler;

	std::string path( fileName.start + 1, fileName.end - 1 );
	char* data;
	unsigned size;
	HRESULT hr = g_includeHandler.Open( 
		*fileName.start == '<' ? D3DXINC_SYSTEM : D3DXINC_LOCAL, 
		path.c_str(), 
		m_fileStack.back().code.start + 1, 
		(LPCVOID*)&data,
		&size );
	if( FAILED( hr ) )
	{
		ShowMessage( EC_FILE_NOT_FOUND, path.c_str() );
	}
	else
	{
		FileContents fileContents;
		char fullPathBuffer[MAX_PATH];
		if( FAILED( g_includeHandler.GetFullPathName( path.c_str(), m_fileStack.back().code.start + 1, fullPathBuffer ) ) )
		{
			fileContents.location.fileName.start = fileName.start + 1;
			fileContents.location.fileName.end = fileName.end - 1;
		}
		else
		{
			size_t len = strlen( fullPathBuffer );
			char* fullPath = AllocateString( len );
			memcpy( fullPath, fullPathBuffer, len );
			fileContents.location.fileName = MakeInlineString( fullPath, fullPath + len );
		}
		fileContents.location.lineNumber = 0;
		fileContents.code = MakeInlineString( data - 1, data + size );
		fileContents.position = data - 1;
		fileContents.isMacro = false;

		m_fileStack.push_back( fileContents );
	}
}

void ParserState::ParseDefine( const PreprocessorDefine& define, const FileLocation& location, const std::vector<InlineString>& arguments )
{
	FileContents fileContents;
	fileContents.location = location;
	fileContents.code = define.value;
	fileContents.position = fileContents.code.start;
	fileContents.isMacro = true;
	for( size_t i = 0; i < define.parameters.size(); ++i )
	{
		PreprocessorDefine def;
		def.value = arguments[i];
		fileContents.arguments[define.parameters[i]] = def;
	}
	m_fileStack.push_back( fileContents );
}

PreprocessorDefine* ParserState::FindDefine( const InlineString& name )
{
	auto found = m_fileStack.back().arguments.find( name );
	if( found != m_fileStack.back().arguments.end() )
	{
		return &found->second;
	}
	found = m_defines.find( name );
	if( found != m_defines.end() )
	{
		return &found->second;
	}
	return nullptr;
}

void ParserState::AddPragma( const InlineString& string )
{
	Pragma pragma;
	pragma.pragma = string;
	pragma.location = GetCurrentLocation();
	pragma.used = false;
	m_pragmas.push_back( pragma );
}

void ParserState::ResetPragmaUsage()
{
	for( auto it = m_pragmas.begin(); it != m_pragmas.end(); ++it )
	{
		it->used = false;
	}
}

bool ParserState::GetPragma( const FileLocation& location, InlineString& string )
{
	for( auto it = m_pragmas.begin(); it != m_pragmas.end(); ++it )
	{
		if( !it->used && it->location.fileName == location.fileName && it->location.lineNumber <= location.lineNumber )
		{
			string = it->pragma;
			it->used = true;
			return true;
		}
	}
	return false;
}

const std::set<Symbol*>& ParserState::GetDX9TextureFunctions()
{
	return m_dx9TextureFunctions;
}

const std::map<const char*, Symbol*>& ParserState::GetDX9Functions()
{
	return m_dx9Functions;
}

void ParserState::ShowMessageImpl( const FileLocation& location, ErrorCode errorCode, va_list args )
{
	static const char* errorMessages[] = {
		"unexpected token \'%s\'",
		"unexpected end of file",
		"unexpected character \'%c\'",
		"undeclared identifier \"%s\"",
		"identifier \"%s\" redefinition",
		"function parameter \"%s\" redefinition",
		"invalid pack offset",
		"invalid register",
		"effect contains no techniques",
		"annotation type mismatch for annotation \"%s\"",
		"explicit register bindings for scalaras are deprecated",
		"no suitable function override found for function call \"%s\"",

		"array, matrix, vector, or indexable object type expected in index expression",
		"invalid subscript \'%s\'",
		"type mismatch",
		"int or unsigned int type required",
		"scalar, vector, or matrix expected",
		"cannot convert from \'%s\' to \'%s\'",

		"invalid state name \"%s\"",
		"invalid state value \"%s\"",
		"type \"%s\" is not supported in state assignment",
		"use of uninitialized variable \"%s\"",
		"structs are not supported in state assignment",
		"invalid implicit type cast",
		"operator \"%s\" is not supported for given argument types",
		"side effects are not permitted in state assignments (operator \"%s\")",
		"index [%i] is out of range",
		"invalid swizzle \"%s\"",
		"function calls are not supported in state assignments",
		"incorrect number of arguments to numeric type constructor",
		"sampler \"%s\" is used both as %s and as %s sampler",
		"texture \"%s\" is declared as %s, but is used as %s",
		"sampler \"%s\" does not have texture assignment, but is used in DX9 texture function",

		"undeclared preprocessor symbol \'%\'",
		"unexpected preprocessor directive \'elif\'",
		"unexpected preprocessor directive \'else\'",
		"unexpected preprocessor directive \'endif\'",
		"file \'%s\' not found",
		"unexpected end of file in macro expansion",
		"not enough actual parameters for macro \'%s\'",
		"invalid include path",
		"syntax error : unexpected end of line",
		"%s",
		"state \'%s\' is deprecated",
		"cannot bind the same variable to multiple constants in the same constant bank",
	};

	char buffer1[2048], buffer2[2048];
	std::string fileName( location.fileName.start, location.fileName.end );
	if( errorCode == EC_REGISTER_ASSIGNMENT_DEPRECATED || 
		errorCode == EC_MISMATCHED_TEXTURE_TYPE || 
		errorCode == EC_STATE_DEPRECATED ||
		errorCode == EC_SAMPLER_WITHOUT_TEXTURE
		)
	{
		sprintf_s( buffer1, "%s(%i): warning t%i: ", fileName.c_str(), location.lineNumber, errorCode + 1000 );
	}
	else
	{
		sprintf_s( buffer1, "%s(%i): error t%i: ", fileName.c_str(), location.lineNumber, errorCode + 1000 );
		m_hasErrors = true;
	}
	vsprintf_s( buffer2, errorMessages[errorCode], args );
	std::string message = buffer1;
	message += buffer2;
	message += "\n";
	g_messages.AddMessage( message.c_str() );
}
