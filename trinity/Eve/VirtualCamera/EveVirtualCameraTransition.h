// Copyright © 2023 CCP ehf.

#pragma once

BLUE_DECLARE( EveVirtualCamera );
BLUE_DECLARE( Tr2CurveScalar );

BLUE_CLASS( EveVirtualCameraTransitionBase ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraTransitionBase( IRoot* lockobj = NULL );
	~EveVirtualCameraTransitionBase();

	virtual bool IsComplete() const = 0;
	virtual EveVirtualCameraPtr GetCamera();
	virtual void SetSource( const EveVirtualCameraPtr& camera );
	virtual void SetTarget( const EveVirtualCameraPtr& camera );
	virtual void Play();
	virtual void Stop();
	virtual void Update( float deltaTime );

protected:
	EveVirtualCameraPtr m_sourceCamera;
	EveVirtualCameraPtr m_targetCamera;
	EveVirtualCameraPtr m_transitionCamera;
};

BLUE_CLASS( EveVirtualCameraTransitionCut ) :
	public EveVirtualCameraTransitionBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraTransitionCut( IRoot* lockobj = NULL );
	~EveVirtualCameraTransitionCut();

	virtual bool IsComplete() const override;
	virtual void Update( float deltaTime ) override;
};

TYPEDEF_BLUECLASS( EveVirtualCameraTransitionCut );

BLUE_CLASS( EveVirtualCameraTransitionLerp ) :
	public EveVirtualCameraTransitionBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraTransitionLerp( IRoot* lockobj = NULL );
	~EveVirtualCameraTransitionLerp();

	virtual bool IsComplete() const override;
	virtual void Play() override;
	virtual void Update( float deltaTime ) override;

	float GetTransitionTime() const;
	void SetTransitionTime( float value );

private:
	float m_transitionTime;
	float m_localTime;
	Tr2CurveScalarPtr m_transitionCurve;
};

TYPEDEF_BLUECLASS( EveVirtualCameraTransitionLerp );
