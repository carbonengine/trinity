////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#pragma once

#ifndef EveMultiEffect_h
#define EveMultiEffect_h

#include "Eve/IEveSpaceObject2.h"
#include "ITr2DynamicBindingOwner.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "ITr2CurveSetOwner.h"

BLUE_DECLARE( EveMultiEffectParameter );
BLUE_DECLARE_VECTOR( EveMultiEffectParameter );

BLUE_DECLARE( Tr2DynamicBinding );
BLUE_DECLARE_VECTOR( Tr2DynamicBinding );

BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE_IVECTOR( ITr2Controller );

BLUE_DECLARE( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );


BLUE_CLASS( EveMultiEffect ) :
	public IListNotify,
	public IInitialize,
	public IEveSpaceObject2,
	public ITr2DynamicBindingOwner,
	public ITr2ControllerOwner,
	public ITr2CurveSetOwner
{
public: 
	EXPOSE_TO_BLUE();

	EveMultiEffect( IRoot* lockobj = NULL );
	
	void Rebind( bool onlyUpdateBindings = 0 );
	bool SetParameter( BlueSharedString parameterName, IRoot* object );
	EveMultiEffectParameter* GetParameterByName( BlueSharedString parameterName );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name );
	void StartControllers();
	void GetBindingRoots( std::unordered_map<std::string, IRoot*>& variables );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2DynamicBindingOwner
	std::unordered_map<std::string, IRoot*> GetParameterMap() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	virtual void PlayCurveSet( const std::string& name, const std::string& rangeName );
	virtual void StopCurveSet( const std::string& name );
	virtual void UpdateCurveSet( const std::string& name, Be::Time time );
	virtual float GetCurveSetDuration( const std::string& name ) const;
	virtual float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const;
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 &position ) const;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	void GetLocalToWorldTransform( Matrix &transform ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

private:
	BlueSharedString m_name;
	PEveMultiEffectParameterVector m_parameters;
	PTr2DynamicBindingVector m_bindings;

	PITr2ControllerVector m_controllers;
	PTriCurveSetVector m_curveSets;
	PTr2ExternalParameterVector m_externalParameters;
};

TYPEDEF_BLUECLASS( EveMultiEffect );
#endif