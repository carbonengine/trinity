// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct Texture : public WithValidRenderContext
{
};

TEST_F( Texture, TextureIsInvalidBeforeCreation )
{
	Tr2TextureAL tex;
	EXPECT_FALSE( tex.IsValid() );
}

TEST_F( WithRenderContext, Creating2DTextureWithoutRenderContextFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
}

TEST_F( WithRenderContext, CreatingCubeTextureWithoutRenderContextFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
}

TEST_F( WithRenderContext, CreatingVolumeTextureWithoutRenderContextFails )
{
	uint32_t pixels[4 * 4 * 4] = { 0 };
	Tr2SubresourceData initialData;
	initialData.m_sysMemPitch = 4 * 4;
	initialData.m_sysMemSlicePitch = 4 * 4 * 4;
	initialData.m_sysMem = pixels;

	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, &initialData, *renderContext ) );
}

TEST_F( Texture, CreatingImmutable2DTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, nullptr, *renderContext ) );
}

TEST_F( Texture, CreatingImmutableCubeTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, *renderContext ) );
}

TEST_F( Texture, CreatingVolumeTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 128, 1 ), Tr2GpuUsage::SHADER_RESOURCE, nullptr, *renderContext ) );
}

TEST_F( Texture, Texture2DIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
}

#if TRINITY_PLATFORM_SUPPORTS_TEXTURE_ARRAYS
TEST_F( Texture, Texture2DArrayIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1, 2 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
	EXPECT_EQ( 2, tex.GetArraySize() );
}
#else
TEST_F( Texture, Texture2DArrayFailsOnUnsupportingPlatforms )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1, 2 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
}
#endif

TEST_F( Texture, TextureCubeIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_CUBE, tex.GetType() );
}

TEST_F( Texture, TextureVolumeIsValidAfterCreation )
{
	ENSURE_GPU_OR_SKIP
	uint32_t pixels[4 * 4 * 4 * 4] = { 0 };
	Tr2SubresourceData initialData;
	initialData.m_sysMemPitch = 4 * 4;
	initialData.m_sysMemSlicePitch = 4 * 4 * 4;
	initialData.m_sysMem = pixels;

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 4, 4, 4, 1 ), Tr2GpuUsage::SHADER_RESOURCE, &initialData, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_3D, tex.GetType() );
}

TEST_F( Texture, CanCreateMipMapped2DTexture )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( 8, tex.GetTrueMipCount() );
}

TEST_F( Texture, CanCreateMipMappedCubeTexture )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 0 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( 8, tex.GetTrueMipCount() );
}

TEST_F( Texture, TextureEqualsItself )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex == tex );
}

TEST_F( Texture, DifferentTexturesAreNotEqual )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex1;
	ASSERT_HRESULT_SUCCEEDED( tex1.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	Tr2TextureAL tex2;
	ASSERT_HRESULT_SUCCEEDED( tex2.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_FALSE( tex1 == tex2 );
}

TEST_F( Texture, LockingInvalidTextureFails )
{
	Tr2TextureAL tex;
	const void* constData;
	void* data;
	uint32_t pitch;
	ASSERT_HRESULT_FAILED( tex.MapForReading( Tr2TextureSubresource( 0 ), constData, pitch, *renderContext ) );
	ASSERT_HRESULT_FAILED( tex.MapForWriting( Tr2TextureSubresource( 0 ), data, pitch, *renderContext ) );
}

TEST_F( Texture, TextureHasMemoryClass )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	auto memoryClass = tex.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( Texture, CanCreateCompressed2DTexture )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_BC1_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
	EXPECT_EQ( PIXEL_FORMAT_BC1_UNORM, tex.GetFormat() );
}

TEST_F( Texture, CanCreateCompressedCubeTexture )
{
	ENSURE_GPU_OR_SKIP
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_BC1_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_CUBE, tex.GetType() );
	EXPECT_EQ( PIXEL_FORMAT_BC1_UNORM, tex.GetFormat() );
}
