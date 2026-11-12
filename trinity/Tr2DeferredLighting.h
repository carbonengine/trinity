// Copyright © 2026 CCP ehf.

#pragma once

BLUE_DECLARE( Tr2Effect );

class Tr2RenderContext;
class Tr2TextureAL;


class Tr2DeferredLighting
{
public:
	explicit Tr2DeferredLighting( const char* effectFilePath );
	~Tr2DeferredLighting();

	Tr2DeferredLighting( const Tr2DeferredLighting& ) = delete;
	Tr2DeferredLighting& operator=( const Tr2DeferredLighting& ) = delete;

	void Render( const Tr2TextureAL& colorMap, const Tr2TextureAL& depthMap, const Tr2TextureAL& albedoMap, const Tr2TextureAL& specularMap, const Tr2TextureAL& normalMap, const Tr2TextureAL& parametersMap, Tr2RenderContext& renderContext );

	Tr2Effect* GetEffect() const;

private:
	Tr2EffectPtr m_effect;
};
