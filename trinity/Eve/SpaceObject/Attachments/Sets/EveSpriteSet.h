// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveSpriteSet_H
#define EveSpriteSet_H


#include "IEveSpaceObjectAttachment.h"
#include "EveSpriteSetItem.h"
#include "ITr2Renderable.h"
#include "Utilities/BoundingBox.h"
#include "Lights/ITr2LightOwner.h"

BLUE_DECLARE( EveSpriteSet );
BLUE_DECLARE_VECTOR( EveSpriteSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2DebugRenderer );

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class Tr2QuadRenderer;

struct EveSpriteLight
{
	EveSpriteLight();
	EveSpriteLight( const LightData& lightData, float blinkPhase, float blinkRate, float minScale, float maxScale, uint32_t index, const std::wstring& profilePath );

	LightData lightData;
	float blinkPhase;
	float blinkRate;
	float minScale;
	float maxScale;
	Tr2LightProfileResPtr lightProfile;

	uint32_t index;
	Matrix boneMatrix;
};

BLUE_CLASS( EveSpriteSet ) :
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveSpriteSet( IRoot* lockobj = NULL );
	~EveSpriteSet();

	// structs
	struct PoolVertex
	{
		Vector3 position;
		Float_16 activation;
		Float_16 blinkPhase;
		Float_16 blinkRate;
		Float_16 minScale;
		Float_16 maxScale;
		Float_16 falloff;
		uint32_t color;
		uint32_t warpColor;

		static const Tr2VertexDefinition& GetDefinition();
	};

	struct SpriteData
	{
		Vector3 position;
		uint32_t boneIndex;
	};

	// Note: Call Clear, Add (as many times as needed), then PrepareResources
	void Clear();
	void Add( const Vector3& pos, float blinkRate, float blinkPhase, float minScale, float maxScale, float falloff, const Color& color, const Color& warpColor );
	void Add( const Vector3& pos, float scale, const Color& color, const Color& warpColor );
	void Add( EveSpriteSetItemPtr newItem );

	// Rebuild resources
	void Rebuild();

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	virtual void UpdateLights( const Matrix& parentTransform, const Float4x3* bones, size_t boneCount, float parentStrength, float boosterGain ) override;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	virtual void AddToQuadRenderer( Tr2QuadRenderer & quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const Float4x3* bones, size_t boneCount ) override;
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	void AddLightFromSOF( const EveSpriteLight& light );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	void AddBoosterGlowToQuadRenderer( Tr2QuadRenderer & quadRenderer, const Matrix& world, float boosterGain, float warpIntensity );

	EveSpriteSetItemVector* GetSprites();
	const char* GetName();
	void SetName( const char* name );

	Tr2EffectPtr GetEffect();
	void SetEffect( Tr2EffectPtr effect );

	void SetSkinned( bool skinned );


	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

private:
	// Persisted members
	bool m_display;
	bool m_skinned;
	std::string m_name;
	unsigned m_effectHash;
	float m_intensity;

	PEveSpriteSetItemVector m_sprites;
	Tr2EffectPtr m_effect;

	// buffers for globals quadrenderer
	TrackableStdVector<PoolVertex> m_buffer;
	TrackableStdVector<SpriteData> m_spriteData;

	// bounding box functions
	AxisAlignedBoundingBox GetAabb( const Float4x3* bones, size_t boneCount ) const;

	// bounding boxes that are static
	AxisAlignedBoundingBox m_aabb;
	// bounding boxes are grouped together by bone index
	std::vector<std::pair<int, AxisAlignedBoundingBox>> m_boundingBoxes;

	std::vector<EveSpriteLight> m_lights;
	float m_activationStrength;
};

TYPEDEF_BLUECLASS( EveSpriteSet );

#endif // EveSpriteSet_H