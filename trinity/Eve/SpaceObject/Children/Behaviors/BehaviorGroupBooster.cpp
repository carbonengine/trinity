////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "BehaviorGroupBooster.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"
#include "Tr2QuadRenderer.h"
#include "Tr2LightManager.h"
#include "include/TriMath.h"
#include "Eve/SpaceObject/Children/EveChildQuad.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierHalo.h"



namespace
{
	ALResult GetBoxVB( Tr2BufferAL& vb, Tr2PrimaryRenderContext& renderContext )
	{
		const uint32_t vertexCount = 4 * 6;
		BehaviorGroupBooster::BoosterVertex vertices[vertexCount];
		auto p = &vertices[0];
		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );

		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );

		return vb.Create( sizeof( BehaviorGroupBooster::BoosterVertex ), vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &vertices[0], renderContext );

	}
	
	Tr2VertexDefinition& GetQuadDefinition()
	{
		static Tr2VertexDefinition def;
		if( def.empty() )
		{
			def.Add( def.FLOAT32_1, def.TEXCOORD, 5 );

			def.Add( def.FLOAT32_4, def.POSITION, 0, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 1, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 2, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 3, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 4, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 5, 1, 1 );

			def.Add( def.FLOAT16_4, def.TEXCOORD, 0, 1, 1 );
			def.Add( def.FLOAT16_2, def.TEXCOORD, 1, 1, 1 );
		}
		return def;
	}

}


BehaviorGroupBooster::BehaviorGroupBooster( IRoot* lockobj ) :
	m_display( true ),
	m_boosterOffset( 0.0, 0.0, 0.0 ),
	m_atlasIndex0( 0 ),
	m_atlasIndex1( 0 ),
	m_lightRadius( 3.5 ),
	m_lightColor( 1.0, 1.0, 1.0, 1.0 ),
	m_vertexBuffer( BlueSharedString( "BoosterBoxVB" ), GetBoxVB ),
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_haloFlareHash( 0 ),
	m_haloFlareOffset( 0, 0, 0 ),
	m_haloFlareScale( 1, 1, 1 ),
	m_haloFlareBrightness( 0 ),
	m_haloFlareColor( 1, 1, 1, 1 ),
	m_haloFlareNoiseAmplitude( 0.2f ),
	m_haloFlareNoiseSpeed( 1.0f ),
	m_haloFlareNoiseOctaves( 1 ),
	m_haloMatrix( IdentityMatrix() ),
	m_ambientFlareHash( 0 ),
	m_ambientFlareOffset( 0, 0, 0 ),
	m_ambientFlareScale( 1, 1, 1 ),
	m_ambientFlareBrightness( 0 ),
	m_ambientFlareColor( 1, 1, 1, 1 ),
	m_ambientFlareNoiseAmplitude( 0.2f ),
	m_ambientFlareNoiseSpeed( 1.0f ),
	m_ambientFlareNoiseOctaves( 1 ),
	m_flareCount( 0 ),
	m_displayBoosters( true ),
	m_displayHazeFlare( true ),
	m_displayAmbientFlare( true ),
	m_haloFlares( "BehaviorGroupBooster::m_haloFlares" ),
	m_ambientFlares( "BehaviorGroupBooster::m_ambientFlares" ),
	m_ambientFlare( Quad() ),
	m_haloFlare( Quad() )
{
	m_haloModifier.CreateInstance();
}


bool BehaviorGroupBooster::Initialize()
{
	if( m_ambientFlareEffect != nullptr )
	{
		InitializeAmbientFlare();
	}
	if( m_haloFlareEffect != nullptr )
	{
		InitializeHaloFlare();
	}
	SetupQuads();

	static Tr2VertexDefinition s_boosterInstancedVertex;
	if( s_boosterInstancedVertex.empty() )
	{
		Tr2VertexDefinition& vd = s_boosterInstancedVertex;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_2, vd.TEXCOORD, 0 );
		// stream 1
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 ); // transform row 1
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2, 1, 1 ); // transform row 2
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 3, 1, 1 ); // intensity, wavephase, atlasIndex0, atlasindex1
	}

	// create vertex-declarartion
	m_vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_boosterInstancedVertex );
	return true;
}

void BehaviorGroupBooster::InitializeEffects()
{
	if( m_boosterEffect == nullptr )
	{
		m_boosterEffect = CreateBoosterEffect( BlueSharedString( "BOOSTER_LOD_HIGH" ) );
	}
	if( m_ambientFlareEffect == nullptr )
	{
		m_ambientFlareEffect = CreateFlareEffect();
		InitializeAmbientFlare();
	}
	if( m_haloFlareEffect == nullptr )
	{
		m_haloFlareEffect = CreateFlareEffect();		
		InitializeHaloFlare();
	}
	SetupQuads();
}

void BehaviorGroupBooster::InitializeHaloFlare()
{
	m_haloFlareHash = m_haloFlareEffect->GetHashValue();
	Tr2QuadRenderer::Instance()->RegisterEffect( m_haloFlareHash, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_haloFlareEffect );
}

void BehaviorGroupBooster::InitializeAmbientFlare()
{
	m_ambientFlareHash = m_ambientFlareEffect->GetHashValue();
	Tr2QuadRenderer::Instance()->RegisterEffect( m_ambientFlareHash, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_ambientFlareEffect );
}

bool BehaviorGroupBooster::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_haloFlareEffect ) )
	{
		if( m_haloFlareEffect != nullptr )
		{
			InitializeHaloFlare();
			SetupQuads();
		}
		else 
		{
			m_haloFlares.clear();
		}
	}
	else if( IsMatch( value, m_ambientFlareEffect ) )
	{
		if( m_ambientFlareEffect != nullptr )
		{
			InitializeAmbientFlare();
			SetupQuads();
		}
		else 
		{
			m_ambientFlares.clear();
		}
	}
	else
	{
		SetupQuads();
	}
	return true;
}

void BehaviorGroupBooster::SetupQuads()
{
	bool createAmbientFlares = m_ambientFlareEffect != nullptr;
	bool createHaloFlares = m_haloFlareEffect != nullptr;

	if( !createAmbientFlares && !createHaloFlares )
	{
		return;
	}

	auto ambientTransform = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), IdentityQuaternion(), m_ambientFlareOffset );
	auto haloTransform = TransformationMatrix( m_haloFlareScale, IdentityQuaternion(), Vector3( 0.f, 0.f, 0.f ) );

	m_ambientFlare = Quad( IdentityMatrix(), ambientTransform, m_ambientFlareColor, 0.0f );
	m_haloFlare = Quad( IdentityMatrix(), haloTransform, m_haloFlareColor, 0.0f );

	// turn the halo correctly so it works with the halo modifier
	m_haloMatrix = RotationMatrix( Quaternion( 0, 1, 0, 0 ) );
	
	AdjustFlareLists();
}

void BehaviorGroupBooster::AdjustFlareLists()
{
	bool createAmbientFlares = m_ambientFlareEffect != nullptr;
	bool createHaloFlares = m_haloFlareEffect != nullptr;

	if( !createAmbientFlares && !createHaloFlares )
	{
		return;
	}

	if( m_flareCount == 0)
	{
		return;
	}

	m_ambientFlares.resize( m_flareCount, m_ambientFlare );
	m_haloFlares.resize( m_flareCount, m_haloFlare );
}

bool BehaviorGroupBooster::GetDisplay() const
{
	return m_display;
}

float BehaviorGroupBooster::GetLightSize() const
{
	if( m_display )
	{
		return m_lightRadius;
	}
	return 0.0f;
}

Vector3 BehaviorGroupBooster::GetOffset() const
{
	return m_boosterOffset;
}

unsigned int BehaviorGroupBooster::GetAtlasIndex0() const {
	return m_atlasIndex0;
}

unsigned int BehaviorGroupBooster::GetAtlasIndex1() const {
	return m_atlasIndex1;
}

// Initializes the effect with some default values that make sence
void BehaviorGroupBooster::SetupBoosterEffect( Tr2EffectPtr effect )
{
	Color innerColor( 10.0, 13.0, 15.0, 0.0 );
	float innerNoiseSpeed( 6.0 );
	Vector4 innerNoiseAmplituteStart( -0.05, -0.05, -0.05, -0.05 );
	Vector4 innerNoiseAmplituteEnd( 0.1, 0.1, 0.1, 0.2 );
	Vector4 innerNoiseFrequency( 0.1, 0.1, 0.0, 0.1 );
	Color outerColor( 15.0, 13.0, 13.0, 0.0 );
	float outerNoiseSpeed( 6.0 );
	Vector4 outerNoiseAmplituteStart( -0.05, -0.05, -0.05, -0.05 );
	Vector4 outerNoiseAmplituteEnd( 0.14, 0.7, 0.14, 0.14 );
	Vector4 outerNoiseFrequency( -0.14, -0.14, -0.14, 0.1 );
	float shapeAtlasHeight( 256 );
	float shapeAtlasCount( 8 );
	Vector4 boosterScale( 1.0, 1.0, 1.0, 1.0 );

	effect->AddParameterFloat( BlueSharedString( "NoiseSpeed0" ), innerNoiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeStart0" ), &innerNoiseAmplituteStart );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeEnd0" ), &innerNoiseAmplituteEnd );
	effect->AddParameterVector4( BlueSharedString( "NoiseFrequency0" ), &innerNoiseFrequency );
	effect->AddParameterColor( BlueSharedString( "Color0" ), &innerColor );
	effect->AddParameterFloat( BlueSharedString( "NoiseSpeed1" ), outerNoiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeStart1" ), &outerNoiseAmplituteStart );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeEnd1" ), &outerNoiseAmplituteEnd );
	effect->AddParameterColor( BlueSharedString( "Color1" ), &outerColor );

	// omitted warping, since these drones don't warp yet

	Vector4 shapeAtlasSize( float( shapeAtlasHeight ), float( shapeAtlasCount ), 0, 0 );
	effect->AddParameterVector4( BlueSharedString( "ShapeAtlasSize" ), &shapeAtlasSize );
	effect->AddParameterVector4( BlueSharedString( "BoosterScale" ), &boosterScale );
	
	effect->AddResourceTexture2D( BlueSharedString( "ShapeMap" ), "res:/dx9/model/booster/shape01.dds" );
	effect->AddResourceTexture2D( BlueSharedString( "GradientMap0" ), "res:/dx9/model/booster/gradient01.dds" );
	effect->AddResourceTexture2D( BlueSharedString( "GradientMap1" ), "res:/dx9/model/booster/gradient02.dds" );
	effect->AddResourceTexture2D( BlueSharedString( "NoiseMap" ), "res:/Texture/Global/noise32cube_volume.dds" );
}

Tr2EffectPtr BehaviorGroupBooster::CreateBoosterEffect( const BlueSharedString& lodOption )
{
	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->StartUpdate();

	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/DroneBoosterVolumetric.fx" );
	effect->SetOption( BlueSharedString( "BOOSTER_LOD" ), lodOption );
	
	SetupBoosterEffect( effect );
		
	// finish effect and set it
	effect->EndUpdate();
	return effect;
}

Tr2EffectPtr BehaviorGroupBooster::CreateFlareEffect()
{
	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->StartUpdate();

	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/FlareQuad.fx" );

	// finish effect and set it
	effect->EndUpdate();
	return effect;
}

void BehaviorGroupBooster::CreateBuffer() 
{
	if( Tr2Renderer::GetShaderModel() >= TR2SM_3_0_HI )
	{
		m_vertexBuffer = Tr2ProceduralBuffer( BlueSharedString( "BoosterBoxVB" ), GetBoxVB );
	}
}

void BehaviorGroupBooster::RebuildFlareBuffer( unsigned int count )
{
	m_flareCount = count;
	AdjustFlareLists();
}

Tr2EffectPtr BehaviorGroupBooster::GetEffect()
{
	return m_boosterEffect;
}

unsigned int BehaviorGroupBooster::GetVertexDeclaration()
{
	return m_vertexDeclarationHandle;
}

void BehaviorGroupBooster::Draw( Tr2RenderContext& renderContext, Tr2BufferAL* instanceBuffer, unsigned int offset, unsigned int stride, unsigned int count )
{
	if( !instanceBuffer || !m_displayBoosters)
	{
		return;
	}

	if( Tr2Renderer::GetShaderModel() < TR2SM_3_0_HI )
	{
		// Don't render these except on high!
		return;
	}
	
	auto indexBuffer = Tr2Renderer::GetQuadListIndexBuffer( 6 );
	if( !indexBuffer )
	{
		return;
	}
	
	renderContext.m_esm.ApplyVertexDeclaration( GetVertexDeclaration() );
	renderContext.m_esm.ApplyIndexBuffer( *indexBuffer ); 

	// stream0: "indexed", booster shape
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer.GetSharedResource(), 0, sizeof( BoosterVertex ) );
	// stream1: the instance buffer
	renderContext.m_esm.ApplyStreamSource( 1, *instanceBuffer, offset, stride );
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	renderContext.DrawIndexedInstanced( 4 * 6, 0, 2 * 6, count);
}

void BehaviorGroupBooster::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{	
	if( m_ambientFlareEffect != nullptr )
	{
		m_ambientFlareHash = m_ambientFlareEffect->GetHashValue();
		quadRenderer.RegisterEffect( m_ambientFlareHash, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_ambientFlareEffect );
	}

	if( m_haloFlareEffect != nullptr )
	{
		m_haloFlareHash = m_haloFlareEffect->GetHashValue();
		quadRenderer.RegisterEffect( m_haloFlareHash, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_haloFlareEffect );
	}
}

void BehaviorGroupBooster::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display )
	{
		return;
	}
	
	if( !m_haloFlares.empty() && m_haloFlareEffect && m_displayHazeFlare )
	{
		quadRenderer.AddQuads( m_haloFlareHash, &m_haloFlares[0], m_flareCount );
	}

	if( !m_ambientFlares.empty() && m_ambientFlareEffect && m_displayAmbientFlare )
	{
		quadRenderer.AddQuads( m_ambientFlareHash, &m_ambientFlares[0], m_flareCount );
	}
}

void BehaviorGroupBooster::AddLight( Tr2LightManager& lightManager, Vector3 position, float radiusModifier, int agentIndex, const Matrix& parentTransform )
{
	Vector4 color = m_lightColor;
	if( m_ambientFlareNoiseAmplitude != 0.f )
	{
		float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() + TimeFromMS( agentIndex * 10 ) ) * m_ambientFlareNoiseSpeed, 2.f, 2.f, m_ambientFlareNoiseOctaves ) );
		color *= ( ( noise + 1.0f ) / 2.0f ) * m_ambientFlareNoiseAmplitude;
	}

	lightManager.AddPointLight( position, radiusModifier * m_lightRadius, color );
}

void BehaviorGroupBooster::AddFlare( const Matrix& agentTransform, float lod, float intensity, unsigned int agentIndex, float shipBoundingSphereRadius, float groupScale )
{
    // This is unlikely, but can happen during editing (importing a booster before it has been told how many flares there are)	
    if( m_flareCount == 0 )
	{
		return;
	}

	Matrix groupScaleMatrix = ScalingMatrix(groupScale, groupScale, groupScale);

	if( m_haloFlareEffect && m_haloFlares.size() == m_flareCount )
	{
		auto& q = m_haloFlares[agentIndex];
		float haloModification = ( 1.0f + lod ) * ( 1.0f + lod ) * ( lod - 1.0f ) * ( lod - 1.0f );
		haloModification = max( 0.0f, haloModification );
		float scaledBrightness = 0.1f + 0.9f * intensity * m_haloFlareBrightness * haloModification;
		
		// offset the halo towards the center of the ship when we lod out
		Vector3 modOffset = haloModification * m_haloFlareOffset;
		auto offset = TranslationMatrix( modOffset ) * groupScaleMatrix; // scale the offset with the group so it moves further out with increasing groupScale

		auto haloMatrix = m_haloModifier->ApplyTransform( m_haloMatrix * offset * agentTransform, 0, nullptr );		

		if( m_haloFlareNoiseAmplitude != 0.f )
		{
			float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() + TimeFromMS( agentIndex * 10 ) ) * m_haloFlareNoiseSpeed, 2.f, 2.f, m_haloFlareNoiseOctaves ) );
			scaledBrightness *= ( ( noise + 1.0f ) / 2.0f ) * m_haloFlareNoiseAmplitude;
		}

		q.m_brightness[0] = Float_16( scaledBrightness );
		q.m_parentTransform0 = Vector4( haloMatrix._11, haloMatrix._21, haloMatrix._31, haloMatrix._41 );
		q.m_parentTransform1 = Vector4( haloMatrix._12, haloMatrix._22, haloMatrix._32, haloMatrix._42 );
		q.m_parentTransform2 = Vector4( haloMatrix._13, haloMatrix._23, haloMatrix._33, haloMatrix._43 );
	}

	if( m_ambientFlareEffect && m_ambientFlares.size() == m_flareCount )
	{
		auto& q = m_ambientFlares[agentIndex];

		float mod = ( 1.0f - lod ) * ( lod + 1.0f ) * ( lod - 1.0f ) * ( lod - 1.0f );
		float scaledBrightness = ( 0.25f + 0.25f * intensity + 0.5f * mod * intensity ) * m_ambientFlareBrightness;
		Vector3 nearScale = mod * m_ambientFlareScale * groupScale;
		float farScale = ( 1.0f - mod ) * shipBoundingSphereRadius * groupScale * 2.0f;
		
		// the further away from the camera, the closer the flare is to the center of the agent
		Vector3 modOffset = mod * m_ambientFlareOffset * groupScale; 

		if( m_ambientFlareNoiseAmplitude != 0.f )
		{
			float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() + TimeFromMS( agentIndex * 10 ) ) * m_ambientFlareNoiseSpeed, 2.f, 2.f, m_ambientFlareNoiseOctaves ) );
			scaledBrightness *= ( ( noise + 1.0f ) / 2.0f ) * m_ambientFlareNoiseAmplitude;
		}

		q.m_brightness[0] = Float_16( scaledBrightness  );
		q.m_parentTransform0 = Vector4( agentTransform._11, agentTransform._21, agentTransform._31, agentTransform._41 );
		q.m_parentTransform1 = Vector4( agentTransform._12, agentTransform._22, agentTransform._32, agentTransform._42 );
		q.m_parentTransform2 = Vector4( agentTransform._13, agentTransform._23, agentTransform._33, agentTransform._43 );

		// adjust the scale
		q.m_localTransform0.x = nearScale.x + farScale;
		q.m_localTransform1.y = nearScale.y + farScale;
		q.m_localTransform2.z = nearScale.z + farScale;

		// adjust the offset
		q.m_localTransform0.w = modOffset.x;
		q.m_localTransform1.w = modOffset.y;
		q.m_localTransform2.w = modOffset.z;
	}
}

void BehaviorGroupBooster::RenderBoosterDebug( ITr2DebugRenderer2& renderer, Tr2DebugObjectReference owner, const Matrix& agentTransform )
{
	auto boosterTransform = TranslationMatrix( m_boosterOffset );	
	
	renderer.DrawCylinder(
		owner,
		boosterTransform * agentTransform,
		Vector3( 0, 0, 0 ),
		Vector3( 0, 0, -1 ),
		1.0f,
		8,
		ITr2DebugRenderer2::Lit,
		Tr2DebugColor( 0x88ffff00, 0x22ffff00 ) );
}

void BehaviorGroupBooster::RenderLightDebug( ITr2DebugRenderer2& renderer, Tr2DebugObjectReference owner, const Matrix& agentTransform )
{
	Color c = m_lightColor;
	c.a = 0.5;
	renderer.DrawSphere( owner, agentTransform, m_lightRadius, 10, ITr2DebugRenderer2::Solid, c );
}