#pragma once
#ifndef EveBoosterSet2_H
#define EveBoosterSet2_H



#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"
#include "Tr2PerObjectData.h"
#include "include/TriColor.h"

// forwards
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriFrustum;
struct ViewDistanceInfo;
class Tr2QuadRenderer;
class Tr2LightManager;
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE( EveTrailsSet );
BLUE_DECLARE( TriColor );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );

// constants
// maximum number of spline control points per trail
const unsigned int EVE_MAX_CONTROL_POINT_COUNT = 5;
// total size of ringbuffer containing pos offsets
const unsigned int EVE_MAX_POSITION_OFFSET_COUNT = 300;
// pos offset ringbuffer time delta
const float EVE_POSITION_OFFSET_DELTAT = 0.0167f;

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for boosters, trails, etc.
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveBoosterSetPerObjectData : public Tr2PerObjectData
{
public:
	struct VertexShaderData
	{
		// vs per object data
		Matrix shipMatrix;

		// additional data used for lighting, effects, etc.
		float boosterIntensity;
		float shipSpeed;
		float maxBoosterSize;
		float padding;

		// trail data
		Vector4 trailsControlPositions[EVE_MAX_CONTROL_POINT_COUNT];
		Vector4 trailsControlNormals[EVE_MAX_CONTROL_POINT_COUNT];
	};
	struct PixelShaderData
	{
		// trail data
		float boosterIntensity;
		float trailIntensity;
		float warpIntensity;
		float padding2;
	};

	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	// the data
	VertexShaderData m_vsData;
	PixelShaderData m_psData;
};

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's boosters. This includes the
//   booster, the bunch of lensflares (=spriteset) at the booster exhaust point.
//   Lensflares use the EveSpriteSet class which renders them with instancing.
//   The booster itself is also rendered with instancing and generated star-shaped
//   geometry, all done in this class.
//   The object is part of EveShip2
// SeeAlso:
//   EveShip2, EveSpriteSet
// --------------------------------------------------------------------------------
BLUE_CLASS( EveBoosterSet2 ):
	public IInitialize,
	public INotify,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	enum Shape
	{
		STAR,
		BOX,
		SHAPE_COUNT,
	};

	EveBoosterSet2( IRoot* lockobj = NULL );
	~EveBoosterSet2();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	bool HasTransparentBatches();
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	float GetSortValue();
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	// vertex data from stream0: booster geometry
	struct BoosterVertex
	{
		Vector3 position;
		Vector2 texCoord;
	};
	// vertex data for stream1: transform and random wave-phase
	struct InstanceVertex
	{
		Matrix transform;
		Vector4 functionality;
		float wavePhase;
		float atlasIndex0;
		float atlasIndex1;
	};

	// timing
	void Update( float deltaT, Be::Time t, const Matrix* parentMatrix, ITriVectorFunctionPtr ballPosition, ITriQuaternionFunctionPtr ballRotation );
	void UpdateTrails( float deltaT, Be::Time t );
	// manage individual exhaust points
	void Clear();
	void Add( const Matrix* localMatrix, const Vector4* functionality, bool hasTrail, uint32_t atlasIndex0, uint32_t atlasIndex1 );
	// set internal visual data
	void SetData( float glowScale, const Color* glowColor, float symHaloScale, float haloScaleX, float haloScaleY, const Color* haloColor, bool alwaysOn );
	void SetVolumetric( bool isVolumetric );
	void SetLightData( float offset, float flickerAmplitude, float flickerFrequency, float radius, const Color& color, float warpRadius, const Color& warpColor );
	void SetEffect( Tr2EffectPtr effect );
	void SetGlow( EveSpriteSetPtr glow );
	void SetTrail( EveTrailsSetPtr trail );
	// rendering
	void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables );
	// query booster intensity
	float GetBoosterIntensity() const;
	// just debug info
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	// get the transformed bounding sphere, ready for use
	void GetBoundingSphere( Vector4& boundingSphere ) const;
	void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;

	void RegisterWithQuadRenderer( Tr2QuadRenderer& pool );
	void AddToQuadRenderer( Tr2QuadRenderer& pool, const Matrix& world );

	void GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const;

private:
	// re-alloc and init the instance vertex buffers
	void RebuildInstanceData( Tr2RenderContext& renderContext );
	// calculated the booster intensity (=gain) from ship's ball
	float CalculateIntensity( ITriVectorFunctionPtr ball, Be::Time t );
	// calculate the spline's tangents (based on catmull-rom)
	void CalculateSplineData( float deltaT );

	// toggle display
	bool m_display;
	// show them always on strength 1.0 (for jessica editing)
	bool m_alwaysOn;
	float m_alwaysOnIntensity;
	// toggle debug drawing
	bool m_drawDebugInfo;
	// LODs
	float m_boosterLOD;
	float m_trailsLOD;

	// parent ship transform
	Matrix m_parentTransform;
	// parent ship data
	float m_parentSpeed;
	Quaternion m_parentRotation;

	// bounding info (is setup dynamically)
	Vector4 m_boosterBoundingSphere;
	Vector3 m_trailsBoundsMin, m_trailsBoundsMax;

	// the shader used for rendering the instanced boosters
	Tr2EffectPtr m_effect;
	// need special vertex declaration for multi-stream rendering
	unsigned int m_vertexDeclHandle;
	// vertex buffers for multi-stream rendering
	Tr2VertexBufferAL m_vertexBuffer;
	Tr2VertexBufferAL m_instanceBuffer;

	// holds all the lensflares of this booster
	EveSpriteSetPtr m_glows;

	// holds all the trails of this booster
	EveTrailsSetPtr m_trails;

	// indivual data of each booster (position, etc.)
	struct SingleBoosterData
	{
		Matrix transform;
		Vector4 functionality;
		Vector3 lightPosition;
		float lightRadius;
		float lightPhase;
		uint32_t atlasIndex0;
		uint32_t atlasIndex1;
	};
	std::vector<SingleBoosterData> m_singleBoosters;

	bool m_isVolumetric;

	// booster gain
	float m_maxVel;
	float m_lastAccFactor;
	float m_lastValue;
	float m_overallIntensity;
	float m_warpIntensity;

	// data of the booster
	float m_maxSize;
	float m_glowScale;
	float m_symHaloScale;
	float m_haloScaleX;
	float m_haloScaleY;
	Color m_glowColor;
	Color m_haloColor;

	// data of the trails
	float m_trailIntensity;

	// trail spline data
	Vector3 m_trailsControlPositions[EVE_MAX_CONTROL_POINT_COUNT];
	Vector3 m_trailsControlNormals[EVE_MAX_CONTROL_POINT_COUNT];
	float m_trailsControlNormalsFactor[EVE_MAX_CONTROL_POINT_COUNT];
	float m_trailsSequenceLength[EVE_MAX_CONTROL_POINT_COUNT];
	float m_trailsSmoothing;
	float m_trailsTotalLength;

	// trail ingame positioning
	XMVECTOR* m_trailsOffsets;
	unsigned int m_trailsOffsetLatest;
	Vector3 m_trailsOffsetAccu;
	float m_trailsTimeToNext;
	float m_trailsTimeDelta;

	// toggle trail physics updates
	bool m_physicsUpdate;
	// toggle destiny updates
	bool m_destinyUpdate;
	// trail static positions
	Vector3 m_trailsStaticOffsets[EVE_MAX_CONTROL_POINT_COUNT];

	float m_lightOffset;
	float m_lightRadius;
	float m_lightWarpRadius;
	float m_lightFlickerAmplitude;
	float m_lightFlickerFrequency;
	Color m_lightColor;
	Color m_lightWarpColor;
};

TYPEDEF_BLUECLASS( EveBoosterSet2 );

#endif // EveBoosterSet2_H