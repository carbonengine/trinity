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
	}
	return nullptr;
}

static const char* GetTextureTypeName( int textureType )
{
	static std::map<int, char*> s_textureType;
	if( s_textureType.empty() )
	{
		s_textureType[OP_TEXTURE1D] = "1D";
		s_textureType[OP_TEXTURE2D] = "2D";
		s_textureType[OP_TEXTURE3D] = "3D";
		s_textureType[OP_TEXTURECUBE] = "CUBE";
	}
	return s_textureType[textureType];
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
		state.ShowMessage( token.fileLocation, EC_NO_OVERRIDE, ToString( token.stringValue ).c_str() );
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
					ASTNode *var = new ASTNode( NT_VAR_IDENTIFIER, node->GetLocation(), node->GetScope(), nullptr );
					var->SetSymbol( texture );
					result = new ASTNode( NT_POSTFIX_EXPRESSION, node->GetLocation(), node->GetScope(), &token );
					result->AddChild( var );
					result->AddChild( node );

					const char* sample;
					if( ContainsSubstring( node->GetSymbol()->name, "lod" ) )
					{
						static const char* s_sample = "SampleLevel";
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
									ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									dot->AddChild( coord );
									node->RemoveChild( 1 );
									node->InsertChild( 1, dot );
									coord = dot;
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
						static const char* s_sample = "SampleBias";
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
									ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									dot->AddChild( coord );
									node->RemoveChild( 1 );
									node->InsertChild( 1, dot );
									coord = dot;
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
						static const char* s_sample = "Sample";
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
									ASTNode* dot = new ASTNode( NT_POSTFIX_EXPRESSION, lod->GetLocation(), lod->GetScope(), &swizzle );
									dot->AddChild( coord );
									node->RemoveChild( 1 );
									node->InsertChild( 1, dot );
									coord = dot;
								}

								expr->AddChild( coord );
								expr->AddChild( dot );
								node->RemoveChild( 1 );
								node->InsertChild( 1, expr );
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
								node->RemoveChild( 1 );
								node->InsertChild( 1, expr );
							}
						}
					}
					else if( node->GetChildrenCount() == 4 || ContainsSubstring( node->GetSymbol()->name, "grad" ) )
					{
						// this is a form tex##( sampler, coord, ddx, ddy )
						static const char* s_sample = "SampleGrad";
						sample = s_sample;
					}
					else
					{
						static const char* s_sample = "Sample";
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
			node->RemoveChild( i );
			node->InsertChild( i, ret );
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
									for( auto it = texture->registerSpecifier.begin(); it != texture->registerSpecifier.end(); ++it )
									{
										it->second.registerType = 't';
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
					if( samplerArg->definition && samplerArg->definition->GetNodeType() == NT_FUNCTION_PARAMETER
						&& node->GetSymbol() && node->GetSymbol()->definition )
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

static void SubstituteSybmols( ASTNode* node, std::map<Symbol*, Symbol*> symbolSubstitution )
{
	if( node == nullptr )
	{
		return;
	}
	if( node->GetNodeType() == NT_VAR_IDENTIFIER && node->GetSymbol() != nullptr )
	{
		auto found = symbolSubstitution.find( node->GetSymbol() );
		if( found != symbolSubstitution.end() )
		{
			node->SetSymbol( found->second );
		}
	}
	for( unsigned i = 0; i < node->GetChildrenCount(); ++i )
	{
		SubstituteSybmols( node->GetChild( i ), symbolSubstitution );
	}
}

void ConvertTextureFunctionsDX11( ParserState& state )
{
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
	TransferSRGBToTexturesDX11( state, state.GetTree() );
}
