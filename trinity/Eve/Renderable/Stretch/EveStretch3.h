// Copyright © 2020 CCP ehf.

#pragma once
#include <IStretchAudio.h>
#include <ITr2Audio.h>
#include "Eve/IEveFiringEffectElement.h"
#include "Eve/IEveSpaceObject2.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"
#include "ITr2DynamicBindingOwner.h"
#include "ITr2SoundEmitterOwner.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Lights/ITr2LightOwner.h"
#include "Lights/Tr2Light.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

BLUE_DECLARE( EveStretch3 );
BLUE_DECLARE( TriFloat );
BLUE_DECLARE( EveChildModifierStretch );

BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );
BLUE_DECLARE_INTERFACE( ITr2Controller );

BLUE_DECLARE_INTERFACE( ITriVectorFunction );

BLUE_DECLARE_IVECTOR( ITr2Controller );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2DynamicBinding );
BLUE_DECLARE_VECTOR( Tr2DynamicBinding );


BLUE_CLASS( EveStretch3 ) :
	public IEveFiringEffectElement,
	public ITr2DebugRenderable,
	public ITr2DynamicBindingOwner,
	public IEveSpaceObject2,
	public ITr2ControllerOwner,
	public INotify,
	public IListNotify,
	public IInitialize,
	public ITr2CurveSetOwner,
	public ITr2SoundEmitterOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();
	EveStretch3( IRoot* lockobj = NULL );
	~EveStretch3();

	void Rebind( bool onlyUpdateBindings = 0 );
	IEveSpaceObject2* GetSourceSpaceObject() const;
	IEveSpaceObject2* GetDestSpaceObject() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager * impostors );
	virtual bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time t );
	virtual void GetModelCenterWorldPosition( Vector3 & position ) const;
	virtual bool GetLocalBoundingBox( Vector3 & min, Vector3 & max );
	virtual void GetLocalToWorldTransform( Matrix & transform ) const;
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	virtual void SetControllerVariable( const char* name, float value );
	virtual void HandleControllerEvent( const char* name );
	virtual void StartControllers();
	virtual void GetBindingRoots( std::unordered_map<std::string, IRoot*> & variables );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value ) override;

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;

	//////////////////////////////////////////////////////////////////////////
	// IEveFiringEffectElement
	void SetDestObjectScale( float scale ) override;
	void StartMoving() override;
	float GetCurveDuration() override;
	void StartFiring( float delay ) override;
	void StopFiring() override;
	void SetFiringTransform( const Matrix& source, const Vector3& dest ) override;
	void SetFiringTransform( const Vector3& source, const Vector3& dest ) override;
	void DisplayEndPoints( bool displaySource, bool displayDest ) override;
	void UpdateEffectAsync( const EveUpdateContext& updateContext ) override;
	void UpdateEffectSync( const EveUpdateContext& updateContext ) override;
	void SetDisplay( bool display ) override;

	//////////////////////////////////////////////////////////////////////////
	// ITr2DynamicBindingOwner
	std::unordered_map<std::string, IRoot*> GetParameterMap() const override;

	//////////////////////////////////////////////////////////////////////////
	// ITr2SoundEmitterOwner
	ITr2AudEmitterPtr FindSoundEmitter( const char* name ) override;

	IEveSpaceObjectChild* GetSourceObject();
	void SetSourceObject( IEveSpaceObjectChild * sourceObject );
	IEveSpaceObjectChild* GetDestObject();
	void SetDestObject( IEveSpaceObjectChild * destObject );
	IEveSpaceObjectChild* GetStretchObject();
	void SetStretchObject( IEveSpaceObjectChild * stretchObject );
	IEveSpaceObjectChild* GetMoveObject();
	void SetMoveObject( IEveSpaceObjectChild * moveObject );

private:
	enum StretchState
	{
		STRETCH_STATE_UNDEFINED,
		STRETCH_STATE_STARTING,
		STRETCH_STATE_STARTED,
		STRETCH_STATE_STOPPING
	};

	void RunOnComponents( std::function<void( IEveSpaceObjectChild* )> func ) const;
	float RunOnComponentsGetMax( std::function<float( IEveSpaceObjectChild* )> func ) const;

	void SetSourceSpaceObject( IEveSpaceObject2 * spaceObject );
	void SetDestSpaceObject( IEveSpaceObject2 * spaceObject );
	void InitializeBindings();

	std::string m_name;

	bool m_display;
	bool m_update;

	float m_destObjectScale;

	Vector3 m_sourcePosition;
	Vector3 m_destinationPosition;

	ITriVectorFunctionPtr m_source;
	ITriVectorFunctionPtr m_dest;

	EveChildModifierStretchPtr m_stretchModifier;

	BlueWeakRef<IEveSpaceObject2> m_sourceSpaceObject;
	BlueWeakRef<IEveSpaceObject2> m_destSpaceObject;

	IEveSpaceObjectChildPtr m_sourceObject;
	IEveSpaceObjectChildPtr m_destObject;
	IEveSpaceObjectChildPtr m_stretchObject;
	IEveSpaceObjectChildPtr m_moveObject;
	PTriCurveSetVector m_curveSets;

	Be::Time m_startTime;
	float m_delay;
	PITr2ControllerVector m_controllers;
	PTr2DynamicBindingVector m_dynamicBindings;

	// These can't be stored directly here as we must allow
	// value bindings to the length and move progression
	TriFloatPtr m_length;
	TriFloatPtr m_moveProgression;

	Matrix m_sourceMatrix;
	bool m_isMuzzleEffect;

	StretchState m_stretchState;

	IStretchAudioPtr m_stretchAudio;
	ITr2AudioPtr m_audio;
};

TYPEDEF_BLUECLASS( EveStretch3 );
