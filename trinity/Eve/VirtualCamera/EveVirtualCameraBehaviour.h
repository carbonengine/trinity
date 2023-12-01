#pragma once

BLUE_DECLARE( EveVirtualCamera );
BLUE_DECLARE( Tr2CurveScalar );

// =============================================================================
// Float Behaviour Interface / Base
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourFloatBase ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	
	EveVirtualCameraBehaviourFloatBase( const char* name );
	virtual ~EveVirtualCameraBehaviourFloatBase();
	
	const std::string& GetName() const; 
	virtual void SetName( const std::string& name );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value ) override;

	virtual float Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) = 0;

	virtual bool IsActive();

protected:
	std::string m_name;
	bool m_active;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourFloatBase );

// =============================================================================
// Set Value
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourFloatSet ) :
	public EveVirtualCameraBehaviourFloatBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourFloatSet( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourFloatSet();

	virtual float Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

private:
	float m_value;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourFloatSet );

// =============================================================================
// Add Value
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourFloatAdd ) :
	public EveVirtualCameraBehaviourFloatBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourFloatAdd( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourFloatAdd();

	virtual void SetName( const std::string& name ) override;

	virtual float Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

private:
	float m_value;
	Tr2CurveScalarPtr m_scaleCurve;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourFloatAdd );

// =============================================================================
// Noise
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourFloatNoise ) :
	public EveVirtualCameraBehaviourFloatBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourFloatNoise( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourFloatNoise();

	virtual void SetName( const std::string& name ) override;

	virtual float Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
	
private:
	float m_frequency;
	int32_t m_octaves;
	float m_magnitude;
	Tr2CurveScalarPtr m_magnitudeCurve;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourFloatNoise );

// =============================================================================
// Damping
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourFloatDamping ) :
	public EveVirtualCameraBehaviourFloatBase
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourFloatDamping( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourFloatDamping();

	virtual float Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

private:
	float m_lastValue;
	float m_dampingRatio;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourFloatDamping );

// =============================================================================
// Vector3 Behaviour Interface / Base
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Base ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	
	EveVirtualCameraBehaviourVector3Base( const char* name );
	virtual ~EveVirtualCameraBehaviourVector3Base();
	
	virtual const std::string& GetName() const; 
	virtual void SetName( const std::string& name );
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value ) override;

	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) = 0;

	virtual bool IsActive();

protected:
	std::string m_name;
	bool m_active;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Base );

// =============================================================================
// Move Between / Lerp
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3MoveBetween ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3MoveBetween( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3MoveBetween();

	virtual void SetName( const std::string& name ) override;
	
	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

private:
	Vector3 m_start;
	Vector3 m_end;
	Tr2CurveScalarPtr m_interpolationCurve;
	bool m_proportional;
	bool m_world;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3MoveBetween );

// =============================================================================
// Offset
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Offset ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3Offset( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3Offset();

	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

private:
	Vector3 m_offset;
	bool m_proportional;
	bool m_world;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Offset );

// =============================================================================
// Orbit
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Orbit ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3Orbit( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3Orbit();

	virtual void SetName( const std::string& name ) override;
	
	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
	
private:
	float m_start;
	float m_end;
	float m_distance;
	Tr2CurveScalarPtr m_distanceScalarCurve;
	Tr2CurveScalarPtr m_orbitCurve;
	bool m_proportional;
	bool m_world;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Orbit );

// =============================================================================
// Move Foward / Dolly
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3MoveForward ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3MoveForward( IRoot* lockobj = NULL, const char* name = "Move Forward" );
	~EveVirtualCameraBehaviourVector3MoveForward();

	virtual void SetName( const std::string& name ) override;
	
	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;

protected:
	float GetCurrentValue( const EveVirtualCamera& camera, float localElapsedTime, float anchorRadius );

private:
	float m_value;
	Tr2CurveScalarPtr m_scaleCurve;
	bool m_proportional;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3MoveForward );

// =============================================================================
// Move Right / Truck
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3MoveRight ) :
	public EveVirtualCameraBehaviourVector3MoveForward
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3MoveRight( IRoot* lockobj = NULL );

	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3MoveRight );

// =============================================================================
// Move Up / Pedestal
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3MoveUp ) :
	public EveVirtualCameraBehaviourVector3MoveForward
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3MoveUp( IRoot* lockobj = NULL );
	
	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3MoveUp );

// =============================================================================
// Shake
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Shake ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3Shake( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3Shake();

	virtual void SetName( const std::string& name ) override;
	
	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
	
private:
	float m_frequency;
	int32_t m_octaves;
	Vector3 m_magnitude;
	Tr2CurveScalarPtr m_magnitudeCurve;
	bool m_scaleByView;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Shake );

// =============================================================================
// Damping
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Damping ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3Damping( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3Damping();

	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
	
private:
	Vector3 m_lastPosition;
	float m_dampingRatio;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Damping );


// =============================================================================
// Inertia
// =============================================================================

BLUE_CLASS( EveVirtualCameraBehaviourVector3Inertia ) :
	public EveVirtualCameraBehaviourVector3Base
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCameraBehaviourVector3Inertia( IRoot* lockobj = NULL );
	~EveVirtualCameraBehaviourVector3Inertia();

	virtual Vector3 Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection ) override;
	
private:
	Vector3 m_lastPosition;
	Vector3 m_lastVelocity;
	float m_inertiaFactor;
};

TYPEDEF_BLUECLASS( EveVirtualCameraBehaviourVector3Inertia );
