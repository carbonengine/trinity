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
#include <regex>

extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern bool g_printWarnings;
extern unsigned g_optimizationLevel;
extern bool g_avoidFlowControl;



bool MakeEffectAnnotationFromSymbolAnnotation( const SymbolAnnotation& annotation, Annotation& result, bool& isSRGB, bool& isAutoregister )
{
	switch( annotation.type )
	{
	case OP_FLOAT:
	case OP_HALF:
	case OP_DOUBLE:
		result.type = ANNOTATION_TYPE_FLOAT;
		result.floatValue = float( ParseFloat( annotation.value.stringValue.start, annotation.value.stringValue.end ) );
		return true;

	case OP_UINT:
	case OP_INT:
		result.type = ANNOTATION_TYPE_INT;
		result.intValue = ParseNumber( annotation.value.stringValue.start, annotation.value.stringValue.end );
		return true;

	case OP_BOOL:
		result.type = ANNOTATION_TYPE_BOOL;
		result.intValue = annotation.value.intValue ? 1 : 0;
		if( ToString( annotation.name ) == "Tr2sRGB" )
		{
			isSRGB = result.intValue != 0;
		}
		else if( ToString( annotation.name ) == "AutoRegister" )
		{
			isAutoregister = result.intValue != 0;
		}
		return true;

	case OP_STRING:
		result.type = ANNOTATION_TYPE_STRING;
		result.stringValue = g_stringTable.AddString( ParseString( annotation.value.stringValue ).c_str() );
		return true;
	}

	return false;
}

static bool GetTextureType( const D3D11_SHADER_INPUT_BIND_DESC& desc, TextureType& type )
{
	switch( desc.Type )
	{
	case D3D_SIT_TEXTURE:
		type = TEX_TYPE_TYPELESS;
		switch( desc.Dimension )
		{
		case D3D10_SRV_DIMENSION_TEXTURE1D:
			type = TEX_TYPE_1D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURE2D:
			type = TEX_TYPE_2D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURE3D:
			type = TEX_TYPE_3D;
			break;
		case D3D10_SRV_DIMENSION_TEXTURECUBE:
			type = TEX_TYPE_CUBE;
			break;
		case D3D10_SRV_DIMENSION_BUFFER:
			type = TEX_TYPE_BUFFER;
			break;
		}
		break;
	case D3D_SIT_TBUFFER:
		type = TEX_TYPE_TBUFFER;
		break;
	case D3D_SIT_STRUCTURED:
		type = TEX_TYPE_STRUCTURED_BUFFER;
		break;
	case D3D_SIT_BYTEADDRESS:
		type = TEX_TYPE_BYTEADDRESS_BUFFER;
		break;
	case D3D_SIT_UAV_RWTYPED:
		type = TEX_TYPE_UAV_RWTYPED;
		break;
	case D3D_SIT_UAV_RWSTRUCTURED:
		type = TEX_TYPE_UAV_RWSTRUCTURED;
		break;
	case D3D_SIT_UAV_RWBYTEADDRESS:
		type = TEX_TYPE_UAV_RWBYTEADDRESS;
		break;
	case D3D_SIT_UAV_APPEND_STRUCTURED:
		type = TEX_TYPE_UAV_APPEND_STRUCTURED;
		break;
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
		type = TEX_TYPE_UAV_CONSUME_STRUCTURED;
		break;
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		type = TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER;
		break;
	default:
		return false;
	}
	return true;
}

RegisterInputType GetRegisterType( const D3D11_SHADER_INPUT_BIND_DESC& desc )
{
	switch( desc.Type )
	{
	case D3D_SIT_CBUFFER:
		return RT_CONSTANT_BUFFER;
	case D3D_SIT_TEXTURE:
		switch( desc.Dimension )
		{
		case D3D_SRV_DIMENSION_TEXTURE1D:
			return RT_SRV_TEXTURE1D;
		case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
			return RT_SRV_TEXTURE1DARRAY;
		case D3D_SRV_DIMENSION_TEXTURE2D:
			return RT_SRV_TEXTURE2D;
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			return RT_SRV_TEXTURE2DARRAY;
		case D3D_SRV_DIMENSION_TEXTURE2DMS:
			return RT_SRV_TEXTURE2DMS;
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return RT_SRV_TEXTURE2DMSARRAY;
		case D3D_SRV_DIMENSION_TEXTURE3D:
			return RT_SRV_TEXTURE3D;
		case D3D_SRV_DIMENSION_TEXTURECUBE:
			return RT_SRV_TEXTURECUBE;
		case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
			return RT_SRV_TEXTURECUBEARRAY;
		default:
			return RT_SRV_BUFFER;
		}
	case D3D_SIT_SAMPLER:
		return RT_SAMPLER;
	case D3D_SIT_UAV_RWTYPED:
		switch( desc.Dimension )
		{
		case D3D_SRV_DIMENSION_TEXTURE1D:
			return RT_UAV_TEXTURE1D;
		case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
			return RT_UAV_TEXTURE1DARRAY;
		case D3D_SRV_DIMENSION_TEXTURE2D:
			return RT_UAV_TEXTURE2D;
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			return RT_UAV_TEXTURE2DARRAY;
		case D3D_SRV_DIMENSION_TEXTURE2DMS:
			return RT_UAV_TEXTURE2DMS;
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return RT_UAV_TEXTURE2DMSARRAY;
		case D3D_SRV_DIMENSION_TEXTURE3D:
			return RT_UAV_TEXTURE3D;
		case D3D_SRV_DIMENSION_TEXTURECUBE:
			return RT_UAV_TEXTURECUBE;
		case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
			return RT_UAV_TEXTURECUBEARRAY;
		default:
			return RT_UAV_BUFFER;
		}
	case D3D_SIT_STRUCTURED:
		return RT_SRV_STRUCTURED_BUFFER;
	case D3D_SIT_UAV_RWSTRUCTURED:
	case D3D_SIT_UAV_APPEND_STRUCTURED:
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		return RT_UAV_STRUCTURED_BUFFER;
	// D3D_SIT_TBUFFER???
	case D3D_SIT_BYTEADDRESS:
		return RT_SRV_BUFFER;
	case D3D_SIT_UAV_RWBYTEADDRESS:
		return RT_UAV_BUFFER;
	default:
		return RT_SRV_BUFFER;
	}
}

static bool GetStageData( ParserState& parserState, ID3D11ShaderReflection* reflection, StageInput& stage, std::map<StringReference, ParameterAnnotation> &annotations )
{
	tmFunction( 0, 0 );

	D3D11_SHADER_DESC reflDesc;
	if( FAILED( reflection->GetDesc( &reflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}

	reflection->GetThreadGroupSize( &stage.threadGroupSize[0], &stage.threadGroupSize[1], &stage.threadGroupSize[2] );

	for( unsigned cbIndex = 0; cbIndex < reflDesc.ConstantBuffers; ++cbIndex )
	{
		ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex( cbIndex );

		D3D11_SHADER_BUFFER_DESC cbDesc;
		cb->GetDesc( &cbDesc );

		if( strcmp( cbDesc.Name, "$Globals" ) )
		{
			D3D11_SHADER_INPUT_BIND_DESC resDesc;
			if( FAILED( reflection->GetResourceBindingDescByName( cbDesc.Name, &resDesc ) ) )
			{
				continue;
			}
			if( resDesc.BindPoint != 0 || resDesc.Type != D3D_SIT_CBUFFER )
			{
				continue;
			}
		}

		for( unsigned i = 0; i < cbDesc.Variables ; ++i )
		{
			ID3D11ShaderReflectionVariable* variable = cb->GetVariableByIndex( i );

			D3D11_SHADER_VARIABLE_DESC varDesc;
			if( FAILED( variable->GetDesc( &varDesc ) ) )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader constant #%i description", i );
				}
				continue;
			}
			if( ( varDesc.uFlags & D3D_SVF_USED ) == 0 )
			{
				continue;
			}

			ID3D11ShaderReflectionType* type = variable->GetType();
			D3D11_SHADER_TYPE_DESC typeDesc;
			if( FAILED( type->GetDesc( &typeDesc ) ) )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader constant \"%s\" type description", varDesc.Name );
				}
				continue;
			}
				
			Constant constant;
			constant.name = g_stringTable.AddString( varDesc.Name );
			constant.offset = varDesc.StartOffset;
			constant.size = varDesc.Size;

			switch( typeDesc.Type )
			{
			case D3D10_SVT_FLOAT:
				constant.type = CONSTANT_TYPE_FLOAT;
				break;
			case D3D10_SVT_INT:
				constant.type = CONSTANT_TYPE_INT;
				break;
			case D3D10_SVT_UINT:
				constant.type = CONSTANT_TYPE_UINT;
				break;
			case D3D10_SVT_BOOL:
				constant.type = CONSTANT_TYPE_BOOL;
				break;
			default:
				constant.type = CONSTANT_TYPE_OTHER;
			}
			switch( typeDesc.Class )
			{
			case D3D10_SVC_SCALAR:
				constant.dimension = 1;
				break;
			case D3D10_SVC_VECTOR:
				constant.dimension = 4;
				break;
			case D3D10_SVC_MATRIX_ROWS:
			case D3D10_SVC_MATRIX_COLUMNS:
				constant.dimension = 16;
				break;
			default:
				constant.dimension = 1;
			}
			constant.elements = typeDesc.Elements;
			constant.isSRGB = false;
			constant.isAutoregister = false;

			if( varDesc.DefaultValue )
			{
				stage.defaultValues.resize( std::max( stage.defaultValues.size(), size_t( constant.offset + constant.size ) ) );
				memcpy( &stage.defaultValues[constant.offset], varDesc.DefaultValue, constant.size );
			}

			if( annotations.find( constant.name ) == annotations.end() )
			{
				ParameterAnnotation paramAnnotations;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( varDesc.Name );
				if( symbol && symbol->annotations )
				{
					for( auto a = symbol->annotations->begin(); a != symbol->annotations->end(); ++a )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *a, result, constant.isSRGB, constant.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( a->name ).c_str() )] = result;
						}
					}
					if( !paramAnnotations.annotations.empty() )
					{
						annotations[constant.name] = paramAnnotations;
					}
				}
			}

			stage.constants.push_back( constant );
		}
	}

	for( unsigned i = 0; i < reflDesc.BoundResources; ++i )
	{
		D3D11_SHADER_INPUT_BIND_DESC desc;
		if( FAILED( reflection->GetResourceBindingDesc(i, &desc) ) )
		{
			if( g_printWarnings )
			{
				g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader resource #%i description", i );
			}
			continue;
		}
		switch( desc.Type )
		{
		case D3D_SIT_SAMPLER:
			{
				stage.registerInputs.push_back( { RT_SAMPLER, desc.BindPoint } );
			
				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr || symbol->definition == nullptr )
				{
					if( g_printWarnings )
					{
						g_messages.AddMessage( "\\memory(0): warning X0000: Could not find sampler \"%s\" definition in the source code", desc.Name );
					}
					continue;
				}
				Sampler sampler;
				sampler.name = g_stringTable.AddString( desc.Name );
				if( !GetSamplerState( parserState, symbol->definition, sampler ) )
				{
					return false;
				}
				stage.samplers[uint8_t( desc.BindPoint )] = sampler;
			}
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			{
				stage.registerInputs.push_back( { GetRegisterType( desc ), desc.BindPoint } );

				Uav uav;
				uav.isAutoregister = false;
				std::string uavName = desc.Name;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr )
				{
					continue;
				}
				if( symbol && symbol->annotations )
				{
					ParameterAnnotation paramAnnotations;

					bool srgb;
					uav.isAutoregister = false;

					for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *ai, result, srgb, uav.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;					
						}
					}

					if( !paramAnnotations.annotations.empty() )
					{
						annotations[ g_stringTable.AddString( uavName.c_str() ) ] = paramAnnotations;
					}
				}


				uav.name = g_stringTable.AddString( uavName.c_str() );
				if( !GetTextureType( desc, uav.type ) )
				{
					continue;
				}
				stage.uavs[uint8_t( desc.BindPoint )] = uav;
			}
			break;
		case D3D_SIT_CBUFFER:
		{
			stage.registerInputs.push_back( { RT_CONSTANT_BUFFER, desc.BindPoint } );
			break;
		}
		default:
			{
				stage.registerInputs.push_back( { GetRegisterType(desc ), desc.BindPoint } );

				Texture texture;
				texture.isSRGB = false;
				texture.isAutoregister = false;
				std::string textureName = desc.Name;

				Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
				if( symbol == nullptr )
				{
					continue;
				}
				// For legacy sampling functions (like tex2D) the reflection will create "dummy"
				// texture object with the same name as the sampler, so we need to find the
				// actual texture object from sampler definition.
				if( symbol && symbol->type.IsSampler() )
				{
					if( symbol->definition )
					{
						ASTNode* states = symbol->definition->GetChildOrNull( 1 );
						if( states && states->GetNodeType() == NT_SAMPLER_STATE_LIST )
						{
							for( size_t k = 0; k < states->GetChildrenCount(); ++k )
							{
								ASTNode* state = states->GetChild( k );
								std::string name = ToString( state->GetToken()->stringValue );
								if( _stricmp( name.c_str(), "Texture" ) == 0 )
								{
									ASTNode* value = state->GetChildOrNull( 0 );
									if( value && value->GetSymbol() )
									{
										textureName = ToString( value->GetSymbol()->name );
									}
								}
								else if( _stricmp( name.c_str(), "SRGBTexture" ) == 0 )
								{
									Sampler sampler;
									sampler.srgbTexture = 0;
									ParseStateAssignment( parserState, state, g_samplerStates, &sampler );
									texture.isSRGB = sampler.srgbTexture != 0;
								}
							}
						}
					}
				}
				else if( symbol && symbol->annotations )
				{
					ParameterAnnotation paramAnnotations;
				
					texture.isSRGB = false;
					texture.isAutoregister = false;

					for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *ai, result, texture.isSRGB, texture.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;					
						}
					}

					if( !paramAnnotations.annotations.empty() )
					{
						annotations[ g_stringTable.AddString( textureName.c_str() ) ] = paramAnnotations;
					}
				}


				texture.name = g_stringTable.AddString( textureName.c_str() );
				if( !GetTextureType( desc, texture.type ) )
				{
					continue;
				}
				stage.textures[uint8_t( desc.BindPoint )] = texture;
			}
		}
	}
	for( unsigned k = 0; k < reflDesc.InputParameters; ++k )
	{
		D3D11_SIGNATURE_PARAMETER_DESC desc;
		if( FAILED( reflection->GetInputParameterDesc( k, &desc ) ) )
		{
			g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader input parameter description" );
			return false;
		}
		if( desc.SystemValueType == D3D10_NAME_UNDEFINED )
		{
			bool found = false;
			PipelineInputDescription input;
			for( int n = 0; n < UC_NUM_USAGE_CODE; ++n )
			{
				if( _stricmp( desc.SemanticName, GetStringForUsageCode( n ) ) == 0 )
				{
					input.name = uint8_t( n );
					input.registerIndex = uint8_t( desc.Register );
					input.index = uint8_t( desc.SemanticIndex );
					input.usedMask = desc.ReadWriteMask;
					switch( desc.ComponentType )
					{
					case D3D_REGISTER_COMPONENT_FLOAT32:
						input.type = CONSTANT_TYPE_FLOAT;
						break;
					case D3D_REGISTER_COMPONENT_SINT32:
						input.type = CONSTANT_TYPE_INT;
						break;
					case D3D_REGISTER_COMPONENT_UINT32:
						input.type = CONSTANT_TYPE_UINT;
						break;
					default:
						input.type = CONSTANT_TYPE_OTHER;
						break;
					}
					if( desc.Mask >= ( 1 << 7 ) )
					{
						input.dimension = 8;
					}
					else if( desc.Mask >= ( 1 << 6 ) )
					{
						input.dimension = 7;
					}
					else if( desc.Mask >= ( 1 << 5 ) )
					{
						input.dimension = 6;
					}
					else if( desc.Mask >= ( 1 << 4 ) )
					{
						input.dimension = 5;
					}
					else if( desc.Mask >= ( 1 << 3 ) )
					{
						input.dimension = 4;
					}
					else if( desc.Mask >= ( 1 << 2 ) )
					{
						input.dimension = 3;
					}
					else if( desc.Mask >= ( 1 << 1 ) )
					{
						input.dimension = 2;
					}
					else
					{
						input.dimension = 1;
					}
					stage.pipelineInputs.push_back( input );
					found = true;
					break;
				}
			}
			if( !found && g_printWarnings )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: Shader uses unsupported input semantics \"%s\"", desc.SemanticName );
				return false;
			}
		}
	}
	return true;
}


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
			os << ( *it )->name;
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
	tmFunction( 0, 0 );

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

	ASTNode* functionHeader  = entryPointSymbol->definition->GetChildOrNull( 0 );
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
				( vposPath.back()->type.builtInType == OP_FLOAT && vposPath.back()->type.width == 4 && vposPath.back()->type.height == 1 ) )
			{
				fixVPOSType = false;
			}
		}
	}

	if( !wrapUniforms && !fixVPOS && !fixVPOSType && ( shaderStage != VERTEX_STAGE || outPositionPath.empty() ) )
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
						os << swizzle[std::min( k, positionPath.back()->type.width - 1 ) ];
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
						os << swizzle[std::min( k, symbol->type.width - 1 ) ];
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
	tmFunction( 0, 0 );

	return true;
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
			if( vsDesc.Register == psDesc.Register && ( ( ~vsDesc.Mask & psDesc.Mask ) == 0 ) )
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
	std::strstream os;
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
	tmFunction( 0, 0 );

	if( !listing.enabled() )
	{
		return;
	}

	CComPtr<ID3DBlob> disassembly;
	if( SUCCEEDED( D3DDisassemble( effectData->GetBufferPointer(), effectData->GetBufferSize(), D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, nullptr, &disassembly ) ) )
	{
		listing.literal( "asm" ).literal( reinterpret_cast<const char*>( disassembly->GetBufferPointer() ) );
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


void PrintStageInfo( YamlOutput& listing, const StageInput& stage, const EffectData& result )
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
				.end();
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
				.literal( "type" ).literal( ToString( it->type ) )
				.literal( "dimension" ).literal( it->dimension )
				.end();
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

std::string SanitizeCode( const std::string& src )
{
	tmFunction( 0, 0 );
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


bool EffectCompilerDX11::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result )
{
	return CompileEffect( source, sourceLength, defines, result, { nullptr, false } );
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

bool EffectCompilerDX11::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, const CompileOptions& compileOptions )
{
	tmFunction( 0, 0 );

	CComPtr<ID3D10Blob> effectData;
	CComPtr<ID3D10Blob> errors;

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

	std::vector<ASTNode*> techniqueNodes;
	state.GetTree()->FindNodes( NT_TECHNIQUE, techniqueNodes );
	if( techniqueNodes.empty() )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: No technique found" );
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
			listing.list();
			Pass outPass;
			ASTNode* passNode = techniqueNode->GetChild( passIx );
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
				effectData = nullptr;
				errors = nullptr;


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
				os << HLSL{ state.GetTree(), &state.GetSymbolTable() };

				std::string entryPoint = ToString( shaderNode->GetChild( 1 )->GetSymbol()->name );

				std::string patchEntryPoint = entryPoint;
				switch( PatchShader( stage.type, shaderNode->GetChild( 1 ), state, os, patchEntryPoint ) )
				{
				case PATCH_ERROR:
					return false;
				}
				state.ResetPragmaUsage();
				std::string code( os.str(), os.str() + os.pcount() );

				bool hasCompiled = false;
				{
					std::lock_guard scope( m_compiledCS );
					auto found = m_compiled.find( code );
					if( found != end( m_compiled ) )
					{
						effectData = found->second;
						if( !effectData )
						{
							return false;
						}
						hasCompiled = true;
					}
				}
				if( !hasCompiled )
				{
					HRESULT hr;
					{
						tmZone( 0, 0, "D3DCompile" );
						hr = D3DCompile(
							code.c_str(),
							code.length(),
							"\\memory",
							nullptr,
							nullptr,
							patchEntryPoint.c_str(),
							profile.c_str(),
							( compileOptions.minShaderVersion ? 0 : D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY ) | GetOptimizationLevel() | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | ( g_avoidFlowControl ? D3DCOMPILE_AVOID_FLOW_CONTROL : 0 ),
							0,
							&effectData,
							&errors );
					}
					if( FAILED( hr ) )
					{
						{
							std::lock_guard scope( m_compiledCS );
							m_compiled[code] = nullptr;
						}
						if( errors )
						{
							g_messages.AddMessages( errors );
						}
						return false;
					}
					{
						std::lock_guard scope( m_compiledCS );
						m_compiled[code] = effectData;
					}

					if( g_printWarnings && errors )
					{
						g_messages.AddMessages( errors );
					}
				}

				CComPtr<ID3DBlob> strippedEffectData;
				{
					tmZone( 0, 0, "D3DStripShader" );
					if( FAILED( D3DStripShader(
							effectData->GetBufferPointer(),
							effectData->GetBufferSize(),
							D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS,
							&strippedEffectData ) ) )
					{
						strippedEffectData = effectData;
					}
				}
				stage.shaderSize = uint32_t( strippedEffectData->GetBufferSize() );
				stage.shaderDataStr = g_stringTable.AddString( strippedEffectData->GetBufferPointer(), strippedEffectData->GetBufferSize() );

				CComPtr<ID3D11ShaderReflection> reflection;
				{
					tmZone( 0, 0, "D3DReflect" );

					if( FAILED( D3DReflect( effectData->GetBufferPointer(), effectData->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection.p ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection" );
						return false;
					}
				}

				if( !GetStageData( state, reflection, stage, result.annotations ) )
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
							if( MakeEffectAnnotationFromSymbolAnnotation( *a, symbolAnnotation, isSrgb, isAutoregister ) )
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
						.literal( "profile" )
						.literal( profile )
						.literal( "original" )
						.dict()
						.literal( "entryPoint" )
						.literal( patchEntryPoint )
						.literal( "source" )
						.literal( SanitizeCode( code ) );
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
		result.techniques.push_back( technique );
		listing.end(); // pases list
		listing.end(); // technique dict
	}
	listing.end(); // technique list

	return true;
}
#endif
