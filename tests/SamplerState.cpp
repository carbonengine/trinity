#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct SamplerState : public WithValidRenderContext
{
};


TEST_F( SamplerState, CanCreateSamplerState )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
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

TEST_F( SamplerState, SamplerStateEqualsItself )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
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

TEST_F( SamplerState, DifferentSamplerStatesAreNotEqual )
{
	float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
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

TEST_F( SamplerState, SamplerStateHasMemoryClass )
{
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
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
