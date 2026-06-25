// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveShip2_H
#define EveShip2_H


#include "EveMobile.h"
#include "TriFloat.h"
#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"

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
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext ) override;
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext ) override;
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform ) override;
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors ) override;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) override;
	virtual void GetParentData( EveSpaceObject2::ParentData * pd ) const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable - overriding EveSpaceObject2 implementations
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget - overriding EveSpaceObject2 implementations
	virtual void RebuildCachedData( BlueAsyncRes * p );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

	void RegisterComponents() override;
	void UnRegisterComponents() override;

	EveBoosterSet2* GetBoosters();
	void SetBoosters( EveBoosterSet2 * boosters );
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

	virtual void UpdateBoosters( const EveUpdateContext& updateContext );

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
	void SetAudioParameter( IRoot * aud );
	IRoot* GetAudioParameter() const;

	// on ship info displays
	uint32_t m_displayKillCounterValue;
};

TYPEDEF_BLUECLASS( EveShip2 );

#endif // EveShip2_H
