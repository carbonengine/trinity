#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, TextureIsInvalidBeforeCreation )
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

TEST_F( WithValidRenderContext, CreatingImmutable2DTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingImmutableCubeTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingVolumeTextureWithoutInitialDataFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 128, 1 ), Tr2GpuUsage::SHADER_RESOURCE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, Texture2DIsValidAfterCreation )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX11 || TRINITY_PLATFORM == TRINITY_STUB 
TEST_F( WithValidRenderContext, Texture2DArrayIsValidAfterCreation )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1, 2 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
	EXPECT_EQ( 2, tex.GetArraySize() );
}
#else
TEST_F( WithValidRenderContext, Texture2DArrayFailsOnUnsupportingPlatforms )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_FAILED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1, 2 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
}
#endif

TEST_F( WithValidRenderContext, TextureCubeIsValidAfterCreation )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_CUBE, tex.GetType() );
}

TEST_F( WithValidRenderContext, TextureVolumeIsValidAfterCreation )
{
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

TEST_F( WithValidRenderContext, CanCreateMipMapped2DTexture )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 0, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( 8, tex.GetTrueMipCount() );
}

TEST_F( WithValidRenderContext, CanCreateMipMappedCubeTexture )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 128, 128, 1, 0 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( 8, tex.GetTrueMipCount() );
}

TEST_F( WithValidRenderContext, TextureIsInvalidAfterDestruction )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	tex.Destroy();
	EXPECT_FALSE( tex.IsValid() );
}

TEST_F( WithValidRenderContext, TextureEqualsItself )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex == tex );
}

TEST_F( WithValidRenderContext, DifferentTexturesAreNotEqual )
{
	Tr2TextureAL tex1;
	ASSERT_HRESULT_SUCCEEDED( tex1.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	Tr2TextureAL tex2;
	ASSERT_HRESULT_SUCCEEDED( tex2.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_FALSE( tex1 == tex2 );
}

TEST_F( WithValidRenderContext, LockingInvalidTextureFails )
{
	Tr2TextureAL tex;
	void* data;
	uint32_t pitch;
	ASSERT_HRESULT_FAILED( tex.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_FAILED( tex.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, TextureHasMemoryClass )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	auto memoryClass = tex.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( WithValidRenderContext, CanCreateCompressed2DTexture )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_BC1_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_2D, tex.GetType() );
	EXPECT_EQ( PIXEL_FORMAT_BC1_UNORM, tex.GetFormat() );
}

TEST_F( WithValidRenderContext, CanCreateCompressedCubeTexture )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_BC1_UNORM, 128, 128, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	EXPECT_TRUE( tex.IsValid() );
	EXPECT_EQ( TEX_TYPE_CUBE, tex.GetType() );
	EXPECT_EQ( PIXEL_FORMAT_BC1_UNORM, tex.GetFormat() );
}

TEST_F( WithValidRenderContext, Locking2DTextureWithNoOverwriteFails )
{
	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, *renderContext ) );
	void* data;
	uint32_t pitch;
	ASSERT_HRESULT_FAILED( tex.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_NO_OVERWRITE, *renderContext ) );
}
