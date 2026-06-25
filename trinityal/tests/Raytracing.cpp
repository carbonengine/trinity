// Copyright © 2024 CCP ehf.

#include "StdAfx.h"

#include "WithValidRenderContextFixture.h"

#include "../metal/Tr2RtTopLevelAccelerationStructureALMetal.h"

#if TRINITY_PLATFORM_SUPPORTS_RAY_TRACING

struct Raytracing : public WithValidRenderContext
{
};
namespace
{
struct Vector3
{
	float x, y, z;

	Vector3()
	{
	}

	Vector3( float x_, float y_, float z_ ) :
		x( x_ ),
		y( y_ ),
		z( z_ )
	{
	}
};

struct Vector4
{
	float x, y, z, w;
};

Vector3 cubeVertices[] = {
	Vector3( -1, -1, 1 ),
	Vector3( 1, -1, 1 ),
	Vector3( 1, 1, 1 ),
	Vector3( -1, 1, 1 ),
	Vector3( -1, -1, -1 ),
	Vector3( 1, -1, -1 ),
	Vector3( 1, 1, -1 ),
	Vector3( -1, 1, -1 ),
};

uint16_t cubeIndices[] = {
	0,
	1,
	2,
	2,
	3,
	0,
	1,
	5,
	6,
	6,
	2,
	1,
	7,
	6,
	5,
	5,
	4,
	7,
	4,
	0,
	3,
	3,
	7,
	4,
	4,
	5,
	1,
	1,
	0,
	4,
	3,
	2,
	6,
	6,
	7,
	3
};
}

TEST_F( Raytracing, BLASIsInvalidBeforeCreation )
{
	Tr2RtBottomLevelAccelerationStructureAL blas;
	EXPECT_FALSE( blas.IsValid() );
}

TEST_F( Raytracing, BLASIsValidAfterCreation )
{
	Tr2BufferAL vb, ib;
	// UNSURE ABOUT CPUUSAGE AND HOW TO NAVIGATE THAT ONE, need to have leave it like this for now because of possible metal bug w. buffers
#if TRINITY_PLATFORM == TRINITY_METAL
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeIndices, *renderContext ) );

#else
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );
#endif

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	ASSERT_TRUE( blas.IsValid() );
}

TEST_F( Raytracing, BLASIsValidAfterUpdate )
{
	Tr2BufferAL vb, ib;
	// UNSURE ABOUT CPUUSAGE AND HOW TO NAVIGATE THAT ONE, need to have leave it like this for now because of possible metal bug w. buffers
#if TRINITY_PLATFORM == TRINITY_METAL
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeIndices, *renderContext ) );

#else
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );
#endif

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::ALLOW_UPDATE, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( blas.Update( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, *renderContext ) );

	ASSERT_TRUE( blas.IsValid() );
}


TEST_F( Raytracing, TLASIsInvalidBeforeCreation )
{
	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_FALSE( tlas.IsValid() );
}

TEST_F( Raytracing, TLASIsValidAfterCreation )
{
	Tr2BufferAL vb, ib;
	// UNSURE ABOUT CPUUSAGE AND HOW TO NAVIGATE THAT ONE, need to have leave it like this for now because of possible metal bug w. buffers
#if TRINITY_PLATFORM == TRINITY_METAL
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeIndices, *renderContext ) );

#else
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );
#endif

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	Tr2RtInstanceAL instance;
	instance.blas = blas.TrinityALImpl_GetObject();
	memset( instance.transform, 0, sizeof( instance.transform ) );
	instance.transform[0][0] = 1;
	instance.transform[1][1] = 1;
	instance.transform[2][2] = 1;
	instance.materialIndex = 0;

	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_HRESULT_SUCCEEDED( tlas.Create( 1, &instance, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	ASSERT_TRUE( tlas.IsValid() );
}

TEST_F( Raytracing, CanCreateStateObject )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};

	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};

	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHit.rs )
	};

	Tr2ShaderSignatureAL signature;
	signature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 );
	signature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 );
	signature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 );

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", 4 * sizeof( float ) );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", 4 * sizeof( float ) );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", 4 * sizeof( float ) );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr );
	stateDesc.AddGlobalSignature( signature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );
}

TEST_F( Raytracing, CanCreateShaderTable )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};

	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};

	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHit.rs )
	};

	Tr2ShaderSignatureAL signature;
	signature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 );
	signature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 );
	signature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 );

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", 4 * sizeof( float ) );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", 4 * sizeof( float ) );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", 4 * sizeof( float ) );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr );
	stateDesc.AddGlobalSignature( signature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );

	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"RayGen_12" );
	shaderTableDesc.AddMissShader( L"Miss_5" );
	shaderTableDesc.AddHitGroup( L"HitGroup" );

	Tr2RtShaderTableAL shaderTable;
	ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );
}

TEST_F( Raytracing, ShaderTableCreationFailsWithInvalidShaderName )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};

	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};

	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHit.rs )
	};

	Tr2ShaderSignatureAL signature;
	signature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 );
	signature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 );
	signature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 );

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", 4 * sizeof( float ) );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", 4 * sizeof( float ) );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", 4 * sizeof( float ) );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr );
	stateDesc.AddGlobalSignature( signature );


	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );

	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"Blah" );

	Tr2RtShaderTableAL shaderTable;

	// metal behaves differently because past the pipeline state we don't really care about the raygen shader
#if TRINITY_PLATFORM == TRINITY_METAL
	ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );
#else
	ASSERT_HRESULT_FAILED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );
#endif
}

/************* renderer *************/

struct QuadRenderer
{
	ALResult Create( const Tr2TextureAL& texture, Tr2PrimaryRenderContextAL* renderContext )
	{
		Tr2ShaderAL vs;
		CR_RETURN_HR( CreateTexCoordAndPositionVS( vs, *renderContext ) );
		Tr2ShaderAL ps;
		CR_RETURN_HR( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

		Tr2ShaderAL shaders2[] = { vs, ps };
		CR_RETURN_HR( m_shaderProgram.Create( shaders2, 2, *renderContext ) );


		Tr2VertexDefinition definition;
		definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
		definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

		CR_RETURN_HR( m_vertexLayout.Create( definition, *renderContext ) );

		const float hw = 0.8f;

		float quad[] = {
			-hw,
			-hw,
			0,
			0,
			1,
			-hw,
			hw,
			0,
			0,
			0,
			hw,
			-hw,
			0,
			1,
			1,

			hw,
			-hw,
			0,
			1,
			1,
			-hw,
			hw,
			0,
			0,
			0,
			hw,
			hw,
			0,
			1,
			0,
		};
		CR_RETURN_HR( m_quadVb.Create( VB_STRIDE, sizeof( quad ) / VB_STRIDE, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, quad, *renderContext ) );

		Tr2SamplerStateAL sampl;
		CR_RETURN_HR( sampl.Create(
			Tr2SamplerDescription(
				Tr2RenderContextEnum::TF_POINT,
				Tr2RenderContextEnum::TA_WRAP,
				1,
				0.0f,
				0.0f ),
			*renderContext ) );

		Tr2ResourceSetDescriptionAL resourceSetDescription( m_shaderProgram );
		resourceSetDescription.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, texture );
		resourceSetDescription.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

		CR_RETURN_HR( m_resourceSet.Create( resourceSetDescription, m_shaderProgram, *renderContext ) );
		return S_OK;
	}

	ALResult Render( Tr2RenderContextAL* renderContext )
	{
		CR_RETURN_HR( renderContext->SetVertexLayout( m_vertexLayout ) );
		CR_RETURN_HR( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		CR_RETURN_HR( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		CR_RETURN_HR( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		CR_RETURN_HR( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		CR_RETURN_HR( renderContext->SetStreamSource( 0, m_quadVb, 0, VB_STRIDE ) );
		CR_RETURN_HR( renderContext->SetShaderProgram( m_shaderProgram ) );
		CR_RETURN_HR( renderContext->SetResourceSet( m_resourceSet ) );
		CR_RETURN_HR( renderContext->DrawPrimitive( 0, 2 ) );
		return S_OK;
	}

	ALResult CreateTexCoordAndPositionVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
	{
		uint8_t bytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
		};

		Tr2ShaderSignatureAL signature;
		signature.Add( Tr2VertexDefinition::TEXCOORD, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 2 );
		signature.Add( Tr2VertexDefinition::POSITION, 0, 1, Tr2ShaderPipelineInputAL::FLOAT, 3 );

		return shader.Create( Tr2RenderContextEnum::VERTEX_SHADER, bytecode, signature, "", renderContext );
	}

	ALResult CreateSampleTextureFromTexCoordPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
	{
		uint8_t bytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
		};

		Tr2ShaderSignatureAL signature;
		signature.Add( Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 );
		signature.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );

		return shader.Create( Tr2RenderContextEnum::PIXEL_SHADER, bytecode, signature, "", renderContext );
	}

	Tr2ShaderProgramAL m_shaderProgram;
	Tr2ResourceSetAL m_resourceSet;
	Tr2BufferAL m_quadVb;
	Tr2VertexLayoutAL m_vertexLayout;
	static const uint32_t VB_STRIDE = 5 * sizeof( float );
};


struct RayGenData
{
	float viewMatrix[4][4];
	float viewOrigin[3];
	float tanHalfFOV;
	float width;
	float height;
};


void Rotate( float* vector, float angle )
{
	float sa = sin( angle );
	float ca = cos( angle );

	float x = vector[0];
	float z = vector[2];
	vector[0] = x * ca - z * sa;
	vector[2] = x * sa + z * ca;
}

void Transpose( float viewMatrix[4][4] )
{
	std::swap( viewMatrix[0][1], viewMatrix[1][0] );
	std::swap( viewMatrix[0][2], viewMatrix[2][0] );
	std::swap( viewMatrix[0][3], viewMatrix[3][0] );

	std::swap( viewMatrix[1][2], viewMatrix[2][1] );
	std::swap( viewMatrix[1][3], viewMatrix[3][1] );

	std::swap( viewMatrix[2][3], viewMatrix[3][2] );
}

/************* render test *************/

TEST_F( Raytracing, TraceRays )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};
	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};
	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHit.rs )
	};

	const uint32_t PAYLOAD_SIZE = 4 * sizeof( float );

	Tr2ShaderSignatureAL signature;
	signature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 ); //uniforms
	signature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 ); // RTOutput
	signature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 ); // accelStruct, set as 1 because of metal compute buffer binding (4+1)

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", PAYLOAD_SIZE );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", PAYLOAD_SIZE );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", PAYLOAD_SIZE );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr );
	stateDesc.AddGlobalSignature( signature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );

	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"RayGen_12" );
	shaderTableDesc.AddMissShader( L"Miss_5" );
	shaderTableDesc.AddHitGroup( L"HitGroup" );

	Tr2RtShaderTableAL shaderTable;
	ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );

	const uint32_t WIDTH = 512;
	const uint32_t HEIGHT = 512;

	Tr2ConstantBufferAL cb;
	cb.Create( 6 * 4 * sizeof( float ), *renderContext );

	// create output texture for shader
	Tr2TextureAL resultTex;
	ASSERT_HRESULT_SUCCEEDED( resultTex.Create( Tr2BitmapDimensions( WIDTH, HEIGHT, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );

	Tr2BufferAL vb, ib;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	Tr2RtInstanceAL instance;
	instance.blas = blas.TrinityALImpl_GetObject();
	memset( instance.transform, 0, sizeof( instance.transform ) );
	instance.transform[0][0] = 1;
	instance.transform[1][1] = 1;
	instance.transform[2][2] = 1;
	instance.materialIndex = 0;

	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_HRESULT_SUCCEEDED( tlas.Create( 1, &instance, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;
	Tr2RegisterMapAL registerMap = Tr2RegisterMapAL( &shaderType, &signature, 1 );

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing
	Tr2ResourceSetDescriptionAL rsDesc( registerMap );
	rsDesc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 1, tlas.GetBuffer() ); // accelerationStructure
	rsDesc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, resultTex ); // RTOutput

	Tr2ResourceSetAL rs;
	rs.Create( rsDesc, state, *renderContext );

	QuadRenderer quadRenderer;
	ASSERT_HRESULT_SUCCEEDED( quadRenderer.Create( resultTex, renderContext ) );

	uint32_t g = 127;
	uint32_t offset = 4;
	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );

		float angle = float( g - 127 ) * 0.01f;

		RayGenData* cbData;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&cbData, *renderContext ) );
		memset( cbData, 0, cb.GetSize() );
		cbData->viewMatrix[0][0] = 1;
		cbData->viewMatrix[1][1] = 1;
		cbData->viewMatrix[2][2] = 1;
		cbData->viewMatrix[3][3] = 1;
		cbData->viewOrigin[2] = -3;
		Rotate( cbData->viewMatrix[0], angle );
		Rotate( cbData->viewMatrix[2], angle );
		Rotate( cbData->viewOrigin, angle );
		Transpose( cbData->viewMatrix );
		cbData->tanHalfFOV = 1;
		cbData->width = float( WIDTH );
		cbData->height = float( HEIGHT );
		cb.Unlock( *renderContext );

		float clearColor[] = { 0, 0, float( g & 0xff ) / 255.f, 0 };
		ASSERT_HRESULT_SUCCEEDED( renderContext->ClearUav( resultTex, 0, clearColor ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::COMPUTE_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( rs ) );

		// mac specific
		renderContext->UseAccelerationStructure( tlas );

		ASSERT_HRESULT_SUCCEEDED( renderContext->DispatchRays( state, shaderTable, L"RayGen_12", WIDTH, HEIGHT, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( quadRenderer.Render( renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );
}

TEST_F( Raytracing, CanUpdateBlas )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};
	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};
	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHit.rs )
	};

	const uint32_t PAYLOAD_SIZE = 4 * sizeof( float );

	Tr2ShaderSignatureAL signature;
	signature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 ); //uniforms
	signature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 ); // RTOutput
	signature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 ); // accelStruct, set as 1 because of metal compute buffer binding (4+1)

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", PAYLOAD_SIZE );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", PAYLOAD_SIZE );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", PAYLOAD_SIZE );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr );
	stateDesc.AddGlobalSignature( signature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );

	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"RayGen_12" );
	shaderTableDesc.AddMissShader( L"Miss_5" );
	shaderTableDesc.AddHitGroup( L"HitGroup" );

	Tr2RtShaderTableAL shaderTable;
	ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );

	const uint32_t WIDTH = 512;
	const uint32_t HEIGHT = 512;

	Tr2ConstantBufferAL cb;
	cb.Create( 6 * 4 * sizeof( float ), *renderContext );

	// create output texture for shader
	Tr2TextureAL resultTex;
	ASSERT_HRESULT_SUCCEEDED( resultTex.Create( Tr2BitmapDimensions( WIDTH, HEIGHT, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );

	Tr2BufferAL vb, ib;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::ALLOW_UPDATE, *renderContext ) );

	Tr2RtInstanceAL instance;
	instance.blas = blas.TrinityALImpl_GetObject();
	memset( instance.transform, 0, sizeof( instance.transform ) );
	instance.transform[0][0] = 1;
	instance.transform[1][1] = 1;
	instance.transform[2][2] = 1;
	instance.materialIndex = 0;

	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_HRESULT_SUCCEEDED( tlas.Create( 1, &instance, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;
	Tr2RegisterMapAL registerMap = Tr2RegisterMapAL( &shaderType, &signature, 1 );

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing
	Tr2ResourceSetDescriptionAL rsDesc( registerMap );
	rsDesc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 1, tlas.GetBuffer() ); // accelerationStructure
	rsDesc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, resultTex ); // RTOutput

	Tr2ResourceSetAL rs;
	rs.Create( rsDesc, state, *renderContext );

	QuadRenderer quadRenderer;
	ASSERT_HRESULT_SUCCEEDED( quadRenderer.Create( resultTex, renderContext ) );

	uint32_t g = 127;
	uint32_t offset = 4;
	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		float angle = 0.2f; //float( g - 127 ) * 0.01f;

		RayGenData* cbData;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&cbData, *renderContext ) );
		memset( cbData, 0, cb.GetSize() );
		cbData->viewMatrix[0][0] = 1;
		cbData->viewMatrix[1][1] = 1;
		cbData->viewMatrix[2][2] = 1;
		cbData->viewMatrix[3][3] = 1;
		cbData->viewOrigin[2] = -3;
		Rotate( cbData->viewMatrix[0], angle );
		Rotate( cbData->viewMatrix[2], angle );
		Rotate( cbData->viewOrigin, angle );
		Transpose( cbData->viewMatrix );
		cbData->tanHalfFOV = 1;
		cbData->width = float( WIDTH );
		cbData->height = float( HEIGHT );
		cb.Unlock( *renderContext );

		float dx = sin( g / 100.f ) * 0.1f;
		float dy = cos( g / 100.f ) * 0.1f;
		Vector3 animatedCube[] = {
			Vector3( -1, -1, 1 ),
			Vector3( 1, -1, 1 ),
			Vector3( 1 + dx, 1, 1 + dy ),
			Vector3( -1 + dx, 1, 1 + dy ),
			Vector3( -1, -1, -1 ),
			Vector3( 1, -1, -1 ),
			Vector3( 1 + dx, 1, -1 + dy ),
			Vector3( -1 + dx, 1, -1 + dy ),
		};

		ASSERT_HRESULT_SUCCEEDED( vb.UpdateBuffer( 0, sizeof( animatedCube ), animatedCube, *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( blas.Update( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, *renderContext ) );

		float clearColor[] = { 0, 0, float( g & 0xff ) / 255.f, 0 };
		ASSERT_HRESULT_SUCCEEDED( renderContext->ClearUav( resultTex, 0, clearColor ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::COMPUTE_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( rs ) );

		// mac specific
		renderContext->UseAccelerationStructure( tlas );

		ASSERT_HRESULT_SUCCEEDED( renderContext->DispatchRays( state, shaderTable, L"RayGen_12", WIDTH, HEIGHT, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( quadRenderer.Render( renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );
}

TEST_F( Raytracing, CanUseLocalConstants )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};

	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};

	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHitWithMaterial.rs )
	};

	Tr2ShaderSignatureAL globalSignature;
	globalSignature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 ); //uniforms
	globalSignature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 ); //RTOutput
	globalSignature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 ); //AccelStruct

	Tr2ShaderSignatureAL localSignature;
	localSignature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 1 );

	const uint32_t PAYLOAD_SIZE = 4 * sizeof( float );

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", PAYLOAD_SIZE );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", PAYLOAD_SIZE );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHit", PAYLOAD_SIZE );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr, localSignature );
	stateDesc.AddGlobalSignature( globalSignature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, renderContext->GetPrimaryRenderContext() ) );

	float* materialData;

	Tr2ConstantBufferAL cb1;
	cb1.Create( 4 * 4, *renderContext );
	cb1.Lock( (void**)&materialData, *renderContext );
	materialData[0] = 1;
	materialData[1] = 0;
	materialData[2] = 1;
	materialData[3] = 1;
	cb1.Unlock( *renderContext );

	Tr2ConstantBufferAL cb2;
	cb2.Create( 4 * 4, *renderContext );
	cb2.Lock( (void**)&materialData, *renderContext );
	materialData[0] = 1;
	materialData[1] = 1;
	materialData[2] = 0;
	materialData[3] = 1;
	cb2.Unlock( *renderContext );


	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"RayGen_12" );
	shaderTableDesc.AddMissShader( L"Miss_5" );
	shaderTableDesc.AddHitGroup( L"HitGroup", Tr2RtLocalMaterialDescriptionAL().SetConstants( 1, cb1 ) );
	shaderTableDesc.AddHitGroup( L"HitGroup", Tr2RtLocalMaterialDescriptionAL().SetConstants( 1, cb2 ) );

	const uint32_t WIDTH = 512;
	const uint32_t HEIGHT = 512;

	Tr2ConstantBufferAL cb;
	cb.Create( 6 * 4 * sizeof( float ), *renderContext );

	Tr2TextureAL result;
	result.Create( Tr2BitmapDimensions( WIDTH, HEIGHT, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, *renderContext );

	Tr2BufferAL vb, ib;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	Tr2RtInstanceAL instances[2];
	instances[0].blas = blas.TrinityALImpl_GetObject();
	memset( instances[0].transform, 0, sizeof( instances[0].transform ) );
	instances[0].transform[0][0] = 0.5f;
	instances[0].transform[1][1] = 0.5f;
	instances[0].transform[2][2] = 0.5f;
	instances[0].transform[0][3] = -0.6f;
	instances[0].materialIndex = 0;

	instances[1].blas = blas.TrinityALImpl_GetObject();
	memset( instances[1].transform, 0, sizeof( instances[1].transform ) );
	instances[1].transform[0][0] = 0.5f;
	instances[1].transform[1][1] = 0.5f;
	instances[1].transform[2][2] = 0.5f;
	instances[1].transform[0][3] = 0.6f;
	instances[1].materialIndex = 1;


	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_HRESULT_SUCCEEDED( tlas.Create( 2, instances, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;
	Tr2RegisterMapAL registerMap = Tr2RegisterMapAL( &shaderType, &globalSignature, 1 );

	Tr2ResourceSetDescriptionAL rsDesc( registerMap );
	rsDesc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 1, tlas.GetBuffer() );
	rsDesc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, result );

	Tr2ResourceSetAL rs;
	rs.Create( rsDesc, state, *renderContext );

	QuadRenderer quadRenderer;
	ASSERT_HRESULT_SUCCEEDED( quadRenderer.Create( result, renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );

		Tr2RtShaderTableAL shaderTable;
		ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );

		float angle = float( g - 127 ) * 0.01f;

		RayGenData* cbData;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&cbData, *renderContext ) );
		memset( cbData, 0, cb.GetSize() );
		cbData->viewMatrix[0][0] = 1;
		cbData->viewMatrix[1][1] = 1;
		cbData->viewMatrix[2][2] = 1;
		cbData->viewMatrix[3][3] = 1;
		cbData->viewOrigin[2] = -3;
		Rotate( cbData->viewMatrix[0], angle );
		Rotate( cbData->viewMatrix[2], angle );
		Rotate( cbData->viewOrigin, angle );
		Transpose( cbData->viewMatrix );
		cbData->tanHalfFOV = 1;
		cbData->width = float( WIDTH );
		cbData->height = float( HEIGHT );
		cb.Unlock( *renderContext );

		float clearColor[] = { 0, float( g & 0xff ) / 255.f, 0, 0 };
		ASSERT_HRESULT_SUCCEEDED( renderContext->ClearUav( result, 0, clearColor ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::COMPUTE_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( rs ) );

		renderContext->UseAccelerationStructure( tlas );

		ASSERT_HRESULT_SUCCEEDED( renderContext->DispatchRays( state, shaderTable, L"RayGen_12", WIDTH, HEIGHT, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( quadRenderer.Render( renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );
}

#if TRINITY_PLATFORM == TRINITY_METAL

struct PerObjectData
{
	Vector4 material;
	Vector4 clipData;
};


TEST_F( Raytracing, CanUsePerObjectData )
{
	uint8_t rayGenCode[] = {
#include INCLUDE_SHADER_CODE( RayGen.rs )
	};

	uint8_t missCode[] = {
#include INCLUDE_SHADER_CODE( Miss.rs )
	};

	uint8_t closestHitCode[] = {
#include INCLUDE_SHADER_CODE( ClosestHitWithMaterial.rs )
	};

	Tr2ShaderSignatureAL globalSignature;
	globalSignature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 ); //uniforms
	globalSignature.Add( Tr2ShaderRegisterAL::UAV_TEXTURE2D, 0 ); //RTOutput
	globalSignature.Add( Tr2ShaderRegisterAL::SRV_BUFFER, 1 ); //AccelStruct

	Tr2ShaderSignatureAL localSignature;
	localSignature.Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 1 );

	const uint32_t PAYLOAD_SIZE = 4 * sizeof( float );

	Tr2RtPipelineStateDescriptionAL stateDesc;
	stateDesc.AddShader( L"RayGen_12", Tr2ShaderBytecodeAL( rayGenCode ), L"RayGen", PAYLOAD_SIZE );
	stateDesc.AddShader( L"Miss_5", Tr2ShaderBytecodeAL( missCode ), L"Miss", PAYLOAD_SIZE );
	stateDesc.AddShader( L"ClosestHit_76", Tr2ShaderBytecodeAL( closestHitCode ), L"ClosestHitWithPerObjData", PAYLOAD_SIZE );
	stateDesc.AddHitGroup( L"HitGroup", nullptr, L"ClosestHit_76", nullptr, localSignature );
	stateDesc.AddGlobalSignature( globalSignature );

	Tr2RtPipelineStateAL state;
	ASSERT_HRESULT_SUCCEEDED( state.CreateRtPipelineState( stateDesc, *renderContext ) );

	PerObjectData* materialData;

	Tr2ConstantBufferAL cb1;
	cb1.Create( sizeof( PerObjectData ), *renderContext );
	cb1.Lock( (void**)&materialData, *renderContext );
	materialData->material.x = 1;
	materialData->material.y = 0;
	materialData->material.z = 1;
	materialData->material.w = 1;

	materialData->clipData.x = 1;
	materialData->clipData.y = 1;
	materialData->clipData.z = 1;
	materialData->clipData.w = 1;
	cb1.Unlock( *renderContext );

	Tr2ConstantBufferAL cb2;
	cb2.Create( sizeof( PerObjectData ), *renderContext );
	cb2.Lock( (void**)&materialData, *renderContext );
	materialData->material.x = 1;
	materialData->material.y = 1;
	materialData->material.z = 0;
	materialData->material.w = 1;

	materialData->clipData.x = 0;
	materialData->clipData.y = 0;
	materialData->clipData.z = 0;
	materialData->clipData.w = 1;
	cb2.Unlock( *renderContext );

	Tr2RtShaderTableDescriptionAL shaderTableDesc;
	shaderTableDesc.AddRayGenShader( L"RayGen_12" );
	shaderTableDesc.AddMissShader( L"Miss_5" );
	shaderTableDesc.AddHitGroup( L"HitGroup", Tr2RtLocalMaterialDescriptionAL().SetConstants( 1, cb1 ) );
	shaderTableDesc.AddHitGroup( L"HitGroup", Tr2RtLocalMaterialDescriptionAL().SetConstants( 1, cb2 ) );

	const uint32_t WIDTH = 512;
	const uint32_t HEIGHT = 512;

	Tr2ConstantBufferAL cb;
	cb.Create( 6 * 4 * sizeof( float ), *renderContext );

	Tr2TextureAL result;
	result.Create( Tr2BitmapDimensions( WIDTH, HEIGHT, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, *renderContext );

	Tr2BufferAL vb, ib;
	// UNSURE ABOUT CPUUSAGE AND HOW TO NAVIGATE THAT ONE, need to have leave it like this for now because of possible metal bug w. buffers
#if TRINITY_PLATFORM == TRINITY_METAL
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN, cubeIndices, *renderContext ) );

#else
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeVertices, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( cubeIndices ) / sizeof( cubeIndices[0] ), Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, cubeIndices, *renderContext ) );
#endif

	Tr2RtBottomLevelAccelerationStructureAL blas;
	ASSERT_HRESULT_SUCCEEDED( blas.Create( { Tr2RtPositionStreamAL( vb ), Tr2RtIndicesStreamAL( ib ) }, Tr2RtBlasGeometryFlags::OPAQUE_GEOMETRY, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	Tr2RtInstanceAL instances[2];
	instances[0].blas = blas;
	memset( instances[0].transform, 0, sizeof( instances[0].transform ) );
	instances[0].transform[0][0] = 0.5f;
	instances[0].transform[1][1] = 0.5f;
	instances[0].transform[2][2] = 0.5f;
	instances[0].transform[0][3] = -0.6f;
	instances[0].materialIndex = 0;

	instances[1].blas = blas;
	memset( instances[1].transform, 0, sizeof( instances[1].transform ) );
	instances[1].transform[0][0] = 0.5f;
	instances[1].transform[1][1] = 0.5f;
	instances[1].transform[2][2] = 0.5f;
	instances[1].transform[0][3] = 0.6f;
	instances[1].materialIndex = 1;


	Tr2RtTopLevelAccelerationStructureAL tlas;
	ASSERT_HRESULT_SUCCEEDED( tlas.Create( 2, instances, Tr2RtBuildFlags::PREFER_FAST_TRACE, *renderContext ) );

	auto shaderType = Tr2RenderContextEnum::COMPUTE_SHADER;
	Tr2RegisterMapAL registerMap = Tr2RegisterMapAL( &shaderType, &globalSignature, 1 );

	Tr2ResourceSetDescriptionAL rsDesc( registerMap );
	rsDesc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 1, tlas.GetBuffer() );
	rsDesc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, result );

	Tr2ResourceSetAL rs;
	rs.Create( rsDesc, state, *renderContext );

	QuadRenderer quadRenderer;
	ASSERT_HRESULT_SUCCEEDED( quadRenderer.Create( result, renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );

		Tr2RtShaderTableAL shaderTable;
		ASSERT_HRESULT_SUCCEEDED( shaderTable.Create( shaderTableDesc, state, *renderContext ) );

		float angle = float( g - 127 ) * 0.01f;

		RayGenData* cbData;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&cbData, *renderContext ) );
		memset( cbData, 0, cb.GetSize() );
		cbData->viewMatrix[0][0] = 1;
		cbData->viewMatrix[1][1] = 1;
		cbData->viewMatrix[2][2] = 1;
		cbData->viewMatrix[3][3] = 1;
		cbData->viewOrigin[2] = -3;
		//Rotate( cbData->viewMatrix[0], angle );
		//Rotate( cbData->viewMatrix[2], angle );
		//Rotate( cbData->viewOrigin, angle );
		Transpose( cbData->viewMatrix );
		cbData->tanHalfFOV = 1;
		cbData->width = float( WIDTH );
		cbData->height = float( HEIGHT );
		cb.Unlock( *renderContext );

		float clearColor[] = { 0, float( g & 0xff ) / 255.f, 0, 0 };
		ASSERT_HRESULT_SUCCEEDED( renderContext->ClearUav( result, 0, clearColor ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::COMPUTE_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( rs ) );

		renderContext->UseAccelerationStructure( tlas );

		ASSERT_HRESULT_SUCCEEDED( renderContext->DispatchRays( state, shaderTable, L"RayGen_12", WIDTH, HEIGHT, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( quadRenderer.Render( renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );
}


#endif

#endif
