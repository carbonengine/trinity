// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveMeshOverlayEffect_H
#define EveMeshOverlayEffect_H


#include "Tr2MeshArea.h"
#include "ITr2Renderable.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "ITr2CurveSetOwner.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( Tr2Effect );
BLUE_DECLARE_INTERFACE( ITr2Controller );
BLUE_DECLARE_IVECTOR( ITr2Controller );

// --------------------------------------------------------------------------------
// Description:
//   This class holds curveSets and Tr2Effects used for overlay effects for SpaceObjects.
//   These effects render all meshes (mesh indices) in the parent's mesh.
// SeeAlso:
//   Tr2SpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveMeshOverlayEffect ) :
	public IInitialize,
	public IListNotify,
	public ITr2ControllerOwner,
	public ITr2CurveSetOwner
{
public:
	EXPOSE_TO_BLUE();

	enum OverlayType
	{
		TYPE_OPAQUEONLY = 0,
		TYPE_ALL,

		TYPE_COUNT,
	};

	EveMeshOverlayEffect( IRoot* lockobj = NULL );
	~EveMeshOverlayEffect();

	const PTr2EffectVector& GetEffects( TriBatchType batchType, bool& success ) const;
	void Update( Be::Time realTime, Be::Time simTime );

	// type is given by a batchtype
	OverlayType GetType( TriBatchType batchType ) const;

	// do we have transparent areas?
	bool HasTransparentArea() const;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value ) override;
	void HandleControllerEvent( const char* name ) override;
	void StartControllers() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	void PlayCurveSet( const std::string& name, const std::string& rangeName ) override;
	void StopCurveSet( const std::string& name ) override;
	float GetCurveSetDuration( const std::string& name ) const override;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const override;

	std::string m_name;

private:
	// general data
	bool m_display;
	bool m_update;

	// all the effects per area
	PTr2EffectVector m_opaqueEffects;
	PTr2EffectVector m_decalEffects;
	PTr2EffectVector m_transparentEffects;
	PTr2EffectVector m_additiveEffects;
	PTr2EffectVector m_distortionEffects;

	PITr2ControllerVector m_controllers;

	// animating this overlay effect
	TriCurveSetPtr m_curveSet;
};

TYPEDEF_BLUECLASS( EveMeshOverlayEffect );
BLUE_DECLARE_VECTOR( EveMeshOverlayEffect );

#endif // EveMeshOverlayEffect_H
