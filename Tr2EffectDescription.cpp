#include "StdAfx.h"
#include "Tr2EffectDescription.h"

Tr2EffectStageInput::Tr2EffectStageInput()
	: m_exists( false )
	, samplers ( "Tr2EffectStageInput::samplers" )
	, resources ( "Tr2EffectStageInput::resources" )
	, uavs ( "Tr2EffectStageInput::uavs" )
	, constants( "Tr2EffectStageInput::constants" )
	, m_shader ( INVALID )
	, m_constantValueSize( 0 )
{
}

Tr2EffectDescription::Tr2EffectDescription()
:	passes	 ( "Tr2EffectDescription::passes" ),
	annotations( "Tr2EffectDescription::annotations" )
{
}

bool Tr2EffectDescription::Read( const void* data, 
								 size_t dataSize, 
								 unsigned version,
								 const char* stringTable, 
								 size_t stringTableSize, 
								 const char* effectName )
{
	passes.clear();
	annotations.clear();

	const uint8_t* buffer = reinterpret_cast<const uint8_t*>( data );
	const uint8_t* bufferEnd = buffer + dataSize;


#define READ( storeType, valueType, value )													\
	if( buffer + sizeof( storeType ) > bufferEnd )											\
	{																						\
		CCP_LOGERR( "Unexpected end of file while reading effect \"%s\"", effectName );		\
		return false;																		\
	}																						\
	value = valueType( *reinterpret_cast<const storeType*>( buffer ) );						\
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


	uint8_t passCount;
	READ( uint8_t, uint8_t, passCount );
	if( passCount > 64 )
	{
		CCP_LOGERR( "Too many passes in effect \"%s\". Corrupt file?", effectName );
		return false;
	}

	passes.resize( passCount );
	for( int passIx = 0; passIx < passCount; ++passIx )
	{
		Tr2Pass& pass = passes[passIx];

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

		for( int stageIx = 0; stageIx < stageCount; ++stageIx )
		{
			Tr2RenderContextEnum::ShaderType type;
			READ( uint8_t, Tr2RenderContextEnum::ShaderType, type );
			
			uint8_t inputCount;
			READ( uint8_t, uint8_t, inputCount );
			if( inputCount > 64 )
			{
				CCP_LOGERR( "Too many shader pipeline inputs in effect \"%s\". Corrupt file?", effectName );
				return false;
			}

			pass.stageInputs[type].m_exists = true;
			pass.stageInputs[type].inputDefinition.elements.resize( inputCount );
			for( int inputIx = 0; inputIx < inputCount; ++inputIx )
			{
				Tr2ShaderInputDefinitionElement& element = pass.stageInputs[type].inputDefinition.elements[inputIx];
				READ( uint8_t, Tr2VertexDefinition::UsageCode, element.usage );
				READ( uint8_t, unsigned, element.registerIndex );
				READ( uint8_t, unsigned, element.usageIndex );
				READ( uint8_t, unsigned, element.usedMask );
			}
			pass.stageInputs[type].inputDefinition.ComputeHash();

			uint32_t shaderSize;
			READ( uint32_t, uint32_t, shaderSize );
			if( buffer + shaderSize > bufferEnd )
			{
				CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
				return false;
			}
			const void* shaderCode = buffer;
			buffer += shaderSize;

			uint32_t shadowShaderSize;
			READ( uint32_t, uint32_t, shadowShaderSize );
			if( buffer + shadowShaderSize > bufferEnd )
			{
				CCP_LOGERR( "Shader binary is too large in effect \"%s\". Corrupt file?", effectName );
				return false;
			}
			const void* shadowShaderCode = buffer;
			buffer += shadowShaderSize;

			pass.stageInputs[type].m_shader = Tr2EffectStateManager::RegisterShader( 
				type, 
				shaderCode, 
				shaderSize, 
				shadowShaderCode, 
				shadowShaderSize, 
				pass.stageInputs[type].inputDefinition );

			if( pass.stageInputs[type].m_shader == unsigned( -1 ) )
			{
				CCP_LOGERR( "Error compiling %s shader in effect \"%s\".", type == Tr2RenderContextEnum::VERTEX_SHADER ? "vertex" : "fragment", effectName );
				passes.clear();
				return false;
			}

			pass.stageInputs[type].threadGroupSize[0] = 0;
			pass.stageInputs[type].threadGroupSize[1] = 0;
			pass.stageInputs[type].threadGroupSize[2] = 0;
			if( version >= 3 )
			{
				READ( uint32_t, uint32_t, pass.stageInputs[type].threadGroupSize[0] );
				READ( uint32_t, uint32_t, pass.stageInputs[type].threadGroupSize[1] );
				READ( uint32_t, uint32_t, pass.stageInputs[type].threadGroupSize[2] );
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
				READ( uint8_t, Tr2EffectConstant::Type, constant.type );
				READ( uint8_t, unsigned, constant.dimension );
				READ( uint32_t, unsigned, constant.elements );
				READ( uint8_t, bool, constant.isSRGB );
				READ( uint8_t, bool, constant.isAutoregister );

				pass.stageInputs[type].constants[constantIx] = constant;
			}

			unsigned constantValueSize;
			READ( uint32_t, unsigned, constantValueSize );
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

				bool isSRGBTexture;
				READ( uint8_t, bool, isSRGBTexture );

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
					borderColor,
					minLOD,
					maxLOD,
					isSRGBTexture );

				samplerSetup.handle = Tr2EffectStateManager::RegisterSamplerSetup( sampler );

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
	}
	return true;
}

