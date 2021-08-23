#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct Shader : public WithValidRenderContext {};

TEST_F( Shader, ShaderIsInvalidBeforeCreation )
{
	Tr2ShaderAL s;
	EXPECT_FALSE( s.IsValid() );
}

TEST_F( Shader, CanCreateShader )
{
	uint8_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	auto vsInput = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		vsInput,
		*renderContext ) );

	EXPECT_TRUE( vs.IsValid() );
	EXPECT_EQ( VERTEX_SHADER, vs.GetType() );

	Tr2ShaderBytecodeAL bytecode;
	ASSERT_HRESULT_SUCCEEDED( vs.GetBytecode( bytecode ) );
	EXPECT_EQ( sizeof( vsBytecode ), bytecode.size );
	EXPECT_EQ( 0, memcmp( bytecode.bytecode, vsBytecode, bytecode.size ) );
}

TEST_F( Shader, ShaderEqualsItself )
{
	uint8_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	auto vsInput = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		vsInput,
		*renderContext ) );

	EXPECT_TRUE( vs == vs );
}

TEST_F( Shader, DifferentShadersAreNotEqual )
{
	uint8_t vsBytecode1[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	auto vsInput = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 );

	Tr2ShaderAL vs1;
	ASSERT_HRESULT_SUCCEEDED( vs1.Create( 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode1,
		vsInput,
		*renderContext ) );

	uint8_t vsBytecode2[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	auto vsInput2 = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 ).Add( Tr2ShaderRegisterAL::CONSTANT_BUFFER, 0 );
	Tr2ShaderAL vs2;
	ASSERT_HRESULT_SUCCEEDED( vs2.Create( 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode2,
		vsInput2,
		*renderContext ) );

	EXPECT_FALSE( vs1 == vs2 );
}

#if( TRINITYPLATFORM == TRINITY_STUB )
TEST_F( Shader, ShaderStoresType )
{
	uint8_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	auto vsInput = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		Tr2RenderContextEnum::HULL_SHADER,
		vsBytecode,
		vsInput,
		*renderContext ) );

	Tr2RenderContextEnum::ShaderType expected = Tr2RenderContextEnum::HULL_SHADER;
	int actual = vs.GetType();
	EXPECT_EQ( expected, actual  );
}
#endif
