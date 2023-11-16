////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef EveSpotlightSet_h
#define EveSpotlightSet_h


#include "IEveSpaceObjectAttachment.h"
#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"
#include "EveSpriteSet.h"
#include "EveSpotlightSetItem.h"
#include "TriFrustum.h"
#include "Utilities/BoundingBox.h"

BLUE_DECLARE( ITriRenderBatchAccumulator );
BLUE_DECLARE( Tr2PerObjectData );
BLUE_DECLARE( EveSpotlightSet );
BLUE_DECLARE_VECTOR( EveSpotlightSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2DebugRenderer );

// --------------------------------------------------------------------------------
// Description:
//   Contains a list of individual spotlight items and renders them efficiently.
// SeeAlso:
//   EveSpriteSet, EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpotlightSet ):
	public IEveSpaceObjectAttachment,
	public IInitialize
{
public:
    EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

    EveSpotlightSet( IRoot* lockobj = NULL );
	~EveSpotlightSet();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo(ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount);

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	void Rebuild();

	// access effects
	Tr2EffectPtr GetConeEffect() const;
	void SetConeEffect( Tr2EffectPtr effect );
	Tr2EffectPtr GetGlowEffect() const;
	void SetGlowEffect( Tr2EffectPtr effect );

	void SetSkinned( bool skinned );

	// access name
	const char* GetName() const;
	void SetName( const char* name );

	// access items
	const EveSpotlightSetItemVector* GetSpotlightItems() const;
	void AddSpotlightItem( EveSpotlightSetItemPtr item );

	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );

	void SetShaderOption (const BlueSharedString& name, const BlueSharedString& value ) override;

private:
	bool m_display;	
	bool m_skinned;
	std::string m_name;
	float m_intensity;

	PEveSpotlightSetItemVector m_spotlightItems;

	Tr2EffectPtr m_coneEffect;
	Tr2EffectPtr m_glowEffect;

	struct GlowPoolVertex
	{
		Vector4 m_transform1;
		Vector4 m_transform2;
		Vector4 m_transform3;
		Float_16 m_spriteColor[3];
		Float_16 m_activation;
		Float_16 m_flareColor[3];
		Float_16 _unused;
		Float_16 m_scale[3];
		Float_16 m_boosterGainInfluence;

		static const Tr2VertexDefinition& GetDefinition();
	};

	struct ConePoolVertex
	{
		Vector4 m_transform1;
		Vector4 m_transform2;
		Vector4 m_transform3;
		Float_16 m_color[3];
		Float_16 m_activation;
		Float_16 m_boosterGainInfluence;
		Float_16 _unused;

		static const Tr2VertexDefinition& GetDefinition();
	};

	unsigned m_coneEffectHash;
	unsigned m_glowEffectHash;
	TrackableStdVector<ConePoolVertex> m_coneBuffer;
	TrackableStdVector<GlowPoolVertex> m_glowBuffer;
	struct SpotlightData
	{
		Matrix transform;
		uint32_t boneIndex;
		float boosterGainInfluence;
	};
	TrackableStdVector<SpotlightData> m_spotlightData;

	void RegisterQuadRendererCone( Tr2QuadRenderer& quadRenderer );
	void RegisterQuadRendererGlow( Tr2QuadRenderer& quadRenderer );

	// bounding box functions
	AxisAlignedBoundingBox GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const;
	// bounding boxes that are static
	AxisAlignedBoundingBox m_aabb;
	// bounding boxes are grouped together by bone index
	std::vector<std::pair<int, AxisAlignedBoundingBox>> m_boundingBoxes;
};

TYPEDEF_BLUECLASS( EveSpotlightSet );
#endif //EveSpotlightSet_h
