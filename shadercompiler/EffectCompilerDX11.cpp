#include "stdafx.h"
#if _WIN32
#include "EffectCompilerDX11.h"
#include "CompileMessageQueue.h"
#include "StringTable.h"
#include "EffectData.h"

#include "SymbolTable.h"
#include "ASTNode.h"

#include "ParserUtils.h"
#include "HLSLParser.h"
#include "ParserState.h"

#include "FXAnalyzer.h"

#include "TextureFunctionConversionDX11.h"

#include "YamlOutput.h"
#include "Macro.h"
#include "OutputHLSL.h"

#include "DxReflection.h"
#include <WorkQueue.h>

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )

enum DxilFourCC
{
	DFCC_Container = DXIL_FOURCC( 'D', 'X', 'B', 'C' ), // for back-compat with tools that look for DXBC containers
	DFCC_ResourceDef = DXIL_FOURCC( 'R', 'D', 'E', 'F' ),
	DFCC_InputSignature = DXIL_FOURCC( 'I', 'S', 'G', '1' ),
	DFCC_OutputSignature = DXIL_FOURCC( 'O', 'S', 'G', '1' ),
	DFCC_PatchConstantSignature = DXIL_FOURCC( 'P', 'S', 'G', '1' ),
	DFCC_ShaderStatistics = DXIL_FOURCC( 'S', 'T', 'A', 'T' ),
	DFCC_ShaderDebugInfoDXIL = DXIL_FOURCC( 'I', 'L', 'D', 'B' ),
	DFCC_ShaderDebugName = DXIL_FOURCC( 'I', 'L', 'D', 'N' ),
	DFCC_FeatureInfo = DXIL_FOURCC( 'S', 'F', 'I', '0' ),
	DFCC_PrivateData = DXIL_FOURCC( 'P', 'R', 'I', 'V' ),
	DFCC_RootSignature = DXIL_FOURCC( 'R', 'T', 'S', '0' ),
	DFCC_DXIL = DXIL_FOURCC( 'D', 'X', 'I', 'L' ),
	DFCC_PipelineStateValidation = DXIL_FOURCC( 'P', 'S', 'V', '0' ),
	DFCC_RuntimeData = DXIL_FOURCC( 'R', 'D', 'A', 'T' ),
	DFCC_ShaderHash = DXIL_FOURCC( 'H', 'A', 'S', 'H' ),
};

extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern bool g_printWarnings;
extern unsigned g_optimizationLevel;
extern bool g_avoidFlowControl;


static bool FindParameterBySemantics( ASTNode* node, const char** semantics, std::vector<Symbol*>* path, bool outParameter = false )
{
	for( unsigned j = 0; j < node->GetChildrenCount(); ++j )
	{
		Symbol* symbol = node->GetChild( j )->GetSymbol();
		if( symbol == nullptr )
		{
			continue;
		}
		if( outParameter )
		{
			if( node->GetChild( j )->GetToken() == 0 || node->GetChild( j )->GetToken()->type == OP_IN )
			{
				continue;
			}
		}
		for( unsigned k = 0; semantics[k]; ++k )
		{
			if( _stricmp( ToString( symbol->semantic ).c_str(), semantics[k] ) == 0 )
			{
				if( path )
				{
					path->clear();
					path->push_back( symbol );
				}
				return true;
			}
		}
		if( node->GetChild( j )->GetType().symbol && node->GetChild( j )->GetType().symbol->definition )
		{
			for( unsigned i = 0; i < node->GetChild( j )->GetType().symbol->definition->GetChildrenCount(); ++i )
			{
				if( FindParameterBySemantics( node->GetChild( j )->GetType().symbol->definition->GetChild( i ), semantics, path ) )
				{
					if( path )
					{
						path->insert( path->begin(), symbol );
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool FindOutputBySemantics( ASTNode* node, const char** semantics, std::vector<Symbol*>* path )
{
	// check "out" function arguments
	if( FindParameterBySemantics( node, semantics, path, true ) )
	{
		return true;
	}
	// check return value if it has semantics
	if( node->GetSymbol() && node->GetSymbol()->semantic.start )
	{
		std::string sematic = ToString( node->GetSymbol()->semantic );
		for( unsigned i = 0; semantics[i]; ++i )
		{
			if( _stricmp( sematic.c_str(), semantics[i] ) == 0 )
			{
				if( path )
				{
					path->push_back( nullptr );
				}
				return true;
			}
		}
	}
	// finally check return value as a structure
	if( node->GetType().symbol &&
		node->GetType().symbol->definition )
	{
		for( unsigned i = 0; i < node->GetType().symbol->definition->GetChildrenCount(); ++i )
		{
			if( FindParameterBySemantics( node->GetType().symbol->definition->GetChild( i ), semantics, path ) )
			{
				if( path )
				{
					path->insert( path->begin(), nullptr );
				}
				return true;
			}
		}
	}
	return false;
}

void PrintValuePath( std::ostream& os, const std::vector<Symbol*>& path, const char* functionSymbol )
{
	bool first = true;
	for( auto it = path.begin(); it != path.end(); ++it )
	{
		if( first )
		{
			first = false;
		}
		else
		{
			os << ".";
		}
		if( *it )
		{
			os << (*it)->name;
		}
		else
		{
			os << functionSymbol;
		}
	}
}

static void PatchSemantics( InputStageType shaderStage, ASTNode* callNode )
{
	Symbol* entryPointSymbol = callNode->GetSymbol();
	if( !entryPointSymbol )
	{
		return;
	}
	ASTNode* functionHeader = entryPointSymbol->definition->GetChildOrNull( 0 );
	if( functionHeader == nullptr )
	{
		return;
	}
	std::vector<Symbol*> targetPath;
	switch( shaderStage )
	{
	case VERTEX_STAGE:
	{
		const char* semantics[] = { "position", nullptr };
		if( FindOutputBySemantics( functionHeader, semantics, &targetPath ) )
		{
			if( targetPath.back() )
			{
				targetPath.back()->semantic = MakeInlineString( "SV_Position" );
			}
			else
			{
				entryPointSymbol->semantic = MakeInlineString( "SV_Position" );
			}
		}
		break;
	}
	case PIXEL_STAGE:
	{
		{
			const char* semantics[] = { "color", "color0", nullptr };
			if( FindOutputBySemantics( functionHeader, semantics, &targetPath ) )
			{
				if( targetPath.back() )
				{
					targetPath.back()->semantic = MakeInlineString( "SV_Target0" );
				}
				else
				{
					entryPointSymbol->semantic = MakeInlineString( "SV_Target0" );
				}
			}
		}
		{
			const char* semantics[] = { "color1", nullptr };
			if( FindOutputBySemantics( functionHeader, semantics, &targetPath ) )
			{
				if( targetPath.back() )
				{
					targetPath.back()->semantic = MakeInlineString( "SV_Target1" );
				}
				else
				{
					entryPointSymbol->semantic = MakeInlineString( "SV_Target1" );
				}
			}
		}
		break;
	}
	}
}

static PatchAction PatchShader( InputStageType shaderStage, ASTNode* callNode, ParserState& state, CodeStream& os, std::string& entryPointName )
{
	ZoneScoped;

	// 1. wrap uniforms
	// 2. fix VPOS

	bool wrapUniforms = false;
	bool fixVPOS = false;
	bool fixVPOSType = false;

	Symbol* entryPointSymbol = callNode->GetSymbol();

	if( entryPointSymbol == nullptr || entryPointSymbol->definition == nullptr )
	{
		return PATCH_ERROR;
	}

	ASTNode* functionHeader = entryPointSymbol->definition->GetChildOrNull( 0 );
	if( functionHeader == nullptr )
	{
		return PATCH_ERROR;
	}

	std::vector<Symbol*> targetPath;
	std::vector<Symbol*> outPositionPath;
	std::vector<Symbol*> vfacePath;

	if( shaderStage == VERTEX_STAGE )
	{
		const char* position[] = { "position", "sv_position", nullptr };
		FindOutputBySemantics( functionHeader, position, &outPositionPath );
	}


	// check for uniform parameters
	if( callNode->GetChildrenCount() )
	{
		unsigned count = 0;
		for( unsigned i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
			{
				++count;
			}
		}
		if( count != callNode->GetChildrenCount() )
		{
			state.ShowMessage( callNode->GetLocation(), EC_NO_OVERRIDE, ToString( functionHeader->GetToken()->stringValue ).c_str(), "" );
			return PATCH_ERROR;
		}
		wrapUniforms = true;
	}

	// find VPOS+POSITION parameters
	std::vector<Symbol*> positionPath;
	std::vector<Symbol*> vposPath;
	const char* vpos[] = { "vpos", nullptr };
	fixVPOSType = fixVPOS = FindParameterBySemantics( functionHeader, vpos, &vposPath );
	if( fixVPOS )
	{
		const char* position[] = { "position", "sv_position", nullptr };
		bool foundPosition = FindParameterBySemantics( functionHeader, position, &positionPath );
		fixVPOS &= foundPosition;
	}

	if( fixVPOSType )
	{
		if( vposPath.back() )
		{
			if( vposPath.back()->type.symbol ||
				(vposPath.back()->type.builtInType == OP_FLOAT && vposPath.back()->type.width == 4 && vposPath.back()->type.height == 1) )
			{
				fixVPOSType = false;
			}
		}
	}

	if( !wrapUniforms && !fixVPOS && !fixVPOSType && (shaderStage != VERTEX_STAGE || outPositionPath.empty()) )
	{
		entryPointName = ToString( entryPointSymbol->name );
		return PATCH_USE;
	}

	if( entryPointSymbol->definition->GetChildOrNull( 2 ) )
	{
		os << HLSL{ entryPointSymbol->definition->GetChildOrNull( 2 ), nullptr };
	}
	// return type
	os << functionHeader->GetType();
	// name
	InlineString name = state.AllocateName();
	os << ' ' << name << "( ";
	bool first = true;

	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			continue;
		}

		Symbol* symbol = functionHeader->GetChild( i )->GetSymbol();
		if( symbol == nullptr )
		{
			return PATCH_ERROR;
		}
		if( fixVPOS && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
		{
			continue;
		}
		if( !first )
		{
			os << ", ";
		}
		else
		{
			first = false;
		}
		if( fixVPOSType && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
		{
			os << "float4 " << functionHeader->GetChild( i )->GetSymbol()->name << ": VPOS";
		}
		else
		{
			os << HLSL{ functionHeader->GetChild( i ), nullptr };
		}
	}
	os << " )";
	// semantics
	if( functionHeader->GetSymbol()->semantic.start )
	{
		os << " : " << functionHeader->GetSymbol()->semantic;
	}
	os << "\n{\n";
	unsigned uniformIndex = 0;
	std::map<size_t, InlineString> defaultUniformArguments;
	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			if( callNode->GetChildrenCount() <= uniformIndex && functionHeader->GetChild( i )->GetChildOrNull( 1 ) != nullptr )
			{
				InlineString uniformName = state.AllocateName();
				Type type = functionHeader->GetChild( i )->GetType();
				type.storageClass = 0;
				os << type << ' ' << uniformName << " = " << HLSL{ functionHeader->GetChild( i )->GetChildOrNull( 1 ), nullptr };
				os << ";\n";
				defaultUniformArguments[i] = uniformName;
			}
			uniformIndex++;
		}
	}

	if( functionHeader->GetSymbol()->type.symbol || functionHeader->GetSymbol()->type.builtInType != OP_VOID )
	{
		os << functionHeader->GetType() << " __returnValue = ";
	}
	// call original function
	uniformIndex = 0;
	os << entryPointSymbol->name << "(";
	for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
	{
		if( i > 0 )
		{
			os << ", ";
		}
		if( IsUniformInputArgument( functionHeader->GetChild( i ) ) )
		{
			if( callNode->GetChildrenCount() > uniformIndex )
			{
				os << HLSL{ callNode->GetChild( uniformIndex ), nullptr };
			}
			else if( functionHeader->GetChild( i )->GetChildOrNull( 1 ) != nullptr )
			{
				os << defaultUniformArguments[i];
			}
			uniformIndex++;
		}
		else
		{
			Symbol* symbol = functionHeader->GetChild( i )->GetSymbol();
			if( symbol == nullptr )
			{
				return PATCH_ERROR;
			}
			if( fixVPOS && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
			{
				// pass VPOS value from POSITION
				PrintValuePath( os, positionPath, "" );
				if( symbol->type.symbol == nullptr && symbol->type.builtInType == OP_FLOAT && symbol->type.height == 1 &&
					positionPath.back()->type.symbol == nullptr && positionPath.back()->type.builtInType == OP_FLOAT &&
					positionPath.back()->type.height == 1 && positionPath.back()->type.width != symbol->type.width )
				{
					const char* swizzle = "xyzw";
					os << ".";
					for( int k = 0; k < symbol->type.width; ++k )
					{
						os << swizzle[std::min( k, positionPath.back()->type.width - 1 )];
					}
				}
			}
			else if( fixVPOSType && symbol->semantic.start && _stricmp( ToString( symbol->semantic ).c_str(), "vpos" ) == 0 )
			{
				Type type = symbol->type;
				os << type << "( " << symbol->name;
				if( type.width != 4 )
				{
					const char* swizzle = "xyzw";
					os << ".";
					for( int k = 0; k < symbol->type.width; ++k )
					{
						os << swizzle[std::min( k, symbol->type.width - 1 )];
					}
				}
				os << " )";
			}
			else
			{
				os << symbol->name;
			}
		}
	}
	os << ");\n";
	if( functionHeader->GetSymbol()->type.symbol || functionHeader->GetSymbol()->type.builtInType != OP_VOID )
	{
		os << "return __returnValue;\n";
	}
	os << "}\n";

	entryPointName = ToString( name );
	return PATCH_USE;
}

bool EffectCompilerDX11::Create()
{
	ZoneScoped;
	return SUCCEEDED( ::DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &m_dxilUtils ) ) );
}

bool MatchShaderInputOutput( ID3D11ShaderReflection* output, ID3D11ShaderReflection* input )
{
	D3D11_SHADER_DESC vsReflDesc;
	if( FAILED( output->GetDesc( &vsReflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}
	D3D11_SHADER_DESC psReflDesc;
	if( FAILED( input->GetDesc( &psReflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}

	for( unsigned k = 0; k < psReflDesc.InputParameters; ++k )
	{
		D3D11_SIGNATURE_PARAMETER_DESC psDesc;
		if( FAILED( input->GetInputParameterDesc( k, &psDesc ) ) )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader input parameter description" );
			return false;
		}
		if( psDesc.SystemValueType != D3D10_NAME_UNDEFINED )
		{
			continue;
		}
		bool found = false;
		for( unsigned n = 0; n < vsReflDesc.OutputParameters; ++n )
		{
			D3D11_SIGNATURE_PARAMETER_DESC vsDesc;
			if( FAILED( output->GetOutputParameterDesc( n, &vsDesc ) ) )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader output parameter description" );
				return false;
			}
			if( vsDesc.Register == psDesc.Register && ((~vsDesc.Mask & psDesc.Mask) == 0) )
			{
				if( _stricmp( vsDesc.SemanticName, psDesc.SemanticName ) || vsDesc.SemanticIndex != psDesc.SemanticIndex )
				{
					g_messages.AddMessage( "\\memory(0): error X0000: Vertex/pixel shader signature mismatch" );
					return false;
				}
				found = true;
				break;
			}
		}
		if( !found )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Vertex/pixel shader signature mismatch" );
			return false;
		}
	}
	return true;
}

std::string PrintPrettyCode( const char* code, const char* indent )
{
	std::stringstream os;
	while( *code )
	{
		const char* end = code;
		while( *end && *end != '\n' )
		{
			++end;
		}
		if( strncmp( code, "#line", 5 ) && code != end )
		{
			os << indent;
			os.write( code, end - code );
			os << std::endl;
		}
		code = end + 1;
		if( *end == 0 )
		{
			break;
		}
	}
	return os.str();
}

void PrintShaderOutListing( YamlOutput& listing, ID3DBlob* effectData, ID3D11ShaderReflection* reflection )
{
	ZoneScoped;

	if( !listing.enabled() )
	{
		return;
	}

	CComPtr<ID3DBlob> disassembly;
	if( SUCCEEDED( D3DDisassemble( effectData->GetBufferPointer(), effectData->GetBufferSize(), D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, nullptr, &disassembly ) ) )
	{
		listing.literal( "asm" ).literal( reinterpret_cast<const char*>(disassembly->GetBufferPointer()) );
	}
	D3D11_SHADER_DESC desc;
	if( reflection && SUCCEEDED( reflection->GetDesc( &desc ) ) )
	{
		listing.literal( "stats" ).dict()
			.literal( "Resources" ).dict()
			.literal( "constantBuffers" ).literal( desc.ConstantBuffers )
			.literal( "boundResources" ).literal( desc.BoundResources )
			.literal( "inputParameters" ).literal( desc.InputParameters )
			.literal( "outputParameters" ).literal( desc.OutputParameters )
			.literal( "tempRegisterCount" ).literal( desc.TempRegisterCount )
			.literal( "tempArrayCount" ).literal( desc.TempArrayCount )
			.end()
			.literal( "Instructions" ).dict()
			.literal( "instructionCount" ).literal( desc.InstructionCount )
			.literal( "defCount" ).literal( desc.DefCount )
			.literal( "textureNormalInstructions" ).literal( desc.TextureNormalInstructions )
			.literal( "textureLoadInstructions" ).literal( desc.TextureLoadInstructions )
			.literal( "textureCompInstructions" ).literal( desc.TextureCompInstructions )
			.literal( "textureBiasInstructions" ).literal( desc.TextureBiasInstructions )
			.literal( "textureGradientInstructions" ).literal( desc.TextureGradientInstructions )
			.literal( "floatInstructionCount" ).literal( desc.FloatInstructionCount )
			.literal( "intInstructionCount" ).literal( desc.IntInstructionCount )
			.literal( "uintInstructionCount" ).literal( desc.UintInstructionCount )
			.literal( "staticFlowControlCount" ).literal( desc.StaticFlowControlCount )
			.literal( "dynamicFlowControlCount" ).literal( desc.DynamicFlowControlCount )
			.literal( "macroInstructionCount" ).literal( desc.MacroInstructionCount )
			.literal( "arrayInstructionCount" ).literal( desc.ArrayInstructionCount )
			.literal( "cutInstructionCount" ).literal( desc.CutInstructionCount )
			.literal( "emitInstructionCount" ).literal( desc.EmitInstructionCount )
			.literal( "cBarrierInstructions" ).literal( desc.cBarrierInstructions )
			.literal( "cInterlockedInstructions" ).literal( desc.cInterlockedInstructions )
			.literal( "cTextureStoreInstructions" ).literal( desc.cTextureStoreInstructions )
			.end()
			.literal( "Misc" ).dict()
			.literal( "GSOutputTopology" ).literal( desc.GSOutputTopology )
			.literal( "inputPrimitive" ).literal( desc.InputPrimitive )
			.literal( "GSMaxOutputVertexCount" ).literal( desc.GSMaxOutputVertexCount )
			.literal( "patchConstantParameters" ).literal( desc.PatchConstantParameters )
			.literal( "cGSInstanceCount" ).literal( desc.cGSInstanceCount )
			.literal( "HSOutputPrimitive" ).literal( desc.HSOutputPrimitive )
			.literal( "HSPartitioning" ).literal( desc.HSPartitioning )
			.literal( "tessellatorDomain" ).literal( desc.TessellatorDomain )
			.end()
			.end();
	}
}

void PrintAnnotations( YamlOutput& listing, const std::map<StringReference, Annotation>& annotations )
{
	if( !listing.enabled() )
	{
		return;
	}
	listing.literal( "annotations" ).list();
	for( auto a = annotations.begin(); a != annotations.end(); ++a )
	{
		listing.dict()
			.literal( "name" ).literal( g_stringTable.GetString( a->first ) )
			.literal( "annotationType" );
		switch( a->second.type )
		{
		case ANNOTATION_TYPE_BOOL:
			listing
				.literal( "bool" )
				.literal( "value" ).literal( a->second.intValue != 0 );
			break;
		case ANNOTATION_TYPE_INT:
			listing
				.literal( "int" )
				.literal( "value" ).literal( a->second.intValue );
			break;
		case ANNOTATION_TYPE_FLOAT:
			listing
				.literal( "float" )
				.literal( "value" ).literal( a->second.floatValue );
			break;
		default:
			listing
				.literal( "string" )
				.literal( "value" ).literal( g_stringTable.GetString( a->second.stringValue ) );
			break;
		}
		listing.end();
	}
	listing.end();
}


YamlOutput& YamlTextureType( YamlOutput& listing, TextureType type )
{
	switch( type )
	{
	case TEX_TYPE_1D:
		listing.literal( "1D texture" );
		return listing;
	case TEX_TYPE_2D:
		listing.literal( "2D texture" );
		return listing;
	case TEX_TYPE_3D:
		listing.literal( "3D texture" );
		return listing;
	case TEX_TYPE_CUBE:
		listing.literal( "CUBE texture" );
		return listing;
	case TEX_TYPE_TYPELESS:
		listing.literal( "typeless texture" );
		return listing;
	case TEX_TYPE_BUFFER:
		listing.literal( "buffer" );
		return listing;
	case TEX_TYPE_STRUCTURED_BUFFER:
		listing.literal( "structured buffer" );
		return listing;
	case TEX_TYPE_TBUFFER:
		listing.literal( "TBuffer" );
		return listing;
	case TEX_TYPE_BYTEADDRESS_BUFFER:
		listing.literal( "byte address buffer" );
		return listing;

	case TEX_TYPE_UAV_RWTYPED:
		listing.literal( "UAV typed" );
		return listing;
	case TEX_TYPE_UAV_RWSTRUCTURED:
		listing.literal( "UAV structured" );
		return listing;
	case TEX_TYPE_UAV_RWBYTEADDRESS:
		listing.literal( "UAV RW byte address" );
		return listing;
	case TEX_TYPE_UAV_APPEND_STRUCTURED:
		listing.literal( "UAV append structured" );
		return listing;
	case TEX_TYPE_UAV_CONSUME_STRUCTURED:
		listing.literal( "UAV consume structured" );
		return listing;
	case TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER:
		listing.literal( "UAV structured with counter" );
		return listing;
	default:
		listing.literal( "other" );
		return listing;
	}
}


void PrintStageInfo( YamlOutput& listing, const StageData& stage, const EffectData& result )
{
	if( !listing.enabled() )
	{
		return;
	}
	if( !stage.constants.empty() )
	{
		listing.literal( "constants" ).list();
		for( auto it = stage.constants.begin(); it != stage.constants.end(); ++it )
		{
			listing.dict()
				.literal( "name" ).literal( g_stringTable.GetString( it->name ) )
				.literal( "constantType" ).literal( ToString( it->type ) )
				.literal( "dimension" ).literal( it->dimension )
				.literal( "arrayElements" ).literal( it->elements )
				.literal( "autoregister" ).literal( it->isAutoregister )
				.literal( "sRGB" ).literal( it->isSRGB );
			auto annotations = result.annotations.find( it->name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
			listing.end();
		}
		listing.end();
	}
	if( !stage.samplers.empty() )
	{
		listing.literal( "samplers" ).list();
		for( auto it = stage.samplers.begin(); it != stage.samplers.end(); ++it )
		{
			listing.dict()
				.literal( "register" ).literal( it->first )
				.literal( "name" ).literal( g_stringTable.GetString( it->second.name ) )
				.literal( "filter" ).literal( int( it->second.filter ) )
				.literal( "comparison" ).literal( int( it->second.comparison ) )
				.literal( "minFilter" ).literal( int( it->second.minFilter ) )
				.literal( "magFilter" ).literal( int( it->second.magFilter ) )
				.literal( "mipFilter" ).literal( int( it->second.mipFilter ) )
				.literal( "addressU" ).literal( int( it->second.addressU ) )
				.literal( "addressV" ).literal( int( it->second.addressV ) )
				.literal( "addressW" ).literal( int( it->second.addressW ) )
				.literal( "mipLODBias" ).literal( it->second.mipLODBias )
				.literal( "maxAnisotropy" ).literal( int( it->second.maxAnisotropy ) )
				.literal( "comparisonFunc" ).literal( int( it->second.comparisonFunc ) )
				.literal( "borderColor" ).list().literal( it->second.borderColor.x ).literal( it->second.borderColor.y ).literal( it->second.borderColor.z ).literal( it->second.borderColor.w ).end()
				.literal( "minLOD" ).literal( it->second.minLOD )
				.literal( "maxLOD" ).literal( it->second.maxLOD )
				.literal( "srgbTexture" ).literal( it->second.srgbTexture != 0 )
				.literal( "isDynamic" ).literal( it->second.isDynamic != 0 );
			auto annotations = result.annotations.find( it->second.name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
			listing.end();
		}
		listing.end();
	}
	if( !stage.textures.empty() )
	{
		listing.literal( "textures" ).list();
		for( auto it = stage.textures.begin(); it != stage.textures.end(); ++it )
		{
			listing.dict()
				.literal( "register" ).literal( it->first )
				.literal( "name" ).literal( g_stringTable.GetString( it->second.name ) )
				.literal( "textureType" ).literal( it->second.type )
				.literal( "autoregister" ).literal( it->second.isAutoregister )
				.literal( "sRGB" ).literal( it->second.isSRGB );
			auto annotations = result.annotations.find( it->second.name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
			listing.end();
		}
		listing.end();
	}
	if( !stage.uavs.empty() )
	{
		listing.literal( "uavs" ).list();
		for( auto it = stage.uavs.begin(); it != stage.uavs.end(); ++it )
		{
			listing.dict()
				.literal( "register" ).literal( it->first )
				.literal( "name" ).literal( g_stringTable.GetString( it->second.name ) )
				.literal( "resourceType" ).literal( it->second.type )
				.literal( "autoregister" ).literal( it->second.isAutoregister );
			auto annotations = result.annotations.find( it->second.name );
			if( annotations != result.annotations.end() )
			{
				PrintAnnotations( listing, annotations->second.annotations );
			}
			listing.end();
		}
		listing.end();
	}
	if( !stage.registerInputs.empty() )
	{
		listing.literal( "registers" ).list();
		for( auto it = stage.registerInputs.begin(); it != stage.registerInputs.end(); ++it )
		{
			listing.dict()
				.literal( "registerType" ).literal( it->registerType )
				.literal( "register" ).literal( it->registerIndex )
				.end();
		}
		listing.end();
	}
}

void PrintStageInfo( YamlOutput& listing, const StageInput& stage, const EffectData& result )
{
	PrintStageInfo( listing, static_cast<const StageData&>(stage), result );
	if( !stage.pipelineInputs.empty() )
	{
		listing.literal( "inputs" ).list();
		for( auto it = stage.pipelineInputs.begin(); it != stage.pipelineInputs.end(); ++it )
		{
			listing.dict()
				.literal( "register" ).literal( it->registerIndex )
				.literal( "name" ).literal( it->name )
				.literal( "index" ).literal( it->index )
				.literal( "usedMask" ).literal( it->usedMask )
				.end();
		}
		listing.end(); // inputs
	}
	if( !stage.staticSamplers.empty() )
	{
		listing.literal( "staticSamplers" ).list();
		for( auto& it : stage.staticSamplers )
		{
			listing.dict()
				.literal( "register" )
				.literal( it.registerIndex )
				.literal( "space" )
				.literal( it.registerSpace )
				.literal( "comparison" )
				.literal( int( it.comparison ) )
				.literal( "minFilter" )
				.literal( int( it.minFilter ) )
				.literal( "magFilter" )
				.literal( int( it.magFilter ) )
				.literal( "mipFilter" )
				.literal( int( it.mipFilter ) )
				.literal( "addressU" )
				.literal( int( it.addressU ) )
				.literal( "addressV" )
				.literal( int( it.addressV ) )
				.literal( "addressW" )
				.literal( int( it.addressW ) )
				.literal( "mipLODBias" )
				.literal( it.mipLODBias )
				.literal( "maxAnisotropy" )
				.literal( int( it.maxAnisotropy ) )
				.literal( "comparisonFunc" )
				.literal( int( it.comparisonFunc ) )
				.literal( "borderColor" )
				.literal( it.borderColor )
				.literal( "minLOD" )
				.literal( it.minLOD )
				.literal( "maxLOD" )
				.literal( it.maxLOD )
				.end();
		}
		listing.end(); //staticSamplers
	}
}

std::string SanitizeCode( const std::string& src )
{
	ZoneScoped;
	std::regex line( "#line[^\\n]*\\n?\\n" );
	return std::regex_replace( src, line, std::string( "" ) );
}


bool ParseShaderName( const InlineString& name, InputStageType& type )
{
	if( _stricmp( ToString( name ).c_str(), "vertexshader" ) == 0 )
	{
		type = VERTEX_STAGE;
	}
	else if( _stricmp( ToString( name ).c_str(), "pixelshader" ) == 0 )
	{
		type = PIXEL_STAGE;
	}
	else if( _stricmp( ToString( name ).c_str(), "computeshader" ) == 0 )
	{
		type = COMPUTE_STAGE;
	}
	else if( _stricmp( ToString( name ).c_str(), "geometryshader" ) == 0 )
	{
		type = GEOMETRY_STAGE;
	}
	else if( _stricmp( ToString( name ).c_str(), "hullshader" ) == 0 )
	{
		type = HULL_STAGE;
	}
	else if( _stricmp( ToString( name ).c_str(), "domainshader" ) == 0 )
	{
		type = DOMAIN_STAGE;
	}
	else
	{
		return false;
	}
	return true;
}

bool EffectCompilerDX11::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, IWorkQueue* workQueue )
{
	return CompileEffect( source, sourceLength, defines, result, { nullptr, false }, workQueue );
}

DWORD GetOptimizationLevel()
{
	switch( g_optimizationLevel )
	{
	case 0:
		return D3DCOMPILE_OPTIMIZATION_LEVEL0;
	case 1:
		return D3DCOMPILE_OPTIMIZATION_LEVEL1;
	case 2:
		return D3DCOMPILE_OPTIMIZATION_LEVEL2;
	default:
		return D3DCOMPILE_OPTIMIZATION_LEVEL3;
	}
}

RegisterInputDescription GetRegisterInputDescription( const Type& type, const RegisterSpecifier& reg )
{
	RegisterInputDescription desc;

	switch( type.builtInType )
	{
	case OP_BUFFER:
	case OP_RAYTRACING_ACCELERATION_STRUCTURE:
		desc.registerType = RT_SRV_BUFFER;
		break;
	case OP_STRUCTUREDBUFFER:
		desc.registerType = RT_SRV_STRUCTURED_BUFFER;
		break;
	case OP_TEXTURE1D:
		desc.registerType = RT_SRV_TEXTURE1D;
		break;
	case OP_TEXTURE1DARRAY:
		desc.registerType = RT_SRV_TEXTURE1DARRAY;
		break;
	case OP_TEXTURE:
	case OP_TEXTURE2D:
		desc.registerType = RT_SRV_TEXTURE2D;
		break;
	case OP_TEXTURE2DARRAY:
		desc.registerType = RT_SRV_TEXTURE2DARRAY;
		break;
	case OP_TEXTURE2DMS:
		desc.registerType = RT_SRV_TEXTURE2DMS;
		break;
	case OP_TEXTURE2DMSARRAY:
		desc.registerType = RT_SRV_TEXTURE2DMSARRAY;
		break;
	case OP_TEXTURE3D:
		desc.registerType = RT_SRV_TEXTURE3D;
		break;
	case OP_TEXTURECUBE:
		desc.registerType = RT_SRV_TEXTURECUBE;
		break;
	case OP_TEXTURECUBEARRAY:
		desc.registerType = RT_SRV_TEXTURECUBEARRAY;
		break;
	case OP_RWBUFFER:
		desc.registerType = RT_UAV_BUFFER;
		break;
	case OP_RWSTRUCTUREDBUFFER:
		desc.registerType = RT_UAV_STRUCTURED_BUFFER;
		break;
	case OP_RWTEXTURE1D:
		desc.registerType = RT_UAV_TEXTURE1D;
		break;
	case OP_RWTEXTURE1DARRAY:
		desc.registerType = RT_UAV_TEXTURE1DARRAY;
		break;
	case OP_RWTEXTURE2D:
		desc.registerType = RT_UAV_TEXTURE2D;
		break;
	case OP_RWTEXTURE2DARRAY:
		desc.registerType = RT_UAV_TEXTURE2DARRAY;
		break;
	case OP_RWTEXTURE3D:
		desc.registerType = RT_UAV_TEXTURE3D;
		break;
	case OP_SAMPLER:
	case OP_SAMPLER1D:
	case OP_SAMPLER2D:
	case OP_SAMPLER3D:
	case OP_SAMPLERCUBE:
	case OP_SAMPLERCOMPARISON:
		desc.registerType = RT_SAMPLER;
		break;
	default:
		desc.registerType = RT_CONSTANT_BUFFER;
		break;
	}
	desc.registerIndex = uint32_t( reg.registerNumber );
	desc.registerSpace = uint8_t( reg.space );
	desc.registerCount = 1;
	for( int i = 0; i < type.arrayDimensions; ++i )
	{
		desc.registerCount *= type.arraySizes[i];
	}
	desc.registerCount = std::max( desc.registerCount, 1u );
	return desc;
}


TextureType TypeToTextureType( const Type& type ) 
{
	switch( type.builtInType )
	{
	case OP_TEXTURE1D:
	case OP_RWTEXTURE1D:
		return TEX_TYPE_1D;
	case OP_TEXTURE:
	case OP_TEXTURE2D:
	case OP_RWTEXTURE2D:
		return TEX_TYPE_2D;
	case OP_TEXTURE3D:
	case OP_RWTEXTURE3D:
		return TEX_TYPE_3D;
	case OP_TEXTURECUBE:
		return TEX_TYPE_CUBE;
	case OP_TEXTURE2DARRAY:
	case OP_RWTEXTURE2DARRAY:
	case OP_TEXTURE2DMS:
	case OP_TEXTURE2DMSARRAY:
	case OP_TEXTURE3DARRAY:
	case OP_RWTEXTURE3DARRAY:
	case OP_TEXTURECUBEARRAY:
		return TEX_TYPE_TYPELESS;
	case OP_BUFFER:
		return TEX_TYPE_BUFFER;
	case OP_STRUCTUREDBUFFER:
		return TEX_TYPE_STRUCTURED_BUFFER;
	case OP_TBUFFER:
		return TEX_TYPE_TBUFFER;
	case OP_BYTEADDRESSBUFFER:
		return TEX_TYPE_BYTEADDRESS_BUFFER;
	case OP_RWBUFFER:
		return TEX_TYPE_UAV_RWTYPED;
	case OP_RWSTRUCTUREDBUFFER:
		return TEX_TYPE_UAV_RWSTRUCTURED;
	case OP_RWBYTEADDRESSBUFFER:
		return TEX_TYPE_UAV_RWBYTEADDRESS;
	case OP_RAYTRACING_ACCELERATION_STRUCTURE:
		return TEX_TYPE_RAYTRACING_ACCELERATION_STRUCTURE;
	default:
		return TEX_TYPE_TYPELESS;
	}
}


bool AddGlobalInputs( StageData& globalsData, std::map<StringReference, ParameterAnnotation>& annotations, const std::vector<GlobalInputElement>& globalInputs, ParserState& state, bool useStaticSamplers )
{
	auto AddAnnotations = [&annotations]( const Symbol* symbol, bool& srgb, bool& autoregister ) {
		if( symbol && symbol->annotations )
		{
			ParameterAnnotation paramAnnotations;
			for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
			{
				Annotation result;
				if( DxReflection::MakeEffectAnnotationFromSymbolAnnotation( *ai, result, srgb, autoregister ) )
				{
					paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;
				}
			}
			if( !paramAnnotations.annotations.empty() )
			{
				annotations[g_stringTable.AddString( symbol->name )] = paramAnnotations;
			}
		}
	};

	for( auto& gi : globalInputs )
	{
		if( !gi.symbol )
		{
			continue;
		}
		auto symbol = gi.symbol;
		if( !symbol->registerSpecifier.empty() )
		{
			auto& reg = symbol->registerSpecifier.begin()->second;
			auto outReg = GetRegisterInputDescription( symbol->type, reg );
			auto existingReg = find_if( begin( globalsData.registerInputs ), end( globalsData.registerInputs ), [&outReg]( auto& x ) {
				return outReg.registerIndex == x.registerIndex && outReg.registerSpace == x.registerSpace && outReg.registerType == x.registerType;
			} );
			if( existingReg != end( globalsData.registerInputs ) )
			{
				continue;
			}

			switch( reg.registerType )
			{
			case 's': {
				Sampler sampler;
				if( !GetSamplerState( state, symbol->definition, sampler ) )
				{
					return false;
				}
				StaticSampler staticSampler;
				if( symbol->type.arrayDimensions == 0 && useStaticSamplers && ConvertToStaticSampler( staticSampler, sampler ) )
				{
					staticSampler.registerIndex = reg.registerNumber;
					staticSampler.registerSpace = uint8_t( reg.space );
					auto found = find_if( begin( globalsData.staticSamplers ), end( globalsData.staticSamplers ), [&staticSampler]( auto& x ) {
						return staticSampler.registerIndex == x.registerIndex && staticSampler.registerSpace == x.registerSpace;
					} );
					if( found == end( globalsData.staticSamplers ) )
					{
						globalsData.staticSamplers.push_back( staticSampler );
					}
				}
				else
				{
					sampler.name = g_stringTable.AddString( symbol->name );
					globalsData.samplers[uint8_t( reg.registerNumber )] = sampler;
					globalsData.registerInputs.push_back( outReg );
				}
				bool srgb, autoregister;
				AddAnnotations( symbol, srgb, autoregister );
				break;
			}
			case 'u': {
				globalsData.registerInputs.push_back( outReg );

				Uav uav;
				uav.name = g_stringTable.AddString( symbol->name );
				uav.type = TypeToTextureType( symbol->type );
				uav.count = outReg.registerCount;
				uav.isAutoregister = false;

				bool srgb;
				AddAnnotations( symbol, srgb, uav.isAutoregister );
				globalsData.uavs[uint8_t( reg.registerNumber )] = uav;
				break;
			}
			case 't': {
				globalsData.registerInputs.push_back( outReg );

				Texture texture;
				texture.name = g_stringTable.AddString( symbol->name );
				texture.type = TypeToTextureType( symbol->type );
				texture.count = outReg.registerCount;
				texture.isSRGB = false;
				texture.isAutoregister = false;

				AddAnnotations( symbol, texture.isSRGB, texture.isAutoregister );
				globalsData.textures[uint8_t( reg.registerNumber )] = texture;
				break;
			}
			case 'b': {
				globalsData.registerInputs.push_back( outReg );
				break;
			}
			default:
				break;
			}
		}
	}
	return true;
}

bool EffectCompilerDX11::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, const CompileOptions& compileOptions, IWorkQueue* workQueue )
{
	ZoneScoped;

	ParserState state( MakeInlineString( source, source + sourceLength ) );
	for( auto it = begin( defines ); it != end( defines ); ++it )
	{
		PreprocessorDefine d;
		d.location.fileName = MakeInlineString( "" );
		d.location.lineNumber = 0;
		d.value = MakeInlineString( it->value.c_str() );
		state.m_defines[MakeInlineString( it->name.c_str() )] = d;

	}

	if( !state.Parse() )
	{
		return false;
	}

	PatchCBuffers( state );
	TransferSRGBToTexturesDX11( state );
	ConvertTextureFunctionsDX11( state );
	MergeSamplers( state );

	std::vector<ASTNode*> techniqueNodes;
	state.GetTree()->FindNodes( NT_TECHNIQUE, techniqueNodes );
	if( techniqueNodes.empty() )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: No technique or libraries found" );
		return false;
	}

	YamlListing listing;
	listing.dict();
	listing.literal( "permutation" ).dict()
		.literal( "platform" ).literal( "DX11" )
		.literal( "id" ).literal( "000" )
		.literal( "defines" ).dict();
	for( auto it = begin( defines ); it != end( defines ); ++it )
	{
		listing.literal( it->name ).literal( it->value );
	}
	listing.end();
	listing.literal( "techniques" ).list();

	for( auto techniqueIt = techniqueNodes.begin(); techniqueIt != techniqueNodes.end(); ++techniqueIt )
	{
		auto techniqueNode = *techniqueIt;
		Technique technique;
		technique.name = g_stringTable.AddString( ToString( techniqueNode->GetToken()->stringValue ).c_str() );

		listing.dict()
			.literal( "name" ).literal( techniqueNode->GetToken()->stringValue )
			.literal( "passes" ).list();

		for( size_t passIx = 0; passIx < techniqueNode->GetChildrenCount(); ++passIx )
		{
			ASTNode* passNode = techniqueNode->GetChild( passIx );
			if( passNode->GetNodeType() != NT_PASS )
			{
				continue;
			}
			listing.list();
			Pass outPass;
			CComPtr<ID3D11ShaderReflection> reflections[6];
			for( size_t stateIx = 0; stateIx < passNode->GetChildrenCount(); ++stateIx )
			{
				if( passNode->GetChild( stateIx )->GetNodeType() == NT_STATE_ASSIGNMENT )
				{
					DWORD stateCode = 0;
					DWORD value = 0;
					if( ParseStateAssignment( state, passNode->GetChild( stateIx ), g_renderStates, &value ) )
					{
						std::string name = ToString( passNode->GetChild( stateIx )->GetToken()->stringValue );
						for( int i = 0; g_renderStateNames[i].name; ++i )
						{
							if( _stricmp( name.c_str(), g_renderStateNames[i].name ) == 0 )
							{
								stateCode = g_renderStateNames[i].value;
							}
						}
						switch( stateCode )
						{
						case -1:
						case -2:
						case -3:
						case -4:
						case -5:
						case -6:
							if( value != 0 )
							{
								state.ShowMessage( passNode->GetChild( stateIx )->GetLocation(), EC_INVALID_STATE_VALUE, name.c_str() );
							}
							break;
						case 0:
							state.ShowMessage( passNode->GetChild( stateIx )->GetLocation(), EC_STATE_DEPRECATED, name.c_str() );
							break;
						default:
							outPass.states[stateCode] = value;
						}
					}
					continue;
				}

				if( passNode->GetChild( stateIx )->GetNodeType() != NT_SHADER_ASSIGNMENT )
				{
					continue;
				}

				ASTNode* shaderNode = passNode->GetChild( stateIx );

				StageInput stage;
				if( !ParseShaderName( shaderNode->GetToken()->stringValue, stage.type ) )
				{
					state.ShowMessage( shaderNode->GetToken()->fileLocation, EC_INVALID_STATE, ToString( shaderNode->GetToken()->stringValue ).c_str() );
					return false;
				}

				std::string profile = ToString( shaderNode->GetChild( 0 )->GetToken()->stringValue );
				if( compileOptions.minShaderVersion )
				{
					profile = profile.substr( 0, 3 ) + compileOptions.minShaderVersion;
				}
				else
				{
					if( profile[0] == 'v' )
					{
						profile = "vs_5_0";
					}
					else if( profile[0] == 'p' )
					{
						profile = "ps_5_0";
					}
				}
				ID3D10Blob* effectData = nullptr;
				CComPtr<ID3D10Blob> errors = nullptr;


				if( shaderNode->GetChild( 1 )->GetSymbol() == nullptr )
				{
					return false;
				}

				if( compileOptions.minShaderVersion )
				{
					PatchSemantics( stage.type, shaderNode->GetChild( 1 ) );
				}

				state.GetSymbolTable().ResetUsedFlag();
				MarkUsedSymbols( shaderNode->GetChild( 1 ), state );

				if( compileOptions.addSpaces )
				{
					CreateGlobalsCB( state );
					AssignRegisters( state.GetTree(), stage.type );
				}

				CompilerInputStream os( state, ShadingLanguage::HLSL );
				os << HLSL{ state.GetTree(), & state.GetSymbolTable() };

				std::string entryPoint = ToString( shaderNode->GetChild( 1 )->GetSymbol()->name );

				std::string patchEntryPoint = entryPoint;
				switch( PatchShader( stage.type, shaderNode->GetChild( 1 ), state, os, patchEntryPoint ) )
				{
				case PATCH_ERROR:
					return false;
				}
				state.ResetPragmaUsage();
				std::string code = os.str();


				// Let only one thread compile this permutation.
				bool needsToCompile = false;
				std::shared_ptr<SyncData> syncData;
				{
					std::lock_guard scope( m_compiledCS );
					auto found = m_compiled.find( code );
					if ( found == end( m_compiled ) )
					{
						// Yep, this thread will have to compile. Make an entry into the cache.
						syncData = std::make_shared<SyncData>();
						m_compiled[code] = syncData;
						needsToCompile = true;
					}
					else
					{
						// Some other thread is going to compile for us. Grab his stuff from the cache.
						syncData = found->second;
					}
				}

				if( !needsToCompile )
				{
					// Let's wait for the other thread to compile for us
					if( workQueue )
					{
						workQueue->OnBlocked();
					}
					{
						std::unique_lock<std::mutex> lock( syncData->mutex );
						syncData->conditionVariable.wait( lock, [&syncData] { return syncData->compiled; } );
					}
					if( workQueue )
					{
						workQueue->OnUnblocked();
					}
				}
				else
				{
					// We need to compile, nobody else is doing it for this permutation.
					CComPtr<ID3D10Blob> compiledEffectData;
					HRESULT hr;
					{
						ZoneScopedN( "D3DCompile" );
						hr = D3DCompile(
							code.c_str(),
							code.length(),
							"\\memory",
							nullptr,
							nullptr,
							patchEntryPoint.c_str(),
							profile.c_str(),
							( compileOptions.minShaderVersion ? D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES : D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY ) | GetOptimizationLevel() | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | ( g_avoidFlowControl ? D3DCOMPILE_AVOID_FLOW_CONTROL : 0 ),
							0,
							&compiledEffectData,
							&errors );
					}
					
					if( FAILED( hr ) )
					{
						// We failed compilation, let's print errors.
						syncData->passResource = nullptr;
						if( errors )
						{
							g_messages.AddMessages( errors );
						}
					}
					else
					{
						// Compilation succeeded! Hand over the resource to the cache entry.
						syncData->passResource.Attach( compiledEffectData.Detach() );
					}

					// Let's wake up everyone waiting for this permutation's compilation.
					{
						std::lock_guard scope( syncData->mutex );
						syncData->compiled = true;
					}
					syncData->conditionVariable.notify_all();
				}

				// Grab a raw pointer from the cache. This is to avoid whatever funny business is going on with CComPtr reference counters.
				effectData = syncData->passResource;

				{
					// Not sure if this is necessary to avoid reference counter bugs. But anyway...
					std::lock_guard scope( m_compiledCS );
					syncData.reset();
				}

				if( !effectData )
				{
					// No resource on the cache entry! This means compilation must have failed.
					return false;
				}


				auto handleStrippedData = [&]( ID3DBlob* blob ) 
				{
					// No idea what happens when assigning strippedEffectData = effectData, with effectData now being a raw pointer, and not taking any chances...
					stage.shaderSize = uint32_t( blob->GetBufferSize() );
					stage.shaderDataStr = g_stringTable.AddString( blob->GetBufferPointer(), blob->GetBufferSize() );
					stage.source = code;
				};
				CComPtr<ID3DBlob> strippedEffectData;
				{
					ZoneScopedN( "D3DStripShader" );
					if( FAILED( D3DStripShader(
						effectData->GetBufferPointer(),
						effectData->GetBufferSize(),
						D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS,
						&strippedEffectData ) ) )
					{
						handleStrippedData( effectData );
					}
					else
					{
						handleStrippedData( strippedEffectData );
					}
				}


				CComPtr<ID3D11ShaderReflection> reflection;
				{
					ZoneScopedN( "D3DReflect" );

					if( FAILED( D3DReflect( effectData->GetBufferPointer(), effectData->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection.p ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection" );
						return false;
					}
				}
				
				if( !DxReflection::ProcessReflection<DxReflection::ReflectionDx11>( state, reflection.p, compileOptions.useStaticSamplers, stage, result.annotations ) )
				{
					return false;
				}
				{
					stage.annotations.annotations.clear();
					Symbol* symbol = shaderNode->GetChild( 1 )->GetSymbol();
					if( symbol && symbol->annotations )
					{
						for( auto a = symbol->annotations->begin(); a != symbol->annotations->end(); ++a )
						{
							Annotation symbolAnnotation;
							bool isSrgb, isAutoregister;
							if( DxReflection::MakeEffectAnnotationFromSymbolAnnotation( *a, symbolAnnotation, isSrgb, isAutoregister ) )
							{
								stage.annotations.annotations[g_stringTable.AddString( ToString( a->name ).c_str() )] = symbolAnnotation;
							}
						}
					}
				}

				if( !stage.defaultValues.empty() )
				{
					stage.defaultValuesStr = g_stringTable.AddString( &stage.defaultValues[0], stage.defaultValues.size() );
				}
				else
				{
					stage.defaultValuesStr = INVALID_REFERENCE;
				}

				if( listing.enabled() )
				{
					listing.dict()
						.literal( "profile" ).literal( profile )
						.literal( "original" ).dict()
						.literal( "entryPoint" ).literal( patchEntryPoint )
						.literal( "source" ).literal( SanitizeCode( code ) );
					PrintShaderOutListing( listing, effectData, reflection );
					listing.end();
				}

				reflections[stage.type] = reflection;

				PrintStageInfo( listing, stage, result );
				listing.end();

				outPass.stages.push_back( stage );
			}


			// Check if PS input matches VS output
			InputStageType pipelineStages[] = {
				VERTEX_STAGE,
				HULL_STAGE,
				DOMAIN_STAGE,
				GEOMETRY_STAGE,
				PIXEL_STAGE, };
			for( int i = 0; i < 6; ++i )
			{
				if( reflections[i] )
				{
					for( int j = 0; j < sizeof( pipelineStages ) / sizeof( InputStageType ); ++j )
					{
						if( i == pipelineStages[j] )
						{
							for( int k = j - 1; k >= 0; --k )
							{
								if( reflections[pipelineStages[k]] )
								{
									if( !MatchShaderInputOutput( reflections[pipelineStages[k]], reflections[i] ) )
									{
										return false;
									}
									break;
								}
							}
							break;
						}
					}
				}
			}
			technique.passes.push_back( outPass );

			listing.end(); // stages list
		}

		listing.end(); // passes list
		listing.literal( "libraries" ).list();

		for( size_t passIx = 0; passIx < techniqueNode->GetChildrenCount(); ++passIx )
		{
			ASTNode* libNode = techniqueNode->GetChild( passIx );
			if( libNode->GetNodeType() != NT_LIBRARY )
			{
				continue;
			}
			listing.dict()
				.literal( "name" ).literal( libNode->GetToken()->stringValue )
				.literal( "exports" ).list();

			Library library;
			library.payloadSize = 0;
			library.hitGroupName = g_stringTable.AddString( "" );

			std::map<std::string, std::string> shaders;
			std::vector<GlobalInputElement> globalInputs;

			state.GetSymbolTable().ResetUsedFlag();

			for( size_t i = 0; i < libNode->GetChildrenCount(); ++i )
			{
				auto childNode = libNode->GetChild( i );
				if( childNode->GetNodeType() == NT_SHADER_ASSIGNMENT )
				{
					ShaderExport shaderExport;
                    if( auto parsed = ParseRtShaderName( childNode->GetToken()->stringValue ) )
                    {
                        shaderExport.type = parsed.value();
                    }
                    else
					{
						state.ShowMessage( childNode->GetToken()->fileLocation, EC_INVALID_STATE, ToString( childNode->GetToken()->stringValue ).c_str() );
						return false;
					}
					listing.literal( childNode->GetChild( 1 )->GetSymbol()->name );
					shaderExport.name = g_stringTable.AddString( ToString( childNode->GetChild( 1 )->GetSymbol()->name ).c_str() );
					library.exports.push_back( shaderExport );

					if( !ProcessGlobalInputAttribute( state, childNode->GetChild( 1 )->GetSymbol()->definition, globalInputs ) )
					{
						return false;
					}
					for ( auto& gi : globalInputs )
					{
						if( gi.symbol )
						{
							gi.symbol->used = true;
							if( auto type = gi.symbol->type.symbol )
							{
								const_cast<Symbol*>( type )->used = true;
							}
						}
					}
					MarkUsedSymbols( childNode->GetChild( 1 ), state );
				}
				else if( childNode->GetNodeType() == NT_STATE_ASSIGNMENT )
				{
					auto name = childNode->GetToken()->stringValue;
					if( EqualsCaseInsensitive( name, "payloadsize" ) )
					{
						Type type;
						type.FromTokenType( OP_UINT );
						ExpressionValue value;
						if( !EvaluateExpression( state, childNode->GetChildOrNull( 0 ), type, value, nullptr ) )
						{
							state.ShowMessage( childNode->GetToken()->fileLocation, EC_INVALID_STATE, ToString( name ).c_str() );
							return false;
						}
						library.payloadSize = uint32_t( value[0].intValue );
					}
					else if( EqualsCaseInsensitive( name, "hitgroupname" ) )
					{
						Type type;
						type.FromTokenType( OP_STRING );
						ExpressionValue value;
						if( !EvaluateExpression( state, childNode->GetChildOrNull( 0 ), type, value, nullptr ) )
						{
							state.ShowMessage( childNode->GetToken()->fileLocation, EC_INVALID_STATE, ToString( name ).c_str() );
							return false;
						}
						library.hitGroupName = g_stringTable.AddString( value[0].stringValue.c_str() );
					}
				}
			}
			listing.end(); // exports list

			if( compileOptions.addSpaces )
			{
				CreateGlobalsCB( state );
				AssignRegisters( state.GetTree(), 8, globalInputs );
			}


			CompilerInputStream os( state, ShadingLanguage::HLSL );
			os << HLSL{ state.GetTree(), & state.GetSymbolTable() };

			CComPtr<IDxcBlobEncoding> src;

			//m_dxilLibrary->CreateBlobWithEncodingFromPinned( os.str(), UINT32( os.pcount() ), CP_UTF8, &src );
			std::string code = os.str();
			m_dxilUtils->CreateBlobFromPinned( code.c_str(), UINT32(os.str().size() ), CP_UTF8, &src);

			library.source = code;


			// Let only one thread compile this permutation.
			bool needsToCompile = false;
			std::shared_ptr<SyncData> syncData;
			{
				std::lock_guard scope( m_compiledCS );
				auto found = m_compiled.find( code );
				if( found == end( m_compiled ) )
				{
					// Yep, this thread will have to compile. Make an entry into the cache.
					syncData = std::make_shared<SyncData>();
					m_compiled[code] = syncData;
					needsToCompile = true;
				}
				else
				{
					// Some other thread is going to compile for us. Grab his stuff from the cache.
					syncData = found->second;
				}
			}

			if( !needsToCompile )
			{
				// Let's wait for the other thread to compile for us
				if( workQueue )
				{
					workQueue->OnBlocked();
				}
				{
					std::unique_lock<std::mutex> lock( syncData->mutex );
					syncData->conditionVariable.wait( lock, [&syncData] { return syncData->compiled; } );
				}
				if( workQueue )
				{
					workQueue->OnUnblocked();
				}
			}
			else
			{
				// We need to compile, nobody else is doing it for this permutation.
				CComPtr<IDxcCompiler> compiler;
				DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler ) );

				CComPtr<IDxcOperationResult> opResult;
				compiler->Compile(
					src,
					L"memory",
					L"",
					L"lib_6_3",
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					&opResult );

				HRESULT hrCompilation;
				opResult->GetStatus( &hrCompilation );
				
				if( FAILED( hrCompilation ) )
				{
					// We failed compilation, let's print errors.
					syncData->libraryResource = nullptr;
					CComPtr<IDxcBlobEncoding> messages;
					if( SUCCEEDED( opResult->GetErrorBuffer( &messages ) ) )
					{
						g_messages.AddMessages( messages );
					}
				}
				else
				{
					// Compilation succeeded! Hand over the resource to the cache entry.
					CComPtr<IDxcBlob> compiled;
					opResult->GetResult( &compiled );
					syncData->libraryResource.Attach( compiled.Detach() );
				}

				// Let's wake up everyone waiting for this permutation's compilation.
				{
					std::lock_guard scope( syncData->mutex );
					syncData->compiled = true;
				}
				syncData->conditionVariable.notify_all();
			}

			// Grab a raw pointer from the cache. This is to avoid whatever funny business is going on with CComPtr reference counters.
			IDxcBlob* compiled = syncData->libraryResource;

			{
				// Not sure if this is necessary to avoid reference counter bugs. But anyway...
				std::lock_guard scope( m_compiledCS );
				syncData.reset();
			}

			if( !compiled )
			{
				// No resource on the cache entry! This means compilation must have failed.
				return false;
			}


			library.shaderDataStr = g_stringTable.AddString( compiled->GetBufferPointer(), compiled->GetBufferSize() );
			library.shaderSize = uint32_t( compiled->GetBufferSize() );

			CComPtr<IDxcContainerReflection> reflection;
			DxcCreateInstance( CLSID_DxcContainerReflection, IID_PPV_ARGS( &reflection ) );
			reflection->Load( compiled );
			UINT32 shaderIdx;
			reflection->FindFirstPartKind( DFCC_DXIL, &shaderIdx );

			CComPtr<ID3D12LibraryReflection> shaderReflection;
			reflection->GetPartReflection( shaderIdx, IID_PPV_ARGS( &shaderReflection ) );
			D3D12_LIBRARY_DESC desc;
			shaderReflection->GetDesc( &desc );

			for( UINT i = 0; i < desc.FunctionCount; ++i )
			{
				if( !DxReflection::ProcessReflection<DxReflection::FunctionDx12>( state, shaderReflection->GetFunctionByIndex( i ), compileOptions.useStaticSamplers, library.globalInputs, result.annotations, DxReflection::ReflectionTarget::GLOBAL_INPUT, globalInputs ) )
				{
					return false;
				}
				if( !DxReflection::ProcessReflection<DxReflection::FunctionDx12>( state, shaderReflection->GetFunctionByIndex( i ), compileOptions.useStaticSamplers, library.localInputs, result.annotations, DxReflection::ReflectionTarget::LOCAL_INPUT, globalInputs ) )
				{
					return false;
				}
			}

			AddGlobalInputs( library.globalInputs, result.annotations, globalInputs, state, compileOptions.useStaticSamplers );

			technique.libraries.push_back( library );

			if( listing.enabled() )
			{
				CComPtr<IDxcCompiler> compiler;
				DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler ) );

				listing.literal( "profile" ).literal( "lib_6_3" );
				listing.literal( "original" ).dict();
				std::string osSrc;
				osSrc.reserve( os.str().size() );
				listing.literal( "source" ).literal( SanitizeCode( std::string( os.str(), osSrc.size() ) ) );
				CComPtr<IDxcBlobEncoding> disassembly;
				if( SUCCEEDED( compiler->Disassemble( compiled, &disassembly ) ) )
				{
					listing.literal( "asm" ).literal( reinterpret_cast<const char*>(disassembly->GetBufferPointer()) );
				}
				listing.end();
				PrintStageInfo( listing, library.globalInputs, result );
			}
			listing.end();
		}

		result.techniques.push_back( technique );
		listing.end(); // libraries list
		listing.end(); // technique dict
	}
	listing.end(); // technique list

	return true;
}
#endif
