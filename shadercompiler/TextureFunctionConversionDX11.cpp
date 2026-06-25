// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "TextureFunctionConversionDX11.h"
#include "ASTNode.h"
#include "HLSLParser.h"
#include "EffectData.h"
#include "ParserUtils.h"
#include "FXAnalyzer.h"
#include "ParserState.h"

struct SamplerToTexture
{
	Symbol* texture;
	bool processed;
};

struct ParameterInfo
{
	unsigned parameterIndex;
	unsigned argumentIndex;
};

static int GetTextureType( const InlineString& name )
{
	const char* s_textureName[] = { "1D", "2D", "3D", "CUBE" };
	const int s_textureType[] = { OP_TEXTURE1D, OP_TEXTURE2D, OP_TEXTURE3D, OP_TEXTURECUBE };
	for( unsigned i = 0; i < sizeof( s_textureType ) / sizeof( int ); ++i )
	{
		if( ContainsSubstring( name, s_textureName[i] ) )
		{
			return s_textureType[i];
		}
	}
	assert( false );
	return TEX_TYPE_TYPELESS;
}

static Symbol* ExtractTextureState( Symbol* symbol )
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
				return value->GetSymbol();
			}
		}
	}
	return nullptr;
}

static ASTNode* GetSamplerArg( ASTNode* arg )
{
	if( arg == nullptr )
	{
		return nullptr;
	}
	switch( arg->GetNodeType() )
	{
	case NT_VAR_IDENTIFIER:
		return arg;
	case NT_EXPRESSION:
		if( arg->GetToken()->type == OP_LEFT_PAREN )
		{
			return GetSamplerArg( arg->GetChildOrNull( 0 ) );
		}
	default:
		break;
	}
	return nullptr;
}

static const char* GetTextureTypeName( int textureType )
{
	switch( textureType )
	{
	case OP_TEXTURE1D:
		return "1D";
	case OP_TEXTURE2D:
		return "2D";
	case OP_TEXTURE3D:
		return "3D";
	case OP_TEXTURECUBE:
		return "CUBE";
	default:
		return "";
	}
}

static bool AssignTextureType( ParserState& state, Symbol* texture, int type, const ScannerToken& token )
{
	switch( texture->type.builtInType )
	{
	case OP_TEXTURE1D:
	case OP_TEXTURE2D:
	case OP_TEXTURE3D:
	case OP_TEXTURECUBE:
		if( texture->type.builtInType != type )
		{
			state.ShowMessage( token.fileLocation, EC_MISMATCHED_TEXTURE_TYPE, ToString( texture->name ).c_str(), GetTextureTypeName( texture->type.builtInType ), GetTextureTypeName( type ) );
			texture->type.builtInType = type;
			if( texture->definition )
			{
				ASTNode* textureType = texture->definition->GetChildOrNull( 0 );
				if( textureType )
				{
					textureType->SetType( texture->type );
				}
			}
		}
		break;
	case OP_TEXTURE:
		texture->type.builtInType = type;
		if( texture->definition )
		{
			ASTNode* textureType = texture->definition->GetChildOrNull( 0 );
			if( textureType )
			{
				textureType->SetType( texture->type );
			}
		}
		break;
	default:
		state.ShowMessage( token.fileLocation, EC_NO_OVERRIDE, ToString( token.stringValue ).c_str(), "" );
		return false;
	}
	return true;
}

static Symbol* FindTextureToSampler( ParserState& state, ASTNode* sampler, std::map<Symbol*, SamplerToTexture>& samplers, int usedType, const ScannerToken& token )
{
	sampler = GetSamplerArg( sampler );
	if( sampler && sampler->GetSymbol() )
	{
		Symbol* texture = nullptr;
		auto found = samplers.find( sampler->GetSymbol() );
		if( found != samplers.end() && found->second.texture )
		{
			int oldType = found->second.texture->type.builtInType;
			if( usedType != OP_TEXTURE && oldType != usedType )
			{
				state.ShowMessage( token.fileLocation, EC_MISMATCHED_SAMPLER_TYPE, ToString( sampler->GetSymbol()->name ).c_str(), GetTextureTypeName( usedType ), GetTextureTypeName( oldType ) );
			}
			return found->second.texture;
		}
		else if( sampler->GetSymbol()->definition && sampler->GetSymbol()->definition->GetNodeType() == NT_FUNCTION_PARAMETER )
		{
			InlineString str = state.AllocateName();
			texture = state.GetSymbolTable().AddSymbol( str );
			texture->type.FromTokenType( usedType );
			if( !sampler->GetSymbol()->registerSpecifier.empty() )
			{
				texture->registerSpecifier = sampler->GetSymbol()->registerSpecifier;
				for( auto it = texture->registerSpecifier.begin(); it != texture->registerSpecifier.end(); ++it )
				{
					it->second.registerType = 't';
				}
			}
		}
		else
		{
			texture = ExtractTextureState( sampler->GetSymbol() );
			if( texture == nullptr )
			{
				state.ShowMessage( token.fileLocation, EC_SAMPLER_WITHOUT_TEXTURE, ToString( sampler->GetSymbol()->name ).c_str() );

				ASTNode* texDecl = new ASTNode( NT_NAME_DECLARATION, sampler->GetLocation(), sampler->GetScope(), nullptr );

				Type type;
				type.FromTokenType( OP_TEXTURE );
				texDecl->SetType( type );

				texture = new Symbol;
				texture->name = sampler->GetSymbol()->name;
				texture->type = type;
				texDecl->SetSymbol( texture );
				state.GetSymbolTable().AddSymbol( texture, ALLOW_OVERRIDES );

				state.GetTree()->InsertChild( 0, texDecl );
			}

			AssignTextureType( state, texture, usedType, token );
			if( !sampler->GetSymbol()->registerSpecifier.empty() )
			{
				if( texture->registerSpecifier.empty() )
				{
					texture->registerSpecifier = sampler->GetSymbol()->registerSpecifier;
					for( auto it = texture->registerSpecifier.begin(); it != texture->registerSpecifier.end(); ++it )
					{
						it->second.registerType = 't';
					}
				}
			}
		}
		SamplerToTexture record;
		record.texture = texture;
		record.processed = false;
		samplers[sampler->GetSymbol()] = record;
		return texture;
	}
	return nullptr;
}

static ASTNode* FindDX9TexSampleCalls( ParserState& state,
									   ASTNode* node,
									   const std::set<Symbol*>& dx9TextureFunctions,
									   std::map<Symbol*, SamplerToTexture>& samplers )
{
	if( node == nullptr )
	{
		return nullptr;
	}
	ASTNode* result = nullptr;
	if( node->GetNodeType() == NT_FUNCTION_CALL )
	{
		if( node->GetSymbol() )
		{
			auto found = dx9TextureFunctions.find( node->GetSymbol() );
			if( found != dx9TextureFunctions.end() )
			{
				int type = GetTextureType( node->GetSymbol()->name );
				Symbol* texture = FindTextureToSampler( state, node->GetChildOrNull( 0 ), samplers, type, *node->GetToken() );

				if( texture )
				{
					Symbol* otherSymbol = node->GetScope()->Lookup( texture->name );
					if( otherSymbol != nullptr && otherSymbol != texture )
					{
						// we have a local symbol with the same name as the texture
						// rename it
						otherSymbol->name = state.AllocateName();
					}
					ScannerToken token;
					token.fileLocation.fileName.start = token.fileLocation.fileName.end = nullptr;
					token.fileLocation.lineNumber = 0;
					token.type = OP_DOT;
					ASTNode* var = new ASTNode( NT_VAR_IDENTIFIER, node->GetLocation(), node->GetScope(), nullptr );
					var->SetSymbol( texture );
					var->SetType( texture->type );
					result = new ASTNode( NT_POSTFIX_EXPRESSION, node->GetLocation(), node->GetScope(), &token );
					result->AddChild( var );
					result->AddChild( node );

					const char* sample;
					if( ContainsSubstring( node->GetSymbol()->name, "lod" ) )
					{
						const char* s_sample = "SampleLevel";
						sample = s_sample;
						ASTNode* coord = node->GetChildOrNull( 1 );
						if( coord )
						{
							// we need to convert tex##lod( sampler, xyzw ) to
							// texture.SampleLevel( sampler, xyz, w )
							// TODO: we do it in a dirty way: texture.SampleLevel( sampler, xyzw.###, (xyzw).w )
							// which is not OK if "xyzw" contains side effects
							if( coord->GetType().width != 1 )
							{
								ASTNode* lod = coord->Copy();
								ScannerToken swizzle;
								swizzle.fileLocation.fileName.start = swizzle.fileLocation.fileName.end = nullptr;
								swizzle.fileLocation.lineNumber = 0;
								swizzle.intValue = 0;
								swizzle.stringValue = MakeInlineString( "w" );
								swizzle.type = OP_ID;

								ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
								dot->AddChild( lod );
								node->InsertChild( 2, dot );

								bool doSwizzle = true;
								switch( node->GetSymbol()->name.start[3] )
								{
								case '1':
									swizzle.stringValue = MakeInlineString( "x" );
									break;
								case '2':
									swizzle.stringValue = MakeInlineString( "xy" );
									break;
								case '3':
								case 'C':
									swizzle.stringValue = MakeInlineString( "xyz" );
									break;
								default:
									doSwizzle = false;
								}
								if( doSwizzle )
								{
									ASTNode* swizzleDot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									swizzleDot->AddChild( coord );
									node->ReplaceChild( 1, swizzleDot );
									coord = swizzleDot;
								}
							}
							else
							{
								node->InsertChild( 2, coord->Copy() );
							}
						}
					}
					else if( ContainsSubstring( node->GetSymbol()->name, "bias" ) )
					{
						const char* s_sample = "SampleBias";
						sample = s_sample;
						ASTNode* coord = node->GetChildOrNull( 1 );
						if( coord )
						{
							// we need to convert tex##bias( sampler, xyzw ) to
							// texture.SampleBias( sampler, xyz, w )
							// TODO: we do it in a dirty way: texture.SampleLevel( sampler, xyzw, (xyzw).w )
							// which is not OK if "xyzw" contains side effects
							if( coord->GetType().width != 1 )
							{
								ASTNode* lod = coord->Copy();
								ScannerToken swizzle;
								swizzle.fileLocation.fileName.start = swizzle.fileLocation.fileName.end = nullptr;
								swizzle.fileLocation.lineNumber = 0;
								swizzle.intValue = 0;
								swizzle.stringValue.start = "w";
								swizzle.stringValue.end = swizzle.stringValue.start + 1;
								swizzle.type = OP_ID;

								ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
								dot->AddChild( lod );
								node->InsertChild( 2, dot );

								bool doSwizzle = true;
								switch( node->GetSymbol()->name.start[3] )
								{
								case '1':
									swizzle.stringValue = MakeInlineString( "x" );
									break;
								case '2':
									swizzle.stringValue = MakeInlineString( "xy" );
									break;
								case '3':
								case 'C':
									swizzle.stringValue = MakeInlineString( "xyz" );
									break;
								default:
									doSwizzle = false;
								}
								if( doSwizzle )
								{
									ASTNode* swizzleDot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									swizzleDot->AddChild( coord );
									node->ReplaceChild( 1, swizzleDot );
									coord = swizzleDot;
								}
							}
							else
							{
								node->InsertChild( 2, coord->Copy() );
							}
						}
					}
					else if( ContainsSubstring( node->GetSymbol()->name, "proj" ) )
					{
						const char* s_sample = "Sample";
						sample = s_sample;
						ASTNode* coord = node->GetChildOrNull( 1 );
						if( coord )
						{
							// we need to convert tex##proj( sampler, xyzw ) to
							// texture.Sample( sampler, xyz / w )
							// TODO: we do it in a dirty way: texture.SampleLevel( sampler, xyzw / (xyzw).w )
							// which is not OK if "xyzw" contains side effects
							if( coord->GetType().width != 1 )
							{
								ASTNode* lod = coord->Copy();
								ScannerToken swizzle;
								swizzle.fileLocation.fileName.start = swizzle.fileLocation.fileName.end = nullptr;
								swizzle.fileLocation.lineNumber = 0;
								swizzle.intValue = 0;
								swizzle.stringValue.start = "w";
								swizzle.stringValue.end = swizzle.stringValue.start + 1;
								swizzle.type = OP_ID;

								ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
								dot->AddChild( lod );

								ScannerToken div;
								div.fileLocation.fileName.start = div.fileLocation.fileName.end = nullptr;
								div.fileLocation.lineNumber = 0;
								div.intValue = 0;
								div.stringValue.start = nullptr;
								div.stringValue.end = nullptr;
								div.type = OP_SLASH;

								ASTNode* expr = new ASTNode( NT_EXPRESSION, lod->GetLocation(), lod->GetScope(), &div );

								bool doSwizzle = true;
								switch( node->GetSymbol()->name.start[3] )
								{
								case '1':
									swizzle.stringValue = MakeInlineString( "x" );
									break;
								case '2':
									swizzle.stringValue = MakeInlineString( "xy" );
									break;
								case '3':
								case 'C':
									swizzle.stringValue = MakeInlineString( "xyz" );
									break;
								default:
									doSwizzle = false;
								}
								if( doSwizzle )
								{
									ASTNode* swizzleDot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									swizzleDot->AddChild( coord );
									node->ReplaceChild( 1, swizzleDot );
									coord = swizzleDot;
								}

								expr->AddChild( coord );
								expr->AddChild( dot );
								node->ReplaceChild( 1, expr );
							}
							else
							{
								ScannerToken div;
								div.fileLocation.fileName.start = div.fileLocation.fileName.end = nullptr;
								div.fileLocation.lineNumber = 0;
								div.intValue = 0;
								div.stringValue.start = nullptr;
								div.stringValue.end = nullptr;
								div.type = OP_SLASH;

								ASTNode* expr = new ASTNode( NT_EXPRESSION, coord->GetLocation(), coord->GetScope(), &div );
								expr->AddChild( coord );
								expr->AddChild( coord->Copy() );
								node->ReplaceChild( 1, expr );
							}
						}
					}
					else if( node->GetChildrenCount() == 4 || ContainsSubstring( node->GetSymbol()->name, "grad" ) )
					{
						// this is a form tex##( sampler, coord, ddx, ddy )
						const char* s_sample = "SampleGrad";
						sample = s_sample;
					}
					else
					{
						const char* s_sample = "Sample";
						sample = s_sample;
					}
					token.type = OP_ID;
					token.stringValue = MakeInlineString( sample );

					node->SetSymbol( nullptr );
					node->SetToken( &token );
				}
			}
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		ASTNode* ret = FindDX9TexSampleCalls( state, node->GetChild( i ), dx9TextureFunctions, samplers );
		if( ret )
		{
			node->ReplaceChild( i, ret );
		}
	}
	return result;
}

static void PatchFunctionHeaders( ASTNode* node,
								  std::map<Symbol*, SamplerToTexture>& samplers,
								  std::map<Symbol*, std::vector<ParameterInfo>>& functions )
{
	if( node == nullptr )
	{
		return;
	}

	if( node->GetNodeType() == NT_FUNCTION_HEADER )
	{
		std::vector<ParameterInfo> patchedParams;
		unsigned callIndex = 0;
		for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
		{
			auto found = samplers.find( node->GetChild( i )->GetSymbol() );
			if( found == samplers.end() || found->second.processed )
			{
				++callIndex;
				continue;
			}

			ASTNode* newParam = new ASTNode( NT_FUNCTION_PARAMETER, node->GetLocation(), node->GetScope(), nullptr );
			found->second.texture->definition = newParam;
			newParam->SetType( found->second.texture->type );
			newParam->SetSymbol( found->second.texture );

			node->InsertChild( i, newParam );

			ParameterInfo info;
			info.argumentIndex = callIndex;
			info.parameterIndex = i;
			patchedParams.push_back( info );
			++i;
			++callIndex;
			found->second.processed = true;
		}
		if( !patchedParams.empty() )
		{
			functions[node->GetSymbol()] = patchedParams;
		}
	}

	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		PatchFunctionHeaders( node->GetChild( i ), samplers, functions );
	}
}

static void PatchCalls( ParserState& state,
						ASTNode* node,
						std::map<Symbol*, SamplerToTexture>& samplers,
						const std::map<Symbol*, std::vector<ParameterInfo>>& functions )
{
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_FUNCTION_CALL )
	{
		auto found = functions.find( node->GetSymbol() );
		if( found != functions.end() )
		{
			for( auto it = found->second.rbegin(); it != found->second.rend(); ++it )
			{
				Symbol* samplerArg = nullptr;
				ASTNode* arg = GetSamplerArg( node->GetChildOrNull( it->argumentIndex ) );
				if( arg )
				{
					samplerArg = arg->GetSymbol();
				}
				if( samplerArg == nullptr )
				{
					assert( false );
					return;
				}
				Symbol* texture = nullptr;
				auto textureFound = samplers.find( samplerArg );
				if( textureFound == samplers.end() )
				{
					if( samplerArg->definition && samplerArg->definition->GetNodeType() != NT_FUNCTION_PARAMETER )
					{
						texture = ExtractTextureState( samplerArg );

						if( texture )
						{
							if( !samplerArg->registerSpecifier.empty() )
							{
								if( texture->registerSpecifier.empty() )
								{
									texture->registerSpecifier = samplerArg->registerSpecifier;
									for( auto& registerSpecifier : texture->registerSpecifier )
									{
										registerSpecifier.second.registerType = 't';
									}
								}
							}
						}
					}
				}
				else
				{
					texture = textureFound->second.texture;
				}
				if( texture == nullptr )
				{
					if( samplerArg->definition && samplerArg->definition->GetNodeType() == NT_FUNCTION_PARAMETER && node->GetSymbol() && node->GetSymbol()->definition )
					{
						ASTNode* header = node->GetSymbol()->definition;
						if( header->GetNodeType() == NT_FUNCTION_DEFINITION )
						{
							header = header->GetChildOrNull( 0 );
						}
						if( header )
						{
							unsigned paramNumber = it->parameterIndex;
							ASTNode* textureParam = header->GetChildOrNull( paramNumber );
							if( textureParam )
							{
								int type = textureParam->GetType().builtInType;
								texture = FindTextureToSampler( state, node->GetChildOrNull( 0 ), samplers, type, *node->GetToken() );
							}
						}
					}
					if( texture == nullptr || texture->type.symbol )
					{
						assert( false );
						return;
					}
				}
				ASTNode* header = node->GetSymbol()->definition;
				if( header && header->GetNodeType() == NT_FUNCTION_DEFINITION )
				{
					header = header->GetChildOrNull( 0 );
				}
				if( header )
				{
					ASTNode* param = header->GetChildOrNull( it->parameterIndex );
					if( param && param->GetSymbol() )
					{
						int usedType = param->GetSymbol()->type.builtInType;
						AssignTextureType( state, texture, usedType, *node->GetToken() );
					}
				}
				ASTNode* textureArg = new ASTNode( NT_VAR_IDENTIFIER, node->GetLocation(), node->GetScope(), nullptr );
				textureArg->SetSymbol( texture );
				node->InsertChild( it->argumentIndex, textureArg );
			}
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		PatchCalls( state, node->GetChild( i ), samplers, functions );
	}
}

void ConvertTextureFunctionsDX11( ParserState& state )
{
	ZoneScoped;

	std::map<Symbol*, SamplerToTexture> samplers;
	std::map<Symbol*, std::vector<ParameterInfo>> functions;

	FindDX9TexSampleCalls( state, state.GetTree(), state.GetDX9TextureFunctions(), samplers );
	if( state.HasErrors() )
	{
		return;
	}
	while( true )
	{
		PatchFunctionHeaders( state.GetTree(), samplers, functions );
		if( state.HasErrors() || functions.empty() )
		{
			break;
		}

		PatchCalls( state, state.GetTree(), samplers, functions );
		if( state.HasErrors() )
		{
			break;
		}

		functions.clear();
	}
}

static void TransferSRGBToTexturesDX11( ParserState& state, ASTNode* node )
{
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_SAMPLER_STATE_LIST )
	{
		Symbol* texture = nullptr;
		ASTNode* isSRGB = nullptr;

		for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
		{
			if( node->GetChild( i ) && node->GetChild( i )->GetToken() )
			{
				if( _stricmp( ToString( node->GetChild( i )->GetToken()->stringValue ).c_str(), "texture" ) == 0 )
				{
					texture = node->GetChild( i )->GetChild( 0 )->GetSymbol();
				}
				else if( _stricmp( ToString( node->GetChild( i )->GetToken()->stringValue ).c_str(), "SRGBTexture" ) == 0 )
				{
					isSRGB = node->GetChild( i );
				}
			}
		}
		if( texture && isSRGB )
		{
			Sampler sampler;
			sampler.srgbTexture = 0;
			ParseStateAssignment( state, isSRGB, g_samplerStates, &sampler );

			SymbolAnnotation annotation;
			annotation.type = OP_BOOL;
			annotation.name = MakeInlineString( "Tr2sRGB" );
			annotation.value.fileLocation.fileName = MakeInlineString( "" );
			annotation.value.fileLocation.lineNumber = 0;
			annotation.value.intValue = sampler.srgbTexture ? 1 : 0;
			annotation.value.type = OP_BOOL_CONST;
			annotation.value.stringValue = MakeInlineString( sampler.srgbTexture ? "true" : "false" );

			if( texture->annotations == nullptr )
			{
				texture->annotations = new SymbolAnnotations();
			}
			texture->annotations->push_back( annotation );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		TransferSRGBToTexturesDX11( state, node->GetChild( i ) );
	}
}

void TransferSRGBToTexturesDX11( ParserState& state )
{
	ZoneScoped;

	TransferSRGBToTexturesDX11( state, state.GetTree() );
}

void SplitCoordVec( ASTNode* call, const Type& textureType, size_t coordOffset = 1 )
{
	switch( textureType.builtInType )
	{
	case OP_TEXTURE1DARRAY:
	case OP_TEXTURE2DARRAY:
	case OP_TEXTURE3DARRAY:
	case OP_TEXTURECUBEARRAY: {
		const char* xyzw = "xyzw";

		auto coord = call->GetChild( coordOffset );
		ScannerToken swizzle = ScannerToken::ID( MakeInlineString( xyzw, xyzw + coord->GetType().width - 1 ) );
		ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
		dot->AddChild( coord->Copy() );
		call->ReplaceChild( coordOffset, dot );

		swizzle = ScannerToken::ID( MakeInlineString( xyzw + coord->GetType().width - 1, xyzw + coord->GetType().width ) );
		dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
		dot->AddChild( coord->Copy() );

		ScannerToken typeToken = ScannerToken::FromTokenType( OP_UINT );
		typeToken.intValue = 1;
		ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
		cast->SetType( TypeFromTokenType( OP_UINT ) );
		cast->AddChild( dot );

		call->InsertChild( coordOffset + 1, cast );
		break;
	}
	default:
		break;
	}
};


void PatchMetalBufferCalls( ParserState& state, ASTNode* node )
{
	if( !node )
	{
		return;
	}
	if( node->GetNodeType() == NT_POSTFIX_EXPRESSION &&
		node->GetToken() &&
		node->GetToken()->type == OP_DOT &&
		node->GetChildrenCount() == 2 &&
		node->GetChild( 0 )->GetType().IsBuffer() &&
		node->GetChild( 1 )->GetNodeType() == NT_FUNCTION_CALL )
	{
		ASTNode* call = node->GetChild( 1 );
		auto functionToken = *call->GetToken();
		if( functionToken.stringValue == "Load" )
		{
			// b.Load(index) -> b[index]
			if( call->GetChildrenCount() == 2 )
			{
				state.ShowMessage( call->GetLocation(), EC_CUSTOM_ERROR, "Metal does not support Load with status argument" );
			}
			else
			{
				auto token = *node->GetToken();
				token.type = OP_LEFT_BRACKET;
				node->SetToken( &token );
				node->AddChild( call->GetChild( 0 ) );
				node->RemoveChild( 1 );
			}
		}
		else if( functionToken.stringValue == "GetDimensions" )
		{
			state.ShowMessage( call->GetLocation(), EC_CUSTOM_ERROR, "Metal does not support GetDimensions method for buffers" );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		PatchMetalBufferCalls( state, node->GetChild( i ) );
	}
}

ASTNode* PatchMetalTextureCall( ASTNode* node )
{
	auto textureType = node->GetChild( 0 )->GetType();
	ASTNode* call = node->GetChild( 1 );

	auto WrapInOption = [=]( const std::vector<size_t>& childIndexes, const char* option ) {
		ScannerToken token = ScannerToken::ID( MakeInlineString( option ) );
		token.fileLocation = call->GetLocation();
		ASTNode* options = new ASTNode( NT_FUNCTION_CALL, call->GetLocation(), call->GetScope(), &token );
		options->SetSymbol( nullptr );
		for( auto child : childIndexes )
		{
			options->AddChild( call->GetChild( child ) );
		}
		call->ReplaceChild( unsigned( childIndexes[0] ), options );
		for( size_t i = childIndexes.size() - 1; i > 0; --i )
		{
			call->RemoveChild( unsigned( childIndexes[i] ) );
		}
	};

	bool isGatherCall = false;

	auto functionToken = *call->GetToken();
	if( functionToken.stringValue == "Sample" )
	{
		// t.Sample(sampler, coord [,offset]) -> t.sample(sampler, coord, [offset])
		functionToken.stringValue = MakeInlineString( "sample" );
		// texture arrays: t.Sample(sampler, coord [,offset]) -> t.sample(sampler, coord., coord., [offset])
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "SampleBias" )
	{
		// t.SampleBias(sampler, coord, bias) -> t.sample(sampler, coord, bias(bias))
		// TODO: support offset?
		functionToken.stringValue = MakeInlineString( "sample" );
		WrapInOption( { 2 }, "bias" );
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "SampleGrad" )
	{
		// t.SampleGrad(sampler, coord, dx, dy) -> t.sample(sampler, coord, gradient#(dx, dy))
		// TODO: support offset?
		functionToken.stringValue = MakeInlineString( "sample" );
		switch( textureType.builtInType )
		{
		case OP_TEXTURE3D:
			WrapInOption( { 2, 3 }, "gradient3d" );
			break;
		case OP_TEXTURECUBE:
			WrapInOption( { 2, 3 }, "gradientcube" );
			break;
		default:
			WrapInOption( { 2, 3 }, "gradient2d" );
		}
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "SampleLevel" )
	{
		// t.SampleLevel(sampler, coord, lod) -> t.sample(sampler, coord, level(lod))
		// TODO: support offset?
		functionToken.stringValue = MakeInlineString( "sample" );
		WrapInOption( { 2 }, "level" );
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "GatherRed" )
	{
		isGatherCall = true;
		functionToken.stringValue = MakeInlineString( "gather" );

		// Add x component enumeration to the function
		//auto x = ScannerToken::FromTokenType(OP_INT_CONST, call->GetLocation());
		//x.stringValue = MakeInlineString("component::x");
		//ASTNode* component = new ASTNode(NT_CONSTANT, call->GetLocation(), call->GetScope(), &x);
		//call->AddChild(component);

		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "GatherGreen" )
	{
		isGatherCall = true;
		functionToken.stringValue = MakeInlineString( "gather" );

		// Add y component enumeration to the function
		auto y = ScannerToken::FromTokenType( OP_INT_CONST, call->GetLocation() );
		y.stringValue = MakeInlineString( "component::y" );
		ASTNode* component = new ASTNode( NT_CONSTANT, call->GetLocation(), call->GetScope(), &y );
		call->AddChild( component );

		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "GatherBlue" )
	{
		isGatherCall = true;
		functionToken.stringValue = MakeInlineString( "gather" );

		// Add z component enumeration to the function
		auto z = ScannerToken::FromTokenType( OP_INT_CONST, call->GetLocation() );
		z.stringValue = MakeInlineString( "component::z" );
		ASTNode* component = new ASTNode( NT_CONSTANT, call->GetLocation(), call->GetScope(), &z );
		call->AddChild( component );

		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "Load" )
	{
		// normal textures: t.Load(coord) -> t.read(coord.xy, coord.z)
		// MS textures: t.Load(coord, sample) -> t.read(coord, sample)
		// TODO: support offset?
		functionToken.stringValue = MakeInlineString( "read" );
		auto coord = call->GetChild( 0 );

		if( node->GetChild( 0 )->GetSymbol() &&
			( textureType.builtInType == OP_RWTEXTURE1D ||
			  textureType.builtInType == OP_RWTEXTURE1DARRAY ||
			  textureType.builtInType == OP_RWTEXTURE2D ||
			  textureType.builtInType == OP_RWTEXTURE2DARRAY ||
			  textureType.builtInType == OP_RWTEXTURE3D ||
			  textureType.builtInType == OP_RWTEXTURE3DARRAY ) )
		{
			node->GetChild( 0 )->GetSymbol()->type.metalTextureAccess = 1;
		}
		if( textureType.builtInType != OP_TEXTURE2DMS && textureType.builtInType != OP_TEXTURE2DMSARRAY )
		{
			const char* xyzw = "xyzw";

			ScannerToken swizzle = ScannerToken::ID( MakeInlineString( xyzw, xyzw + coord->GetType().width - 1 ) );
			ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
			auto t = coord->GetType();
			--t.width;
			dot->SetType( t );
			dot->AddChild( coord->Copy() );
			if( coord->GetType().builtInType != OP_UINT )
			{
				ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
				typeToken.intValue = 4;
				ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
				Type castType = coord->GetType();
				castType.width -= 1;
				castType.builtInType = OP_UINT;
				cast->SetType( castType );
				cast->AddChild( dot );
				dot = cast;
			}
			call->ReplaceChild( size_t( 0 ), dot );

			swizzle = ScannerToken::ID( MakeInlineString( xyzw + coord->GetType().width - 1, xyzw + coord->GetType().width ) );
			dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
			dot->AddChild( coord->Copy() );
			call->InsertChild( 1, dot );
		}
		else if( coord->GetType().builtInType != OP_UINT )
		{
			ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
			typeToken.intValue = 4;
			ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
			Type castType = coord->GetType();
			castType.builtInType = OP_UINT;
			cast->SetType( castType );
			cast->AddChild( coord );
			call->ReplaceChild( size_t( 0 ), cast );
		}
		SplitCoordVec( call, textureType, 0 );
	}
	else if( functionToken.stringValue == "SampleCmp" )
	{
		// t.SampleCmp(sampler, coord, cmp [,offset]) -> t.sample_compare(sampler, coord, cmp, [offset])
		functionToken.stringValue = MakeInlineString( "sample_compare" );
		// texture arrays: t.SampleCmp(sampler, coord, cmp [,offset]) -> t.sample_compare(sampler, coord., coord., [offset])
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "SampleCmpLevelZero" )
	{
		// t.SampleCmpLevelZero(sampler, coord, cmp [,offset]) -> t.sample_compare(sampler, coord, level(0), [offset])
		functionToken.stringValue = MakeInlineString( "sample_compare" );
		// texture arrays: t.SampleCmpLevelZero(sampler, coord, cmp [,offset]) -> t.sample_compare(sampler, coord., coord., level(0), [offset])
		auto zero = ScannerToken::FromTokenType( OP_INT_CONST, call->GetLocation() );
		zero.stringValue = MakeInlineString( "0" );
		call->InsertChild( 3, new ASTNode( NT_CONSTANT, call->GetLocation(), call->GetScope(), &zero ) );
		WrapInOption( { 3 }, "level" );
		SplitCoordVec( call, textureType );
	}
	else if( functionToken.stringValue == "GetDimensions" )
	{
		// replace tex.GetDimensions(...) with GetDemensions(tex, ...)
		call->InsertChild( 0, node->GetChild( 0 ) );
		return call;
	}
	else
	{
		return node;
	}

	call->SetToken( &functionToken );

	if( !textureType.isDepthTexture && !isGatherCall && textureType.templateParameter && textureType.templateParameter->width != 4 )
	{
		const char* xyzw = "xyzw";
		ScannerToken swizzle = ScannerToken::ID( MakeInlineString( xyzw, xyzw + textureType.templateParameter->width ) );
		ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, call->GetLocation(), call->GetScope(), &swizzle );
		dot->AddChild( node );

		Type callType = *textureType.templateParameter;
		callType.width = 4;
		node->SetType( callType );

		dot->SetType( *textureType.templateParameter );

		return dot;
	}
	return node;
}

static bool IsTextureIndexing( ASTNode* node )
{
	return node->GetNodeType() == NT_POSTFIX_EXPRESSION &&
		node->GetToken() &&
		node->GetToken()->type == OP_LEFT_BRACKET &&
		node->GetChild( 0 )->GetType().IsTexture() &&
		node->GetChild( 0 )->GetType().arrayDimensions == 0;
}


ASTNode* PatchMetalTextureCalls( ParserState& state, ASTNode* node, bool rightHandSide = true )
{
	if( !node )
	{
		return node;
	}
	if( node->GetNodeType() == NT_POSTFIX_EXPRESSION &&
		node->GetToken() &&
		node->GetToken()->type == OP_DOT &&
		node->GetChildrenCount() == 2 &&
		node->GetChild( 0 )->GetType().IsTexture() &&
		node->GetChild( 1 )->GetNodeType() == NT_FUNCTION_CALL )
	{
		node = PatchMetalTextureCall( node );
	}
	else if( IsTextureIndexing( node ) )
	{
		if( rightHandSide )
		{
			if( node->GetChild( 0 )->GetSymbol() &&
				( node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE1D ||
				  node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE1DARRAY ||
				  node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE2D ||
				  node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE2DARRAY ||
				  node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE3D ||
				  node->GetChild( 0 )->GetType().builtInType == OP_RWTEXTURE3DARRAY ) )
			{
				node->GetChild( 0 )->GetSymbol()->type.metalTextureAccess = 1;
			}

			// t[x] -> t.read(x)
			auto functionToken = ScannerToken::ID( MakeInlineString( "read" ), node->GetLocation() );
			auto call = new ASTNode( NT_FUNCTION_CALL, node->GetLocation(), node->GetScope(), &functionToken );
			auto nodeToken = *node->GetToken();
			nodeToken.type = OP_DOT;
			node->SetToken( &nodeToken );
			call->AddChild( node->GetChild( 1 ) );
			node->ReplaceChild( 1, call );

			auto coord = call->GetChild( 0 );
			if( coord->GetType().builtInType != OP_UINT )
			{
				ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
				typeToken.intValue = 4;
				ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
				Type castType = coord->GetType();
				castType.builtInType = OP_UINT;
				cast->SetType( castType );
				cast->AddChild( coord );
				call->ReplaceChild( size_t( 0 ), cast );
			}

			auto textureType = node->GetChild( 0 )->GetType();
			// For indexing texture arrays we need to separate the array index from the coordinate
			auto SplitIndex = [&]( uint32_t dimension ) {
				auto arg = call->GetChild( 0 );

				const char* xyzw = "xyzw";
				auto dot = NewDot( state, arg, MakeInlineString( xyzw, xyzw + dimension ) );
				call->ReplaceChild( size_t( 0 ), dot );

				dot = NewDot( state, arg->Copy(), MakeInlineString( xyzw + dimension, xyzw + dimension + 1 ) );
				call->InsertChild( 1, dot );
			};
			switch( textureType.builtInType )
			{
			case OP_TEXTURE1DARRAY:
			case OP_RWTEXTURE1DARRAY:
				SplitIndex( 1 );
				break;
			case OP_TEXTURE2DARRAY:
			case OP_RWTEXTURE2DARRAY:
				SplitIndex( 2 );
				break;
			case OP_TEXTURE3DARRAY:
			case OP_RWTEXTURE3DARRAY:
				SplitIndex( 3 );
				break;
			default:
				break;
			}
			// MLS tex.read is always a 4 component vector. If the original texture is not a 4 component vector, we need to swizzle the result
			if( !textureType.isDepthTexture && textureType.templateParameter && textureType.templateParameter->width != 4 )
			{
				const char* xyzw = "xyzw";
				ScannerToken swizzle = ScannerToken::ID( MakeInlineString( xyzw, xyzw + textureType.templateParameter->width ) );
				ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, call->GetLocation(), call->GetScope(), &swizzle );
				dot->AddChild( node );

				Type callType = *textureType.templateParameter;
				callType.width = 4;
				node->SetType( callType );

				dot->SetType( *textureType.templateParameter );

				node = dot;
			}
			SplitCoordVec( call, textureType, 0 );
		}
		else
		{
			state.ShowMessage( node->GetLocation(), EC_CUSTOM_ERROR, "cannot rewite texture assignment for Metal" );
		}
	}
	bool isAssignment = false;
	if( node->GetNodeType() == NT_EXPRESSION )
	{
		switch( node->GetToken()->type )
		{
		case OP_EQUAL:
			if( IsTextureIndexing( node->GetChild( 0 ) ) )
			{
				// t[x] = y; -> t.write(y, x);
				auto functionToken = ScannerToken::ID( MakeInlineString( "write" ), node->GetLocation() );
				auto call = new ASTNode( NT_FUNCTION_CALL, node->GetLocation(), node->GetScope(), &functionToken );
				auto nodeToken = *node->GetToken();
				nodeToken.type = OP_DOT;
				node->GetChild( 0 )->SetToken( &nodeToken );

				auto value = node->GetChild( 1 );
				if( value->GetType().width != 4 )
				{
					ScannerToken ctrToken = ScannerToken::FromTokenType( value->GetType().builtInType );
					ctrToken.intValue = ( 1 << 8 ) | 4;
					auto ctr = new ASTNode( NT_FUNCTION_CALL, node->GetLocation(), node->GetScope(), &ctrToken );
					Type type = value->GetType();
					type.width = 4;
					ctr->SetType( type );
					ctr->AddChild( value );

					for( int i = value->GetType().width; i < 4; i++ )
					{
						auto zero = ScannerToken::FromTokenType( OP_INT_CONST, value->GetLocation() );
						zero.stringValue = MakeInlineString( "0" );
						ctr->AddChild( new ASTNode( NT_CONSTANT, value->GetLocation(), value->GetScope(), &zero ) );
					}
					value = ctr;
				}
				call->AddChild( value );
				if( node->GetChild( 0 )->GetChild( 0 )->GetType().IsTextureArray() )
				{
					auto coord = node->GetChild( 0 )->GetChild( 1 );

					const char* xyzw = "xyzw";

					ScannerToken swizzle = ScannerToken::ID( MakeInlineString( xyzw, xyzw + coord->GetType().width - 1 ) );
					ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
					dot->AddChild( coord->Copy() );

					if( coord->GetType().builtInType != OP_UINT )
					{
						ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
						typeToken.intValue = 4;
						ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
						Type castType = coord->GetType();
						castType.width -= 1;
						castType.builtInType = OP_UINT;
						cast->SetType( castType );
						cast->AddChild( dot );
						dot = cast;
					}

					call->AddChild( dot );

					swizzle = ScannerToken::ID( MakeInlineString( xyzw + coord->GetType().width - 1, xyzw + coord->GetType().width ) );
					dot = new ASTNode( NT_POSTFIX_EXPRESSION, coord->GetLocation(), coord->GetScope(), &swizzle );
					dot->AddChild( coord->Copy() );

					if( coord->GetType().builtInType != OP_UINT )
					{
						ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
						typeToken.intValue = 4;
						ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
						Type castType = coord->GetType();
						castType.width = 1;
						castType.builtInType = OP_UINT;
						cast->SetType( castType );
						cast->AddChild( dot );
						dot = cast;
					}
					call->AddChild( dot );
				}
				else
				{
					auto coord = node->GetChild( 0 )->GetChild( 1 );
					if( coord->GetType().builtInType != OP_UINT )
					{
						ScannerToken typeToken = ScannerToken::FromTokenType( OP_FLOAT );
						typeToken.intValue = 4;
						ASTNode* cast = new ASTNode( NT_CAST_EXPRESSION, coord->GetLocation(), coord->GetScope(), &typeToken );
						Type castType = coord->GetType();
						castType.builtInType = OP_UINT;
						cast->SetType( castType );
						cast->AddChild( coord );
						call->AddChild( cast );
					}
					else
					{
						call->AddChild( coord );
					}
				}
				node->GetChild( 0 )->ReplaceChild( 1, call );
				node = node->GetChild( 0 );
			}
			else
			{
				isAssignment = true;
			}
			break;
		case OP_MUL_ASSIGN:
		case OP_DIV_ASSIGN:
		case OP_MOD_ASSIGN:
		case OP_ADD_ASSIGN:
		case OP_SUB_ASSIGN:
		case OP_LEFT_ASSIGN:
		case OP_RIGHT_ASSIGN:
		case OP_AND_ASSIGN:
		case OP_XOR_ASSIGN:
		case OP_OR_ASSIGN:
			isAssignment = true;
			break;
		default:
			break;
		}
	}

	for( size_t i = 0; i < node->GetChildrenCount(); ++i )
	{
		node->ReplaceChild( unsigned( i ), PatchMetalTextureCalls( state, node->GetChild( i ), isAssignment && i == 0 ? false : rightHandSide ) );
	}
	return node;
}

void ConvertTextureFunctionsToMetal( ParserState& state )
{
	ConvertTextureFunctionsDX11( state );
	PatchMetalTextureCalls( state, state.GetTree() );
	PatchMetalBufferCalls( state, state.GetTree() );
}


void MergeSamplers( ParserState& state )
{
	std::map<Sampler, Symbol*> samplers;

	for( auto child : state.GetTree()->GetChildren() )
	{
		if( child->GetNodeType() == NT_VAR_DECLARATION_LIST )
		{
			for( auto name : child->GetChildren() )
			{
				if( name && name->GetSymbol() && name->GetSymbol()->type.IsSampler() )
				{
					Sampler sampler;
					if( GetSamplerState( state, name, sampler ) && !sampler.isDynamic )
					{
						auto found = samplers.find( sampler );
						if( found != samplers.end() )
						{
							state.GetTree()->Map( [&]( auto node ) {
								if( node->GetNodeType() != NT_NAME_DECLARATION && node->GetSymbol() == name->GetSymbol() )
								{
									node->SetSymbol( found->second );
								}
								return node;
							} );
						}
						else
						{
							samplers[sampler] = name->GetSymbol();
						}
					}
				}
			}
		}
	}
}
int32_t BindlessTextureType( int type )
{
	switch( type )
	{
	case OP_BINDLESSHANDLETEXTURE2D:
		return TEX_TYPE_2D;
	case OP_BINDLESSHANDLETEXTURE3D:
		return TEX_TYPE_3D;
	case OP_BINDLESSHANDLETEXTURECUBE:
		return TEX_TYPE_CUBE;
	default:
		return TEX_TYPE_INVALID;
	}
}
