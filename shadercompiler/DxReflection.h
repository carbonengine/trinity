// Copyright © 2023 CCP ehf.

#pragma once

#include "stdafx.h"
#include "EffectData.h"
#include "ParserState.h"
#include "SymbolTable.h"
#include "CompileMessageQueue.h"
#include "FxAnalyzer.h"
#include "HLSLParser.h"
#include "ParserUtils.h"
#include "ASTNode.h"


extern StringTable g_stringTable;
extern CompileMessageQueue g_messages;
extern bool g_printWarnings;
extern unsigned g_optimizationLevel;
extern bool g_avoidFlowControl;
extern StateDescription g_samplerStates[];


namespace DxReflection
{
template <class F>
struct ArgType;

template <typename C, typename R, typename... Args>
struct ArgType<R ( C::* )( Args... ) noexcept>
{
	static const size_t nargs = sizeof...( Args );

	typedef R result_type;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
};

bool MakeEffectAnnotationFromSymbolAnnotation( const SymbolAnnotation& annotation, Annotation& result, bool& isSRGB, bool& isAutoregister );


template <typename T>
static bool GetTextureType( const T& desc, TextureType& type )
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
	case 12: // D3D_SIT_RTACCELERATIONSTRUCTURE:
		type = TEX_TYPE_RAYTRACING_ACCELERATION_STRUCTURE;
		break;
	default:
		return false;
	}
	return true;
}

template <typename T>
RegisterInputType GetRegisterType( const T& desc )
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

inline uint8_t GetSpaceFromDesc( const D3D11_SHADER_INPUT_BIND_DESC& )
{
	return 0;
}

inline uint8_t GetSpaceFromDesc( const D3D12_SHADER_INPUT_BIND_DESC& desc )
{
	return uint8_t( desc.Space );
}

struct ReflectionDx11
{
	using Reflection = ID3D11ShaderReflection;
	using ShaderDesc = D3D11_SHADER_DESC;
	using BufferDesc = D3D11_SHADER_BUFFER_DESC;
	using InputBindDesc = D3D11_SHADER_INPUT_BIND_DESC;
	using VariableDesc = D3D11_SHADER_VARIABLE_DESC;
	using TypeDesc = D3D11_SHADER_TYPE_DESC;
	using SignatureParamDesc = D3D11_SIGNATURE_PARAMETER_DESC;
};

struct FunctionDx12
{
	using Reflection = ID3D12FunctionReflection;
	using ShaderDesc = D3D12_FUNCTION_DESC;
	using BufferDesc = D3D12_SHADER_BUFFER_DESC;
	using InputBindDesc = D3D12_SHADER_INPUT_BIND_DESC;
	using VariableDesc = D3D12_SHADER_VARIABLE_DESC;
	using TypeDesc = D3D12_SHADER_TYPE_DESC;
	using SignatureParamDesc = D3D12_SIGNATURE_PARAMETER_DESC;
};

enum class ReflectionTarget
{
	SHADER_INPUT,
	LOCAL_INPUT,
	GLOBAL_INPUT,
};

template <typename Traits>
static bool ProcessReflection( ParserState& parserState, typename Traits::Reflection* reflection, bool collectStaticSamplers, StageData& stage, std::map<StringReference, ParameterAnnotation>& annotations, ReflectionTarget reflectionTarget = ReflectionTarget::SHADER_INPUT, const std::vector<GlobalInputElement>& filter = {} )
{
	ZoneScoped;

	typename Traits::ShaderDesc reflDesc;
	if( FAILED( reflection->GetDesc( &reflDesc ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Could not get shader reflection description" );
		return false;
	}

	for( unsigned cbIndex = 0; cbIndex < reflDesc.ConstantBuffers; ++cbIndex )
	{
		auto cb = reflection->GetConstantBufferByIndex( cbIndex );

		typename Traits::BufferDesc cbDesc;
		cb->GetDesc( &cbDesc );

		if( strcmp( cbDesc.Name, "$Globals" ) )
		{
			typename Traits::InputBindDesc resDesc;
			if( FAILED( reflection->GetResourceBindingDescByName( cbDesc.Name, &resDesc ) ) )
			{
				continue;
			}
			if( resDesc.BindPoint != 0 || resDesc.Type != D3D_SIT_CBUFFER )
			{
				continue;
			}
		}

		if( reflectionTarget == ReflectionTarget::GLOBAL_INPUT )
		{
			continue;
		}

		for( unsigned i = 0; i < cbDesc.Variables; ++i )
		{
			auto variable = cb->GetVariableByIndex( i );

			typename Traits::VariableDesc varDesc;
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

			auto type = variable->GetType();
			typename Traits::TypeDesc typeDesc;
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
			case D3D_SVT_FLOAT:
				constant.type = CONSTANT_TYPE_FLOAT;
				break;
			case D3D_SVT_INT:
				constant.type = CONSTANT_TYPE_INT;
				break;
			case D3D_SVT_UINT:
				constant.type = CONSTANT_TYPE_UINT;
				break;
			case D3D_SVT_BOOL:
				constant.type = CONSTANT_TYPE_BOOL;
				break;
			default:
				constant.type = CONSTANT_TYPE_OTHER;
			}
			switch( typeDesc.Class )
			{
			case D3D_SVC_SCALAR:
				constant.dimension = 1;
				break;
			case D3D_SVC_VECTOR:
				constant.dimension = uint8_t( varDesc.Size / 4 );
				break;
			case D3D_SVC_MATRIX_ROWS:
			case D3D_SVC_MATRIX_COLUMNS:
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

			if( Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( varDesc.Name ) )
			{
				if( annotations.find( constant.name ) == annotations.end() )
				{
					ParameterAnnotation paramAnnotations;
					if( symbol->annotations )
					{
						for( auto a = symbol->annotations->begin(); a != symbol->annotations->end(); ++a )
						{
							Annotation result;
							if( MakeEffectAnnotationFromSymbolAnnotation( *a, result, constant.isSRGB, constant.isAutoregister ) )
							{
								paramAnnotations.annotations[g_stringTable.AddString( ToString( a->name ).c_str() )] = result;
							}
						}
					}
					if( symbol->type.IsBindlessHandle() )
					{
						Annotation symbolAnnotation;
						symbolAnnotation.type = ANNOTATION_TYPE_INT;
						symbolAnnotation.intValue = symbol->type.builtInType == OP_BINDLESSHANDLESAMPLER ? 100 : BindlessTextureType( symbol->type.builtInType );
						paramAnnotations.annotations[g_stringTable.AddString( "BindlessHandleType" )] = symbolAnnotation;
					}
					if( !paramAnnotations.annotations.empty() )
					{
						annotations[constant.name] = paramAnnotations;
					}
				}
				if( symbol->type.IsBindlessHandle() && symbol->type.builtInType == OP_BINDLESSHANDLESAMPLER )
				{
					auto found = std::find_if( begin( parserState.m_bindlessSamplers ), end( parserState.m_bindlessSamplers ), [symbol]( const auto& bs ) { return bs.name == symbol; } );
					if( found != end( parserState.m_bindlessSamplers ) )
					{
						Sampler sampler;
						if( GetSamplerState( parserState, found->definition, sampler ) )
						{
							sampler.name = constant.name;
							stage.samplers[uint8_t( 100 + ( found - begin( parserState.m_bindlessSamplers ) ) )] = sampler;
						}
					}
				}
			}
			stage.constants.push_back( constant );
		}
	}

	auto GetRegisterSpace = []( Symbol* symbol ) -> uint8_t {
		if( symbol )
		{
			if( !symbol->registerSpecifier.empty() )
			{
				auto space = symbol->registerSpecifier.begin()->second.space;
				if( space > 0 )
				{
					return uint8_t( space );
				}
			}
		}
		return 0;
	};


	for( unsigned i = 0; i < reflDesc.BoundResources; ++i )
	{
		typename Traits::InputBindDesc desc;
		if( FAILED( reflection->GetResourceBindingDesc( i, &desc ) ) )
		{
			if( g_printWarnings )
			{
				g_messages.AddMessage( "\\memory(0): warning X0000: Could not get shader resource #%i description", i );
			}
			continue;
		}

		switch( reflectionTarget )
		{
		case ReflectionTarget::LOCAL_INPUT:
			if( desc.Type == D3D_SIT_CBUFFER )
			{
				auto found = std::find_if( begin( filter ), end( filter ), [desc]( const auto& f ) { return f.name == desc.Name; } );
				if( found != end( filter ) )
				{
					continue;
				}
				break;
			}
			else
			{
				continue;
			}
		case ReflectionTarget::GLOBAL_INPUT:
			if( desc.Type == D3D_SIT_CBUFFER )
			{
				auto found = std::find_if( begin( filter ), end( filter ), [desc]( const auto& f ) { return f.name == desc.Name; } );
				if( found == end( filter ) )
				{
					continue;
				}
			}
			else
			{
				auto found = std::find_if( begin( filter ), end( filter ), [desc]( const auto& f ) { return f.name == desc.Name; } );
				if( found == end( filter ) )
				{
					g_messages.AddMessage( "\\memory(0): error X0000: Ray tracing shader may not use SRV/UAV/Sampler \"%s\" not declared in the globalinput", desc.Name );
					return false;
				}
			}
			break;
		default:
			break;
		}

		switch( desc.Type )
		{
		case D3D_SIT_SAMPLER: {
			//stage.registerInputs.push_back( { RT_SAMPLER, desc.BindPoint } );

			Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
			if( symbol == nullptr || symbol->definition == nullptr )
			{
				if( g_printWarnings )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Could not find sampler \"%s\" definition in the source code", desc.Name );
				}
				stage.registerInputs.push_back( { RT_SAMPLER, desc.BindPoint, desc.BindCount, GetRegisterSpace( symbol ) } );
				continue;
			}
			Sampler sampler;
			//sampler.name = g_stringTable.AddString( desc.Name );
			if( !GetSamplerState( parserState, symbol->definition, sampler ) )
			{
				return false;
			}
			StaticSampler staticSampler;
			if( desc.BindCount == 1 && collectStaticSamplers && ConvertToStaticSampler( staticSampler, sampler ) )
			{
				staticSampler.registerIndex = desc.BindPoint;
				staticSampler.registerSpace = GetRegisterSpace( symbol );
				stage.staticSamplers.push_back( staticSampler );
			}
			else
			{
				sampler.name = g_stringTable.AddString( desc.Name );
				stage.samplers[uint8_t( desc.BindPoint )] = sampler;
				stage.registerInputs.push_back( { RT_SAMPLER, desc.BindPoint, desc.BindCount, GetRegisterSpace( symbol ) } );

				if( symbol && symbol->annotations )
				{
					ParameterAnnotation paramAnnotations;
					for( auto ai = symbol->annotations->begin(); ai != symbol->annotations->end(); ++ai )
					{
						Annotation result;
						bool srgb, autoregister;
						if( MakeEffectAnnotationFromSymbolAnnotation( *ai, result, srgb, autoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( ToString( ai->name ).c_str() )] = result;
						}
					}
					if( !paramAnnotations.annotations.empty() )
					{
						annotations[g_stringTable.AddString( desc.Name )] = paramAnnotations;
					}
				}
			}
			//stage.samplers[uint8_t( desc.BindPoint )] = sampler;
		}
		break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: {
			//stage.registerInputs.push_back( { GetRegisterType( desc ), desc.BindPoint } );
			Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
			stage.registerInputs.push_back( { GetRegisterType( desc ), desc.BindPoint, desc.BindCount, GetRegisterSpace( symbol ) } );

			Uav uav;
			uav.count = desc.BindCount;
			uav.isAutoregister = false;
			std::string uavName = desc.Name;

			//Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
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
					annotations[g_stringTable.AddString( uavName.c_str() )] = paramAnnotations;
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
		case D3D_SIT_CBUFFER: {
			stage.registerInputs.push_back( { GetRegisterType( desc ), desc.BindPoint, desc.BindCount, GetSpaceFromDesc( desc ) } );
			break;
		}
		default: {
			Symbol* symbol = parserState.GetSymbolTable().LookupGlobal( desc.Name );
			stage.registerInputs.push_back( { GetRegisterType( desc ), desc.BindPoint, desc.BindCount, GetRegisterSpace( symbol ) } );

			Texture texture;
			texture.count = desc.BindCount;
			texture.isSRGB = false;
			texture.isAutoregister = false;
			std::string textureName = desc.Name;


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
					if( states && states->GetNodeType() == ASTNodeType::NT_SAMPLER_STATE_LIST )
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
					annotations[g_stringTable.AddString( textureName.c_str() )] = paramAnnotations;
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
	return true;
}

template <typename Traits>
static bool ProcessReflection( ParserState& parserState, typename Traits::Reflection* reflection, bool collectStaticSamplers, StageInput& stage, std::map<StringReference, ParameterAnnotation>& annotations )
{
	if( !ProcessReflection<Traits>( parserState, reflection, collectStaticSamplers, static_cast<StageData&>( stage ), annotations ) )
	{
		return false;
	}

	typename Traits::ShaderDesc reflDesc;
	reflection->GetDesc( &reflDesc );

	reflection->GetThreadGroupSize( &stage.threadGroupSize[0], &stage.threadGroupSize[1], &stage.threadGroupSize[2] );

	for( unsigned k = 0; k < reflDesc.InputParameters; ++k )
	{
		typename Traits::SignatureParamDesc desc;
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
}
