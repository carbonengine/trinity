// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveCloudVolumeBall_H
#define EveCloudVolumeBall_H

#include "Tr2DebugRenderer.h"
#include "include/ITriEffectParameter.h"

BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( EveCloudEditableVolume );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;

// --------------------------------------------------------------------------------
// Description:
//   A ball for EveCloudEditableVolume objects.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveCloudVolumeBall ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveCloudVolumeBall( IRoot* lockobj = NULL );
	~EveCloudVolumeBall();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	struct BallData
	{
		Vector3 m_position;
		float m_radius;
		Color m_selfIllumination;
		float m_opacity;
		float m_falloff;
	} m_ballData;

	BlueWeakRef<EveCloudEditableVolume> m_owner;
};

TYPEDEF_BLUECLASS( EveCloudVolumeBall );
BLUE_DECLARE_VECTOR( EveCloudVolumeBall );


// --------------------------------------------------------------------------------
// Description:
//   An "editor" for volume textures used by EveCloud. Maintains a list of balls
//   (EveCloudVolumeBall) that are rasterized into a volume texture.
// --------------------------------------------------------------------------------
BLUE_CLASS( EveCloudEditableVolume ) :
	public IInitialize,
	public INotify,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	EveCloudEditableVolume( IRoot* lockobj = NULL );
	~EveCloudEditableVolume();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList );

	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& worldTransform );

	void OnVolumeModified();

	void Update( Be::Time time );
	void RenderDebugInfo( const Matrix& world, Tr2RenderContext& renderContext );

	TriTextureRes* GetTexture() const;

private:
	static const size_t MAX_FRAMES = 4;

	enum Status
	{
		Working,
		StopRequested,
		DataReady,
		Aborted,
	};
	struct RasterizeParams
	{
		CcpAtomic<uint32_t> status;
		std::vector<EveCloudVolumeBall::BallData> balls[MAX_FRAMES];
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		std::unique_ptr<uint8_t[]> pixels;
	};
	static uint32_t ThreadProc( void* context );
	static uint32_t ThreadProcAnimated( void* context );
	static void RasterizeBalls( RasterizeParams & params );
	static void RasterizeBallsAnimated( RasterizeParams & params );
	static void RasterizeBall( const EveCloudVolumeBall::BallData& ball, const RasterizeParams& params, float* pixels );

	Tr2HostBitmapPtr Rasterize();

	RasterizeParams m_currentParams;
	CcpThreadHandle_t m_thread;

	TriTextureResPtr m_texture;
	Tr2HostBitmapPtr m_bitmap;
	PEveCloudVolumeBallVector m_balls;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_depth;
	bool m_renderDebugInfo;
	bool m_animated;
	bool m_volumeDirty;
	bool m_updating;
	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( EveCloudEditableVolume );
BLUE_DECLARE_VECTOR( EveCloudEditableVolume );


BLUE_CLASS( EveCloudVolumeTextureParameter ) :
	public ITriEffectResourceParameter
{

public:
	EveCloudVolumeTextureParameter( IRoot* lockobj = nullptr );
	~EveCloudVolumeTextureParameter();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader * effectRes );

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectResourceParameter
	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	unsigned GetHashValue( unsigned startingHash ) const;

private:
	BlueSharedString m_name;
	EveCloudEditableVolumePtr m_volume;
	bool m_isUsedByEffect;
};

TYPEDEF_BLUECLASS( EveCloudVolumeTextureParameter );
BLUE_DECLARE_VECTOR( EveCloudVolumeTextureParameter );

#endif