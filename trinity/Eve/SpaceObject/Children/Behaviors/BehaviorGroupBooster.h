// Copyright © 2020 CCP ehf.

#pragma once
#include "Tr2ProceduralResources.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2QuadRenderer );
BLUE_DECLARE( Tr2LightManager );
BLUE_DECLARE( EveChildQuad );
BLUE_DECLARE( EveChildModifierHalo );
BLUE_DECLARE( TriFrustum );
struct Tr2RenderBatch;


// --------------------------------------------------------------------------------------
// Description:
//   A class that describes look of behavior group boosters
//   This is very hardcoded, since we don't support multiple booster types
// --------------------------------------------------------------------------------------
BLUE_CLASS( BehaviorGroupBooster ) :
	public IInitialize,
	public INotify
{
public:
	// booster vertex data
	struct BoosterVertex
	{
		Vector3 position;
		Vector2 texCoord;
	};

	struct Quad
	{
		Quad()
		{
		}

		Quad( const Matrix& parentTransform, const Matrix& localTransform, const Color& color, float brightness )
		{
			m_parentTransform0 = Vector4( parentTransform._11, parentTransform._21, parentTransform._31, parentTransform._41 );
			m_parentTransform1 = Vector4( parentTransform._12, parentTransform._22, parentTransform._32, parentTransform._42 );
			m_parentTransform2 = Vector4( parentTransform._13, parentTransform._23, parentTransform._33, parentTransform._43 );

			m_localTransform0 = Vector4( localTransform._11, localTransform._21, localTransform._31, localTransform._41 );
			m_localTransform1 = Vector4( localTransform._12, localTransform._22, localTransform._32, localTransform._42 );
			m_localTransform2 = Vector4( localTransform._13, localTransform._23, localTransform._33, localTransform._43 );

			m_color[0] = Float_16( color.r );
			m_color[1] = Float_16( color.g );
			m_color[2] = Float_16( color.b );
			m_color[3] = Float_16( color.a );
			m_brightness[0] = Float_16( brightness );
			m_brightness[1] = Float_16( 0.f );
		}

		Vector4 m_parentTransform0;
		Vector4 m_parentTransform1;
		Vector4 m_parentTransform2;

		Vector4 m_localTransform0;
		Vector4 m_localTransform1;
		Vector4 m_localTransform2;

		Float_16 m_color[4];
		Float_16 m_brightness[2];
	};

	EXPOSE_TO_BLUE();
	BehaviorGroupBooster( IRoot* lockobj = NULL );

	// IInitialize
	bool Initialize() override;

	void InitializeEffects();

	// INotify
	bool OnModified( Be::Var * value ) override;

	void CreateBuffer();
	Tr2EffectPtr GetEffect();
	unsigned int GetVertexDeclaration() const;
	void RebuildFlareBuffer( unsigned int count );

	Tr2RenderBatch GetBatch( Tr2BufferAL * instanceBuffer, unsigned int startInstance, unsigned int instanceDataStride, unsigned int count ) const;

	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	void AddLight( Tr2LightManager & lightManager, Vector3 position, float radiusModifier, int agentIndex, const Matrix& parentTransform );
	void AddFlare( const Matrix& agentTransform, float lod, float intensity, unsigned int agentIndex, float shipBoundingSphereRadius, float groupScale );

	void RenderBoosterDebug( ITr2DebugRenderer2 & renderer, Tr2DebugObjectReference owner, const Matrix& transform );
	void RenderLightDebug( ITr2DebugRenderer2 & renderer, Tr2DebugObjectReference owner, const Matrix& transform );

	bool GetDisplay() const;
	float GetLightSize() const;

	Vector3 GetOffset() const;
	unsigned int GetAtlasIndex0() const;
	unsigned int GetAtlasIndex1() const;

private:
	Tr2EffectPtr CreateBoosterEffect( const BlueSharedString& lodOption );
	Tr2EffectPtr CreateFlareEffect();

	void InitializeHaloFlare();
	void InitializeAmbientFlare();

	void SetupBoosterEffect( Tr2EffectPtr effect );
	void SetupQuads();
	void AdjustFlareLists();

	Vector3 m_boosterOffset;
	float m_lightRadius;
	Color m_lightColor;
	uint32_t m_atlasIndex0;
	uint32_t m_atlasIndex1;

	bool m_display;

	Tr2EffectPtr m_boosterEffect;

	Tr2EffectPtr m_haloFlareEffect;
	unsigned int m_haloFlareHash;

	Vector3 m_haloFlareOffset;
	Vector3 m_haloFlareScale;
	float m_haloFlareBrightness;
	Color m_haloFlareColor;
	float m_haloFlareNoiseAmplitude;
	float m_haloFlareNoiseSpeed;
	uint32_t m_haloFlareNoiseOctaves;
	EveChildModifierHaloPtr m_haloModifier;
	Matrix m_haloMatrix;

	Tr2EffectPtr m_ambientFlareEffect;
	unsigned int m_ambientFlareHash;
	Vector3 m_ambientFlareOffset;
	Vector3 m_ambientFlareScale;
	float m_ambientFlareBrightness;
	Color m_ambientFlareColor;
	float m_ambientFlareNoiseAmplitude;
	float m_ambientFlareNoiseSpeed;
	uint32_t m_ambientFlareNoiseOctaves;

	TrackableStdVector<Quad> m_ambientFlares;
	TrackableStdVector<Quad> m_haloFlares;

	// the flare instances (which are copied to fill the vectors)
	Quad m_ambientFlare;
	Quad m_haloFlare;

	unsigned int m_flareCount;

	Tr2ProceduralBuffer m_vertexBuffer;
	unsigned int m_vertexDeclarationHandle;

	// debug stuff
	bool m_displayBoosters;
	bool m_displayHazeFlare;
	bool m_displayAmbientFlare;
};

TYPEDEF_BLUECLASS( BehaviorGroupBooster );
