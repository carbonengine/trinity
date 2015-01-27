////////////////////////////////////////////////////////////
//
//    Created:   November 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2InteriorSHLightingSolver.h"
#include "TriRenderBatch.h"

using namespace Tr2RenderContextEnum;

const unsigned int Tr2InteriorSHLightingSolver::PIXELS_PER_SAMPLE = 7;

Tr2InteriorSHLightingSolver::Tr2InteriorSHLightingSolver()
:	m_shRenderBatches( NULL ),
	m_textureSize( 256 )
{
	PrepareResources();
	GlobalStore().RegisterPlaceholderTextureVariable( "SHLightingMap" );
	GlobalStore().RegisterPlaceholderTextureVariable( "SHSampleMap" );

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_shRenderBatches = CCP_NEW( "Tr2InteriorSHLightingSolver1Sample/m_shRenderBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
}

Tr2InteriorSHLightingSolver::~Tr2InteriorSHLightingSolver()
{
	CCP_DELETE( m_shRenderBatches );
	Clear();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Releases textures created by the solver.
// Arguments:
//   s - The TriStorage class of the resource set we need to release
// --------------------------------------------------------------------------------------
void Tr2InteriorSHLightingSolver::ReleaseResources( TriStorage s )
{
	m_sampleTexture.Destroy();
	m_shTexture.Destroy();

	// Make sure that these are cleared
	GlobalStore().RegisterPlaceholderTextureVariable( "SHLightingMap" );
	GlobalStore().RegisterPlaceholderTextureVariable( "SHSampleMap" );

}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Creates position and resulting SH coefficients textures.
// --------------------------------------------------------------------------------------
bool Tr2InteriorSHLightingSolver::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	CR_RETURN_VAL(	
			m_sampleTexture.Create2D(	m_textureSize, 
										m_textureSize, 
										1, 
										Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT, 
										0, 
										nullptr, 
										renderContext )
			, false );

	CR_RETURN_VAL( 
			m_shTexture.Create(			m_textureSize, 
										m_textureSize, 
										1, 
										Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT, 
										renderContext )
			, false );

	return true;
}

// -------------------------------------------------------------
// Description:
//   Adds a volume representing a transparent area that nees
//   SH coefficients computed. The result of computation is stored
//   in provided per-area data.
// Arguments:
//   min  - Min bounds of the area in local space
//   max  - Max bounds of the area in local space
//   transform  - Transform matrix from local to world space
//   perAreaData - Per-area data block that is used to store the
//                 result of computation. This pointer needs to
//                 valid until ITr2InteriorSHLightingSolver::Solve
//                 is called.
// -------------------------------------------------------------
void Tr2InteriorSHLightingSolver::AddVolume( const Vector3& min, const Vector3& max, const Matrix& transform, Tr2PerObjectDataPSBuffer* perAreaData )
{
	SampleData data;
	data.min = min;
	data.max = max;
	data.transform = transform;
	data.perAreaData = perAreaData;
	m_samples.push_back( data );
}

// -------------------------------------------------------------
// Description:
//   Fills a pixel in position texture with specified data.
// Arguments:
//   x  - X coordinate of the pixel
//   y  - Y coordinate of the pixel
//   data  - Texture's locked memory address
//   pitch - Size of texture row in bytes
//   point  - Point position in world space
//   shIndex  - SH coefficient index identifier (from 0 to 7)
// -------------------------------------------------------------
void Tr2InteriorSHLightingSolver::FillPixel( int x, int y, void* data, unsigned pitch, const Vector3& point, int shIndex )
{
	D3DXFLOAT16 *pixel = reinterpret_cast<D3DXFLOAT16*>(
		reinterpret_cast<char*>( data ) + y * pitch + sizeof( D3DXFLOAT16 ) * 4 * x );
	pixel[0] = point.x;
    pixel[1] = point.y;
    pixel[2] = point.z;
    pixel[3] = float( shIndex );
}

// -------------------------------------------------------------
// Description:
//   Fills pixels for all SH coefficients in position texture.
// Arguments:
//   x  - X coordinate of the pixel
//   y  - Y coordinate of the pixel
//   data  - Texture's locked memory address
//   pitch - Size of texture row in bytes
//   point  - Point position in world space
//   xIncrement  - Number of pixels to step with each coefficient
//                 increment.
// -------------------------------------------------------------
void Tr2InteriorSHLightingSolver::FillCoefficients( int x, int y, void* data, unsigned pitch, const Vector3& point, int xIncrement )
{
	for( int i = 0; i < PIXELS_PER_SAMPLE; ++i )
	{
		FillPixel( x + xIncrement * i, y, data, pitch, point, i );
	}
}

// -------------------------------------------------------------
// Description:
//   Clears all added volumes from the solver.
//   Also clears the variable store variables, just in case something wants to keep referencing them
// -------------------------------------------------------------
void Tr2InteriorSHLightingSolver::Clear()
{
	m_samples.clear();

	// Make sure that these are cleared
	GlobalStore().RegisterPlaceholderTextureVariable( "SHLightingMap" );
	GlobalStore().RegisterPlaceholderTextureVariable( "SHSampleMap" );


}

// -------------------------------------------------------------
// Description:
//   Performs SH lighting computation for stored areas usign light
//   set provided. Performs a special GPU lighting pass on a small
//   texture containing SH probe positions.
// Arguments:
//   visibleLights  - A set of visible light sources
//   renderContext - Current render context
// -------------------------------------------------------------
void Tr2InteriorSHLightingSolver::Solve( const ITr2InteriorLightVector& visibleLights, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_sampleTexture.IsValid() || !m_shTexture.IsValid() )
	{
		Clear();
		return;
	}

	if( m_samples.empty() )
	{
		Clear();
		return;
	}
	
	const float scalingCoefficient = 1.f / float( m_textureSize );
	unsigned samplesPerRow = m_textureSize / PIXELS_PER_SAMPLE;
	unsigned quadsPerRow = samplesPerRow / 2;
	unsigned maxSize = m_textureSize / 4  * quadsPerRow;

	// Enlarge textures to fit all samples
	if( m_samples.size() > maxSize )
	{
		do
		{
			m_textureSize *= 2;
			samplesPerRow = m_textureSize / PIXELS_PER_SAMPLE;
			quadsPerRow = samplesPerRow / 2;
			maxSize = m_textureSize / 4 * quadsPerRow;
		}
		while( m_samples.size() > maxSize );

		CCP_LOG( "Tr2InteriorSHLightingSolver enlarges textures to %u x %u to accomodate for all SH samples", m_textureSize, m_textureSize );
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();

		if( !m_sampleTexture.IsValid() || !m_shTexture.IsValid() )
		{
			Clear();
			return;
		}
	}

	m_sampleTextureMirror.resize( 
		"Tr2InteriorSHLightingSolver::m_sampleTextureMirror", 
		m_textureSize * m_textureSize * Tr2RenderContextEnum::GetBytesPerPixel( m_sampleTexture.GetFormat() ) );

	void* data = m_sampleTextureMirror.get();
	unsigned pitch = m_textureSize * Tr2RenderContextEnum::GetBytesPerPixel( m_sampleTexture.GetFormat() );

	unsigned index = 0;
	XMVECTOR det;
	for( std::vector<SampleData>::iterator it = m_samples.begin(); it != m_samples.end(); ++it, ++index )
	{
		int x = ( index % quadsPerRow ) * PIXELS_PER_SAMPLE * 2;
		int y = ( index / quadsPerRow ) * 4;
		const float fx = static_cast<float>( x );
		const float fy = static_cast<float>( y );

		struct
		{
			Matrix worldToTexture;
			Vector4 bounds;
			Vector4 offsets;
		} areaData;
		areaData.worldToTexture = XMMatrixInverse( &det, it->transform );
		XMMATRIX origin = XMMatrixTranslation( -it->min.x, -it->min.y, -it->min.z );
		XMMATRIX scaling = XMMatrixScaling( 
			2.f / ( it->max.x - it->min.x ) * scalingCoefficient, 
			2.f / ( it->max.y - it->min.y ) * scalingCoefficient, 
			1.f / ( it->max.z - it->min.z ) );
		
		XMMATRIX translation = XMMatrixTranslation(
			( 0.5f + fx ) * scalingCoefficient,
			( 0.5f + fy ) * scalingCoefficient,
			0.f );
		areaData.worldToTexture = XMMatrixMultiply(
			XMMatrixMultiply( areaData.worldToTexture, origin ),
			XMMatrixMultiply( scaling, translation ) );
		areaData.bounds = Vector4( 
			( 0.5f + fx ) * scalingCoefficient, 
			( 0.5f + fy ) * scalingCoefficient,
			( 1.5f + fx ) * scalingCoefficient,
			( 1.5f + fy ) * scalingCoefficient );
		areaData.offsets = Vector4(	2.f * scalingCoefficient, 0.f, 0.f, 2.f * scalingCoefficient );
		it->perAreaData->CopyToPSFloatBuffer( areaData );

		XMMATRIX xtransform = it->transform;
		Vector3 point( XMVector3TransformCoord( XMVectorSet( it->min.x, it->min.y, it->min.z, 0.0f ), xtransform ) );
		FillCoefficients( x, y, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->min.x, it->max.y, it->min.z, 0.0f ), xtransform );
		FillCoefficients( x, y + 1, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->max.x, it->min.y, it->min.z, 0.0f ), xtransform );
		FillCoefficients( x + 1, y, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->max.x, it->max.y, it->min.z, 0.0f ), xtransform );
		FillCoefficients( x + 1, y + 1, data, pitch, point, 2 );

		int y2 = y + 2;

		point = XMVector3TransformCoord( XMVectorSet( it->min.x, it->min.y, it->max.z, 0.0f ), xtransform );
		FillCoefficients( x, y2, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->min.x, it->max.y, it->max.z, 0.0f ), xtransform );
		FillCoefficients( x, y2 + 1, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->max.x, it->min.y, it->max.z, 0.0f ), xtransform );
		FillCoefficients( x + 1, y2, data, pitch, point, 2 );

		point = XMVector3TransformCoord( XMVectorSet( it->max.x, it->max.y, it->max.z, 0.0f ), xtransform );
		FillCoefficients( x + 1, y2 + 1, data, pitch, point, 2 );
	}

	if( FAILED( m_sampleTexture.UpdateSubresource( 0, 0, m_textureSize, m_textureSize, m_sampleTextureMirror.get(), pitch, renderContext ) ) )
	{
		Clear();
		return;
	}

	Tr2Renderer::PushViewport();

	Tr2Renderer::PushRenderTarget( m_shTexture, renderContext );
	Tr2Renderer::PushDepthStencilBuffer( nullDS, renderContext );

	for( auto it = visibleLights.begin(); it != visibleLights.end(); ++it )
	{
		(*it)->GetSHBatches( m_shRenderBatches );
	}
	m_shRenderBatches->Finalize();
	
	GlobalStore().RegisterVariable( "SHSampleMap", &m_sampleTexture );

	renderContext.Clear( CLEARFLAGS_TARGET, 0, 0.f, 0 );

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_LIGHT );
	renderContext.RenderLightBatches( m_shRenderBatches );
	renderContext.m_esm.ApplyStandardStates(Tr2EffectStateManager::RM_ALPHA);
	renderContext.m_esm.EndManagedRendering();

	Tr2Renderer::PopRenderTarget( renderContext );
	Tr2Renderer::PopDepthStencilBuffer( renderContext );
	Tr2Renderer::PopViewport();

	m_shRenderBatches->Clear();

	GlobalStore().RegisterVariable( "SHLightingMap", &m_shTexture.GetTexture() );
	
	m_samples.clear();
}

