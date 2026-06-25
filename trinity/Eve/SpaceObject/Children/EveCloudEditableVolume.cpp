// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveCloudEditableVolume.h"

#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"
#include "Tr2Renderer.h"
#include "Curves/TriCurveSet.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"

EveCloudVolumeBall::EveCloudVolumeBall( IRoot* lockobj )
{
	m_ballData.m_position = Vector3( 0.f, 0.f, 0.f );
	m_ballData.m_radius = 0.f;
	m_ballData.m_selfIllumination = Color( 0.f, 0.f, 0.f, 0.f );
	m_ballData.m_opacity = 0.f;
	m_ballData.m_falloff = 1.f;
}

EveCloudVolumeBall::~EveCloudVolumeBall()
{
}

bool EveCloudVolumeBall::OnModified( Be::Var* value )
{
	if( !!m_owner )
	{
		m_owner->OnVolumeModified();
	}
	return true;
}


EveCloudEditableVolume::EveCloudEditableVolume( IRoot* lockobj ) :
	PARENTLOCK( m_balls ),
	m_width( 64 ),
	m_height( 64 ),
	m_depth( 64 ),
	m_thread( 0 ),
	m_renderDebugInfo( false ),
	m_animated( false ),
	m_volumeDirty( false ),
	m_updating( false ),
	PARENTLOCK( m_curveSets )
{
	m_texture.CreateInstance();
	m_bitmap.CreateInstance();

	m_balls.SetNotify( this );
}

EveCloudEditableVolume::~EveCloudEditableVolume()
{
	if( m_thread )
	{
		m_currentParams.status = StopRequested;
		uint32_t result;
		CcpJoinThread( m_thread, result );
	}
}

bool EveCloudEditableVolume::Initialize()
{
	OnVolumeModified();
	return true;
}

bool EveCloudEditableVolume::OnModified( Be::Var* value )
{
	OnVolumeModified();
	return true;
}

Tr2HostBitmapPtr EveCloudEditableVolume::Rasterize()
{
	OnVolumeModified();
	if( m_thread )
	{
		uint32_t result;
		CcpJoinThread( m_thread, result );
		m_thread = 0;
		if( m_currentParams.status == DataReady )
		{
			m_bitmap->CreateVolume( m_currentParams.width, m_currentParams.height, m_currentParams.depth, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
			memcpy( m_bitmap->GetRawData(), m_currentParams.pixels.get(), m_bitmap->GetRawDataSize() );
			m_texture->CreateFromHostBitmap( m_bitmap );
			if( m_volumeDirty )
			{
				OnVolumeModified();
			}
		}
	}
	return m_bitmap;
}

void EveCloudEditableVolume::Update( Be::Time time )
{
	if( m_currentParams.status == DataReady )
	{
		uint32_t result;
		CcpJoinThread( m_thread, result );
		m_thread = 0;
		m_bitmap->CreateVolume( m_currentParams.width, m_currentParams.height, m_currentParams.depth, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
		memcpy( m_bitmap->GetRawData(), m_currentParams.pixels.get(), m_bitmap->GetRawDataSize() );
		m_texture->CreateFromHostBitmap( m_bitmap );
		if( m_volumeDirty )
		{
			OnVolumeModified();
		}
	}
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( time, time );
	}
}

void EveCloudEditableVolume::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* theList )
{
	switch( event & BELIST_EVENTMASK )
	{
	case BELIST_INSERTED:
		if( event & BELIST_LOADING )
		{
			return;
		}
		else
		{
			EveCloudVolumeBallPtr sphere = BlueCastPtr( value );
			if( sphere )
			{
				sphere->m_owner = this;
			}
			OnVolumeModified();
		}
		break;
	case BELIST_REMOVED:
		if( event & BELIST_UNLOADING )
		{
			return;
		}
		else
		{
			EveCloudVolumeBallPtr sphere = BlueCastPtr( value );
			if( sphere )
			{
				sphere->m_owner = nullptr;
			}
			OnVolumeModified();
		}
		break;
	case BELIST_LOADFINISHED:
		for( auto it = m_balls.begin(); it != m_balls.end(); ++it )
		{
			( *it )->m_owner = this;
		}
		break;
	case BELIST_UNLOADSTART:
		for( auto it = m_balls.begin(); it != m_balls.end(); ++it )
		{
			( *it )->m_owner = nullptr;
		}
		break;
	}
}

void EveCloudEditableVolume::OnVolumeModified()
{
	if( m_updating )
	{
		return;
	}
	if( m_thread )
	{
		m_volumeDirty = true;
		return;
	}
	m_volumeDirty = false;
	m_updating = true;
	m_currentParams.width = m_width;
	m_currentParams.height = m_height;
	m_currentParams.depth = m_depth;
	m_currentParams.pixels = std::unique_ptr<uint8_t[]>( new uint8_t[m_width * m_height * m_depth * 4] );
	m_currentParams.status = Working;
	if( m_animated )
	{
		for( size_t f = 0; f < MAX_FRAMES; ++f )
		{
			for( auto curveSet = m_curveSets.begin(); curveSet != m_curveSets.end(); ++curveSet )
			{
				bool needToResume = ( *curveSet )->IsPlaying();
				double resumeTime = ( *curveSet )->GetScaledTime();
				( *curveSet )->Stop();
				( *curveSet )->Play();
				( *curveSet )->Update( 0 );
				( *curveSet )->Update( float( f ) / ( MAX_FRAMES - 1 ) );
				if( needToResume )
				{
					( *curveSet )->PlayFrom( resumeTime );
				}
			}
			m_currentParams.balls[f].resize( m_balls.size() );
			for( size_t i = 0; i < m_balls.size(); ++i )
			{
				m_currentParams.balls[f][i] = m_balls[i]->m_ballData;
			}
		}
		m_thread = CcpCreateThread( &ThreadProcAnimated, &m_currentParams, CCP_THREAD_PRIORITY_NORMAL );
	}
	else
	{
		m_currentParams.balls[0].resize( m_balls.size() );
		for( size_t i = 0; i < m_balls.size(); ++i )
		{
			m_currentParams.balls[0][i] = m_balls[i]->m_ballData;
		}

		m_thread = CcpCreateThread( &ThreadProc, &m_currentParams, CCP_THREAD_PRIORITY_NORMAL );
	}
	m_updating = false;
}

uint32_t EveCloudEditableVolume::ThreadProc( void* context )
{
	RasterizeBalls( *static_cast<RasterizeParams*>( context ) );
	return 0;
}

uint32_t EveCloudEditableVolume::ThreadProcAnimated( void* context )
{
	RasterizeBallsAnimated( *static_cast<RasterizeParams*>( context ) );
	return 0;
}

void EveCloudEditableVolume::RasterizeBalls( RasterizeParams& params )
{
	std::unique_ptr<float[]> pixels( new float[params.width * params.height * params.depth * 4] );
	memset( pixels.get(), 0, sizeof( float ) * ( params.width * params.height * params.depth * 4 ) );
	for( auto it = params.balls[0].begin(); it != params.balls[0].end(); ++it )
	{
		RasterizeBall( *it, params, pixels.get() );
		if( params.status != Working )
		{
			params.status = Aborted;
			return;
		}
	}
	const float* srcPixel = pixels.get();
	uint8_t* destPixel = params.pixels.get();
	for( uint32_t i = 0; i < params.width * params.height * params.depth * 4; ++i )
	{
		destPixel[i] = uint8_t( std::min( std::max( TriLinearToGamma( srcPixel[i] ) * 255.f, 0.f ), 255.f ) );
	}
	params.status = DataReady;
}

void EveCloudEditableVolume::RasterizeBallsAnimated( RasterizeParams& params )
{
	std::unique_ptr<float[]> pixels( new float[params.width * params.height * params.depth * 4] );
	for( size_t f = 0; f < MAX_FRAMES; ++f )
	{
		memset( pixels.get(), 0, sizeof( float ) * ( params.width * params.height * params.depth * 4 ) );
		for( auto it = params.balls[f].begin(); it != params.balls[f].end(); ++it )
		{
			RasterizeBall( *it, params, pixels.get() );
			if( params.status != Working )
			{
				params.status = Aborted;
				return;
			}
		}
		const uint32_t channels[] = { 2, 1, 0, 3 };
		const float* srcPixel = pixels.get();
		uint8_t* destPixel = params.pixels.get() + channels[f];
		for( uint32_t i = 0; i < params.width * params.height * params.depth * 4; i += 4 )
		{
			destPixel[i] = uint8_t( std::min( std::max( TriLinearToGamma( srcPixel[i + 3] ) * 255.f, 0.f ), 255.f ) );
		}
	}
	params.status = DataReady;
}

void EveCloudEditableVolume::RasterizeBall( const EveCloudVolumeBall::BallData& ball, const RasterizeParams& params, float* pixels )
{
	auto selfIllumination = TriGammaToLinear( ball.m_selfIllumination );
	int minX = std::max( int( ( ball.m_position.x + 0.5 - ball.m_radius ) * params.width ), 0 );
	int minY = std::max( int( ( ball.m_position.y + 0.5 - ball.m_radius ) * params.height ), 0 );
	int minZ = std::max( int( ( ball.m_position.z + 0.5 - ball.m_radius ) * params.depth ), 0 );

	int maxX = std::min( int( ( ball.m_position.x + 0.5 + ball.m_radius ) * params.width ), int( params.width ) - 1 );
	int maxY = std::min( int( ( ball.m_position.y + 0.5 + ball.m_radius ) * params.height ), int( params.height ) - 1 );
	int maxZ = std::min( int( ( ball.m_position.z + 0.5 + ball.m_radius ) * params.depth ), int( params.depth ) - 1 );

	for( int k = minZ; k <= maxZ; ++k )
	{
		float z = float( k ) / params.depth - 0.5f;
		float dz = z - ball.m_position.z;
		dz *= dz;
		for( int j = minY; j <= maxY; ++j )
		{
			if( params.status == StopRequested )
			{
				return;
			}
			float y = float( j ) / params.height - 0.5f;
			float dy = y - ball.m_position.y;
			dy *= dy;
			for( int i = minX; i <= maxX; ++i )
			{
				float x = float( i ) / params.width - 0.5f;
				float dx = x - ball.m_position.x;
				dx *= dx;

				float distance = std::min( 1.f, sqrtf( dx + dy + dz ) / ball.m_radius );
				float alpha = pow( 1.f - distance, ball.m_falloff );

				int offset = ( i + j * params.width + k * params.width * params.height ) * 4;

				float destR = pixels[offset + 2];
				float destG = pixels[offset + 1];
				float destB = pixels[offset + 0];

				pixels[offset] = destB + selfIllumination.b * alpha;
				pixels[offset + 1] = destG + selfIllumination.g * alpha;
				pixels[offset + 2] = destR + selfIllumination.r * alpha;
				pixels[offset + 3] += alpha * ball.m_opacity;
			}
		}
	}
}

void EveCloudEditableVolume::RenderDebugInfo( const Matrix& world, Tr2RenderContext& renderContext )
{
	if( !m_renderDebugInfo )
	{
		return;
	}
	Matrix scale = ScalingMatrix( 0.5f, 0.5f, 0.5f );
	Tr2Renderer::DrawOrientedBox( scale * world, 0xffff0000 );

	for( auto it = m_balls.begin(); it != m_balls.end(); ++it )
	{
		Vector3 position = TransformCoord( ( *it )->m_ballData.m_position, world );
		Tr2Renderer::DrawSphere( position,
								 ( *it )->m_ballData.m_radius * world._11,
								 10,
								 ( uint32_t( ( *it )->m_ballData.m_opacity * 255 ) << 24 ) | ( *it )->m_ballData.m_selfIllumination );
	}
}

TriTextureRes* EveCloudEditableVolume::GetTexture() const
{
	return m_texture;
}

void EveCloudEditableVolume::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Cloud Balls" );
}

void EveCloudEditableVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform )
{
	if( renderer.HasOption( GetRawRoot(), "Cloud Balls" ) )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		uint32_t color = 0x888888ff;

		for( auto it = begin( m_balls ); it != end( m_balls ); ++it )
		{
			renderer.DrawSphere( *it, worldTransform, ( *it )->m_ballData.m_position, ( *it )->m_ballData.m_radius, 16, Tr2DebugRenderer::Wireframe, color );
			renderer.DrawSphere( *it, worldTransform, ( *it )->m_ballData.m_position, ( *it )->m_ballData.m_radius, 16, Tr2DebugRenderer::Solid, 0 );
		}
	}
}



EveCloudVolumeTextureParameter::EveCloudVolumeTextureParameter( IRoot* lockobj ) :
	m_isUsedByEffect( false )
{
}

EveCloudVolumeTextureParameter::~EveCloudVolumeTextureParameter()
{
}

const char* EveCloudVolumeTextureParameter::GetParameterName() const
{
	return m_name.c_str();
}

void EveCloudVolumeTextureParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_isUsedByEffect = false;
	if( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( !resource )
	{
		return;
	}
	m_isUsedByEffect = true;
}

bool EveCloudVolumeTextureParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	TriTextureRes* resource = m_volume ? m_volume->GetTexture() : nullptr;
	bool isSrgb = ( flags & RESOURCE_FLAG_SRGB ) != 0;
	auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	if( Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{

		return resourceDesc.SetSrv( stage, registerIndex, *tex, colorSpace );
	}
	else
	{
		return resourceDesc.SetSrv( stage, registerIndex, Tr2Renderer::GetFallbackTexture( Tr2EffectResource::TEXTURE_3D, m_name.c_str() ), colorSpace );
	}
}

unsigned EveCloudVolumeTextureParameter::GetHashValue( unsigned startingHash ) const
{
	return CcpHashFNV1( &m_volume.p, sizeof( m_volume.p ), startingHash );
}
