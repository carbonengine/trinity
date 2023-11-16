////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//
//    Description:
// 	    Base class to provide audio functionality to EveStretch effects
// 	    All creating and positioning of audio emitters is handled by the class
// 		and not the sound designer. This should be used in Jessica if a sound designer
//      wants automatic emitter positioning but is defining audio event triggers manually (aka using a controller).

#pragma once

#include "Tr2DebugRenderer.h"
#include "ITr2Audio.h"
#include "ITr2AudEmitter.h"

BLUE_CLASS( Tr2AudioStretchBase ):
	public IInitialize,
    public ITr2Audio,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	// IInitialize
	bool Initialize() override;

	Tr2AudioStretchBase( IRoot* lockobj = NULL );
	virtual ~Tr2AudioStretchBase();

	///////////// ITr2Audio /////////////////////////////////////////////
	// Places the source and destination audio emitters at the source and
	// destination positions of the stretch effect. Places the stretch
	// emitter between those two points relative to the camera.
	void Update( Vector3& sourcePosition, Vector3& destPosition );
	ITr2AudEmitterPtr FindEmitterByName( const char* name ) override;

	// debug
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );
protected:
	ITr2AudEmitterPtr m_sourceEmitter;
	ITr2AudEmitterPtr m_destEmitter;
	ITr2AudEmitterPtr m_stretchEmitter;
};

TYPEDEF_BLUECLASS( Tr2AudioStretchBase );
