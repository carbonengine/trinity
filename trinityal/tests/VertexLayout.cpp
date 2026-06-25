// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

struct VertexLayout : public WithValidRenderContext
{
};

void InitializeSampleVertexDefinition( Tr2VertexDefinition& def )
{
	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	def.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );
}

TEST_F( VertexLayout, VertexLayoutIsInvalidBeforeCreation )
{
	Tr2VertexLayoutAL layout;
	EXPECT_FALSE( layout.IsValid() );
}

TEST_F( WithRenderContext, CreatingVertexLayoutWithoutRenderContextFails )
{
	Tr2VertexLayoutAL layout;
	Tr2VertexDefinition def;
	InitializeSampleVertexDefinition( def );
	ASSERT_HRESULT_FAILED( layout.Create( def, *renderContext ) );
}

TEST_F( VertexLayout, VertexLayoutIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2VertexLayoutAL layout;
	Tr2VertexDefinition def;
	InitializeSampleVertexDefinition( def );
	ASSERT_HRESULT_SUCCEEDED( layout.Create( def, *renderContext ) );
	EXPECT_TRUE( layout.IsValid() );
}

TEST_F( VertexLayout, VertexLayoutEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2VertexLayoutAL layout;
	Tr2VertexDefinition def;
	InitializeSampleVertexDefinition( def );
	ASSERT_HRESULT_SUCCEEDED( layout.Create( def, *renderContext ) );
	EXPECT_TRUE( layout == layout );
}

TEST_F( VertexLayout, DifferentVertexLayoutsAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2VertexLayoutAL layout1;
	Tr2VertexDefinition def;
	InitializeSampleVertexDefinition( def );
	ASSERT_HRESULT_SUCCEEDED( layout1.Create( def, *renderContext ) );

	Tr2VertexLayoutAL layout2;
	ASSERT_HRESULT_SUCCEEDED( layout2.Create( def, *renderContext ) );

	EXPECT_FALSE( layout1 == layout2 );
}

TEST_F( VertexLayout, VertexLayoutMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2VertexLayoutAL layout;
	Tr2VertexDefinition def;
	InitializeSampleVertexDefinition( def );
	ASSERT_HRESULT_SUCCEEDED( layout.Create( def, *renderContext ) );
	auto memoryClass = layout.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
