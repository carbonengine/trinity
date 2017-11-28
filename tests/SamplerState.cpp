#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, CanCreateSamplerState )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc(
		TF_ANISOTROPIC,
		TF_LINEAR,
		TF_POINT,
		false,
		TA_WRAP,
		TA_CLAMP,
		TA_BORDER,
		0.5f,
		4,
		CMP_ALWAYS,
		borderColor,
		0.1f,
		3.2f );

	ASSERT_HRESULT_SUCCEEDED( ss.Create( desc, *renderContext ) );
	EXPECT_TRUE( ss.IsValid() );
}

TEST_F( WithValidRenderContext, SamplerStateEqualsItself )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc(
		TF_ANISOTROPIC,
		TF_LINEAR,
		TF_POINT,
		false,
		TA_WRAP,
		TA_CLAMP,
		TA_BORDER,
		0.5f,
		4,
		CMP_ALWAYS,
		borderColor,
		0.1f,
		3.2f );

	ASSERT_HRESULT_SUCCEEDED( ss.Create( desc, *renderContext ) );
	EXPECT_TRUE( ss == ss );
}

TEST_F( WithValidRenderContext, DifferentSamplerStatesAreNotEqual )
{
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc1(
		TF_ANISOTROPIC,
		TF_LINEAR,
		TF_POINT,
		false,
		TA_WRAP,
		TA_CLAMP,
		TA_BORDER,
		0.5f,
		4,
		CMP_ALWAYS,
		borderColor,
		0.1f,
		3.2f );
	Tr2SamplerStateAL ss1;
	ASSERT_HRESULT_SUCCEEDED( ss1.Create( desc1, *renderContext ) );

	Tr2SamplerDescription desc2(
		TF_LINEAR,
		TF_LINEAR,
		TF_LINEAR,
		false,
		TA_WRAP,
		TA_CLAMP,
		TA_BORDER,
		0.5f,
		4,
		CMP_ALWAYS,
		borderColor,
		0.1f,
		3.2f );
	Tr2SamplerStateAL ss2;
	ASSERT_HRESULT_SUCCEEDED( ss2.Create( desc2, *renderContext ) );

	EXPECT_FALSE( ss1 == ss2 );
}

TEST_F( WithValidRenderContext, SamplerStateHasMemoryClass )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc(
		TF_ANISOTROPIC,
		TF_LINEAR,
		TF_POINT,
		false,
		TA_WRAP,
		TA_CLAMP,
		TA_BORDER,
		0.5f,
		4,
		CMP_ALWAYS,
		borderColor,
		0.1f,
		3.2f );
	ASSERT_HRESULT_SUCCEEDED( ss.Create( desc, *renderContext ) );

	auto memoryClass = ss.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
