// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveChildBoosterSet_H
#define EveChildBoosterSet_H



#include "ITr2Renderable.h"
#include "Tr2DeviceResource.h"
#include "Tr2PerObjectData.h"
#include "include/TriColor.h"
#include "Tr2DebugRenderer.h"
#include "Tr2ProceduralResources.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/EveEntity.h"
#include "Lights/ITr2LightOwner.h"
#include "EveSpaceObjectChild.h"
#include "Tr2RingBuffer.h"
#include "Controllers/ITr2ControllerOwner.h"

// forwards
class ITriRenderBatchAccumulator;
class TriFrustum;
class Tr2QuadRenderer;
class Tr2LightManager;
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( EveSpriteSet );

// --------------------------------------------------------------------------------
// Description:
//   This class holds the per object data for boosters, etc.
// SeeAlso:
//   Tr2PerObjectData
// --------------------------------------------------------------------------------
class EveChildBoosterSetPerObjectData : public Tr2PerObjectData
{
public:
	struct VertexShaderData
	{
		// vs per object data
		Matrix worldMatrix;

		// additional data
		float padding0;
		float padding1;
		float maxBoosterSize;
		uint32_t instanceOffset;
	};
	struct PixelShaderData
	{
		float padding0;
		float padding1;
		float warpIntensity;
		float padding2;
	};

	void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const override;
	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const override;

	// the data
	VertexShaderData m_vsData;
	PixelShaderData m_psData;
};

BLUE_DECLARE( EveChildBoosterSet );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering child boosters. This includes the
//   booster, the bunch of lensflares (=spriteset) at the booster exhaust point.
//   Lensflares use the EveSpriteSet class which renders them with instancing.
//   The booster itself is also rendered with instancing, all done in this class.
// SeeAlso:
//   EveSpriteSet
// --------------------------------------------------------------------------------
BLUE_CLASS_IMPL( EveChildBoosterSet )
class EveChildBoosterSet : public EveSpaceObjectChild,
						   public IInitialize,
						   public INotify,
						   public Tr2DeviceResource,
						   public ITr2LightOwner,
						   public EveEntity,
						   public ITr2Renderable,
						   public ITr2ControllerOwner,
						   public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	static constexpr const char* DEFAULT_DRIVE_NAME = "ThrustMain";
	static constexpr const char* WARP_DRIVE_NAME = "WarpState";
	static constexpr const char* DEFAULT_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/Booster/ChildBoosterVolumetric.fx";

	EveChildBoosterSet( IRoot* lockobj = NULL );
	~EveChildBoosterSet();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value ) override;

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;
	float GetSortValue() override;
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;

	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

private:
	bool OnPrepareResources() override;
	struct EveBoosterFlareParams GetFlareParams() const;

public:
	// timing
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	// manage individual exhaust points
	void Clear();
	void Add( const Matrix& localMatrix, uint32_t atlasIndex0, uint32_t atlasIndex1, float lightScale = 1 );
	// set internal visual data
	void SetData(
		float glowScale,
		const Color& glowColor,
		const Color& warpGlowColor,
		float symHaloScale,
		float haloScaleX,
		float haloScaleY,
		const Color& haloColor,
		const Color& warpHaloColor );
	void SetLightData( float offset, float flickerAmplitude, float flickerFrequency, float radius, const Color& color, float warpRadius, const Color& warpColor );
	void SetEffect( Tr2Effect* effect, Tr2Effect* effectFar );
	void SetGlow( EveSpriteSetPtr glow );
	// rendering
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*>& renderables ) override;
	// get the transformed bounding sphere, ready for use
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const override;

	void RegisterWithQuadRenderer( Tr2QuadRenderer& pool ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;

	void SetDriveName( const std::string& driveName );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager& lightManager ) const override;

	void SetControllerVariable( const char* name, float value ) override;

private:
	// indivual data of each booster (position, etc.)
	struct SingleBoosterData
	{
		Matrix transform;
		Vector3 lightPosition;
		float lightRadius;
		float lightPhase;
		uint32_t atlasIndex0;
		uint32_t atlasIndex1;
		float wavePhase;
	};
	std::vector<SingleBoosterData> m_singleBoosters;

	// toggle display
	bool m_display;
	float m_thrust;

	// bounding info (is setup dynamically)
	Vector4 m_boosterBoundingSphere;

	// the shader used for rendering the instanced boosters
	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_effectFar;

	// need special vertex declaration for stream rendering
	unsigned int m_vertexDeclHandle;
	// vertex buffers for stream rendering
	Tr2ProceduralBuffer m_vertexBuffer;

	// holds all the lensflares of this booster
	EveSpriteSetPtr m_glows;
	bool m_flareLodEnabled;
	bool m_glowsVisible;

	float m_warpIntensity;

	// data of the booster
	float m_maxSize;
	float m_glowScale;
	float m_symHaloScale;
	float m_haloScaleX;
	float m_haloScaleY;
	Color m_glowColor;
	Color m_haloColor;
	Color m_warpGlowColor;
	Color m_warpHaloColor;

	float m_lightOffset;
	float m_lightRadius;
	float m_lightWarpRadius;
	float m_lightFlickerAmplitude;
	float m_lightFlickerFrequency;
	Color m_lightColor;
	Color m_lightWarpColor;

	std::string m_driveName;

	Tr2RingBufferOffsets m_ringBufferOffsets;
	std::vector<Tr2ChildBoosterInstanceData> m_ringBufferData;

	Matrix m_parentTransform;
	float m_parentScale;
	bool m_boosterHighLod;
	bool m_boostersVisible;
	bool m_isVisible;

	// Has UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;
};

TYPEDEF_BLUECLASS( EveChildBoosterSet );

#endif // EveChildBoosterSet_H
