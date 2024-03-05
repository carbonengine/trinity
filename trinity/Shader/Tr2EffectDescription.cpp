#include "StdAfx.h"
#include "Tr2EffectDescription.h"

const BlueSharedString DEFAULT_TECHNIQUE = BlueSharedString( "Main" );
const BlueSharedString ANY_TECHNIQUE = BlueSharedString();


Tr2EffectStageInput::Tr2EffectStageInput()
	: m_exists( false )
	, samplers ( "Tr2EffectStageInput::samplers" )
	, resources ( "Tr2EffectStageInput::resources" )
	, uavs ( "Tr2EffectStageInput::uavs" )
	, constants( "Tr2EffectStageInput::constants" )
	, annotation( "Tr2EffectStageInput::annotation" )
	, m_shader ( INVALID )
	, m_constantValueSize( 0 )
{
	constantValues[0] = 0;
}

Tr2EffectDescription::Tr2EffectDescription()
:	techniques( "Tr2EffectDescription::techniques" ),
	annotations( "Tr2EffectDescription::annotations" )
{
}

namespace
{

template <typename A, typename B>
void CastToType( A& a, B b )
{
	a = A( b );
}

template <typename B>
void CastToType( bool& a, B b )
{
	a = b != 0;
}

}

bool Tr2EffectDescription::Read( const void* data, 
								 size_t dataSize, 
								 unsigned version,
								 const char* stringTable, 
								 size_t stringTableSize, 
								 const char* effectName )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	techniques.clear();
	annotations.clear();

	const uint8_t* buffer = reinterpret_cast<const uint8_t*>( data );
	const uint8_t* bufferEnd = buffer + dataSize;


#define READ( storeType, valueType, value )													\
	if( buffer + sizeof( storeType ) > bufferEnd )											\
	{																						\
		CCP_LOGERR( "Unexpected end of file while reading effect \"%s\"", effectName );		\
		return false;																		\
	}																						\
	CastToType( value, *reinterpret_cast<const storeType*>( buffer ) );						\
	buffer += sizeof( storeType );


#define READ_STRING( value )																\
	{																						\
		uint32_t offset;																	\
		READ( uint32_t, uint32_t, offset );													\
		if( offset >= stringTableSize )														\
		{																					\
			CCP_LOGERR( "Invalid string offset in effect \"%s\"", effectName );				\
			return false;																	\
		}																					\
		value = stringTable + offset;														\
	}

	auto ReadAnnotations = [&]( Tr2EffectParameterAnnotationMap& annotationMap ) -> bool
	{
		uint8_t annotationCount;
		READ( uint8_t, uint8_t, annotationCount );
		annotationMap.resize( annotationCount );

		for( int annotationIx = 0; annotationIx < annotationCount; ++annotationIx )
		{
			Tr2EffectParameterAnnotation& annotation = annotationMap[annotationIx];
			READ_STRING( annotation.name );
			READ( uint8_t, Tr2EffectParameterAnnotation::Type, annotation.type );

			if( annotation.type == Tr2EffectParameterAnnotation::STRING )
			{
				READ_STRING( annotation.stringValue );
			}
			else
			{
				READ( uint32_t, int, annotation.intValue );
			}
		}
		return true;
	};

	uint8_t techniqueCount = 1;
	if( version > 6 )
	{
		READ( uint8_t, uint8_t, techniqueCount );
	}
	techniques.resize( techniqueCount );

	for( uint8_t technique = 0; technique < techniqueCount; ++technique )
	{
		if( version > 6 )
		{
			const char* name;
			READ_STRING( name );
			techniques[technique].name = BlueSharedString( name );
		}
		else
		{
			techniques[technique].name = DEFAULT_TECHNIQUE;
		}
		techniques[technique].shaderTypeMask = 0;

		uint8_t passCount;
		READ( uint8_t, uint8_t, passCount );
		if( passCount > 64 )
		{
			CCP_LOGERR( "Too many passes in effect \"%s\". Corrupt file?", effectName );
			return false;
		}

		techniques[technique].passes.resize( passCount );
		for( int passIx = 0; passIx < passCount; ++passIx )
		{
			Tr2Pass& pass = techniques[technique].passes[passIx];
			pass.shaderTypeMask = 0;

			for( unsigned stageIx = 0; stageIx != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIx )
			{
				pass.stageInputs[stageIx].m_exists = false;
				pass.stageInputs[stageIx].m_shader = Tr2EffectStageInput::INVALID;
				pass.stageInputs[stageIx].m_constantValueSize = 0;
			}

			uint8_t stageCount;
			READ( uint8_t, uint8_t, stageCount );
			if( stageCount >= Tr2RenderContextEnum::SHADER_TYPE_COUNT )
			{
				CCP_LOGERR( "Too many stages in effect \"%s\". Corrupt file?", effectName );
				return false;
			}

			uint32_t shaderHandles[Tr2RenderContextEnum::SHADER_TYPE_COUNT];

			for( int stageIx = 0; stageIx < stageCount; ++stageIx )
			{
				Tr2RenderContextEnum::ShaderType type;
				READ( uint8_t, Tr2RenderContextEnum::ShaderType, type );
				pass.shaderTypeMask |= 1u << type;

				uint8_t inputCount;
				READ( uint8_t, uint8_t, inputCount );
				if( inputCount > 64 )
				{
					CCP_LOGERR( "Too many shader pipeline inputs in effect \"%s\". Corrupt file?", effectName );
					return false;
				}

				pass.stageInputs[type].m_exists = true;
				pass.stageInputs[type].signature.pipelineInputs.resize( inputCount );
				for( int inputIx = 0; inputIx < inputCount; ++inputIx )
				{
					auto& element = pass.stageInputs[type].signature.pipelineInputs[inputIx];
					READ( uint8_t, Tr2VertexDefinition::UsageCode, element.usage );
					READ( uint8_t, uint32_t, element.registerIndex );
					READ( uint8_t, uint32_t, element.usageIndex );
					READ( uint8_t, uint32_t, element.usedMask );
					if( version > 10 )
					{
						READ( uint8_t, Tr2ShaderPipelineInputAL::Type, element.type );
						READ( uint8_t, uint32_t, element.dimension );
					}
					else
					{
						element.type = element.usage == Tr2VertexDefinition::BLENDINDICES ? Tr2ShaderPipelineInputAL::UINT : Tr2ShaderPipelineInputAL::FLOAT;
						element.dimension = 4;
					}
				}

				if( version > 8 )
				{
					READ( uint8_t, uint8_t, inputCount );
					pass.stageInputs[type].signature.registers.resize( inputCount );
					for( int inputIx = 0; inputIx < inputCount; ++inputIx )
					{
						auto& element = pass.stageInputs[type].signature.registers[inputIx];
						if( version > 9 )
						{
							READ( uint8_t, Tr2ShaderRegisterAL::RegisterType, element.registerType );
						}
						else
						{
							uint8_t oldRegisterType;
							READ( uint8_t, uint8_t, oldRegisterType );
							switch( oldRegisterType )
							{
							case 0:
								element.registerType = Tr2ShaderRegisterAL::CONSTANT_BUFFER;
								break;
							case 1:
								element.registerType = Tr2ShaderRegisterAL::SRV_TEXTURE2D; // best guess
								break;
							case 2:
								element.registerType = Tr2ShaderRegisterAL::UAV_TEXTURE2D; // best guess
								break;
							case 3:
								element.registerType = Tr2ShaderRegisterAL::SAMPLER;
								break;
							default:
								CCP_ASSERT( false );
								element.registerType = Tr2ShaderRegisterAL::SRV_TEXTURE2D;
							}
						}
						READ( uint32_t, unsigned, element.registerIndex );
					}
				}

				uint32_t shaderSize;
				const void* shaderCode;

				if( version < 5 )
				{
					READ( uint32_t, uint32_t, shaderSize );
					if( buffer + shaderSize > bufferEnd )
					{
						CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
						return false;
					}
					shaderCode = buffer;
					buffer += shaderSize;

					uint32_t shadowShaderSize;
					READ( uint32_t, uint32_t, shadowShaderSize );
					if( buffer + shadowShaderSize > bufferEnd )
					{
						CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
						return false;
					}
					buffer += shadowShaderSize;
				}
				else
				{
					uint32_t offset;
					READ( uint32_t, uint32_t, shaderSize );
					READ( uint32_t, uint32_t, offset );
					if( shaderSize > 0 && offset + shaderSize > stringTableSize )
					{
						CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
						return false;
					}
					shaderCode = stringTable + offset;
					if( version < 12 )
					{
						uint32_t shadowShaderSize;
						READ( uint32_t, uint32_t, shadowShaderSize );
						READ( uint32_t, uint32_t, offset );
						if( shadowShaderSize > 0 && offset + shadowShaderSize > stringTableSize )
						{
							CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
							return false;
						}
					}
				}

				pass.stageInputs[type].signature.threadGroupSize = Tr2ShaderThreadGroupSizeAL();
				if( version >= 3 )
				{
					READ( uint32_t, uint32_t, pass.stageInputs[type].signature.threadGroupSize.x );
					READ( uint32_t, uint32_t, pass.stageInputs[type].signature.threadGroupSize.y );
					READ( uint32_t, uint32_t, pass.stageInputs[type].signature.threadGroupSize.z );
				}

				uint32_t constantCount;
				READ( uint32_t, uint32_t, constantCount );
				if( buffer + constantCount > bufferEnd )
				{
					CCP_LOGERR( "Too many shader constants in effect \"%s\". Corrupt file?", effectName );
					return false;
				}

				pass.stageInputs[type].constants.resize( constantCount );
				for( unsigned constantIx = 0; constantIx < constantCount; ++constantIx )
				{
					Tr2EffectConstant constant;

					const char* name;
					READ_STRING( name );
					constant.name = BlueSharedString( name );

					READ( uint32_t, unsigned, constant.offset );
					READ( uint32_t, unsigned, constant.size );
					if( version < 11 )
					{
						uint8_t oldType = 0;
						READ( uint8_t, uint8_t, oldType );
						switch( oldType )
						{
						case 0:
							constant.type = Tr2EffectConstant::FLOAT;
							break;
						case 1:
							constant.type = Tr2EffectConstant::INT;
							break;
						case 2:
							constant.type = Tr2EffectConstant::BOOL;
							break;
						default:
							constant.type = Tr2EffectConstant::OTHER;
							break;
						}
					}
					else
					{
						READ( uint8_t, Tr2EffectConstant::Type, constant.type );
					}
					READ( uint8_t, unsigned, constant.dimension );
					READ( uint32_t, unsigned, constant.elements );
					READ( uint8_t, bool, constant.isSRGB );
					READ( uint8_t, bool, constant.isAutoregister );

					pass.stageInputs[type].constants[constantIx] = constant;
				}

				unsigned constantValueSize;
				READ( uint32_t, unsigned, constantValueSize );

				if( version < 5 )
				{
					if( buffer + constantValueSize > bufferEnd )
					{
						CCP_LOGERR( "Constant blob is too large in effect \"%s\". Corrupt file?", effectName );
						return false;
					}

					pass.stageInputs[type].m_constantValueSize = constantValueSize;

					if( constantValueSize > SHADER_CONSTANTS_MAX )
					{
						CCP_LOGERR( "Effect \"%s\" has more than %i bytes in constant buffer", effectName, SHADER_CONSTANTS_MAX );
						pass.stageInputs[type].m_constantValueSize = SHADER_CONSTANTS_MAX;
					}

					memcpy( pass.stageInputs[type].constantValues, buffer, pass.stageInputs[type].m_constantValueSize );
					buffer += constantValueSize;
				}
				else
				{
					uint32_t constantValueOffset;
					READ( uint32_t, uint32_t, constantValueOffset );
					if( constantValueSize && constantValueOffset + constantValueSize > stringTableSize )
					{
						CCP_LOGERR( "Constant blob is too large in effect \"%s\". Corrupt file?", effectName );
						return false;
					}

					pass.stageInputs[type].m_constantValueSize = constantValueSize;

					if( constantValueSize > SHADER_CONSTANTS_MAX )
					{
						CCP_LOGERR( "Effect \"%s\" has more than %i bytes in constant buffer", effectName, SHADER_CONSTANTS_MAX );
						pass.stageInputs[type].m_constantValueSize = SHADER_CONSTANTS_MAX;
					}

					if( constantValueSize )
					{
						memcpy( pass.stageInputs[type].constantValues, stringTable + constantValueOffset, pass.stageInputs[type].m_constantValueSize );
					}
				}

				uint8_t textureCount;
				READ( uint8_t, uint8_t, textureCount );
				if( textureCount > 64 )
				{
					CCP_LOGERR( "Too many textures in effect \"%s\". Corrupt file?", effectName );
					return false;
				}

				for( int textureIx = 0; textureIx < textureCount; ++textureIx )
				{
					uint8_t registerIndex;
					READ( uint8_t, uint8_t, registerIndex );

					Tr2EffectResource resource;

					READ_STRING( resource.name );
					READ( uint8_t, Tr2EffectResource::Type, resource.type );
					READ( uint8_t, bool, resource.isSRGB );
					READ( uint8_t, bool, resource.isAutoregister );

					pass.stageInputs[type].resources[registerIndex] = resource;
				}


				uint8_t samplerCount;
				READ( uint8_t, uint8_t, samplerCount );
				if( samplerCount > 64 )
				{
					CCP_LOGERR( "Too many samplers in effect \"%s\". Corrupt file?", effectName );
					return false;
				}

				for( int samplerIx = 0; samplerIx < samplerCount; ++samplerIx )
				{
					uint8_t registerIndex;
					READ( uint8_t, uint8_t, registerIndex );

					Tr2SamplerSetup samplerSetup;

					if( version >= 4 )
					{
						READ_STRING( samplerSetup.name );
					}
					else
					{
						samplerSetup.name = nullptr;
					}

					bool comparison;
					READ( uint8_t, bool, comparison );

					Tr2RenderContextEnum::TextureFilter minFilter;
					READ( uint8_t, Tr2RenderContextEnum::TextureFilter, minFilter );
					Tr2RenderContextEnum::TextureFilter magFilter;
					READ( uint8_t, Tr2RenderContextEnum::TextureFilter, magFilter );
					Tr2RenderContextEnum::TextureFilter mipFilter;
					READ( uint8_t, Tr2RenderContextEnum::TextureFilter, mipFilter );

					Tr2RenderContextEnum::TextureAddressMode addressU;
					READ( uint8_t, Tr2RenderContextEnum::TextureAddressMode, addressU );
					Tr2RenderContextEnum::TextureAddressMode addressV;
					READ( uint8_t, Tr2RenderContextEnum::TextureAddressMode, addressV );
					Tr2RenderContextEnum::TextureAddressMode addressW;
					READ( uint8_t, Tr2RenderContextEnum::TextureAddressMode, addressW );

					float mipLODBias;
					READ( float, float, mipLODBias );
					unsigned maxAnisotropy;
					READ( uint8_t, unsigned, maxAnisotropy );

					Tr2RenderContextEnum::CompareFunc comparisonFunc;
					READ( uint8_t, Tr2RenderContextEnum::CompareFunc, comparisonFunc );

					Color borderColor;
					READ( float, float, borderColor.r );
					READ( float, float, borderColor.g );
					READ( float, float, borderColor.b );
					READ( float, float, borderColor.a );

					float minLOD;
					READ( float, float, minLOD );

					float maxLOD;
					READ( float, float, maxLOD );

					if( version < 4 )
					{
						bool isSRGBTexture;
						READ( uint8_t, bool, isSRGBTexture );
					}

					Tr2SamplerDescription sampler(
						minFilter,
						magFilter,
						mipFilter,
						comparison,
						addressU,
						addressV,
						addressW,
						mipLODBias,
						maxAnisotropy,
						comparisonFunc,
						&borderColor.r,
						minLOD,
						maxLOD );

					samplerSetup.sampler.Create( sampler, renderContext );

					pass.stageInputs[type].samplers[registerIndex] = samplerSetup;
				}

				if( version >= 3 )
				{
					uint8_t uavCount;
					READ( uint8_t, uint8_t, uavCount );
					if( uavCount > 64 )
					{
						CCP_LOGERR( "Too many UAV bindings in effect \"%s\". Corrupt file?", effectName );
						return false;
					}

					for( int uavIx = 0; uavIx < uavCount; ++uavIx )
					{
						uint8_t registerIndex;
						READ( uint8_t, uint8_t, registerIndex );

						Tr2EffectResource resource;
						resource.isSRGB = false;

						READ_STRING( resource.name );
						READ( uint8_t, Tr2EffectResource::Type, resource.type );
						READ( uint8_t, bool, resource.isAutoregister );

						pass.stageInputs[type].uavs[registerIndex] = resource;
					}
					if( version >= 8 )
					{
						if( !ReadAnnotations( pass.stageInputs[type].annotation ) )
						{
							return false;
						}
					}
				}
				pass.stageInputs[type].m_shader = Tr2EffectStateManager::RegisterShader(
					type,
					Tr2ShaderBytecodeAL( shaderCode, shaderSize ),
					pass.stageInputs[type].signature,
					effectName);
				shaderHandles[stageIx] = pass.stageInputs[type].m_shader;

				if( pass.stageInputs[type].m_shader == unsigned( -1 ) )
				{
					CCP_LOGERR( "Error compiling %s shader in effect \"%s\".", type == Tr2RenderContextEnum::VERTEX_SHADER ? "vertex" : "fragment", effectName );
					techniques.clear();
					return false;
				}

			}

			pass.shaderProgram = Tr2EffectStateManager::RegisterShaderProgram( shaderHandles, stageCount );
			if( pass.shaderProgram == unsigned( -1 ) )
			{
				CCP_LOGERR( "Error creating shader program in effect \"%s\".", effectName );
				techniques.clear();
				return false;
			}

			pass.resourceSetDesc = Tr2ResourceSetDescriptionAL( *Tr2EffectStateManager::GetShaderProgram( pass.shaderProgram ) );
			for( int32_t type = 0; type < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++type )
			{
				if( !pass.stageInputs[type].m_exists )
				{
					continue;
				}
				for( auto& sampler : pass.stageInputs[type].samplers )
				{
					pass.resourceSetDesc.SetSampler( Tr2RenderContextEnum::ShaderType( type ), sampler.first, sampler.second.sampler );
				}
			}
			
			uint8_t stateCount;
			READ( uint8_t, uint8_t, stateCount );
			if( stateCount > 64 )
			{
				CCP_LOGERR( "Too many render states in effect \"%s\". Corrupt file?", effectName );
				return false;
			}

			Tr2EffectStateManager::Tr2RenderStateSetup states;
			for( int stateIx = 0; stateIx < stateCount; ++stateIx )
			{
				Tr2RenderContextEnum::RenderState state;
				READ( uint32_t, Tr2RenderContextEnum::RenderState, state );
				uint32_t value;
				READ( uint32_t, uint32_t, value );

				states[state] = value;
			}
			pass.renderStates = Tr2EffectStateManager::RegisterRenderStateSetup( states );

			techniques[technique].shaderTypeMask |= pass.shaderTypeMask;
		}
	}
	uint16_t parameterCount;
	READ( uint16_t, uint16_t, parameterCount );
	if( parameterCount > 256 )
	{
		CCP_LOGERR( "Too many annotations in effect \"%s\". Corrupt file?", effectName );
		return false;
	}

	for( int paramIx = 0; paramIx < parameterCount; ++paramIx )
	{
		const char* name;
		READ_STRING( name );
		auto map = annotations.insert( std::make_pair( name, Tr2EffectParameterAnnotationMap( "Tr2EffectParameterAnnotationMap" ) ) );
		Tr2EffectParameterAnnotationMap& annotationMap = map.first->second;

		if( !ReadAnnotations( annotationMap ) )
		{
			return false;
		}
	}
	return true;
}

