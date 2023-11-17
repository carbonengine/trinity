#include "stdafx.h"
#include "EffectCompilerMetal.h"

#include "ASTNode.h"
#include "CompileMessageQueue.h"
#include "EffectData.h"
#include "FXAnalyzer.h"
#include "HLSLParser.h"
#include "Macro.h"
#include "OutputHLSL.h"
#include "ParserState.h"
#include "ParserUtils.h"
#include "StringTable.h"
#include "TextureFunctionConversionDX11.h"
#include "YamlOutput.h"

extern std::string g_metalToolsPath;

const char MSL_INCLUDE[] = 
#include "MSLInclude.h"
;

	// Values below must be synchronized with (propagated from) TrinityAL/metal/MetalWorkQueue.h
#define METAL_MAX_BOUND_BUFFERS  31
#define METAL_MAX_BOUND_TEXTURES 31 // 31+ on iOS, 128 on macOS
#define METAL_MAX_BOUND_SAMPLERS 16
#define METAL_MAX_VERTEX_STREAMS 32
#define METAL_MAX_RENDER_TARGETS  4 // Matches DX12 define (RENDER_TARGET_COUNT)

// #define METAL_VERTEX_STREAM_BUFFER_OFFSET 0
#define METAL_VERTEX_STREAM_BUFFER_COUNT 4
// #define METAL_CONST_BUFFER_OFFSET (METAL_VERTEX_STREAM_BUFFER_OFFSET + METAL_VERTEX_STREAM_BUFFER_COUNT)
#define METAL_CONST_BUFFER_COUNT 20
// #define METAL_SRV_BUFFER_OFFSET (METAL_CONST_BUFFER_OFFSET + METAL_CONST_BUFFER_COUNT)
// #define METAL_SRV_BUFFER_OFFSET (METAL_VERTEX_STREAM_BUFFER_OFFSET + METAL_VERTEX_STREAM_BUFFER_COUNT)
#define METAL_SRV_BUFFER_COUNT 20
// #define METAL_UAV_BUFFER_OFFSET (METAL_SRV_BUFFER_OFFSET + METAL_SRV_BUFFER_COUNT)
#define METAL_UAV_BUFFER_COUNT 7
	static_assert( METAL_VERTEX_STREAM_BUFFER_COUNT + /* METAL_CONST_BUFFER_COUNT + */ METAL_SRV_BUFFER_COUNT + METAL_UAV_BUFFER_COUNT <= METAL_MAX_BOUND_BUFFERS, "buffer overflow" );

// #define METAL_SRV_TEXTURE_OFFSET 0
#define METAL_SRV_TEXTURE_COUNT 24
// #define METAL_UAV_TEXTURE_OFFSET (METAL_SRV_TEXTURE_OFFSET + METAL_SRV_TEXTURE_COUNT)
#define METAL_UAV_TEXTURE_COUNT 7
	static_assert( METAL_SRV_TEXTURE_COUNT + METAL_UAV_TEXTURE_COUNT <= METAL_MAX_BOUND_TEXTURES, "texture overflow" );

extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern bool g_printWarnings;
// TODO
// extern unsigned g_optimizationLevel;
// extern bool g_avoidFlowControl;

namespace
{
	std::mutex s_fileMutex;

	struct AtomicFn
	{
		std::string m_opMsl;
		std::string m_opHlsl;
	};

	inline CodeStream& operator<<( CodeStream& os, const AtomicFn& afun)
	{
		os << "void Interlocked" << afun.m_opHlsl << "(threadgroup uint& dest, uint value, thread uint& original_value) {original_value = atomic_fetch_" << afun.m_opMsl << "_explicit((threadgroup atomic_uint*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(threadgroup uint& dest, uint value) {atomic_fetch_" << afun.m_opMsl << "_explicit((threadgroup atomic_uint*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(threadgroup int& dest, int value, thread int& original_value) {original_value = atomic_fetch_" << afun.m_opMsl << "_explicit((threadgroup atomic_int*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(threadgroup int& dest, int value) {atomic_fetch_" << afun.m_opMsl << "_explicit((threadgroup atomic_int*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(device uint& dest, uint value, thread uint& original_value) {original_value = atomic_fetch_" << afun.m_opMsl << "_explicit((device atomic_uint*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(device uint& dest, uint value) {atomic_fetch_" << afun.m_opMsl << "_explicit((device atomic_uint*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(device int& dest, int value, thread int& original_value) {original_value = atomic_fetch_" << afun.m_opMsl << "_explicit((device atomic_int*)&dest, value, memory_order_relaxed);}\n\n";
		os << "void Interlocked" << afun.m_opHlsl << "(device int& dest, int value) {atomic_fetch_" << afun.m_opMsl << "_explicit((device atomic_int*)&dest, value, memory_order_relaxed);}\n\n";
		return os;
	}

	void PatchStdlibFunctions( ParserState& state )
	{
		Symbol* symbol = state.GetSymbolTable().Lookup( MakeInlineString( "lerp" ) );
		symbol->name = MakeInlineString( "mix" );

		symbol = state.GetSymbolTable().Lookup( MakeInlineString( "frac" ) );
		symbol->name = MakeInlineString( "fract" );

		symbol = state.GetSymbolTable().Lookup( MakeInlineString( "ddx" ) );
		symbol->name = MakeInlineString( "dfdx" );

		symbol = state.GetSymbolTable().Lookup( MakeInlineString( "ddy" ) );
		symbol->name = MakeInlineString( "dfdy" );

		symbol = state.GetSymbolTable().Lookup( MakeInlineString( "atan2" ) );
		symbol->name = MakeInlineString( "precise::atan2" );
	}

	void ReplaceFloatModulo( ParserState& state )
	{
		tmFunction( 0, 0 );

		state.GetTree()->Map( [&]( auto node ) {
			if( node->GetNodeType() == NT_EXPRESSION && node->GetToken()->type == OP_PERCENT )
			{
				if( node->GetChild( 1 )->GetType().IsScalarOrVector() && node->GetChild( 1 )->GetType().builtInType == OP_FLOAT )
				{
					auto token = ScannerToken::ID( MakeInlineString( "fmod" ), node->GetLocation() );
					auto call = new ASTNode( NT_FUNCTION_CALL, node->GetLocation(), node->GetScope(), &token );
					call->AddChild( node->GetChild( 0 ) );
					call->AddChild( node->GetChild( 1 ) );
					std::string diagnosticMessage;
					Symbol* symbol = state.GetSymbolTable().LookupFunction( token.stringValue, call, diagnosticMessage );
					if( symbol )
					{
						if( symbol->intrinsicType )
						{
							call->SetType( ( *symbol->intrinsicType )( call, -1 ) );
						}
						else
						{
							call->SetType( symbol->type );
						}
						call->SetSymbol( symbol );
						return call;
					}
				}
			}
			return node;
		} );
	}


	void ConvertSyncFunctionsToMetal( ASTNode* node )
	{
		if( !node )
		{
			return;
		}

		if( node->GetNodeType() == NT_FUNCTION_CALL )
		{
			Symbol* symbol = node->GetSymbol();
			if( symbol )
			{
				if( symbol->name == "GroupMemoryBarrierWithGroupSync" ||
					symbol->name == "threadgroup_barrier" )
				{
					symbol->name = MakeInlineString( "threadgroup_barrier" );

					ScannerToken token = {};
					token.type = OP_IN;
					token.stringValue = MakeInlineString( "mem_flags::mem_threadgroup" );
					token.fileLocation = node->GetLocation();

					ASTNode* arg = new ASTNode( NT_CONSTANT, node->GetLocation(), node->GetScope(), &token );

					node->AddChild( arg );

					return;
				}
			}
		}

		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* child = node->GetChild( i );
			ConvertSyncFunctionsToMetal( child );
		}
	}

	void ConvertSyncFunctionsToMetal( std::map<Symbol*, ASTNode*>& functions )
	{
		tmFunction( 0, 0 );

		for( auto& it : functions )
		{
			ASTNode* node = it.second;
			assert( node && node->GetNodeType() == NT_FUNCTION_DEFINITION );

			ASTNode* body = node->GetChild( 1 );
			assert( body->GetNodeType() == NT_BLOCK );

			ConvertSyncFunctionsToMetal( body );
		}
	}

	void CollectFunctions( ParserState& state, std::map<Symbol*, ASTNode*>& functions )
	{
		tmFunction( 0, 0 );

		ASTNode* root = state.GetTree();
		if( !root )
		{
			return;
		}

		for( size_t i = 0, n = root->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* node = root->GetChild( i );
			if( node && node->GetNodeType() == NT_FUNCTION_DEFINITION )
			{
				ASTNode* header = node->GetChild( 0 );
				assert( header->GetNodeType() == NT_FUNCTION_HEADER );

				functions[header->GetSymbol()] = node;
			}
		}
	}

	void AddStructDotPrefix( ASTNode* node, const std::set<Symbol*>& structMembers, Symbol* symbol )
	{
		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* childNode = node->GetChild( i );
			if( !childNode )
			{
				continue;
			}

			if( childNode->GetNodeType() == NT_VAR_IDENTIFIER )
			{
				auto it = structMembers.find( childNode->GetSymbol() );
				if( it != structMembers.end() )
				{
					ScannerToken token = {};
					token.type = OP_DOT;

					ASTNode* prefix = new ASTNode( NT_VAR_IDENTIFIER, childNode->GetLocation(), childNode->GetScope(), nullptr );
					prefix->SetSymbol( symbol );

					ASTNode* dotNode = new ASTNode( NT_POSTFIX_EXPRESSION, childNode->GetLocation(), childNode->GetScope(), &token );
					dotNode->AddChild( prefix );
					dotNode->AddChild( childNode );
					dotNode->SetType( childNode->GetType() );

					node->ReplaceChild( i, dotNode );
				}
			}

			AddStructDotPrefix( childNode, structMembers, symbol );
		}
	}

	void AddFunctionArgument( ASTNode* parent, const Symbol* functionSymbol, Symbol* arg )
	{
		assert( parent );

		for( size_t i = 0, n = parent->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* node = parent->GetChild( i );
			if( !node )
			{
				continue;
			}

			if( node->GetNodeType() == NT_FUNCTION_CALL &&
				node->GetSymbol() == functionSymbol )
			{
				ASTNode* child = new ASTNode( NT_VAR_IDENTIFIER, node->GetLocation(), node->GetScope(), nullptr );
				child->SetSymbol( arg );

				node->InsertChild( 0, child );
			}

			AddFunctionArgument( node, functionSymbol, arg );
		}
	}

	void AddFunctionArgument( std::map<Symbol*, ASTNode*>& functions, const Symbol* functionSymbol, Symbol* arg )
	{
		for( auto& it : functions )
		{
			ASTNode* node = it.second;
			assert( node && node->GetNodeType() == NT_FUNCTION_DEFINITION );

			ASTNode* body = node->GetChild( 1 );
			assert( body->GetNodeType() == NT_BLOCK );

			AddFunctionArgument( body, functionSymbol, arg );
		}
	}

	bool SearchFunctionsForGlobals( ASTNode* node, const std::set<Symbol*>& structMembers, const std::map<Symbol*, ASTNode*>& functions, std::map<Symbol*, ASTNode*>& functionsWithGlobals, std::map<Symbol*, ASTNode*>& functionsWithoutGlobals )
	{
		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* childNode = node->GetChild( i );
			if( !childNode )
			{
				continue;
			}

			if( childNode->GetNodeType() == NT_VAR_IDENTIFIER )
			{
				Symbol* symbol = childNode->GetSymbol();

				auto it = structMembers.find( symbol );
				if( it != structMembers.end() )
				{
					return true;
				}
			}
			else if( childNode->GetNodeType() == NT_FUNCTION_CALL )
			{
				Symbol* symbol = childNode->GetSymbol();
				if( symbol )
				{
					if( functionsWithGlobals.find( symbol ) != functionsWithGlobals.end() )
					{
						// The function was aready marked as using globals.
						return true;
					}
					else if( functionsWithoutGlobals.find( symbol ) == functionsWithoutGlobals.end() )
					{
						// If the function wasn't found in both lists it means either it was not checked yet
						// or it's a standard library function.
						auto it = functions.find( symbol );
						if( it != functions.end() )
						{
							ASTNode* definition = it->second;
							assert( definition && definition->GetNodeType() == NT_FUNCTION_DEFINITION );

							[[maybe_unused]] ASTNode* header = definition->GetChild( 0 );
							assert( header->GetNodeType() == NT_FUNCTION_HEADER );
							assert( header->GetSymbol() == symbol );

							ASTNode* body = definition->GetChild( 1 );
							assert( body->GetNodeType() == NT_BLOCK );

							bool found = SearchFunctionsForGlobals( body, structMembers, functions, functionsWithGlobals, functionsWithoutGlobals );
							if( found )
							{
								functionsWithGlobals.insert( { it->first, it->second } );
								return true;
							}
							else
							{
								functionsWithoutGlobals.insert( { it->first, it->second } );
							}
						}
					}
				}
			}

			// Search child nodes.
			bool found = SearchFunctionsForGlobals( childNode, structMembers, functions, functionsWithGlobals, functionsWithoutGlobals );
			if( found )
			{
				return true;
			}
		}
        return false;
	}

	void SearchFunctionsForGlobals( const std::set<Symbol*>& structMembers, const std::map<Symbol*, ASTNode*>& functions, std::map<Symbol*, ASTNode*>& functionsWithGlobals, std::map<Symbol*, ASTNode*>& functionsWithoutGlobals )
	{
		for( auto& it : functions )
		{
			ASTNode* node = it.second;
			assert( node && node->GetNodeType() == NT_FUNCTION_DEFINITION );

			[[maybe_unused]] ASTNode* header = node->GetChild( 0 );
			assert( header->GetNodeType() == NT_FUNCTION_HEADER );

			// Is already processed?
			if( functionsWithGlobals.find( it.first ) != functionsWithGlobals.end() ||
				functionsWithoutGlobals.find( it.first ) != functionsWithoutGlobals.end() )
			{
				continue;
			}

			ASTNode* body = node->GetChild( 1 );
			assert( body->GetNodeType() == NT_BLOCK );

			bool found = SearchFunctionsForGlobals( body, structMembers, functions, functionsWithGlobals, functionsWithoutGlobals );
			if( found )
			{
				functionsWithGlobals.insert( { it.first, it.second } );
			}
			else
			{
				functionsWithoutGlobals.insert( { it.first, it.second } );
			}
		}
	}

	void PatchGlobalsInFunctions( std::map<Symbol*, ASTNode*>& functions, Symbol* typeSymbol, const InlineString& globalsName, const std::set<Symbol*>& structMembers )
	{
		assert( !structMembers.empty() );

		std::map<Symbol*, ASTNode*> functionsWithGlobals;
		std::map<Symbol*, ASTNode*> functionsWithoutGlobals;

		SearchFunctionsForGlobals( structMembers, functions, functionsWithGlobals, functionsWithoutGlobals );

		ScannerToken token = {};
		token.type = OP_INOUT;

		for( auto& it : functionsWithGlobals )
		{
			ASTNode* node = it.second;
			assert( node && node->GetNodeType() == NT_FUNCTION_DEFINITION );

			ASTNode* header = node->GetChild( 0 );
			assert( header->GetNodeType() == NT_FUNCTION_HEADER );

			ASTNode* body = node->GetChild( 1 );
			assert( body->GetNodeType() == NT_BLOCK );

			Symbol* symbol = node->GetScope()->AddSymbol( globalsName, DISALOW_OVERRIDES );
			symbol->type.FromSymbol( typeSymbol );
			symbol->addressSpace = AddressSpace::Constant;

			token.fileLocation = header->GetLocation();

			Type type;
			type.FromSymbol( typeSymbol );

			ASTNode* globalsParam = new ASTNode( NT_FUNCTION_PARAMETER, header->GetLocation(), header->GetScope(), &token );
			globalsParam->SetType( type );
			globalsParam->SetSymbol( symbol );

			header->InsertChild( 0, globalsParam );

			AddFunctionArgument( functionsWithGlobals, header->GetSymbol(), symbol );

			AddStructDotPrefix( body, structMembers, symbol );
		}
	}

	bool IsGlobalVarUsed( const std::map<Symbol*, ASTNode*>& functions, const InlineString& name, ASTNode* node )
	{
		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* childNode = node->GetChild( i );
			if( !childNode )
			{
				continue;
			}

			if( childNode->GetSymbol() &&
				childNode->GetSymbol()->name == name )
			{
				return true;
			}

			bool isUsed = IsGlobalVarUsed( functions, name, childNode );
			if( isUsed )
			{
				return true;
			}

			// Recursively search in functions.
			if( childNode->GetNodeType() == NT_FUNCTION_CALL &&
				childNode->GetSymbol() )
			{
				auto it = functions.find( childNode->GetSymbol() );
				if( it != functions.end() )
				{
					ASTNode* body = it->second->GetChild( 1 );
					assert( body->GetNodeType() == NT_BLOCK );

					if( IsGlobalVarUsed( functions, name, body ) )
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void AddUsedGlobalsAsFunctionParams( std::map<Symbol*, ASTNode*>& functions, const std::vector<ASTNode*>& globals )
	{
		tmFunction( 0, 0 );

		for( auto& it : functions )
		{
			ASTNode* node = it.second;
			assert( node && node->GetNodeType() == NT_FUNCTION_DEFINITION );

			ASTNode* header = node->GetChild( 0 );
			assert( header->GetNodeType() == NT_FUNCTION_HEADER );

			ASTNode* body = node->GetChild( 1 );
			assert( body->GetNodeType() == NT_BLOCK );

			for( auto globalIt : globals )
			{
				Symbol* varSymbol = globalIt->GetSymbol();
				const Type& type = varSymbol->type;

				bool used = IsGlobalVarUsed( functions, varSymbol->name, body );
				if( used )
				{
					ScannerToken token = {};
					token.type = OP_IN;
					token.fileLocation = header->GetLocation();

					Symbol* symbol = node->GetScope()->AddSymbol( varSymbol->name, DISALOW_OVERRIDES );

					// If symbol is null it means the function already has a parameter with such name.
					if( !symbol )
					{
						// Make sure the existing symbol has the same type we expect.
						symbol = node->GetScope()->Lookup( varSymbol->name );
						assert( symbol && symbol->type == varSymbol->type );

						continue;
					}

					symbol->type.FromSymbol( varSymbol );

					// Textures and samplers do not need an address space attribute.
					if( type.symbol &&
						type.symbol->definition &&
						type.symbol->definition->GetNodeType() == NT_STRUCT )
					{
						if( type.storageClass == OP_GROUPSHARED )
						{
							symbol->addressSpace = AddressSpace::Threadgroup;
						}
						else
						{
							symbol->addressSpace = AddressSpace::Constant;
						}

						if( varSymbol->type.symbol && type.arrayDimensions == 0)
						{
							token.type = OP_INOUT;
						}
					}
					else
					{
						switch( type.builtInType )
						{
						case OP_BUFFER:
						case OP_STRUCTUREDBUFFER:
						case OP_RWBUFFER:
						case OP_RWSTRUCTUREDBUFFER:
							symbol->addressSpace = AddressSpace::Device;
							break;
						default:
							if( type.storageClass == OP_GROUPSHARED )
							{
								symbol->addressSpace = AddressSpace::Threadgroup;
								if( type.arrayDimensions == 0 )
								{
									token.type = OP_INOUT;
								}
							}
							break;
						}
					}

					ASTNode* param = new ASTNode( NT_FUNCTION_PARAMETER, header->GetLocation(), header->GetScope(), &token );
					param->SetType( varSymbol->type );
					param->SetSymbol( symbol );

					ASTNode* bracketList = globalIt->GetChildOrNull( 0 );
					if( bracketList )
					{
						param->AddChild( bracketList->Copy() );
					}
					header->InsertChild( 0, param );

					AddFunctionArgument( functions, header->GetSymbol(), symbol );
				}
			}
		}
	}

	void StripSemanticsInsideStucts( ASTNode* structNode )
	{
		for( auto memberNode : structNode->GetChildren() )
		{
			assert( memberNode && memberNode->GetNodeType() == NT_STRUCT_MEMBER );

			for( auto childNode : memberNode->GetChildren() )
			{
				assert( childNode && childNode->GetNodeType() == NT_NAME_DECLARATION );

				Symbol* symbol = childNode->GetSymbol();
				if( symbol && symbol->semantic )
				{
					symbol->semantic.start = symbol->semantic.end = nullptr;
				}
			}
		}
	}

	void CollectGlobals(
			ParserState& state,
			std::map<Symbol*, ASTNode*>& functions,
			std::vector<ASTNode*>& globals )
	{
		tmFunction( 0, 0 );

		ASTNode* root = state.GetTree();
		if( !root )
		{
			return;
		}

		ASTNode* globalsStruct = nullptr;
		ASTNode* uiStruct = nullptr;

		std::set<Symbol*> globalsStructMembers;
		std::set<Symbol*> uiStructMembers;

		// Move all globals to the new buffer.
		for( size_t i = 0, n = root->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* node = root->GetChild( i );
			if( node && node->GetNodeType() == NT_VAR_DECLARATION_LIST )
			{
				ASTNode* childNode = node->GetChildOrNull( 0 );
				if( childNode && childNode->GetNodeType() == NT_NAME_DECLARATION )
				{
					Symbol* symbol = childNode->GetSymbol();
					const Type& type = childNode->GetType();

					if( childNode->GetType().modifier == OP_CONST )
					{
						symbol->addressSpace = AddressSpace::Constant;
					}
					else if(childNode->GetType().storageClass == OP_GROUPSHARED)
					{
						symbol->addressSpace = AddressSpace::Threadgroup;

						globals.push_back(childNode);
						root->RemoveChild( i );
						--i;
						--n;
					}
					else if( symbol->name == "g_uiTransforms" )
					{
						symbol->registerSpecifier.clear();

						if( !uiStruct )
						{
							uiStruct = new ASTNode( NT_STRUCT, node->GetLocation(), node->GetScope(), nullptr );

							RegisterSpecifier reg;
							reg.shaderProfile.start = nullptr;
							reg.shaderProfile.end = nullptr;
							reg.registerType = MetalRegister::CBuffer;
							reg.registerNumber = GetCBufferIndex( MakeInlineString( "g_uiTransforms" ) );
							reg.subComponent = -1;
							reg.space = -1;
							reg.explicitRegister = true;
							reg.explicitSpace = false;

							Symbol* uiSymbol = state.GetSymbolTable().AddSymbol( MakeInlineString( "UITransforms" ) );
							uiSymbol->isTypeName = true;
							uiSymbol->definition = uiStruct;
							uiSymbol->registerSpecifier[reg.shaderProfile] = reg;

							uiStruct->SetSymbol( uiSymbol );

							node->SetNodeType( NT_STRUCT_MEMBER );
							uiStruct->AddChild( node );

							root->ReplaceChild( i, uiStruct );
						}
						else
						{
							node->SetNodeType( NT_STRUCT_MEMBER );
							uiStruct->AddChild( node );

							root->RemoveChild( i );
							--i;
							--n;
						}

						uiStructMembers.insert( symbol );
					}
					else
					{
						if( type.symbol &&
							type.symbol->definition &&
							type.symbol->definition->GetNodeType() == NT_STRUCT )
						{
							assert( symbol );

							// Strip cbuffer registers.
							for( auto& it : symbol->registerSpecifier )
							{
								if( it.second.registerType == MetalRegister::CBuffer )
								{
									symbol->registerSpecifier.erase( it.first );
									break;
								}
							}

							globals.push_back( childNode );

							root->RemoveChild( i );
							--i;
							--n;
						}
						else if( type.builtInType == OP_BUFFER ||
							type.builtInType == OP_STRUCTUREDBUFFER ||
							type.builtInType == OP_RWBUFFER ||
							type.builtInType == OP_RWSTRUCTUREDBUFFER ||
							type.builtInType == OP_TEXTURE ||
							type.builtInType == OP_TEXTURE1D ||
							type.builtInType == OP_TEXTURE1DARRAY ||
							type.builtInType == OP_TEXTURE2D ||
							type.builtInType == OP_TEXTURE2DARRAY ||
							type.builtInType == OP_TEXTURE3D ||
							// type.builtInType == OP_TEXTURE3DARRAY ||
							type.builtInType == OP_TEXTURECUBE ||
							type.builtInType == OP_TEXTURECUBEARRAY ||
							type.builtInType == OP_TEXTURE2DMS ||
							type.builtInType == OP_TEXTURE2DMSARRAY ||
							type.builtInType == OP_RWTEXTURE1D ||
							type.builtInType == OP_RWTEXTURE1DARRAY ||
							type.builtInType == OP_RWTEXTURE2D ||
							type.builtInType == OP_RWTEXTURE2DARRAY ||
							type.builtInType == OP_RWTEXTURE3D ||
							// type.builtInType == OP_RWTEXTURE3DARRAY ||
							type.builtInType == OP_SAMPLER ||
							type.builtInType == OP_SAMPLER2D ||
							type.builtInType == OP_SAMPLER3D ||
							type.builtInType == OP_SAMPLERCUBE ||
							type.builtInType == OP_SAMPLERCOMPARISON )
						{
							globals.push_back( childNode );

							root->RemoveChild( i );
							--i;
							--n;
						}
						else
						{
							if( !globalsStruct )
							{
								InlineString name = MakeInlineString( "GlobalsData" );
								globalsStruct = new ASTNode( NT_STRUCT, node->GetLocation(), node->GetScope(), nullptr );

								RegisterSpecifier reg;
								reg.shaderProfile.start = nullptr;
								reg.shaderProfile.end = nullptr;
								reg.registerType = MetalRegister::CBuffer;
								reg.registerNumber = GetCBufferIndex( name );
								reg.subComponent = -1;
								reg.space = -1;
								reg.explicitRegister = true;
								reg.explicitSpace = false;

								Symbol* globalsSymbol = state.GetSymbolTable().AddSymbol( name );
								globalsSymbol->isTypeName = true;
								globalsSymbol->definition = globalsStruct;
								globalsSymbol->registerSpecifier[reg.shaderProfile] = reg;

								globalsStruct->SetSymbol( globalsSymbol );

								node->SetNodeType( NT_STRUCT_MEMBER );
								globalsStruct->AddChild( node );

								root->ReplaceChild( i, globalsStruct );
							}
							else
							{
								node->SetNodeType( NT_STRUCT_MEMBER );
								globalsStruct->AddChild( node );

								root->RemoveChild( i );
								--i;
								--n;
							}

							globalsStructMembers.insert( symbol );
						}
					}
				}
			}
		}

		if( globalsStruct && !globalsStructMembers.empty() )
		{
			PatchGlobalsInFunctions( functions, globalsStruct->GetSymbol(), MakeInlineString( "Globals" ), globalsStructMembers );

			StripSemanticsInsideStucts( globalsStruct );
		}

		if( uiStruct && !uiStructMembers.empty() )
		{
			PatchGlobalsInFunctions( functions, uiStruct->GetSymbol(), MakeInlineString( "UI" ), uiStructMembers );

			StripSemanticsInsideStucts( uiStruct );
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
					// .literal( "offset" ).literal( it->offset )
					// .literal( "size" ).literal( it->size )
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

	RegisterSpecifier MetalSystemSemantics( MetalSystemSemanticsType::Enum type )
	{
		return RegisterSpecifier::Register( MetalRegister::System, type );
	}

	const std::map<InlineString, MetalSystemSemanticsType::Enum, ILess> s_systemSemantics = {
		// vertex shader input semantics
		{ MakeInlineString( "SV_VertexID" ), MetalSystemSemanticsType::vertex_id },
		{ MakeInlineString( "SV_InstanceID" ), MetalSystemSemanticsType::instance_id },
		// compute shader
		{ MakeInlineString( "SV_DispatchThreadID" ), MetalSystemSemanticsType::thread_position_in_grid },
		{ MakeInlineString( "SV_GroupThreadID" ), MetalSystemSemanticsType::thread_position_in_threadgroup },
		{ MakeInlineString( "SV_GroupIndex" ), MetalSystemSemanticsType::thread_index_in_threadgroup },
		{ MakeInlineString( "SV_GroupID" ), MetalSystemSemanticsType::threadgroup_position_in_grid },
		{ MakeInlineString( "SV_SampleIndex" ), MetalSystemSemanticsType::sample_id }
		// TODO: Add more.
	};

	void RemapSystemSemanticsDXtoMetal( ASTNode* callNode )
	{
		tmFunction( 0, 0 );

		const InlineString shaderProfile = MakeInlineString( "" );

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return;
		}

		ASTNode* header = entryPointSymbol->definition->GetChildOrNull( 0 );
		assert( header->GetNodeType() == NT_FUNCTION_HEADER );

		for( size_t i = 0, n = header->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* param = header->GetChild( i );
			Symbol* symbol = param->GetSymbol();

			auto it = s_systemSemantics.find( symbol->semantic );
			if( it != s_systemSemantics.end() )
			{
				symbol->registerSpecifier[ shaderProfile ] = MetalSystemSemantics( it->second );
			}
		}
	}

	bool RemapVertexInputSemanticsDXToMetal( StageInput& stage, Symbol* symbol, int& nextAttributeNumber )
	{
		if( !symbol || !symbol->semantic )
		{
			return false;
		}

		std::regex vsSemantics( "((position)|(color)|(normal)|(tangent)|(binormal)|(texcoord)|(blendindices)|(blendweight))(\\d*)", std::regex_constants::icase );

		auto semantic = ToString( symbol->semantic );
		std::smatch match;
		if( std::regex_match( semantic, match, vsSemantics ) )
		{
			PipelineInputDescription input;
			for( int i = 0; i < UC_NUM_USAGE_CODE; ++i )
			{
				if( match[i + 2].matched )
				{
					input.name = uint8_t( i );
					break;
				}
			}
			input.registerIndex = uint8_t( nextAttributeNumber );
			input.index = match[10].str().empty() ? 0 : uint8_t( stoi( match[10] ) );
			input.usedMask = 0xff;
			switch( symbol->type.builtInType )
			{
			case OP_FLOAT:
				input.type = CONSTANT_TYPE_FLOAT;
				break;
			case OP_INT:
				input.type = CONSTANT_TYPE_INT;
				break;
			case OP_UINT:
				input.type = CONSTANT_TYPE_UINT;
				break;
			case OP_BOOL:
				input.type = CONSTANT_TYPE_BOOL;
				break;
			default:
				input.type = CONSTANT_TYPE_OTHER;
				break;
			}
			input.dimension = uint8_t( symbol->type.width * symbol->type.height );

			stage.pipelineInputs.push_back( input );

			RegisterSpecifier reg;
			reg.shaderProfile.start = nullptr;
			reg.shaderProfile.end = nullptr;
			reg.registerType = MetalRegister::Attribute;
			reg.registerNumber = nextAttributeNumber;
			reg.subComponent = -1;
			reg.space = -1;
			reg.explicitRegister = true;
			reg.explicitSpace = false;

			symbol->registerSpecifier[reg.shaderProfile] = reg;
			symbol->semantic.start = symbol->semantic.end = nullptr;

			++nextAttributeNumber;
			return true;
		}
		else
		{
			g_messages.AddMessage( "\\memory(0): warning X0000: Vertex shader uses unsupported input semantics \"%s\"", ToString( symbol->semantic ).c_str() );
			return false;
		}
	}

	bool RemapVertexInputSemanticsDXToMetal( StageInput& stage, ASTNode* node, int& nextAttributeNumber )
	{
		assert( node && node->GetNodeType() == NT_STRUCT );

		bool result = false;

		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* member = node->GetChild( i );
			assert( member && member->GetNodeType() == NT_STRUCT_MEMBER );

			// TODO: Is it actually enough to handle only the first child?
			ASTNode* decl = member->GetChild( 0 );
			assert( decl && decl->GetNodeType() == NT_NAME_DECLARATION );

			Symbol* symbol = decl->GetSymbol();
			const Type& type = symbol->type;

			// Is it a nested struct?
			if( type.symbol &&
				type.symbol->definition &&
				type.symbol->definition->GetNodeType() == NT_STRUCT )
			{
				result = RemapVertexInputSemanticsDXToMetal( stage, type.symbol->definition, nextAttributeNumber ) || result;
			}
			else
			{
				if( !symbol->semantic )
				{
					continue;
				}

				result = RemapVertexInputSemanticsDXToMetal( stage, symbol, nextAttributeNumber ) || result;
			}
		}

		return result;
	}

	void RemapVertexOutputSemanticsDXToMetal( Symbol* symbol )
	{
		if( IEquals( symbol->semantic, "POSITION" ) || IEquals( symbol->semantic, "POSITION0" ) || IEquals( symbol->semantic, "SV_Position" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::position_invariant );
		}
		else if( IEquals( symbol->semantic, "SV_ClipDistance" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::clip_distance );
		}
		else if( IEquals( symbol->semantic, "PSIZE" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::point_size );
		}
		else
		{
			RegisterSpecifier reg;
			reg.shaderProfile.start = nullptr;
			reg.shaderProfile.end = nullptr;
			reg.registerType = MetalRegister::User;
			reg.registerNumber = -1;
			reg.subComponent = -1;
			reg.space = -1;
			reg.explicitRegister = true;
			reg.explicitSpace = false;
			symbol->registerSpecifier[reg.shaderProfile] = reg;
		}
	}

	void RemapVertexOutputSemanticsDXToMetal( ASTNode* node )
	{
		assert( node && node->GetNodeType() == NT_STRUCT );

		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* member = node->GetChild( i );
			assert( member && member->GetNodeType() == NT_STRUCT_MEMBER );

			ASTNode* decl = member->GetChild( 0 );
			assert( decl && decl->GetNodeType() == NT_NAME_DECLARATION );

			Symbol* symbol = decl->GetSymbol();
			RemapVertexOutputSemanticsDXToMetal( symbol );
		}
	}

	bool RemapFragmentInputSemanticsDXToMetal( Symbol* symbol )
	{
		if( !symbol || !symbol->semantic )
		{
			return false;
		}

		bool found = false;

		if( IEquals( symbol->semantic, "SV_Position" ) ||
			IEquals( symbol->semantic, "POSITION" ) ||
			IEquals( symbol->semantic, "POSITION0" ) ||
			IEquals( symbol->semantic, "VPOS" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::position );
			found = true;
		}
		else if( IEquals( symbol->semantic, "VFACE" ) ||
				 IEquals( symbol->semantic, "SV_IsFrontFace" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::front_facing );
			found = true;
		}
		else if( symbol->semantic )
		{
			RegisterSpecifier reg;
			reg.shaderProfile.start = nullptr;
			reg.shaderProfile.end = nullptr;
			reg.registerType = MetalRegister::User;
			reg.registerNumber = -1;
			reg.subComponent = -1;
			reg.space = -1;
			reg.explicitRegister = true;
			reg.explicitSpace = false;

			symbol->registerSpecifier[reg.shaderProfile] = reg;
			found = true;
		}

		return found;
	}

	bool RemapFragmentInputSemanticsDXToMetal( ASTNode* node )
	{
		assert( node && node->GetNodeType() == NT_STRUCT );

		bool result = false;

		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* member = node->GetChild( i );
			// assert( member && member->GetNodeType() == NT_STRUCT_MEMBER );

			ASTNode* decl = member->GetChild( 0 );
			assert( decl && decl->GetNodeType() == NT_NAME_DECLARATION );

			Symbol* symbol = decl->GetSymbol();
			if( !symbol->semantic )
			{
				continue;
			}

			result = RemapFragmentInputSemanticsDXToMetal( symbol ) || result;
		}

		return result;
	}

	void RemapFragmentOutputSemanticsDXToMetal( Symbol* symbol )
	{
		const MetalSystemSemanticsType::Enum colorN[8] = {
			MetalSystemSemanticsType::color_0,
			MetalSystemSemanticsType::color_1,
			MetalSystemSemanticsType::color_2,
			MetalSystemSemanticsType::color_3,
			MetalSystemSemanticsType::color_4,
			MetalSystemSemanticsType::color_5,
			MetalSystemSemanticsType::color_6,
			MetalSystemSemanticsType::color_7
		};

		if( _strnicmp( symbol->semantic.start, "COLOR", 5 ) == 0 )
		{
			const size_t len = symbol->semantic.end - symbol->semantic.start;
			if( len > 5 )
			{
				// Handle cases of COLORN.
				int index = symbol->semantic.start[5] - '0';
				assert( 0 <= index && index < 8 );
                symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( colorN[index] );
			}
			else
			{
                symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::color_0 );
			}
		}
		else if( _strnicmp( symbol->semantic.start, "SV_Target", 9 ) == 0 )
		{
			const size_t len = symbol->semantic.end - symbol->semantic.start;
			if( len > 9 )
			{
				// Handle cases of SV_TargetN.
				int index = symbol->semantic.start[9] - '0';
				assert( 0 <= index && index < 8 );
                symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( colorN[index] );
			}
			else
			{
                symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::color_0 );
			}
		}
		else if( IEquals( symbol->semantic, "DEPTH" ) ||
				 IEquals( symbol->semantic, "SV_Depth" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::depth );
		}
		else if( IEquals( symbol->semantic, "SV_StencilRef" ) )
		{
            symbol->registerSpecifier[MakeInlineString( "" )] = MetalSystemSemantics( MetalSystemSemanticsType::stencil );
		}
		else
		{
			g_messages.AddMessage( "\\memory(0): warning X0000: Fragment shader uses unsupported output semantics \"%s\"", ToString( symbol->semantic ).c_str() );
		}
	}

	void RemapFragmentOutputSemanticsDXToMetal( ASTNode* node )
	{
		assert( node && node->GetNodeType() == NT_STRUCT );

		for( size_t i = 0, n = node->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* member = node->GetChild( i );
			assert( member && member->GetNodeType() == NT_STRUCT_MEMBER );

			ASTNode* decl = member->GetChild( 0 );
			assert( decl && decl->GetNodeType() == NT_NAME_DECLARATION );

			Symbol* symbol = decl->GetSymbol();
			RemapFragmentOutputSemanticsDXToMetal( symbol );
		}
	}

	void PrepareVertexFunction( StageInput& stage, ASTNode* callNode )
	{
		assert( stage.type == VERTEX_STAGE );

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return;
		}

		ASTNode* header = entryPointSymbol->definition->GetChildOrNull( 0 );
		assert( header->GetNodeType() == NT_FUNCTION_HEADER );

		int nextAttributeNumber = 0;
		for( size_t i = 0, n = header->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* param = header->GetChild( i );
			Symbol* symbol = param->GetSymbol();
			const Symbol* typeSymbol = symbol->type.symbol;

			if( typeSymbol )
			{
				// This funciton parameter is of struct type. Check if this struct has declarations
				// with input semantics.
				bool hasInputs = RemapVertexInputSemanticsDXToMetal( stage, typeSymbol->definition, nextAttributeNumber );

				if( hasInputs )
				{
					// [[stage_in]]
					RegisterSpecifier reg;
					reg.shaderProfile.start = nullptr;
					reg.shaderProfile.end = nullptr;
					reg.registerType = MetalRegister::StageIn;
					reg.registerNumber = -1;
					reg.subComponent = -1;
					reg.space = -1;
					reg.explicitRegister = true;
					reg.explicitSpace = false;
					symbol->registerSpecifier[reg.shaderProfile] = reg;
				}
				else
				{
					// Inherit register specifier from the type.
					symbol->registerSpecifier = typeSymbol->registerSpecifier;
				}
			}
			else if( symbol->type.builtInType )
			{
				RemapVertexInputSemanticsDXToMetal( stage, symbol, nextAttributeNumber );
			}
		}

		const Type& returnType = header->GetType();
		if( returnType.symbol )
		{
			ASTNode* def = returnType.symbol->definition;
			if( def && def->GetNodeType() == NT_STRUCT )
			{
				RemapVertexOutputSemanticsDXToMetal( def );
			}
		}
	}

	void PrepareFragmentFunction( StageInput&, ASTNode* callNode )
	{
		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return;
		}

		ASTNode* header = entryPointSymbol->definition->GetChildOrNull( 0 );
		if( !header )
		{
			return;
		}
		assert( header->GetNodeType() == NT_FUNCTION_HEADER );

		for( size_t i = 0, n = header->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* param = header->GetChild( i );
			Symbol* symbol = param->GetSymbol();
			const Symbol* typeSymbol = symbol->type.symbol;

			if( typeSymbol )
			{
				// This funciton parameter is of struct type. Check if this struct has declarations
				// with input semantics.
				bool hasInputs = RemapFragmentInputSemanticsDXToMetal( typeSymbol->definition );

				if( hasInputs )
				{
					// [[stage_in]]
					RegisterSpecifier reg;
					reg.shaderProfile.start = nullptr;
					reg.shaderProfile.end = nullptr;
					reg.registerType = MetalRegister::StageIn;
					reg.registerNumber = -1;
					reg.subComponent = -1;
					reg.space = -1;
					reg.explicitRegister = true;
					reg.explicitSpace = false;
					symbol->registerSpecifier[reg.shaderProfile] = reg;
				}
				else
				{
					// Inherit register specifier from the type.
					symbol->registerSpecifier = typeSymbol->registerSpecifier;
				}
			}
			else if( symbol->type.builtInType )
			{
				if( param->GetToken() && param->GetToken()->type == OP_OUT )
				{
					RemapFragmentOutputSemanticsDXToMetal( symbol );
				}
				else
				{
					RemapFragmentInputSemanticsDXToMetal( symbol );
				}
			}
		}

		const Type& returnType = header->GetType();
		if( returnType.symbol )
		{
			ASTNode* def = returnType.symbol->definition;
			if( def && def->GetNodeType() == NT_STRUCT )
			{
				RemapFragmentOutputSemanticsDXToMetal( def );
			}
		}
	}

	void PrepareKernelFunction( StageInput&, ASTNode* )
	{
	}

	void GetThreadGroupSize( StageInput& stage, ParserState& state, ASTNode* callNode )
	{
		stage.threadGroupSize[0] = 1;
		stage.threadGroupSize[1] = 1;
		stage.threadGroupSize[2] = 1;

		if( stage.type != COMPUTE_STAGE )
		{
			return;
		}

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return;
		}

		ASTNode* attribList = entryPointSymbol->definition->GetChildOrNull( 2 );
		if( attribList && attribList->GetNodeType() == NT_FUNCTION_ATTRIBUTE_LIST )
		{
			// Find [numthreads(X, Y, Z)] attribute.
			for( size_t i = 0, n = attribList->GetChildrenCount(); i < n; ++i )
			{
				ASTNode* attribute = attribList->GetChild( i );
				assert( attribute->GetNodeType() == NT_FUNCTION_ATTRIBUTE );

				if( attribute->GetToken()->stringValue != "numthreads" )
				{
					continue;
				}

				for( size_t j = 0; j < 3; ++j )
				{
					ASTNode* child = attribute->GetChildOrNull( j );

					InlineString valueString = (child && child->GetToken() ) ? child->GetToken()->stringValue : InlineString{ nullptr, nullptr };
					unsigned long value = valueString ? std::strtoul( valueString.start, nullptr, 10 ) : 1;

					stage.threadGroupSize[ j ] = (uint32_t) value;
				}
				
				if( stage.threadGroupSize[0] * stage.threadGroupSize[1] * stage.threadGroupSize[2] > 512 )
				{
					state.ShowMessage( attribute->GetLocation(), EC_CUSTOM_WARNING, "number of threads per group exceeds Intel limit of 512; computer shader invoketion will fail on Intel macs" );
				}
			}
		}
	}

	bool AutoAssignRegisters( ParserState& state, ASTNode* callNode )
	{
		tmFunction( 0, 0 );

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return false;
		}

		ASTNode* functionHeader = entryPointSymbol->definition->GetChildOrNull( 0 );
		if( !functionHeader )
		{
			return false;
		}

		auto GetAssignedSymbolNames = []( const std::vector<Symbol*>& symbols, size_t offset, size_t count ) {
			std::string result;
			for( size_t i = 0; i < count; ++i )
			{
				auto symbol = symbols[i + offset];
				if( !result.empty() )
				{
					result += ", ";
				}
				result += ToString( symbol->name );
			}
			return result;
		};

		auto RecordRegister = [&state]( int index, std::vector<Symbol*>& registers, ASTNode* node, const char* registerBankName ) {
			Symbol* symbol = node->GetSymbol();

			if( index < 0 )
			{
				state.ShowMessage(
					node->GetLocation(),
					EC_CUSTOM_ERROR,
					"Couldn't allocate %s register for %s. Reason: Invalid register index.",
					registerBankName,
					ToString( symbol->name ).c_str() );
				return false;
			}

			if( !registers[index] )
			{
				registers[index] = symbol;
				return true;
			}
			else
			{
				state.ShowMessage(
					node->GetLocation(),
					EC_CUSTOM_ERROR,
					"Couldn't allocate %s register for %s. Reason: register %d already assigned to %s.",
					registerBankName,
					ToString( symbol->name ).c_str(),
					index,
					ToString( registers[index]->name ).c_str() );
				return false;
			}
		};

		std::vector<Symbol*> srvs( METAL_MAX_BOUND_BUFFERS, nullptr );
		std::vector<Symbol*> uavs( METAL_MAX_BOUND_TEXTURES, nullptr );
		std::vector<Symbol*> samplers( METAL_MAX_BOUND_SAMPLERS, nullptr );

		RegisterSpecifier reg;
		reg.shaderProfile.start = nullptr;
		reg.shaderProfile.end = nullptr;
		reg.registerType = MetalRegister::Invalid;
		reg.registerNumber = -1;
		reg.subComponent = -1;
		reg.space = -1;
		reg.explicitRegister = true;
		reg.explicitSpace = false;

		// Accumulate already used registers.
		for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			ASTNode* paramNode = functionHeader->GetChild( i );
			Symbol* symbol = paramNode->GetSymbol();

			const Type& type = paramNode->GetType();
			if( type.symbol &&
				type.symbol->definition &&
				type.symbol->definition->GetNodeType() == NT_STRUCT )
			{
				if( symbol->registerSpecifier.empty() )
				{
					int index = GetCBufferIndex( symbol );
					if( index >= 0 )
					{
						assert( index < METAL_CONST_BUFFER_COUNT );

						if( RecordRegister( index, srvs, paramNode, "SRV" ) )
						{
							reg.registerType = MetalRegister::CBuffer;
							reg.registerNumber = index;

							symbol->registerSpecifier[reg.shaderProfile] = reg;
						}
						else
						{
							return false;
						}
					}
				}
				else
				{
					// Note: We only check the first register.
					const RegisterSpecifier& existingReg = symbol->registerSpecifier.cbegin()->second;

					if( existingReg.registerType == MetalRegister::CBuffer ||
						existingReg.registerType == MetalRegister::SRV )
					{
						int index = existingReg.registerNumber;
						// assert( index < METAL_SRV_BUFFER_COUNT );
						// assert( index < METAL_CONST_BUFFER_COUNT );

						if( !RecordRegister( index, srvs, paramNode, "SRV" ) )
						{
							return false;
						}
					}
					else if( existingReg.registerType == MetalRegister::UAV )
					{
						int index = existingReg.registerNumber;
						// assert( index < METAL_UAV_BUFFER_COUNT );

						if( !RecordRegister( index, uavs, paramNode, "UAV" ) )
						{
							return false;
						}
					}
				}

				continue;
			}

			if( symbol->registerSpecifier.empty() )
			{
				continue;
			}

			// Note: We only check the first register.
			const RegisterSpecifier& existingReg = symbol->registerSpecifier.cbegin()->second;

			switch( type.builtInType )
			{
			case OP_BUFFER:
			case OP_STRUCTUREDBUFFER:
			{
				int index = existingReg.registerNumber;
				if( existingReg.registerType == MetalRegister::CBuffer )
				{
					assert( index < METAL_CONST_BUFFER_COUNT );
				}
				else if( existingReg.registerType == MetalRegister::SRV )
				{
					assert( index < METAL_SRV_BUFFER_COUNT );
				}
				else
				{
					// Should never get here.
					assert( false );
				}

				if( !RecordRegister( index, srvs, paramNode, "SRV" ) )
				{
					return false;
				}
				break;
			}

			case OP_RWBUFFER:
			case OP_RWSTRUCTUREDBUFFER:
			{
				assert( existingReg.registerType == MetalRegister::UAV );
				assert( existingReg.registerNumber < METAL_UAV_BUFFER_COUNT );

				int index = existingReg.registerNumber;

				if( !RecordRegister( index, uavs, paramNode, "UAV" ) )
				{
					return false;
				}
				break;
			}

			case OP_TEXTURE:
			case OP_TEXTURE1D:
			case OP_TEXTURE1DARRAY:
			case OP_TEXTURE2D:
			case OP_TEXTURE2DARRAY:
			case OP_TEXTURE3D:
			// case OP_TEXTURE3DARRAY:
			case OP_TEXTURECUBE:
			case OP_TEXTURECUBEARRAY:
			case OP_TEXTURE2DMS:
			case OP_TEXTURE2DMSARRAY:
			{
				assert( existingReg.registerType == MetalRegister::Texture );
				assert( existingReg.registerNumber < METAL_SRV_TEXTURE_COUNT );

				int index = existingReg.registerNumber;
				if( !RecordRegister( index, srvs, paramNode, "SRV" ) )
				{
					return false;
				}
				break;
			}

			case OP_RWTEXTURE1D:
			case OP_RWTEXTURE1DARRAY:
			case OP_RWTEXTURE2D:
			case OP_RWTEXTURE2DARRAY:
			case OP_RWTEXTURE3D:
			// case OP_RWTEXTURE3DARRAY:
			{
				assert( existingReg.registerType == MetalRegister::UAV );
				assert( existingReg.registerNumber < METAL_UAV_TEXTURE_COUNT );

				int index = existingReg.registerNumber;
				if( !RecordRegister( index, uavs, paramNode, "UAV" ) )
				{
					return false;
				}
				break;
			}

			case OP_SAMPLER:
			case OP_SAMPLER2D:
			case OP_SAMPLER3D:
			case OP_SAMPLERCUBE:
			case OP_SAMPLERCOMPARISON:
			{
				assert( existingReg.registerType == MetalRegister::Sampler );

				int index = existingReg.registerNumber;

				if( !RecordRegister( index, samplers, paramNode, "sampler" ) )
				{
					return false;
				}
				break;
			}

			default:
				break;
			}
		}

		int nextFreeSRV = 0;
		int nextFreeUAV = 0;
		int nextFreeSampler = 0;

		// Assign registers.
		for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			ASTNode* paramNode = functionHeader->GetChild( functionHeader->GetChildrenCount() - 1 - i );
			assert( paramNode );

			Symbol* symbol = paramNode->GetSymbol();
			assert( symbol );

			if( !symbol->registerSpecifier.empty() )
			{
				// The register already assigned.
				continue;
			}

			const Type& type = paramNode->GetType();
			if( type.symbol &&
				type.symbol->definition &&
				type.symbol->definition->GetNodeType() == NT_STRUCT )
			{
				int index = GetCBufferIndex( symbol );
				assert( index < METAL_CONST_BUFFER_COUNT );

				if( RecordRegister( index, srvs, paramNode, "SRV" ) )
				{
					reg.registerType = MetalRegister::CBuffer;
					reg.registerNumber = index;
					symbol->registerSpecifier[reg.shaderProfile] = reg;
				}
				else
				{
					return false;
				}
				continue;
			}

			reg.registerType = MetalRegister::Invalid;
			reg.registerNumber = -1;

			auto FindUnusedRegister = []( int& start, int limit, const std::vector<Symbol*>& registers, int& index ) {
				for( int i = start; i < limit; ++i )
				{
					if( !registers[i] )
					{
						index = i;
						start = i + 1;
						return true;
					}
				}
				return false;
			};

			switch( type.builtInType )
			{
			case OP_BUFFER:
			case OP_STRUCTUREDBUFFER:
			{
				reg.registerType = MetalRegister::SRV;
				if( FindUnusedRegister( nextFreeSRV, METAL_SRV_BUFFER_COUNT, srvs, reg.registerNumber ) )
				{
					srvs[reg.registerNumber] = symbol;
				}
				else
				{
					state.ShowMessage(
						callNode->GetLocation(),
						EC_CUSTOM_ERROR,
						"Couldn't allocate an SRV register for %s. Reason: no free registers left. Already assigned SRVs: %s",
						ToString( symbol->name ).c_str(),
						GetAssignedSymbolNames( srvs, 0, METAL_SRV_BUFFER_COUNT ).c_str() );
					return false;
				}

				break;
			}

			case OP_RWBUFFER:
			case OP_RWSTRUCTUREDBUFFER:
			{
				reg.registerType = MetalRegister::UAV;

				if( FindUnusedRegister( nextFreeUAV, METAL_UAV_BUFFER_COUNT, uavs, reg.registerNumber ) )
				{
					uavs[reg.registerNumber] = symbol;
				}
				else
				{
					state.ShowMessage(
						callNode->GetLocation(),
						EC_CUSTOM_ERROR,
						"Couldn't allocate a UAV register for %s. Reason: no free registers left. Already assigned UAVs: %s",
						ToString( symbol->name ).c_str(),
						GetAssignedSymbolNames( uavs, 0, METAL_UAV_BUFFER_COUNT ).c_str() );
					return false;
				}

				break;
			}

			case OP_TEXTURE:
			case OP_TEXTURE1D:
			case OP_TEXTURE1DARRAY:
			case OP_TEXTURE2D:
			case OP_TEXTURE2DARRAY:
			case OP_TEXTURE3D:
			// case OP_TEXTURE3DARRAY:
			case OP_TEXTURECUBE:
			case OP_TEXTURECUBEARRAY:
			case OP_TEXTURE2DMS:
			case OP_TEXTURE2DMSARRAY:
			{
				reg.registerType = MetalRegister::Texture;

				if( FindUnusedRegister( nextFreeSRV, METAL_SRV_TEXTURE_COUNT, srvs, reg.registerNumber ) )
				{
					srvs[reg.registerNumber] = symbol;
				}
				else
				{
					state.ShowMessage(
						callNode->GetLocation(),
						EC_CUSTOM_ERROR,
						"Couldn't allocate an SRV register for %s. Reason: no free registers left. Already assigned SRVs: %s",
						ToString( symbol->name ).c_str(),
						GetAssignedSymbolNames( srvs, 0, METAL_SRV_TEXTURE_COUNT ).c_str() );
					return false;
				}

				break;
			}

			case OP_RWTEXTURE1D:
			case OP_RWTEXTURE1DARRAY:
			case OP_RWTEXTURE2D:
			case OP_RWTEXTURE2DARRAY:
			case OP_RWTEXTURE3D:
			// case OP_RWTEXTURE3DARRAY:
			{
				reg.registerType = MetalRegister::UAV;

				if( FindUnusedRegister( nextFreeUAV, METAL_UAV_TEXTURE_COUNT, uavs, reg.registerNumber ) )
				{
					uavs[reg.registerNumber] = symbol;
				}
				else
				{
					state.ShowMessage(
						callNode->GetLocation(),
						EC_CUSTOM_ERROR,
						"Couldn't allocate a UAV register for %s. Reason: no free registers left. Already assigned UAVs: %s",
						ToString( symbol->name ).c_str(),
						GetAssignedSymbolNames( uavs, 0, METAL_UAV_TEXTURE_COUNT ).c_str() );
					return false;
				}

				break;
			}

			case OP_SAMPLER:
			case OP_SAMPLER2D:
			case OP_SAMPLER3D:
			case OP_SAMPLERCUBE:
			case OP_SAMPLERCOMPARISON:
				{
					reg.registerType = MetalRegister::Sampler;

					if( FindUnusedRegister( nextFreeSampler, METAL_MAX_BOUND_SAMPLERS, samplers, reg.registerNumber ) )
					{
						samplers[reg.registerNumber] = symbol;
					}
					else
					{
						state.ShowMessage(
							callNode->GetLocation(),
							EC_CUSTOM_ERROR,
							"Couldn't allocate a sampler register for %s. Reason: no free registers left. Already assigned samplers: %s",
							ToString( symbol->name ).c_str(),
							GetAssignedSymbolNames( samplers, 0, METAL_MAX_BOUND_SAMPLERS ).c_str() );
						return false;
					}
				}
				break;

			default:
				break;
			}

			symbol->registerSpecifier[reg.shaderProfile] = reg;
		}

		return true;
	}

	ASTNode* NewStructMember( ParserState& state, Symbol* sourceSymbol )
	{
		auto symbol = state.GetSymbolTable().AddSymbol( state.AllocateNameWithPrefix( sourceSymbol->name ) );
		auto nameDecl = state.NewNode( NT_NAME_DECLARATION );
		symbol->definition = nameDecl;
		symbol->type = sourceSymbol->type;
		symbol->interpolationModifier = sourceSymbol->interpolationModifier;
		symbol->semantic = sourceSymbol->semantic;
		nameDecl->SetSymbol( symbol );
		nameDecl->SetType( sourceSymbol->type );

		auto member = state.NewNode( NT_STRUCT_MEMBER );
		member->AddChild( nameDecl );
		member->SetType( sourceSymbol->type );
		return member;
	}

	// Collects fields from nested structures and copies them int a flat structure
	// In the process gathers access expressions for source struct fields (in the form blah.foo.bar...)
	void GatherFields( ASTNode* destStruct, std::vector<ASTNode*>& accessors, ASTNode* accessParent, ASTNode* sourceStruct, ParserState& state )
	{
		for( auto member : sourceStruct->GetChildren() )
		{
			if( member->GetType().IsStruct() )
			{
				for( auto name : member->GetChildren() )
				{
					GatherFields( destStruct, accessors, NewDot( state, accessParent->Copy(), name->GetSymbol() ), name->GetType().symbol->definition, state );
				}
			}
			else
			{
				for( auto name : member->GetChildren() )
				{
					destStruct->AddChild( NewStructMember( state, name->GetSymbol() ) );
					accessors.push_back( NewDot( state, accessParent->Copy(), name->GetSymbol() ) );
				}
			}
		}
	}

	bool HasSystemRegister( Symbol* symbol )
	{
		assert( symbol );

		bool hasSystemRegister = false;
		for( auto& it : symbol->registerSpecifier )
		{
			if( it.second.registerType == MetalRegister::System )
			{
				hasSystemRegister = true;
				break;
			}
		}

		return hasSystemRegister;
	}

	// Given a function header, gathers all inputs for [[stage_in]] into a flat structure
	void GatherInputs( ASTNode* destStruct, std::vector<ASTNode*>& accessors, const std::vector<Symbol*>& params, ASTNode* header, ParserState& state )
	{
		for( size_t i = 0; i < header->GetChildrenCount(); ++i )
		{
			auto arg = header->GetChild( i );

			if( IsUniformInputArgument( arg ) || arg->GetSymbol()->addressSpace != AddressSpace::None )
			{
				continue;
			}
			bool isIn = arg->GetToken() == 0 || arg->GetToken()->type == OP_IN;
			if( !isIn )
			{
				continue;
			}

			Symbol* symbol = arg->GetSymbol();
			if( !symbol )
			{
				continue;
			}

			if( HasSystemRegister( symbol ) )
			{
				continue;
			}

			if( symbol->type.IsStruct() )
			{
				GatherFields( destStruct, accessors, NewVarIdentifier( state, params[i] ), symbol->type.symbol->definition, state );
			}
			else if( symbol->type.IsScalarOrVector() )
			{
				destStruct->AddChild( NewStructMember( state, symbol ) );
				accessors.push_back( NewVarIdentifier( state, params[i] ) );
			}
		}
	}

	// Given a function header, gathers all function outputs into a flat structure
	void GatherOutputs( ASTNode* destStruct, std::vector<ASTNode*>& accessors, const std::vector<Symbol*>& params, ASTNode* header, ParserState& state )
	{
		for( size_t i = 0; i < header->GetChildrenCount(); ++i )
		{
			auto arg = header->GetChild( i );

			if( IsUniformInputArgument( arg ) || arg->GetSymbol()->addressSpace != AddressSpace::None )
			{
				continue;
			}

			bool isIn = arg->GetToken() == 0 || arg->GetToken()->type == OP_IN;
			if( isIn )
			{
				continue;
			}

			Symbol* symbol = arg->GetSymbol();
			if( symbol == nullptr )
			{
				continue;
			}

			auto access = NewVarIdentifier( state, params[i] );

			if( symbol->type.IsStruct() )
			{
				GatherFields( destStruct, accessors, access, symbol->type.symbol->definition, state );
			}
			else if( symbol->type.IsScalarOrVector() )
			{
				destStruct->AddChild( NewStructMember( state, symbol ) );
				accessors.push_back( access );
			}
		}
		if( header->GetType().IsStruct() || header->GetType().IsScalarOrVector() )
		{
			Symbol* symbol = header->GetSymbol();
			auto access = NewVarIdentifier( state, params.back() );

			if( symbol->type.IsStruct() )
			{
				GatherFields( destStruct, accessors, access, symbol->type.symbol->definition, state );
			}
			else
			{
				destStruct->AddChild( NewStructMember( state, symbol ) );
				accessors.push_back( access );
			}
		}
	}
	
	// Patches VPOS usages for pixel shaders: a common Dx9 legacy pattern of passing both POSITION (from
	// vertex shader) and VPOS. Function remaps VPOS inputs to POSION so that there are no duplicate semantics
	// in the stage_in struct.
	bool PatchVpos( ParserState& state, ASTNode* inputsStruct, std::vector<ASTNode*>& argumentAccessors )
	{
		Type float4 = TypeFromTokenType( OP_FLOAT );
		float4.width = 4;
		
		// First find POSITION input
		ASTNode* position = nullptr;
		size_t positionIndex = 0;
		for( size_t index = 0; index < inputsStruct->GetChildrenCount(); ++index )
		{
			auto child = inputsStruct->GetChild( index );
			auto member = child->GetChild( 0 );
			if( IEquals( member->GetSymbol()->semantic, "SV_Position" ) || IEquals( member->GetSymbol()->semantic, "POSITION" ) || IEquals( member->GetSymbol()->semantic, "POSITION0" ) )
			{
				if( child->GetType() != float4 )
				{
					state.ShowMessage( EC_CUSTOM_ERROR, "Pixel shader %s input needs to be a float4", ToString( member->GetSymbol()->semantic ).c_str() );
					return false;
				}
				position = child;
				positionIndex = index;
				break;
			}
		}
		
		// Find any other POSITION/VPOS inputs and remap them to the first POSITION
		for( size_t index = 0; index < inputsStruct->GetChildrenCount(); ++index )
		{
			auto child = inputsStruct->GetChild( index );
			if( child == position )
			{
				continue;
			}
			auto member = child->GetChild( 0 );
			if( IEquals( member->GetSymbol()->semantic, "SV_Position" ) || IEquals( member->GetSymbol()->semantic, "POSITION" ) || IEquals( member->GetSymbol()->semantic, "POSITION0" ) || IEquals( member->GetSymbol()->semantic, "VPOS" ) )
			{
				if( !child->GetType().IsScalarOrVector() || child->GetType().builtInType != OP_FLOAT )
				{
					state.ShowMessage( EC_CUSTOM_ERROR, "Pixel shader %s input needs to be a float vector", ToString( member->GetSymbol()->semantic ).c_str() );
					return false;
				}
				if( position )
				{
					if( position->GetType() != child->GetType() )
					{
						const char* swizzles = "xyzw";
						argumentAccessors[index] = NewDot( state,
														  argumentAccessors[positionIndex]->Copy(),
														  MakeInlineString( swizzles, swizzles + 4 - child->GetType().width ) );
					}
					else
					{
						argumentAccessors[index] = argumentAccessors[positionIndex]->Copy();
					}
					inputsStruct->RemoveChild( index );
					--index;
				}
				else if( child->GetType().width != 4 )
				{
					const char* swizzles = "xyzw";
					argumentAccessors[index] = NewDot( state,
													  argumentAccessors[index]->Copy(),
													  MakeInlineString( swizzles, swizzles + 4 - child->GetType().width ) );
					child->SetType( float4 );
					member->SetType( float4 );
					member->GetSymbol()->type = float4;
				}
			}
		}
		return true;
	}

	void ApplyPackedModifier( ASTNode* structNode, bool tightPacking )
	{
		for( auto member : structNode->GetChildren() )
		{
			for( auto var : member->GetChildren() )
			{
				auto& type = var->GetSymbol()->type;
				if( type.IsVector() && ( tightPacking || type.width == 3 ) && type.builtInType != OP_BOOL )
				{
					type.modifier = OP_PACKOFFSET; // let us abuse this token type!
				}
				else if( type.IsStruct() )
				{
					ApplyPackedModifier( type.symbol->definition, tightPacking );
				}
			}
		}
	}

	void ApplyPackedModifiersToConstantBuffers( ASTNode* functionDefinition )
	{
		ASTNode* functionHeader = functionDefinition->GetChildOrNull( 0 );
		if( !functionHeader )
		{
			return;
		}

		for( auto arg : functionHeader->GetChildren() )
		{
			if( Symbol* symbol = arg->GetSymbol() )
			{
				auto& type = symbol->type;
				if( type.IsStruct() && GetCBufferIndex( symbol ) >= 0 )
				{
					ApplyPackedModifier( type.symbol->definition, false );
				}
				else if( type.symbol == nullptr && ( type.builtInType == OP_STRUCTUREDBUFFER || type.builtInType == OP_RWSTRUCTUREDBUFFER ) )
				{
					if( type.templateParameter && type.templateParameter->IsStruct() )
					{
						ApplyPackedModifier( type.templateParameter->symbol->definition, true );
					}
				}
			}
		}
	}


	// Creates a new shader functions with all the inputs/outputs layed out for Metal:
	// - all [[stage_in]] inputs are put into a new flat struct
	// - all outputs are put into a new struct
	// - uniform arguments are moved to local variables inside the shader
	// - all the other arguments from the original entry point are passed as is
	// Returns new entry point function definition (added to AST root)
	ASTNode* PatchShader( InputStageType shaderType, ASTNode* callNode, ParserState& state )
	{
		tmFunction( 0, 0 );

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return nullptr;
		}

		ASTNode* functionHeader = entryPointSymbol->definition->GetChildOrNull( 0 );
		if( !functionHeader )
		{
			return nullptr;
		}

		InlineString shaderAttribute;
		ScannerToken shaderName = {};
		shaderName.type = OP_ID;
		switch ( shaderType )
		{
		case VERTEX_STAGE:
			shaderName.stringValue = MakeInlineString( "mainVS" );
			shaderAttribute = MakeInlineString( "vertex" );
			break;
		case PIXEL_STAGE:
			shaderName.stringValue = MakeInlineString( "mainPS" );
			shaderAttribute = MakeInlineString( "fragment" );
			break;
		case COMPUTE_STAGE:
			shaderName.stringValue = MakeInlineString( "mainCS" );
			shaderAttribute = MakeInlineString( "kernel" );
			break;
		default:
			state.ShowMessage( callNode->GetLocation(), EC_CUSTOM_ERROR, "Shader type %i is not supported by Metal", int( shaderType ) );
			return nullptr;
		}

		state.GetCurrentLocation().fileName = shaderName.stringValue;
		state.GetCurrentLocation().lineNumber = 1;

		ASTNode* inputsStruct = NewStruct( state, state.AllocateNameWithPrefix( "StageInData" ) );
		ASTNode* outputsStruct = NewStruct( state, state.AllocateNameWithPrefix( "StageOutData" ) );

		auto header = state.NewNode( NT_FUNCTION_HEADER, shaderName );
		auto shaderSymbol = state.GetSymbolTable().AddSymbol( shaderName.stringValue, ALLOW_OVERRIDES );
		shaderSymbol->isFunction = true;
		header->SetSymbol( shaderSymbol );

		state.GetSymbolTable().EnterScope();

		auto shaderBody = state.NewNode( NT_BLOCK );

		std::vector<Symbol*> params;

		size_t uniformArgIndex = 0;

		for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			// Create local variable declarations for original shader function arguments

			ASTNode* sourceArg = functionHeader->GetChild( i );
			Symbol* sourceSymbol = sourceArg->GetSymbol();
			ASTNode* bracketList = sourceArg->GetChildOrNull( 0 );

			if( sourceSymbol->addressSpace != AddressSpace::Threadgroup &&
                !IsUniformInputArgument( sourceArg ) &&
				( ( !sourceArg->GetType().IsStruct() && !sourceArg->GetType().IsScalarOrVector() ) ||
					sourceSymbol->addressSpace != AddressSpace::None ||
					HasSystemRegister( sourceSymbol ) ) )
			{
				params.push_back( nullptr );
				continue;
			}

			auto declList = NewVarDeclaration( state,
					sourceArg->GetType(),
					state.AllocateName( sourceSymbol->name ) );
            ASTNode* declaration = declList->GetChild( 0 );
			auto nameSymbol = declaration->GetSymbol();
            nameSymbol->addressSpace = sourceSymbol->addressSpace;
			shaderBody->AddChild( declList );

			params.push_back( nameSymbol );

			if( IsUniformInputArgument( sourceArg ) )
			{
				// uniform arguments are initialized with whatever was passed in the call node

				if( callNode->GetChildrenCount() <= uniformArgIndex )
				{
					state.ShowMessage( callNode->GetLocation(), EC_NO_OVERRIDE, ToString( functionHeader->GetToken()->stringValue ).c_str(), "" );
					return nullptr;
				}

				declaration->AddChild( nullptr );
				declaration->AddChild( callNode->GetChild( uniformArgIndex++ ) );
			}
			else if( bracketList )
			{
				declaration->AddChild( bracketList->Copy() );
			}
		}
		if( callNode->GetChildrenCount() != uniformArgIndex )
		{
			state.ShowMessage( callNode->GetLocation(), EC_NO_OVERRIDE, ToString( functionHeader->GetToken()->stringValue ).c_str(), "" );
			return nullptr;
		}

		bool hasReturnType = functionHeader->GetType() != TypeFromTokenType( OP_VOID );
		if( hasReturnType )
		{
			// Local variable for return value

			auto declList = NewVarDeclaration( state, functionHeader->GetType(), state.AllocateNameWithPrefix( "result" ) );
			auto nameSymbol = declList->GetChild( 0 )->GetSymbol();
			shaderBody->AddChild( declList );

			params.push_back( nameSymbol );
		}

		std::vector<ASTNode*> inputAccessors;
		std::vector<ASTNode*> outputAccessors;

		GatherInputs( inputsStruct, inputAccessors, params, functionHeader, state );
		GatherOutputs( outputsStruct, outputAccessors, params, functionHeader, state );

		Symbol* stageInSymbol = nullptr;

		// Don't add an empty input struct.
		if( inputsStruct->GetChildrenCount() == 0 )
		{
			delete inputsStruct;
			inputsStruct = nullptr;
		}
		else
		{
			state.GetTree()->AddChild( inputsStruct );

			// Add [[stage_in]] parameter.
			auto stageIn = NewFunctionParameter( state, TypeFromSymbol( inputsStruct->GetSymbol() ), state.AllocateNameWithPrefix( "stageIn" ) );
			header->AddChild( stageIn );

			stageInSymbol = stageIn->GetSymbol();
			stageInSymbol->registerSpecifier[MakeInlineString( "" )] = RegisterSpecifier::Register( MetalRegister::StageIn );
		}

		// Don't add an empty output struct.
		if( outputsStruct->GetChildrenCount() == 0 )
		{
			delete outputsStruct;
			outputsStruct = nullptr;
		}
		else
		{
			state.GetTree()->AddChild( outputsStruct );
		}

		std::vector<ASTNode*> argumentAccessors;

		if( inputsStruct )
		{
			for( size_t i = 0; i < inputsStruct->GetChildrenCount(); ++i )
			{
				argumentAccessors.push_back( NewDot( state, NewVarIdentifier( state, stageInSymbol ), inputsStruct->GetChild( i )->GetChild( 0 )->GetSymbol() ) );
			}

			if( shaderType == PIXEL_STAGE )
			{
				if( !PatchVpos( state, inputsStruct, argumentAccessors ) )
				{
					return nullptr;
				}
			}
		}

		for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			// Add parameters for the rest of original shader inputs
			ASTNode* sourceArg = functionHeader->GetChild( i );
			Symbol* sourceSymbol = sourceArg->GetSymbol();

			if( sourceSymbol->addressSpace != AddressSpace::Threadgroup &&
                !IsUniformInputArgument( sourceArg ) &&
				( ( !sourceArg->GetType().IsStruct() && !sourceArg->GetType().IsScalarOrVector() ) ||
					sourceSymbol->addressSpace != AddressSpace::None ||
					HasSystemRegister( sourceSymbol ) ) )
			{
				auto arg = NewFunctionParameter( state, sourceArg->GetType(), state.AllocateName( sourceSymbol->name ) );
				arg->GetSymbol()->addressSpace = sourceSymbol->addressSpace;
				arg->GetSymbol()->registerSpecifier.swap( sourceSymbol->registerSpecifier );
				// arg->GetSymbol()->registerSpecifier = sourceSymbol->registerSpecifier;

				if( sourceArg->GetType().IsStruct() )
				{
					ScannerToken ref = {};
					ref.type = OP_INOUT;

					arg->SetToken( &ref );
				}

				header->AddChild( arg );
				params[i] = arg->GetSymbol();
			}
		}

		if( inputsStruct )
		{
			for( size_t i = 0; i < inputAccessors.size(); ++i )
			{
				// Add assignments for all inputsStruct fields.
				auto assignment = NewBinaryExpression(
						state,
						OP_EQUAL,
						inputAccessors[i],
						argumentAccessors[i] );
				shaderBody->AddChild( NewExpressionStatement( state, assignment ) );
			}
		}

		// Call the original function
		auto call = state.NewNode( NT_FUNCTION_CALL, ScannerToken::ID( entryPointSymbol->name ) );
		call->SetSymbol( entryPointSymbol );
		call->SetType( functionHeader->GetType() );
		for( size_t i = 0; i < functionHeader->GetChildrenCount(); ++i )
		{
			call->AddChild( NewVarIdentifier( state, params[i] ) );
		}
		if( hasReturnType )
		{
			auto assignment = NewBinaryExpression(
					state,
					OP_EQUAL,
					NewVarIdentifier( state, params.back() ),
					call );

			shaderBody->AddChild( NewExpressionStatement( state, assignment ) );
		}
		else
		{
			shaderBody->AddChild( NewExpressionStatement( state, call ) );
		}

		if( outputsStruct )
		{
			header->SetType( TypeFromSymbol( outputsStruct->GetSymbol() ) );

			// Create a local variable for outputs
			auto resultDeclList = NewVarDeclaration( state, TypeFromSymbol( outputsStruct->GetSymbol() ), state.AllocateNameWithPrefix( "stageOut" ) );
			auto resultSymbol = resultDeclList->GetChild( 0 )->GetSymbol();
			shaderBody->AddChild( resultDeclList );

			for( size_t i = 0; i < outputsStruct->GetChildrenCount(); ++i )
			{
				// Add assignments for all fields in the output struct
				auto assignment = NewBinaryExpression(
						state,
						OP_EQUAL,
						NewDot( state, NewVarIdentifier( state, resultSymbol ), outputsStruct->GetChild( i )->GetChild( 0 )->GetSymbol() ),
						outputAccessors[i] );
				shaderBody->AddChild( NewExpressionStatement( state, assignment ) );
			}

			shaderBody->AddChild( NewReturn( state, NewVarIdentifier( state, resultSymbol ) ) );
		}
		else
		{
			header->SetType( TypeFromTokenType( OP_VOID ) );
		}

		state.GetSymbolTable().LeaveScope();

		auto shader = state.NewNode( NT_FUNCTION_DEFINITION );
		shaderSymbol->type = header->GetType();
		shaderSymbol->definition = shader;
		shader->AddChild( header );
		shader->AddChild( shaderBody );

		auto attribList = state.NewNode( NT_FUNCTION_ATTRIBUTE_LIST );
		attribList->AddChild( state.NewNode( NT_FUNCTION_ATTRIBUTE, ScannerToken::ID( shaderAttribute ) ) );
		shader->AddChild( attribList );
		state.GetTree()->AddChild( shader );

		// If the patched shader ends up using global scalar variables via uniform arguments, we need to
		// convert their references into GlobalsData field lookups
		if( auto globalStructSymbol = state.GetSymbolTable().Lookup( MakeInlineString( "GlobalsData" ) ) )
		{
			std::set<Symbol*> globalsStructMembers;
			for( auto member : globalStructSymbol->definition->GetChildren() )
			{
				globalsStructMembers.insert( member->GetChild( 0 )->GetSymbol() );
			}
			Symbol* globalsParam = nullptr;
			for( auto param : header->GetChildren() )
			{
				if( param->GetType().symbol == globalStructSymbol )
				{
					globalsParam = param->GetSymbol();
					break;
				}
			}
			shaderBody->Map( [&]( auto node ) {
				if( node->GetNodeType() == NT_VAR_IDENTIFIER && globalsStructMembers.find( node->GetSymbol() ) != end( globalsStructMembers ) )
				{
					if( !globalsParam )
					{
						Type type = TypeFromSymbol( globalStructSymbol );
						ScannerToken token = {};
						token.type = OP_INOUT;

						auto param = NewFunctionParameter( state, type, MakeInlineString( "Globals" ) );
						param->SetToken( &token );
						header->AddChild( param );
						globalsParam = param->GetSymbol();
						globalsParam->addressSpace = AddressSpace::Constant;
					}
					return NewDot( state, NewVarIdentifier( state, globalsParam ), node->GetSymbol()->name );
				}
				return node;
			} );
		}

		return shader;
	}

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
				if( annotation.name == "Tr2sRGB" )
				{
					isSRGB = result.intValue != 0;
				}
				else if( annotation.name == "AutoRegister" )
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

	void CalculateSizeAndOffset(const Type& type, size_t& size, size_t& offset)
	{
		size_t typeSize = 0;
		size_t typeAlignment = 0;

		const size_t boolSize[4] = { 1, 2, 4, 4 };
		const size_t boolAlignment[4] = { 1, 2, 4, 4 };

		const size_t intSize[4] = { 4, 8, 16, 16 };
		const size_t intAlignment[4] = { 4, 8, 16, 16 };

		const size_t packedIntSize[4] = { 4, 8, 12, 16 };
		const size_t packedIntAlignment[4] = { 4, 4, 4, 4 };

		const size_t floatSize[4] = { 4, 8, 16, 16 };
		const size_t floatAlignment[4] = { 4, 8, 16, 16 };

		const size_t packedFloatSize[4] = { 4, 8, 12, 16 };
		const size_t packedFloatAlignment[4] = { 4, 4, 4, 4 };

		const size_t floatMatrixSize[ 3 ][ 3 ] = {
			{ 16, 32, 32 }, // 2x2, 2x3, 2x4
			{ 24, 48, 48 }, // 3x2, 3x3, 3x4
			{ 32, 64, 64 }  // 4x2, 4x3, 4x4
		};
		const size_t floatMatrixAlignment[ 3 ][ 3 ] = {
			{ 8, 16, 16 }, // 2x2, 2x3, 2x4
			{ 8, 16, 16 }, // 3x2, 3x3, 3x4
			{ 8, 16, 16 }  // 4x2, 4x3, 4x4
		};

		assert( 1 <= type.width && type.width <= 4 );
		assert( 1 <= type.height && type.height <= 4 );

		bool isPacked = type.modifier == OP_PACKOFFSET;


		switch( type.builtInType )
		{
		case OP_BOOL:
			typeSize = boolSize[ type.width - 1 ];
			typeAlignment = boolAlignment[ type.width - 1 ];
			break;
		case OP_INT:
		case OP_UINT:
			if( isPacked )
			{
				typeSize = packedIntSize[type.width - 1];
				typeAlignment = packedIntAlignment[type.width - 1];
			}
			else
			{
				typeSize = intSize[type.width - 1];
				typeAlignment = intAlignment[type.width - 1];
			}
			break;
		case OP_FLOAT:
			if( type.height == 1 )
			{
				if( isPacked )
				{
					typeSize = packedFloatSize[type.width - 1];
					typeAlignment = packedFloatAlignment[type.width - 1];
				}
				else
				{
					typeSize = floatSize[type.width - 1];
					typeAlignment = floatAlignment[type.width - 1];
				}
			}
			else
			{
				assert( 2 <= type.width && type.width <= 4 );
				assert( 2 <= type.height && type.height <= 4 );

				int row = type.width - 2;
				int col = type.height - 2;

				// Matrices are column-major in MSL.
				typeSize = floatMatrixSize[ col ][ row ];
				typeAlignment = floatMatrixAlignment[ col ][ row ];
			}
			break;
		default:
			assert( false && "Unsupported type" );
			break;
		}

		size = typeSize;

		offset += typeAlignment - 1;
		offset -= offset % typeAlignment;
	}

	bool FillDefaults( ParserState& state, ASTNode* node, void* outDefaultValues, const Type& type )
	{
		Type expectedType = type;
		ExpressionValue value;

		if( type.IsMatrix() )
		{
			state.ShowMessage( node->GetLocation(), EC_UNSUPPORTED_TYPE, type.ToString().c_str() );
			return false;
		}

		if( EvaluateInitializer( state, node, expectedType, value, nullptr ) )
		{
			uint32_t count = type.width * type.height;
			assert( count == value.size() );

			if( type.builtInType == OP_FLOAT )
			{
				float* floatValues = (float*)outDefaultValues;
				for ( size_t i = 0; i < count; ++i )
				{
					*floatValues++ = float( value[i].floatValue );
				}
			}
			else if( type.builtInType == OP_INT )
			{
				int32_t* intValues = (int32_t*)outDefaultValues;
				for ( size_t i = 0; i < count; ++i )
				{
					*intValues++ = value[i].intValue;
				}
			}
			else
			{
				// Type not supported.
				state.ShowMessage( node->GetLocation(), EC_UNSUPPORTED_TYPE, type.ToString().c_str() );
				return false;
			}

			return true;
		}

		return false;
	}

	bool CollectConstants( ParserState& state, StageInput& stage, ASTNode* node, std::map<StringReference, ParameterAnnotation>& annotations )
	{
		assert( node->GetNodeType() == NT_STRUCT );

		size_t offset = 0;

		for( auto memberNode : node->GetChildren() )
		{
			assert( memberNode && memberNode->GetNodeType() == NT_STRUCT_MEMBER );

			for( auto childNode : memberNode->GetChildren() )
			{
				assert( childNode && childNode->GetNodeType() == NT_NAME_DECLARATION );

				Symbol* symbol = childNode->GetSymbol();
				assert( symbol );

				if( !symbol->used )
				{
					continue;
				}

				const Type& type = symbol->type;

				// Multi-dimensional arrays are not supported yet.
				assert( type.arrayDimensions <= 1 );

				size_t size = 0;
				CalculateSizeAndOffset( type, size, offset );

				int elementCount = 1;
				for( int i = 0; i < type.arrayDimensions; ++i )
				{
					elementCount *= type.arraySizes[ i ];
				}

				// Note: This calculation is correct only when type can be tightly packed in an array,
				// i.e. type size divisible by alignment.
				size *= elementCount;

				Constant constant;
				constant.name = g_stringTable.AddString( symbol->name );
				constant.offset = (uint32_t) offset;
				constant.size = (uint32_t) size;

				offset += size;

				switch( type.builtInType )
				{
				case OP_FLOAT:
					constant.type = CONSTANT_TYPE_FLOAT;
					break;
				case OP_INT:
					constant.type = CONSTANT_TYPE_INT;
					break;
				case OP_UINT:
					constant.type = CONSTANT_TYPE_UINT;
					break;
				case OP_BOOL:
					constant.type = CONSTANT_TYPE_BOOL;
					break;
				default:
					constant.type = CONSTANT_TYPE_OTHER;
					break;
				}

				constant.dimension = uint8_t( type.width * type.height );
				constant.elements = (uint32_t) elementCount;
				constant.isSRGB = false;
				constant.isAutoregister = false;

				stage.defaultValues.resize( std::max( stage.defaultValues.size(), size_t( constant.offset + constant.size ) ) );

				// Init with zeroes.
				memset( &stage.defaultValues[constant.offset], 0, constant.size );

				// Has default value?
				ASTNode* defaultValueNode = childNode->GetChildOrNull( 1 );
				if( defaultValueNode )
				{
					if( type.arrayDimensions == 0 )
					{
						if( !FillDefaults( state, defaultValueNode, stage.defaultValues.data() + constant.offset, type ) )
						{
							return false;
						}
					}
					else if( type.arrayDimensions == 1 )
					{
						for( size_t i = 0, n = defaultValueNode->GetChildrenCount(); i < n; ++i )
						{
							size_t elementOffset = constant.offset + i * size / elementCount;
							if( !FillDefaults( state, defaultValueNode->GetChild( i ), stage.defaultValues.data() + elementOffset, type ) )
							{
								return false;
							}
						}
					}
					else
					{
						state.ShowMessage( defaultValueNode->GetLocation(), EC_UNSUPPORTED_TYPE, type.ToString().c_str() );
						return false;
					}
				}

				if( symbol->annotations && annotations.find( constant.name ) == annotations.end() )
				{
					ParameterAnnotation paramAnnotations;

					for( auto a = symbol->annotations->begin(); a != symbol->annotations->end(); ++a )
					{
						Annotation result;
						if( MakeEffectAnnotationFromSymbolAnnotation( *a, result, constant.isSRGB, constant.isAutoregister ) )
						{
							paramAnnotations.annotations[g_stringTable.AddString( a->name )] = result;
						}
					}

					if( !paramAnnotations.annotations.empty() )
					{
						annotations[constant.name] = paramAnnotations;
					}
				}

				stage.constants.push_back( constant );
			}
		}
		return true;
	}

	RegisterInputType TypeToRegisterInputType( const Type& type )
	{
		switch( type.builtInType )
		{
		case OP_BUFFER:
			return RT_SRV_BUFFER;
		case OP_STRUCTUREDBUFFER:
			return RT_SRV_STRUCTURED_BUFFER;
		case OP_RWBUFFER:
			return RT_UAV_BUFFER;
		case OP_RWSTRUCTUREDBUFFER:
			return RT_UAV_STRUCTURED_BUFFER;
		case OP_TEXTURE:
			return RT_SRV_TEXTURE2D;
		case OP_TEXTURE1D:
			return RT_SRV_TEXTURE1D;
		case OP_TEXTURE1DARRAY:
			return RT_SRV_TEXTURE1DARRAY;
		case OP_TEXTURE2D:
			return RT_SRV_TEXTURE2D;
		case OP_TEXTURE2DARRAY:
			return RT_SRV_TEXTURE2DARRAY;
		case OP_TEXTURE3D:
			return RT_SRV_TEXTURE3D;
		case OP_TEXTURECUBE:
			return RT_SRV_TEXTURECUBE;
		case OP_TEXTURECUBEARRAY:
			return RT_SRV_TEXTURECUBEARRAY;
		case OP_TEXTURE2DMS:
			return RT_SRV_TEXTURE2DMS;
		case OP_TEXTURE2DMSARRAY:
			return RT_SRV_TEXTURE2DMSARRAY;
		case OP_RWTEXTURE1D:
			return RT_UAV_TEXTURE1D;
		case OP_RWTEXTURE1DARRAY:
			return RT_UAV_TEXTURE1DARRAY;
		case OP_RWTEXTURE2D:
			return RT_UAV_TEXTURE2D;
		case OP_RWTEXTURE2DARRAY:
			return RT_UAV_TEXTURE2DARRAY;
		case OP_RWTEXTURE3D:
			return RT_UAV_TEXTURE3D;
		case OP_SAMPLER:
		case OP_SAMPLER2D:
		case OP_SAMPLER3D:
		case OP_SAMPLERCUBE:
		case OP_SAMPLERCOMPARISON:
			return RT_SAMPLER;
		default:
			assert( false );
			return RT_SRV_TEXTURE2D;
		}
	}

	bool ExtractAnnotations( const InlineString& name, ParserState& state, ParameterAnnotation& paramAnnotations, bool* srgb, bool* autoregister )
	{
		paramAnnotations.annotations.clear();

		bool localSrgb = false;
		bool localAutoregister = false;

		auto globalSymbol = state.GetSymbolTable().LookupGlobal( ToString( name ).c_str() );
		if( globalSymbol && globalSymbol->annotations )
		{
			
			for( auto& ai : *globalSymbol->annotations )
			{
				Annotation result;
				if( MakeEffectAnnotationFromSymbolAnnotation( ai, result, localSrgb, localAutoregister ) )
				{
					paramAnnotations.annotations[g_stringTable.AddString( ai.name )] = result;
				}
			}
		}
		if( srgb )
		{
			*srgb = localSrgb;
		}
		if( autoregister )
		{
			*autoregister = localAutoregister;
		}
		return !paramAnnotations.annotations.empty();
	}

	bool GetStageData( ParserState& state, StageInput& stage, ASTNode* callNode, std::map<StringReference, ParameterAnnotation>& annotations )
	{
		const InlineString globalsStructName = MakeInlineString( "GlobalsData" );

		ASTNode* root = state.GetTree();
		if( root )
		{
			// Find "GlobalsData" struct.
			for( size_t i = 0, n = root->GetChildrenCount(); i < n; ++i )
			{
				ASTNode* node = root->GetChild( i );
				if( node && node->GetNodeType() == NT_STRUCT &&
					node->GetSymbol() && node->GetSymbol()->name == globalsStructName )
				{
					if( !CollectConstants( state, stage, node, annotations ) )
					{
						return false;
					}
				}
			}
		}

		Symbol* entryPointSymbol = callNode->GetSymbol();
		if( !entryPointSymbol || !entryPointSymbol->definition )
		{
			return false;
		}

		ASTNode* functionHeader = entryPointSymbol->definition->GetChildOrNull( 0 );
		if( !functionHeader )
		{
			return false;
		}

		for( size_t i = 0, n = functionHeader->GetChildrenCount(); i < n; ++i )
		{
			ASTNode* child = functionHeader->GetChild( i );
			assert( child );

			Symbol* symbol = child->GetSymbol();
			assert( symbol );

			const Type& type = child->GetType();

			assert( !symbol->registerSpecifier.empty() );
			const RegisterSpecifier& reg = symbol->registerSpecifier.cbegin()->second;

			switch( reg.registerType )
			{
				case MetalRegister::CBuffer:
					{
						stage.registerInputs.push_back( { RT_CONSTANT_BUFFER, (uint32_t)reg.registerNumber } );
					}
					continue;
				default:
					break;
			}

			switch( type.builtInType )
			{
			case OP_BUFFER:
			case OP_STRUCTUREDBUFFER:
				{
					assert( reg.registerType == MetalRegister::CBuffer ||
							reg.registerType == MetalRegister::SRV );

					stage.registerInputs.push_back( { TypeToRegisterInputType( type ), (uint32_t)reg.registerNumber } );

					if( symbol->used )
					{
						Texture texture;
						texture.name = g_stringTable.AddString( symbol->name );
						texture.type = type.builtInType == OP_BUFFER ? TEX_TYPE_BUFFER : TEX_TYPE_STRUCTURED_BUFFER;

						ParameterAnnotation paramAnnotations;
						if( ExtractAnnotations( symbol->name, state, paramAnnotations, &texture.isSRGB, &texture.isAutoregister ) )
						{
							annotations[texture.name] = paramAnnotations;
						}

						stage.textures[uint8_t( reg.registerNumber )] = texture;
					}
				}
				break;

			case OP_RWBUFFER:
			case OP_RWSTRUCTUREDBUFFER:
				{
					assert( reg.registerType == MetalRegister::UAV );

					stage.registerInputs.push_back( { TypeToRegisterInputType( type ), (uint32_t)reg.registerNumber } );

					if( symbol->used )
					{
						Uav uav;
						uav.name = g_stringTable.AddString( symbol->name );
						uav.type = ( type.builtInType == OP_RWBUFFER ) ? TEX_TYPE_UAV_RWTYPED : TEX_TYPE_UAV_RWSTRUCTURED;

						ParameterAnnotation paramAnnotations;
						if( ExtractAnnotations( symbol->name, state, paramAnnotations, nullptr, &uav.isAutoregister ) )
						{
							annotations[uav.name] = paramAnnotations;
						}

						stage.uavs[uint8_t( reg.registerNumber )] = uav;
					}
				}
				break;

			case OP_TEXTURE:
			case OP_TEXTURE1D:
			case OP_TEXTURE1DARRAY:
			case OP_TEXTURE2D:
			case OP_TEXTURE2DARRAY:
			case OP_TEXTURE3D:
			// case OP_TEXTURE3DARRAY:
			case OP_TEXTURECUBE:
			case OP_TEXTURECUBEARRAY:
			case OP_TEXTURE2DMS:
			case OP_TEXTURE2DMSARRAY:
			{
				assert( reg.registerType == MetalRegister::Texture );

				stage.registerInputs.push_back( { TypeToRegisterInputType( type ), (uint32_t)reg.registerNumber } );

				if( symbol->used )
				{
					Texture texture;
					texture.name = g_stringTable.AddString( symbol->name );

					ParameterAnnotation paramAnnotations;
					if( ExtractAnnotations( symbol->name, state, paramAnnotations, &texture.isSRGB, &texture.isAutoregister ) )
					{
						annotations[texture.name] = paramAnnotations;
					}

					switch( type.builtInType )
					{
					case OP_TEXTURE1D:
					case OP_TEXTURE1DARRAY:
						texture.type = TEX_TYPE_1D;
						break;
					case OP_TEXTURE:
					case OP_TEXTURE2D:
					case OP_TEXTURE2DARRAY:
					case OP_TEXTURE2DMS:
					case OP_TEXTURE2DMSARRAY:
						texture.type = TEX_TYPE_2D;
						break;
					case OP_TEXTURE3D:
					// case OP_TEXTURE3DARRAY:
						texture.type = TEX_TYPE_3D;
						break;
					case OP_TEXTURECUBE:
					case OP_TEXTURECUBEARRAY:
						texture.type = TEX_TYPE_CUBE;
						break;
					default:
						texture.type = TEX_TYPE_INVALID;
						break;
					}

					if( texture.type == TEX_TYPE_INVALID )
					{
						continue;
					}

					stage.textures[uint8_t( reg.registerNumber )] = texture;
				}
				break;
			}
			case OP_RWTEXTURE1D:
			case OP_RWTEXTURE1DARRAY:
			case OP_RWTEXTURE2D:
			case OP_RWTEXTURE2DARRAY:
			case OP_RWTEXTURE3D:
			// case OP_RWTEXTURE3DARRAY:
			{
				assert( reg.registerType == MetalRegister::UAV );

				stage.registerInputs.push_back( { TypeToRegisterInputType( type ), (uint32_t)reg.registerNumber } );

				if( symbol->used )
				{
					Uav uav;
					uav.name = g_stringTable.AddString( symbol->name );

					ParameterAnnotation paramAnnotations;
					if( ExtractAnnotations( symbol->name, state, paramAnnotations, nullptr, &uav.isAutoregister ) )
					{
						annotations[uav.name] = paramAnnotations;
					}

					switch( type.builtInType )
					{
					case OP_RWTEXTURE1D:
					case OP_RWTEXTURE1DARRAY:
						uav.type = TEX_TYPE_1D;
						break;
					case OP_RWTEXTURE2D:
					case OP_RWTEXTURE2DARRAY:
						uav.type = TEX_TYPE_2D;
						break;
					case OP_RWTEXTURE3D:
					// case OP_RWTEXTURE3DARRAY:
						uav.type = TEX_TYPE_3D;
						break;
					default:
						uav.type = TEX_TYPE_INVALID;
						break;
					}

					if( uav.type == TEX_TYPE_INVALID )
					{
						continue;
					}

					stage.uavs[uint8_t( reg.registerNumber )] = uav;
				}

				break;
			}

			case OP_SAMPLER:
			case OP_SAMPLER2D:
			case OP_SAMPLER3D:
			case OP_SAMPLERCUBE:
			case OP_SAMPLERCOMPARISON:
			{
				assert( reg.registerType == MetalRegister::Sampler );

				stage.registerInputs.push_back( { TypeToRegisterInputType( type ), (uint32_t)reg.registerNumber } );

				if( symbol->used )
				{
					Sampler sampler = {};

					auto globalSamplerSymbol = state.GetSymbolTable().LookupGlobal( ToString( symbol->name ).c_str() );
					if( !GetSamplerState( state, globalSamplerSymbol->definition, sampler ) )
					{
						return false;
					}

					if( sampler.addressU == D3D11_TEXTURE_ADDRESS_BORDER || sampler.addressV == D3D11_TEXTURE_ADDRESS_BORDER || sampler.addressW == D3D11_TEXTURE_ADDRESS_BORDER )
					{
						auto& c = sampler.borderColor;
						if( ( c.x != 0 || c.y != 0 || c.z != 0 || c.w != 0 ) &&
							( c.x != 0 || c.y != 0 || c.z != 0 || c.w != 1 ) &&
							( c.x != 1 || c.y != 1 || c.z != 1 || c.w != 1 ) )
						{
							state.ShowMessage( globalSamplerSymbol->definition->GetLocation(), EC_CUSTOM_ERROR, "sampler border color is not one of colors supported by Metal" );
							return false;
						}
					}

					sampler.name = g_stringTable.AddString( symbol->name );

					stage.samplers[uint8_t( reg.registerNumber )] = sampler;
				}

				break;
			}

			default:
				break;
			}
		}

		return true;
	}

	std::pair<int, std::string> RunProcess( const char* commandLine )
	{
		s_fileMutex.lock();
#ifdef _WIN32
		FILE* process = _popen( commandLine, "r" );
#else
		FILE* process = popen( commandLine, "r" );
#endif
		s_fileMutex.unlock();

		if( !process )
		{
			return std::make_pair( -1, "" );
		}
		std::string output;
		char readBuffer[128];
		while( fgets( readBuffer, sizeof( readBuffer ), process ) )
		{
			output += readBuffer;
		}
#ifdef _WIN32
		return std::make_pair( _pclose( process ), output );
#else
        return std::make_pair( pclose( process ), output );
#endif
	}

	template <typename T>
	void SplitString( const std::string& string, char delim, T callback )
	{
		size_t last = 0;
		size_t next = 0;
		while( ( next = string.find( delim, last ) ) != std::string::npos )
		{
			callback( string.substr( last, next - last ) );
			last = next + 1;
		}
		callback( string.substr( last ) );
	}

	struct MetalTool
	{
		std::string name;
	};

	std::string MetalTool( const char* name )
	{
		std::ostringstream cmd;
#ifdef _WIN32
		if( !g_metalToolsPath.empty() )
		{
			cmd << "\"" << g_metalToolsPath << "\\macos\\bin\\" << name << ".exe\"";
		}
		else
		{
			char programFiles[MAX_PATH] = { 0 };
			size_t programFilesSize;
			getenv_s( &programFilesSize, programFiles, "PROGRAMFILES" );

			cmd << "\"" << programFiles << "\\Metal Developer Tools\\macos\\bin\\" << name << ".exe\"";
		}
#else
		cmd << "xcrun -sdk macosx " << name;
#endif
		return cmd.str();
	}


	// Implements a disgusting method of funding which shader parameters turn out to be unused by the shader
	// and emits warnings for them.
	// Uses metal-objdump to disassemble the shader binary, finds a shader definition and looks through parameters.
	// Parameters having "readnone" token are unused. Or so I think...
	void DetectUnusedArguments( const char* libPath, ParserState& state, ASTNode* callNode )
	{
		tmFunction( 0, 0 );

		std::string entryName = ToString( callNode->GetChild( 1 )->GetSymbol()->name );
		auto functionHeader = callNode->GetChild( 1 )->GetSymbol()->definition->GetChild( 0 );

		for( auto child : functionHeader->GetChildren() )
		{
			if( child && child->GetSymbol() )
			{
				child->GetSymbol()->used = true;
			}
		}

		std::ostringstream cmd;
		cmd << MetalTool( "metal-objdump" ) << " -disassemble " << libPath;
		auto objdump = RunProcess( cmd.str().c_str() );
		if( objdump.first != 0 )
		{
			state.ShowMessage( callNode->GetLocation(), EC_CUSTOM_WARNING, "failed to run objdump, unused parameter information is unavailable" );
		}

		size_t argumentIndex = 0;
		size_t stageInIndex = 0;

		std::regex shader( "define.*@" + entryName + "(.*)" );
		std::smatch match;
		if( std::regex_search( objdump.second, match, shader ) )
		{
			auto args = match[1].str();
			SplitString( args, ',', [&]( auto arg ) {
				std::string argName = arg;
				Symbol* argumentSymbol = nullptr;

				if( argumentIndex < functionHeader->GetChildrenCount() )
				{
					argumentSymbol = functionHeader->GetChild( argumentIndex )->GetSymbol();

					bool isStageIn = false;
					for( auto reg : argumentSymbol->registerSpecifier )
					{
						if( reg.second.registerType == MetalRegister::StageIn )
						{
							isStageIn = true;
							break;
						}
					}

					argName = ToString( argumentSymbol->name );
					if( isStageIn )
					{
						argName += "." + ToString( functionHeader->GetChild( argumentIndex )->GetType().symbol->definition->GetChild( stageInIndex )->GetChild( 0 )->GetSymbol()->name );
						if( ++stageInIndex >= functionHeader->GetChild( argumentIndex )->GetType().symbol->definition->GetChildrenCount() )
						{
							++argumentIndex;
						}
					}
					else
					{
						++argumentIndex;
					}
				}

				if( arg.find( "readnone" ) != std::string::npos )
				{
					state.ShowMessage( callNode->GetLocation(), EC_CUSTOM_WARNING, "Unused argument %s", argName.c_str() );
					if( argumentSymbol )
					{
						argumentSymbol->used = false;
					}
				}
			} );
		}
	}

	bool IsIndexOperator( const ASTNode* node )
	{
		return node->GetNodeType() == NT_POSTFIX_EXPRESSION && node->GetToken() && node->GetToken()->type == OP_LEFT_BRACKET;
	}

	void PatchMatrixInitializer( ASTNode* parent, unsigned index, const Type& type )
	{
		// wrap matrix initializer in a "constructor", i.e. { a, b,..} -> float4x4( { a, b, ..} )
		auto initializer = parent->GetChild( index );
		if( type.arrayDimensions > 0 )
		{
			auto elementType = type;
			--elementType.arrayDimensions;

			for( unsigned i = 0; i < initializer->GetChildrenCount(); ++i )
			{
				PatchMatrixInitializer( initializer, i, elementType );
			}
		}
		else
		{
			ScannerToken token;
			token.type = type.builtInType;
			token.fileLocation = initializer->GetLocation();
			token.intValue = type.width | ( type.height << 8 );
			auto constructor = new ASTNode( NT_FUNCTION_CALL, initializer->GetLocation(), initializer->GetScope(), &token );
			constructor->AddChild( initializer );
			initializer->SetType( type );
			constructor->SetType( type );
			parent->ReplaceChild( index, constructor );
		}
	}

	void PatchMatrixRows( ParserState& state, ASTNode* node, ASTNode* parent = nullptr, bool rvalue = true, ASTNode* assignment = nullptr )
	{
		if( node->GetNodeType() == NT_NAME_DECLARATION && node->GetType().IsMatrix() && node->GetChildOrNull( 1 ) && node->GetChild( 1 )->GetNodeType() == NT_INLINE_CONSTRUCTOR )
		{
			PatchMatrixInitializer( node, 1, node->GetType() );
		}
		
		if( node->GetNodeType() == NT_FUNCTION_CALL && node->GetSymbol() == nullptr && node->GetType().IsMatrix() )
		{
			// transpose matrix constructors
			auto copy = node->Copy();
			auto transpose = ScannerToken::ID( MakeInlineString( "transpose" ), node->GetLocation() );
			node->SetToken( &transpose );
			node->GetChildren().clear();
			node->AddChild( copy );

			std::string diagnosticMessage;
			Symbol* symbol = state.GetSymbolTable().LookupFunction( transpose.stringValue, node, diagnosticMessage ); 
			assert( symbol );
			node->SetSymbol( symbol );

			if( symbol->intrinsicType )
			{
				node->SetType( ( *symbol->intrinsicType )( node, -1 ) );
			}
			else
			{
				node->SetType( symbol->type );
			}
			node = copy;
		}
		// looking for matrix[] expressions
		if( IsIndexOperator( node ) && node->GetChild( 0 )->GetType().IsMatrix() && node->GetChild( 0 )->GetType().arrayDimensions == 0 )
		{
			if( parent && parent->GetChild( 0 ) == node && IsIndexOperator( parent ) )
			{
				// m[x][y] -> m[y][x]
				auto x = node->GetChild( 1 );
				node->ReplaceChild( 1, parent->GetChild( 1 ) );
				parent->ReplaceChild( 1, x );
			}
			else
			{
				const auto& matrixType = node->GetChild( 0 )->GetType();
				if( rvalue )
				{
					// replace m[x] with matrixRow(m, x)
					ScannerToken callToken = ScannerToken::ID( MakeInlineString( "matrixRow" ), node->GetLocation() );
					node->SetToken( &callToken );
					node->SetNodeType( NT_FUNCTION_CALL );
				}
				else
				{
					// replace m[x] with MatrixRow#LH<#>(m, x)
					char* name = state.AllocateString( 32 );
#ifdef _WIN32
					sprintf_s( name, 32, "MatrixRow%dLH<%d>", matrixType.height, matrixType.width );
#else
					sprintf( name, "MatrixRow%dLH<%d>", matrixType.height, matrixType.width );
#endif
					ScannerToken callToken = ScannerToken::ID( MakeInlineString( name ), node->GetLocation() );
					node->SetToken( &callToken );
					node->SetNodeType( NT_FUNCTION_CALL );
				}
			}
		}
		if( node->GetNodeType() == NT_EXPRESSION && node->GetToken() )
		{
			switch( node->GetToken()->type )
			{
			case OP_EQUAL:
			case OP_MUL_ASSIGN:
			case OP_DIV_ASSIGN:
			case OP_MOD_ASSIGN:
			case OP_ADD_ASSIGN:
			case OP_SUB_ASSIGN:
			case OP_AND_ASSIGN:
			case OP_OR_ASSIGN:
			case OP_XOR_ASSIGN:
			case OP_LEFT_ASSIGN:
			case OP_RIGHT_ASSIGN:
				PatchMatrixRows( state, node->GetChild( 0 ), node, false, node );
				PatchMatrixRows( state, node->GetChild( 1 ), node, true, node );
				return;
			}
		}
		if( node->GetNodeType() != NT_POSTFIX_EXPRESSION )
		{
			rvalue = true;
		}
		for( auto child : node->GetChildren() )
		{
			if( !child )
			{
				continue;
			}
			PatchMatrixRows( state, child, node, rvalue, assignment );
		}
	}
}

const char* MetalSystemSemanticsType::GetString( int type )
{
	// Note: This array has to be syncronized with MetalSystemSemanticsType::Enum.
	const char* strings[] = {
		"position",
		"position, invariant",
		"front_facing",
		"vertex_id",
		"instance_id",
		"clip_distance",
		"point_size",
		"color(0)",
		"color(1)",
		"color(2)",
		"color(3)",
		"color(4)",
		"color(5)",
		"color(6)",
		"color(7)",
		"depth",
		"stencil",
		"thread_position_in_grid",
		"thread_position_in_threadgroup",
		"thread_index_in_threadgroup",
		"threadgroup_position_in_grid",
		"sample_id"
	};

	const int typeCount = sizeof( strings ) / sizeof( strings[0] );
	if( 0 <= type && type < typeCount )
	{
		return strings[ type ];
	}

	return "!!invalid_system_semantic!!";
};

bool EffectCompilerMetal::Create()
{
	auto compilerOutput = RunProcess( ( MetalTool( "metal" ) + " --version" ).c_str() );
	if( compilerOutput.first != 0 )
	{
		g_messages.AddMessage( "ShaderCompiler: error X0000: Could not invoke external Metal compiler %s. Make sure it is intalled.\n", MetalTool( "metal" ).c_str() );
		return false;
	}
	return true;
}

bool EffectCompilerMetal::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result )
{
	tmFunction( 0, 0 );

	ParserState state( MakeInlineString( source, source + sourceLength ) );
	for( auto& it : defines )
	{
		PreprocessorDefine d;
		d.location.fileName = MakeInlineString( "" );
		d.location.lineNumber = 0;
		d.value = MakeInlineString( it.value.c_str() );
		state.m_defines[MakeInlineString( it.name.c_str() )] = d;
	}

	if( !state.Parse() )
	{
		return false;
	}

	ASTNode* root = state.GetTree();
	if( root )
	{
		SortProgramNodes( root );
	}

	PatchStdlibFunctions( state );
	// PatchCBuffers( state );

	ReplaceFloatModulo( state );

	std::map<Symbol*, ASTNode*> functions;
	std::vector<ASTNode*> globals;

	TransferSRGBToTexturesDX11( state );

	CollectFunctions( state, functions );
	CollectGlobals( state, functions, globals );

	ConvertTextureFunctionsToMetal( state );
	ConvertSyncFunctionsToMetal( functions );

	if( !globals.empty() )
	{
		AddUsedGlobalsAsFunctionParams( functions, globals );

		for( auto it : globals )
		{
			if( it->GetType().symbol ) // Not a built-in type?
			{
				StripSemanticsInsideStucts( it->GetType().symbol->definition );
			}
		}
	}

	PatchMatrixRows( state, state.GetTree() );

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
		.literal( "platform" ).literal( "Metal" )
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
		technique.name = g_stringTable.AddString( techniqueNode->GetToken()->stringValue );

		listing.dict()
			.literal( "name" )
			.literal( techniqueNode->GetToken()->stringValue )
			.literal( "passes" )
			.list();

		for( size_t passIx = 0; passIx < techniqueNode->GetChildrenCount(); ++passIx )
		{
			listing.list();
			Pass outPass;
			ASTNode* passNode = techniqueNode->GetChild( passIx );
			for( size_t stateIx = 0; stateIx < passNode->GetChildrenCount(); ++stateIx )
			{
				if( passNode->GetChild( stateIx )->GetNodeType() == NT_STATE_ASSIGNMENT )
				{
						unsigned stateCode = 0;
						unsigned value = 0;
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
							case unsigned( -1 ):
							case unsigned( -2 ):
							case unsigned( -3 ):
							case unsigned( -4 ):
							case unsigned( -5 ):
							case unsigned( -6 ):
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

				if( shaderNode->GetChild( 1 )->GetSymbol() == nullptr )
				{
						return false;
				}

				// This must be called before PatchShader, because patching doesn't preserve
				// entry function's attributes.
				GetThreadGroupSize( stage, state, shaderNode->GetChild( 1 ) );

				RemapSystemSemanticsDXtoMetal( shaderNode->GetChild( 1 ) );

				std::string shaderCacheKey;
				{
						state.GetSymbolTable().ResetUsedFlag();
						MarkUsedSymbols( shaderNode->GetChild( 1 ), state );

						CompilerInputStream os( state, ShadingLanguage::MSL );
						os << MSL{ state.GetTree(), &state.GetSymbolTable() };
						shaderCacheKey = std::string( os.str(), os.str() + os.pcount() );
						state.GetSymbolTable().ResetUsedFlag();
				}

				auto patchedShader = PatchShader( stage.type, shaderNode->GetChild( 1 ), state );
				if( !patchedShader )
				{
						return false;
				}
				shaderNode->GetChild( 1 )->SetSymbol( patchedShader->GetChild( 0 )->GetSymbol() );

				ApplyPackedModifiersToConstantBuffers( patchedShader );

				switch( stage.type )
				{
				case VERTEX_STAGE:
						PrepareVertexFunction( stage, shaderNode->GetChild( 1 ) );
						break;
				case PIXEL_STAGE:
						PrepareFragmentFunction( stage, shaderNode->GetChild( 1 ) );
						break;
				case COMPUTE_STAGE:
						PrepareKernelFunction( stage, shaderNode->GetChild( 1 ) );
						break;
				default:
						assert( false && "Not supported." );
						break;
				}

				// Assign register numbers to all input textures, buffers, and samplers if they
				// don't have any register assigned already.
				if( !AutoAssignRegisters( state, shaderNode->GetChild( 1 ) ) )
				{
						return false;
				}

				state.GetSymbolTable().ResetUsedFlag();
				MarkUsedSymbols( shaderNode->GetChild( 1 ), state );

				CompilerInputStream os( state, ShadingLanguage::MSL );

				os << MSL_INCLUDE;

				os << AtomicFn{ "add", "Add" };
				os << AtomicFn{ "max", "Max" };
				os << AtomicFn{ "min", "Min" };
				os << AtomicFn{ "and", "And" };
				os << AtomicFn{ "or", "Or" };
				os << AtomicFn{ "xor", "Xor" };

				os << MSL{ state.GetTree(), &state.GetSymbolTable() };

				std::string entryPoint = ToString( shaderNode->GetChild( 1 )->GetSymbol()->name );

				std::string patchEntryPoint = entryPoint;
				state.ResetPragmaUsage();

				std::string code( os.str(), os.str() + os.pcount() );

				std::string stageName = "ps";
				if( stage.type == VERTEX_STAGE )
				{
						stageName = "vs";
				}

				bool hasCompiled = false;
				{
						std::lock_guard scope( m_compiledCS );
						auto found = m_compiled.find( shaderCacheKey );
						if( found != end( m_compiled ) )
						{
							if( found->second.empty() )
							{
								return false;
							}

							stage.shaderSize = uint32_t( found->second.size() );
							stage.shaderDataStr = g_stringTable.AddString( found->second.data(), found->second.size() );
							hasCompiled = true;
						}
				}
				if( !hasCompiled )
				{
						bool shaderWriteSucceeded = false;
						bool shaderCompileSucceeded = false;
						bool shaderBinaryReady = false;

						// "mktemp" function calls to "arc4random" which is not reentrant. So, we need to sync
						// our threads here to avoid parallel execution of this function.
						s_fileMutex.lock();

#ifdef _WIN32
						char srcFilenameTemplate[] = "mtl_tmpXXXXXXX";
						char binFilenameTemplate[] = "air_tmpXXXXXXX";

						_mktemp_s( srcFilenameTemplate );
						_mktemp_s( binFilenameTemplate );

						char* srcFilename = srcFilenameTemplate;
						char* binFilename = binFilenameTemplate;
#else
						char srcFilenameTemplate[] = "src_XXXXXXX.metal";
						char binFilenameTemplate[] = "bin_XXXXXXX.air";

						char* srcFilename = nullptr;
						char* binFilename = nullptr;

						{
							int srcFd = mkstemps( srcFilenameTemplate, 6 );
							int binFd = mkstemps( binFilenameTemplate, 4 );

							srcFilename = ( srcFd != -1 ) ? srcFilenameTemplate : nullptr;
							binFilename = ( binFd != -1 ) ? binFilenameTemplate : nullptr;

							close( srcFd );
							close( binFd );
						}
#endif
						s_fileMutex.unlock();

						if( !srcFilename || !binFilename )
						{
							// Failed to generate a name for temporary file(s).
							return false;
						}

						do
						{
							// Write shader source into temp file.
							{
								tmZone( 0, 0, "Write Source" );

								std::lock_guard withFileMutex( s_fileMutex );
								FILE* file = nullptr;
								if( fopen_s( &file, srcFilename, "w" ) != 0 )
								{
									// Failed to create shader source file.
									break;
								}
								size_t bytesWritten = fwrite( code.c_str(), 1, code.length(), file );
								fclose( file );
								file = nullptr;

								if( bytesWritten != code.length() )
								{
									// Failed to write shader source file.
									break;
								}
							}

							shaderWriteSucceeded = true;

							// Compile shader.
							{
								tmZone( 0, 0, "Call compiler" );

								std::ostringstream cmd;
								cmd << MetalTool( "metal" ) << " -x metal -std=macos-metal2.1 -mmacos-version-min=10.14 ";
								// This switch should probably be done in runtime via a command-line argument to ShaderCompiler.
#if 1
								// Disable shader debug information.
								cmd << "-Wno-unused-variable -Wno-missing-braces 2>&1";
#else
								// Enable shader debug information.
								cmd << "-frecord-sources=yes -gline-tables-only -Wno-unused-variable -Wno-missing-braces 2>&1";
#endif
								for( auto& it : defines )
								{
									cmd << " -D" << it.name << '=' << it.value;
								}
								cmd << " " << srcFilename << " -o " << binFilename;
								// g_messages.AddMessage( "Compile shader: %s", cmd.str().c_str() );

								auto compilerOutput = RunProcess( cmd.str().c_str() );
								if( !compilerOutput.second.empty() )
								{
									g_messages.AddMessage( "%s", compilerOutput.second.c_str() );
								}
								if( compilerOutput.first != 0 )
								{
									// Shader compilation failed.
									break;
								}

								// Disabled unused arguments detection because it provides nothing more than noise
								// DetectUnusedArguments( binFilename, state, shaderNode );
							}

							shaderCompileSucceeded = true;

							// Read shader binary.
							{
								tmZone( 0, 0, "Read binary" );

								std::lock_guard withFileMutex( s_fileMutex );
								FILE* file = nullptr;
								if( fopen_s( &file, binFilename, "rb" ) != 0 )
								{
									// Failed to open compiled shader file.
									break;
								}

								fseek( file, 0, SEEK_END );
								fpos_t pos = 0;
								fgetpos( file, &pos );
								fseek( file, 0, SEEK_SET );

								size_t dataSize = pos;
								std::vector<uint8_t> compiledCode( dataSize );

								size_t readBytes = fread( compiledCode.data(), 1, dataSize, file );

								fclose( file );
								file = nullptr;

								if( readBytes != dataSize )
								{
									// Failed to read compiled shader binary from file.
									break;
								}

								stage.shaderSize = uint32_t( compiledCode.size() );
								stage.shaderDataStr = g_stringTable.AddString( compiledCode.data(), compiledCode.size() );
								// printf( "shader size: %lu\n", (size_t)stage.shaderSize );

								{
									std::lock_guard scope( m_compiledCS );
									m_compiled[shaderCacheKey] = compiledCode;
								}
							}

							shaderBinaryReady = true;
						} while( false );

						// Remove temp files.
						if( shaderWriteSucceeded )
						{
							std::lock_guard withFileMutex( s_fileMutex );
							int removeResult = remove( srcFilename );
							if( removeResult != 0 )
							{
								// Can't delete shader source file.
								// return false;
							}
						}

						if( shaderCompileSucceeded )
						{
							std::lock_guard withFileMutex( s_fileMutex );
							int removeResult = remove( binFilename );
							if( removeResult != 0 )
							{
								// Can't delete compiled shader file.
								// return false;
							}
						}

						if( !shaderBinaryReady )
						{
							{
								std::lock_guard scope( m_compiledCS );
								m_compiled[code] = std::vector<uint8_t>();
							}
							return false;
						}
				}

				if( !GetStageData( state, stage, shaderNode->GetChild( 1 ), result.annotations ) )
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
							Annotation annotation;
							bool isSrgb, isAutoregister;
							if( MakeEffectAnnotationFromSymbolAnnotation( *a, annotation, isSrgb, isAutoregister ) )
							{
								stage.annotations.annotations[g_stringTable.AddString( a->name )] = annotation;
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

				listing.dict()
					.literal( "profile" ).literal( profile )
					.literal( "original" ).dict()
					.literal( "entryPoint" ).literal( patchEntryPoint )
					.literal( "source" ).literal( SanitizeCode( code ) );
				// TODO
				// PrintShaderOutListing( listing, effectData, reflection );
				listing.end();

				PrintStageInfo( listing, stage, result );
				listing.end();

				outPass.stages.push_back( stage );

				for( size_t n = 0; n < state.GetTree()->GetChildrenCount(); ++n )
				{
					if( state.GetTree()->GetChild( n ) == patchedShader )
					{
						state.GetTree()->RemoveChild( n );
						break;
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
