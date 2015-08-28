#pragma once
#ifndef EveShip2_H
#define EveShip2_H


#include "EveMobile.h"
#include "TriFloat.h"

// forwards
BLUE_DECLARE( EveBoosterSet2 );

BLUE_CLASS( EveShip2 ) :
	public EveMobile
{
public:
	EXPOSE_TO_BLUE();

	EveShip2( IRoot* lockobj = NULL );
	~EveShip2();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );
	virtual void UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void GetLights( Tr2LightManager& lightManager ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - overriding EveSpaceObject2 implementations
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget - overriding EveSpaceObject2 implementations
	virtual void RebuildCachedData( BlueAsyncRes* p );

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of animation controller
	virtual bool ExecuteAnimationStateCommand( EveAnimationCmd cmd, const std::string& data, const std::map<std::string, float>& parameters );

	// boosters
	void SetBoosterSet( EveBoosterSet2Ptr set );
	// re-positions all attached boosters to the corresponding locators
	void RebuildBoosterSet();

private:
	// keep track of some ship's speed (in m/s)
	TriFloatPtr m_speed;

	// For Audio
	IRootPtr m_audioSpeedParameter;
	INotifyPtr m_audioSpeedNotify;
	float m_maxSpeed;

	struct
	{
		Be::Var* destinationValue;
		const Be::VarEntry* entry;
		const Be::ClassInfo* classInfo;
		ssize_t offset;

	} m_audioParameterInfo;

	void UpdateShipSpeedForAudio();

	// Property accessors
	void SetAudioParameter( IRoot* aud );
	IRoot* GetAudioParameter() const;

	// boosters and trails
	EveBoosterSet2Ptr m_boosters;
};

TYPEDEF_BLUECLASS( EveShip2 );

#endif // EveShip2_H
