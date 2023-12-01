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
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );
	virtual void GetLights( Tr2LightManager& lightManager ) const;
	virtual void GetParentData( EveSpaceObject2::ParentData * pd ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - overriding EveSpaceObject2 implementations
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget - overriding EveSpaceObject2 implementations
	virtual void RebuildCachedData( BlueAsyncRes* p );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
    virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	// boosters
	void SetBoosterSet( EveBoosterSet2Ptr set );
	// re-positions all attached boosters to the corresponding locators
	void RebuildBoosterSet();

    float GetKillCounterValue() const;
	float GetMaxSpeed() const;
	float GetBoosterIntensity() const;

protected:
	// keep track of some ship's speed (in m/s)
	TriFloatPtr m_speed;
	Vector3 m_acceleration;
	// boosters and trails
	EveBoosterSet2Ptr m_boosters;
	virtual bool DisplayBoosters() const;

	virtual void UpdateBoosters( EveUpdateContext& updateContext );
private:

	// For Audio
	IRootPtr m_audioSpeedParameter;
	INotifyPtr m_audioSpeedNotify;

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

	// on ship info displays
	uint32_t m_displayKillCounterValue;
};

TYPEDEF_BLUECLASS( EveShip2 );

#endif // EveShip2_H
