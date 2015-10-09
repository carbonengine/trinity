////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef EveSpotlightSet_h
#define EveSpotlightSet_h


#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"
#include "EveSpriteSet.h"
#include "EveSpotlightSetItem.h"
#include "TriFrustum.h"

BLUE_DECLARE( ITriRenderBatchAccumulator );
BLUE_DECLARE( Tr2PerObjectData );
BLUE_DECLARE( EveSpotlightSet );
BLUE_DECLARE_VECTOR( EveSpotlightSet );
BLUE_DECLARE( Tr2Effect );

// --------------------------------------------------------------------------------
// Description:
//   Contains a list of individual spotlight items and renders them efficiently.
// SeeAlso:
//   EveSpriteSet, EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpotlightSet ):
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource
{
public:
    EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

    EveSpotlightSet( IRoot* lockobj = NULL );
	~EveSpotlightSet();

	void UseQuadRenderer( bool useQuadRenderer, bool skinned );
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount );

	void GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );
	void SubmitSpriteGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	void Rebuild();

	// access effects
	Tr2EffectPtr GetConeEffect() const;
	void SetConeEffect( Tr2EffectPtr effect );
	Tr2EffectPtr GetGlowEffect() const;
	void SetGlowEffect( Tr2EffectPtr effect );

	// access name
	const char* GetName() const;
	void SetName( const char* name );

	// access items
	const EveSpotlightSetItemVector* GetSpotlightItems() const;
	void AddSpotlightItem( EveSpotlightSetItemPtr item );

	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );


private:
	bool OnPrepareResources();
	bool m_display;	
	bool m_useQuadRenderer;
	bool m_skinned;
	std::string m_name;

	PEveSpotlightSetItemVector m_spotlightItems;

	Tr2EffectPtr m_coneEffect;
	Tr2EffectPtr m_glowEffect;

	unsigned int m_vertexDeclHandle;
	unsigned int m_coneVertexCount;
	Tr2VertexBufferAL m_coneVertexBuffer;

	unsigned int m_spriteVertexCount;
	Tr2VertexBufferAL m_spriteVertexBuffer;

	struct GlowPoolVertex
	{
		Vector4 m_transform1;
		Vector4 m_transform2;
		Vector4 m_transform3;
		D3DXFLOAT16 m_spriteColor[3];
		D3DXFLOAT16 m_activation;
		D3DXFLOAT16 m_flareColor[3];
		D3DXFLOAT16 _unused;
		D3DXFLOAT16 m_scale[3];
		D3DXFLOAT16 m_boosterGainInfluence;

		static const Tr2VertexDefinition& GetDefinition();
	};

	struct ConePoolVertex
	{
		Vector4 m_transform1;
		Vector4 m_transform2;
		Vector4 m_transform3;
		D3DXFLOAT16 m_color[3];
		D3DXFLOAT16 m_activation;
		D3DXFLOAT16 m_boosterGainInfluence;
		D3DXFLOAT16 _unused;

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
};

TYPEDEF_BLUECLASS( EveSpotlightSet );
#endif //EveSpotlightSet_h
