// Copyright © 2026 CCP ehf.

#pragma once

#include "ITr2RenderNode.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriViewport );


BLUE_CLASS( Tr2RenderNodeEffect ) :
	public ITr2RenderNode
{
public:
	Tr2RenderNodeEffect( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	bool Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime ) override;
	void Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) override;

	void AddSource( const char* name, ITr2RenderNode* source, const char* outputName = "" );

private:
	struct Source
	{
		struct Param
		{
			BlueSharedString paramName;
			BlueSharedString outputName;
			size_t outputIndex = 0;
		};
		ITr2RenderNodePtr node;
		std::vector<Param> params;

		std::vector<BlueSharedString> outputNames;
		std::vector<TempOutput> outputs;
	};
	Tr2EffectPtr m_effect;
	PITr2RenderNodeVector m_inputNodes;
	std::vector<Source> m_sources;
	TriViewportPtr m_viewport;
	Tr2EffectStateManager::RenderingMode m_renderingMode = Tr2EffectStateManager::RenderingMode::RM_FULLSCREEN;
};

TYPEDEF_BLUECLASS( Tr2RenderNodeEffect );
