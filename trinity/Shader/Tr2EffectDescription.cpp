#include "StdAfx.h"

#include "Tr2EffectDescription.h"
#include "Tr2Renderer.h"


const BlueSharedString DEFAULT_TECHNIQUE = BlueSharedString( "Main" );
const BlueSharedString ANY_TECHNIQUE = BlueSharedString();


Tr2EffectStageInput::Tr2EffectStageInput()
	: m_exists( false )
	, samplers( "Tr2EffectStageInput::samplers" )
	, resources( "Tr2EffectStageInput::resources" )
	, uavs( "Tr2EffectStageInput::uavs" )
	, constants( "Tr2EffectStageInput::constants" )
	, annotation( "Tr2EffectStageInput::annotation" )
	, m_shader( INVALID )
	, m_constantValueSize( 0 )
{
	constantValues[0] = 0;
}

Tr2EffectDescription::Tr2EffectDescription()
	: techniques( "Tr2EffectDescription::techniques" ),
	annotations( "Tr2EffectDescription::annotations" )
{
}

namespace
{

	template <typename T1, typename T2>
	T1 SanityCheck( T1 value, T2 limit )
	{
		if( value > limit )
		{
			throw std::runtime_error( "Unexpected value" );
		}
		return value;
	}

	class EffectStream
	{
	public:
		EffectStream( const void* data, size_t dataSize, const char* stringTable, size_t stringTableSize )
			:m_current( static_cast<const uint8_t*>(data) ),
			m_end( static_cast<const uint8_t*>(data) + dataSize ),
			m_stringTable( stringTable ),
			m_tableSize( stringTableSize )
		{
		}

		void ReadRaw( void* dest, size_t size )
		{
			if( m_current + size > m_end )
			{
				throw std::runtime_error( "Unexpected end of file while" );
			}
			memcpy( dest, m_current, size );
			m_current += size;
		}

		const void* ReadRaw( size_t size )
		{
			if( m_current + size > m_end )
			{
				throw std::runtime_error( "Unexpected end of file while" );
			}
			auto result = m_current;
			m_current += size;
			return result;
		}

		const char* ReadString( size_t sizeHint = 0 )
		{
			uint32_t offset = Read<uint32_t>();
			if( offset + sizeHint > m_tableSize )
			{
				throw std::runtime_error( "Invalid string offset" );
			}
			return m_stringTable + offset;
		}

		const char* ReadStringOptional( size_t length )
		{
			if( length == 0 )
			{
				Read<uint32_t>();
				return nullptr;
			}
			else
			{
				return ReadString( length );
			}
		}

		template <typename StoreType>
		void Read( StoreType& dest )
		{
			dest = *static_cast<const StoreType*>(ReadRaw( sizeof( StoreType ) ));
		}

		template <typename StoreType>
		StoreType Read()
		{
			return *static_cast<const StoreType*>(ReadRaw( sizeof( StoreType ) ));
		}

	private:
		const uint8_t* m_current;
		const uint8_t* m_end;

		const char* m_stringTable;
		size_t m_tableSize;
	};


	void ReadAnnotations( Tr2EffectParameterAnnotationMap& annotationMap, EffectStream& stream )
	{
		uint8_t annotationCount = stream.Read<uint8_t>();
		annotationMap.resize( annotationCount );

		for( int annotationIx = 0; annotationIx < annotationCount; ++annotationIx )
		{
			Tr2EffectParameterAnnotation& annotation = annotationMap[annotationIx];
			annotation.name = stream.ReadString();
			annotation.type = Tr2EffectParameterAnnotation::Type( stream.Read<uint8_t>() );

			if( annotation.type == Tr2EffectParameterAnnotation::STRING )
			{
				annotation.stringValue = stream.ReadString();
			}
			else
			{
				annotation.intValue = int( stream.Read<uint32_t>() );
			}
		}
	}

	Tr2EffectConstant ReadConstant( EffectStream& stream, unsigned version )
	{
		Tr2EffectConstant constant;
		constant.name = BlueSharedString( stream.ReadString() );
		constant.offset = stream.Read<uint32_t>();
		constant.size = stream.Read<uint32_t>();
		if( version < 11 )
		{
			uint8_t oldType = stream.Read<uint8_t>();
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
			constant.type = Tr2EffectConstant::Type( stream.Read<uint8_t>() );
		}
		constant.dimension = stream.Read<uint8_t>();
		constant.elements = stream.Read<uint32_t>();
		constant.isSRGB = stream.Read<uint8_t>() != 0;
		constant.isAutoregister = stream.Read<uint8_t>() != 0;
		return constant;
	}

	Tr2EffectResource ReadResource( EffectStream& stream, unsigned version )
	{
		Tr2EffectResource resource;
		resource.name = stream.ReadString();
		resource.type = Tr2EffectResource::Type( stream.Read<uint8_t>() );
		// CHECK IS IT IN RIGHT FUNCTION?
		if( version >= 13 )
		{
			stream.Read( resource.arrayElements );
		}
		else
		{
			resource.arrayElements = 1;
		}
		resource.isSRGB = stream.Read<uint8_t>() != 0;
		resource.isAutoregister = stream.Read<uint8_t>() != 0;

		return resource;
	}

	Tr2SamplerDescription ReadSampler( EffectStream& stream )
	{
		bool comparison = stream.Read<uint8_t>() != 0;

		auto minFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );
		auto magFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );
		auto mipFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );

		auto addressU = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );
		auto addressV = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );
		auto addressW = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );

		float mipLODBias = stream.Read<float>();
		unsigned maxAnisotropy = stream.Read<uint8_t>();

		auto comparisonFunc = Tr2RenderContextEnum::CompareFunc( stream.Read<uint8_t>() );

		Color borderColor;
		stream.Read( borderColor.r );
		stream.Read( borderColor.g );
		stream.Read( borderColor.b );
		stream.Read( borderColor.a );

		float minLOD = stream.Read<float>();
		float maxLOD = stream.Read<float>();

		return Tr2SamplerDescription(
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
	}

	void ReadPipelineInputs( std::vector<Tr2ShaderPipelineInputAL>& pipelineInputs, EffectStream& stream, unsigned version )
	{
		uint8_t inputCount = SanityCheck( stream.Read<uint8_t>(), 64 );
		pipelineInputs.resize( inputCount );
		for( int inputIx = 0; inputIx < inputCount; ++inputIx )
		{
			auto& element = pipelineInputs[inputIx];
			element.usage = Tr2VertexDefinition::UsageCode( stream.Read<uint8_t>() );
			element.registerIndex = stream.Read<uint8_t>();
			element.usageIndex = stream.Read<uint8_t>();
			element.usedMask = stream.Read<uint8_t>();
			if( version > 10 )
			{
				element.type = Tr2ShaderPipelineInputAL::Type( stream.Read<uint8_t>() );
				element.dimension = stream.Read<uint8_t>();
			}
			else
			{
				element.type = element.usage == Tr2VertexDefinition::BLENDINDICES ? Tr2ShaderPipelineInputAL::UINT : Tr2ShaderPipelineInputAL::FLOAT;
				element.dimension = 4;
			}
		}
	}
	// CHECK
	void ReadRegisters( Tr2ShaderSignatureAL& signature, EffectStream& stream, unsigned version, uint32_t type )
	{
		auto inputCount = stream.Read<uint8_t>();
		signature.registers.resize( inputCount );
		for( int inputIx = 0; inputIx < inputCount; ++inputIx )
		{
			auto& element = signature.registers[inputIx];
			
			if (version > 9)
			{
				element.registerType = Tr2ShaderRegisterAL::RegisterType( stream.Read<uint8_t>() );
			}
			else
			{
				uint8_t oldRegisterType = stream.Read<uint8_t>();
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
			element.registerIndex = stream.Read<uint32_t>();

			//stream.Read( element.registerIndex );
			if( version > 12 )
			{
				stream.Read( element.arrayCount );
				element.registerSpace = stream.Read<uint8_t>();
			}
			else
			{
				element.arrayCount = 1;
				element.registerSpace = type;
			}
			bool dynamic = true;
			if( element.registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER )
			{
				if( type == Tr2RenderContextEnum::ShaderType::VERTEX_SHADER && element.registerIndex == Tr2Renderer::GetPerFrameVSStartRegister() )
				{
					dynamic = false;
				}
				if( type == Tr2RenderContextEnum::ShaderType::PIXEL_SHADER && element.registerIndex == Tr2Renderer::GetPerFramePSStartRegister() )
				{
					dynamic = false;
				}
			}
			element.dynamic = dynamic;
		}
	
		if( version > 12 )
		{
			stream.Read( inputCount );
			signature.samplers.resize( inputCount );
			for( int inputIx = 0; inputIx < inputCount; ++inputIx )
			{
				auto& element = signature.samplers[inputIx];
				stream.Read( element.registerIndex );
				element.registerSpace = stream.Read<uint8_t>();
				element.sampler.m_isComparisonFilter = stream.Read<uint8_t>();
				element.sampler.m_minFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );
				element.sampler.m_magFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );
				element.sampler.m_mipFilter = Tr2RenderContextEnum::TextureFilter( stream.Read<uint8_t>() );
				element.sampler.m_addressU = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );
				element.sampler.m_addressV = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );
				element.sampler.m_addressW = Tr2RenderContextEnum::TextureAddressMode( stream.Read<uint8_t>() );
				stream.Read( element.sampler.m_mipLODBias );
				element.sampler.m_maxAnisotropy = stream.Read<uint8_t>();
				element.sampler.m_comparisonFunc = Tr2RenderContextEnum::CompareFunc( stream.Read<uint8_t>() );

				uint8_t borderColor;
				stream.Read( borderColor );
				switch( borderColor )
				{
				case 1:
					element.sampler.m_borderColor[0] = 0;
					element.sampler.m_borderColor[1] = 0;
					element.sampler.m_borderColor[2] = 0;
					element.sampler.m_borderColor[3] = 1;
					break;
				case 2:
					element.sampler.m_borderColor[0] = 1;
					element.sampler.m_borderColor[1] = 1;
					element.sampler.m_borderColor[2] = 1;
					element.sampler.m_borderColor[3] = 1;
					break;
				default:
					element.sampler.m_borderColor[0] = 0;
					element.sampler.m_borderColor[1] = 0;
					element.sampler.m_borderColor[2] = 0;
					element.sampler.m_borderColor[3] = 0;
					break;
				}
				stream.Read( element.sampler.m_minLOD );
				stream.Read( element.sampler.m_maxLOD );
			}
		}
	}

	void ReadInput( Tr2EffectStageInput& input, EffectStream& stream, unsigned version, Tr2RenderContextEnum::ShaderType stage, Tr2RenderContext& renderContext )
	{
		uint32_t constantCount = stream.Read<uint32_t>();
		input.constants.resize( constantCount );
		for( unsigned constantIx = 0; constantIx < constantCount; ++constantIx )
		{
			input.constants[constantIx] = ReadConstant( stream, version );
		}

		unsigned constantValueSize = stream.Read<uint32_t>();

		input.m_constantValueSize = constantValueSize;
		if( constantValueSize > SHADER_CONSTANTS_MAX )
		{
			input.m_constantValueSize = SHADER_CONSTANTS_MAX;
		}
		if( version < 5 )
		{
			if( constantValueSize )
			{
				memcpy( input.constantValues, stream.ReadRaw( constantValueSize ), input.m_constantValueSize );
			}
		}
		else
		{
			memcpy( input.constantValues, stream.ReadStringOptional( constantValueSize ), input.m_constantValueSize );
		}

		uint8_t textureCount = SanityCheck( stream.Read<uint8_t>(), 64u );
		for( int textureIx = 0; textureIx < textureCount; ++textureIx )
		{
			uint8_t registerIndex = stream.Read<uint8_t>();
			input.resources[registerIndex] = ReadResource( stream, version );
		}

		uint8_t samplerCount = SanityCheck( stream.Read<uint8_t>(), 64 );
		for( int samplerIx = 0; samplerIx < samplerCount; ++samplerIx )
		{
			uint8_t registerIndex = stream.Read<uint8_t>();

			Tr2SamplerSetup samplerSetup;

			if( version >= 4 )
			{
				samplerSetup.name = stream.ReadString();
			}
			else
			{
				samplerSetup.name = nullptr;
			}

			Tr2SamplerDescription sampler = ReadSampler( stream );

			if( version < 4 )
			{
				stream.Read<bool>(); // isSRGBTexture
			}
			if( version > 12 )
			{
				bool isDynamic = true;
				isDynamic = stream.Read<bool>();
				if( !isDynamic )
				{
					samplerSetup.name = nullptr;
				}
			}

			samplerSetup.sampler.Create( sampler, renderContext.GetPrimaryRenderContext() );

			input.samplers[registerIndex] = samplerSetup;
		}

		if( version >= 3 )
		{
			uint8_t uavCount = SanityCheck( stream.Read<uint8_t>(), 64 );

			for( int uavIx = 0; uavIx < uavCount; ++uavIx )
			{
				uint8_t registerIndex = stream.Read<uint8_t>();

				Tr2EffectResource resource;
				resource.isSRGB = false;
				resource.name = stream.ReadString();
				resource.type = Tr2EffectResource::Type( stream.Read<uint8_t>() );

				if( version >= 13 )
				{
					stream.Read( resource.arrayElements );
				}
				else
				{
					resource.arrayElements = 1;
				}

				resource.isAutoregister = stream.Read<uint8_t>() != 0;

				input.uavs[registerIndex] = resource;
			}
			if( version >= 8 )
			{
				ReadAnnotations( input.annotation, stream );
			}
		}
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

	try
	{

		EffectStream stream( data, dataSize, stringTable, stringTableSize );

		uint8_t techniqueCount = 1;
		if( version > 6 )
		{
			techniqueCount = stream.Read<uint8_t>();
		}
		techniques.resize( techniqueCount );

		for( uint8_t technique = 0; technique < techniqueCount; ++technique )
		{
			if( version > 6 )
			{
				techniques[technique].name = BlueSharedString( stream.ReadString() );
			}
			else
			{
				techniques[technique].name = DEFAULT_TECHNIQUE;
			}
			techniques[technique].shaderTypeMask = 0;

			uint8_t passCount = SanityCheck( stream.Read<uint8_t>(), 64 );
			techniques[technique].passes.resize( passCount );
			for( int passIx = 0; passIx < passCount; ++passIx )
			{
				Tr2Pass& pass = techniques[technique].passes[passIx];
				pass.shaderTypeMask = 0;

				std::vector<Tr2RenderContextEnum::ShaderType> shaderTypes;
				std::vector<Tr2ShaderSignatureAL> signatures;

				for( unsigned stageIx = 0; stageIx != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIx )
				{
					pass.stageInputs[stageIx].m_exists = false;
					pass.stageInputs[stageIx].m_shader = Tr2EffectStageInput::INVALID;
					pass.stageInputs[stageIx].m_constantValueSize = 0;
				}

				uint8_t stageCount = SanityCheck( stream.Read<uint8_t>(), Tr2RenderContextEnum::SHADER_TYPE_COUNT );
				uint32_t shaderHandles[Tr2RenderContextEnum::SHADER_TYPE_COUNT];

				for( int stageIx = 0; stageIx < stageCount; ++stageIx )
				{
					Tr2RenderContextEnum::ShaderType type = Tr2RenderContextEnum::ShaderType( stream.Read<uint8_t>() );
					pass.shaderTypeMask |= 1u << type;
					pass.stageInputs[type].m_exists = true;

					if( version < 14 )
					{
						ReadPipelineInputs( pass.stageInputs[type].signature.pipelineInputs, stream, version );

						if( version > 8 )
						{
							ReadRegisters( pass.stageInputs[type].signature, stream, version, type );
						}
					}

					uint32_t shaderSize;
					const void* shaderCode;

					if( version < 5 )
					{
						stream.Read( shaderSize );
						shaderCode = stream.ReadRaw( shaderSize );

						stream.ReadRaw( stream.Read<uint32_t>() );  // shadow shader
					}
					else
					{
						stream.Read( shaderSize );
						shaderCode = stream.ReadString( shaderSize );


						if( version < 12 )
						{
							stream.Read<uint32_t>(); // shadow shader
							stream.Read<uint32_t>();
						}
					}

					pass.stageInputs[type].signature.threadGroupSize = Tr2ShaderThreadGroupSizeAL();
					if( version >= 3 )
					{
						stream.Read( pass.stageInputs[type].signature.threadGroupSize.x );
						stream.Read( pass.stageInputs[type].signature.threadGroupSize.y );
						stream.Read( pass.stageInputs[type].signature.threadGroupSize.z );
					}
					// CHECK
					if( version >= 14 ) 
					{
						ReadPipelineInputs( pass.stageInputs[type].signature.pipelineInputs, stream, version );
						ReadRegisters( pass.stageInputs[type].signature, stream, version, type );
					}

					ReadInput( pass.stageInputs[type], stream, version, type, renderContext );

					pass.stageInputs[type].m_shader = Tr2EffectStateManager::RegisterShader(
						type,
						Tr2ShaderBytecodeAL( shaderCode, shaderSize ),
						pass.stageInputs[type].signature,
						effectName );
					shaderHandles[stageIx] = pass.stageInputs[type].m_shader;

					if( pass.stageInputs[type].m_shader == unsigned( -1 ) )
					{
						CCP_LOGERR( "Error compiling %s shader in effect \"%s\".", type == Tr2RenderContextEnum::VERTEX_SHADER ? "vertex" : "fragment", effectName );
						techniques.clear();
						return false;
					}

					shaderTypes.push_back( type );
					signatures.push_back( pass.stageInputs[type].signature );

					for( auto& c : pass.stageInputs[type].constants )
					{
						if( c.type != Tr2EffectConstant::UINT || c.dimension != 1 )
						{
							continue;
						}
						auto found = find_if( pass.stageInputs[type].samplers.begin(), pass.stageInputs[type].samplers.end(), [&]( const auto& s ) { return s.second.name && strcmp( s.second.name, c.name.c_str() ) == 0; } );
						if( found != pass.stageInputs[type].samplers.end() )
						{
							if ( c.offset + c.size < pass.stageInputs[type].m_constantValueSize )
							{
								std::fill( pass.stageInputs[type].constantValues + pass.stageInputs[type].m_constantValueSize, pass.stageInputs[type].constantValues + c.offset + c.size, 0 );
							}
							reinterpret_cast<uint32_t*>( pass.stageInputs[type].constantValues + c.offset )[0] = found->second.sampler.GetIndexInHeap();
						}
					}
				}
				
				pass.resourceSetDesc = Tr2ResourceSetDescriptionAL( Tr2RegisterMapAL( shaderTypes.data(), signatures.data(), signatures.size() ) );
				for( uint32_t stageIx = 0; stageIx < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIx )
				{
					if( pass.stageInputs[stageIx].m_exists )
					{
						for( auto sampler = begin( pass.stageInputs[stageIx].samplers ); sampler != end( pass.stageInputs[stageIx].samplers ); ++sampler )
						{
							pass.resourceSetDesc.SetSampler( Tr2RenderContextEnum::ShaderType( stageIx ), sampler->first, sampler->second.sampler );
						}
					}
				}
				pass.shaderProgram = Tr2EffectStateManager::RegisterShaderProgram( shaderHandles, stageCount );
				if( pass.shaderProgram == unsigned( -1 ) )
				{
					CCP_LOGERR( "Error creating shader program in effect \"%s\".", effectName );
					techniques.clear();
					return false;
				}

				uint8_t stateCount = SanityCheck( stream.Read<uint8_t>(), 64 );

				Tr2EffectStateManager::Tr2RenderStateSetup states;
				for( int stateIx = 0; stateIx < stateCount; ++stateIx )
				{
					Tr2RenderContextEnum::RenderState state = Tr2RenderContextEnum::RenderState( stream.Read<uint32_t>() );
					states[state] = stream.Read<uint32_t>();
				}
				pass.renderStates = Tr2EffectStateManager::RegisterRenderStateSetup( states );

				techniques[technique].shaderTypeMask |= pass.shaderTypeMask;
			}
            // raytracing
			if( version > 13 )
			{
				uint8_t librariesCount = stream.Read<uint8_t>();
				techniques[technique].libraries.resize( librariesCount );
				for( uint8_t libIdx = 0; libIdx < librariesCount; ++libIdx )
				{
					auto& library = techniques[technique].libraries[libIdx];
					library.payloadSize = stream.Read<uint32_t>();
					auto bytecodeSize = stream.Read<uint32_t>();
					auto bytecode = stream.ReadString( bytecodeSize );

					library.libraryHandle = Tr2EffectStateManager::RegisterShaderLibrary( Tr2ShaderBytecodeAL( bytecode, bytecodeSize ) );

					auto exportCount = stream.Read<uint32_t>();
					for( uint32_t e = 0; e < exportCount; ++e )
					{
						auto exportType = stream.Read<uint8_t>();
						auto name = stream.ReadString();
						switch( exportType )
						{
						case 0:
							library.rayGenName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( name )) );
							break;
						case 1:
							library.missName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( name )) );
							break;
						case 2:
							library.closestHitName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( name )) );
							break;
						case 3:
							library.anyHitName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( name )) );
							break;
						case 4:
							library.intersectionName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( name )) );
							break;
						}
					}
					library.hitGroupName = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( stream.ReadString() )) );

					auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;
					ReadRegisters( library.globalInput.signature, stream, version, shaderType );
					ReadInput( library.globalInput, stream, version, shaderType, renderContext );
					library.globalResourceSetDesc = Tr2ResourceSetDescriptionAL( Tr2RegisterMapAL( &shaderType, &library.globalInput.signature, 1 ) );
					for( auto sampler = begin( library.globalInput.samplers ); sampler != end( library.globalInput.samplers ); ++sampler )
					{
						library.globalResourceSetDesc.SetSampler( shaderType, sampler->first, sampler->second.sampler );
					}

					ReadRegisters( library.localInput.signature, stream, version, shaderType );
					ReadInput( library.localInput, stream, version, shaderType, renderContext );
					library.localResourceSetDesc = Tr2ResourceSetDescriptionAL( Tr2RegisterMapAL( &shaderType, &library.localInput.signature, 1 ) );
					for( auto sampler = begin( library.localInput.samplers ); sampler != end( library.localInput.samplers ); ++sampler )
					{
						library.localResourceSetDesc.SetSampler( shaderType, sampler->first, sampler->second.sampler );
					}
				}
			}
		}

		uint16_t parameterCount = SanityCheck( stream.Read<uint16_t>(), 256 );
		for( int paramIx = 0; paramIx < parameterCount; ++paramIx )
		{
			const char* name = stream.ReadString();
			auto map = annotations.insert( std::make_pair( name, Tr2EffectParameterAnnotationMap( "Tr2EffectParameterAnnotationMap" ) ) );
			ReadAnnotations( map.first->second, stream );
		}
	}
	catch( const std::runtime_error& exc )
	{
		CCP_LOGERR( "%s in effect \"%s\".", exc.what(), effectName );
		techniques.clear();
		return false;

	}

	for( auto& technique : techniques )
	{
		for( auto& pass : technique.passes )
		{
			uint32_t type = 0;
			for( auto& stage : pass.stageInputs )
			{
				if (stage.m_exists)
				{
					auto IsHeapView = [&]( const char* name ) {
						if( !name )
						{
							return false;
						}
						auto it = std::find_if( annotations.begin(), annotations.end(), [&]( Tr2EffectAnnotationMap::const_reference key ) { return strcmp( key.first, name ) == 0; } );
						if( it != annotations.end() )
						{
							auto value = std::find_if( it->second.begin(), it->second.end(), [&]( const Tr2EffectParameterAnnotation& a ) { return strcmp( a.name, "IsHeapView" ) == 0; } );
							if( value != it->second.end() && value->type == Tr2EffectParameterAnnotation::BOOL )
							{
								return value->boolValue;
							}
						}
						return false;
					};
					for( auto& res : stage.resources )
					{
						auto isHeapView = IsHeapView( res.second.name );
						if( IsHeapView( res.second.name ) )
						{
							pass.resourceSetDesc.SetSrvHeapView( Tr2RenderContextEnum::ShaderType( type ), res.first );
						}
					}
					for( auto& res : stage.uavs )
					{
						auto isHeapView = IsHeapView( res.second.name );
						if( IsHeapView( res.second.name ) )
						{
							pass.resourceSetDesc.SetUavHeapView( Tr2RenderContextEnum::ShaderType( type ), res.first );
						}
					}
					for( auto& sampler : stage.samplers )
					{
						auto isHeapView = IsHeapView( sampler.second.name );
						if( IsHeapView( sampler.second.name ) )
						{
							pass.resourceSetDesc.SetSamplerHeapView( Tr2RenderContextEnum::ShaderType( type ), sampler.first );
						}
					}
				}
				++type;
			}
		}
	}
	return true;
}

