////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveMobile_H
#define EveMobile_H

#include "EveSpaceObject2.h"

// forwards
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveTurretSet );
BLUE_DECLARE_VECTOR( EveTurretSet );

// --------------------------------------------------------------------------------
// Description:
//   This class adds functionalty like turrets to the spaceobjects class
// SeeAlso:
//   EveSpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveMobile ) :
	public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	explicit EveMobile( IRoot* lockobj = nullptr );
	~EveMobile();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// Overrides of EveSpaceObject2 implementations
	void UpdateSyncronous( EveUpdateContext& updateContext ) override;
	void UpdateAsyncronous( EveUpdateContext& updateContext ) override;
	void PrepareShaderData( EveUpdateContext& updateContext ) override;
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) override;
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ) override;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) override;
	void GetLights(Tr2LightManager& lightManager) const override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) override;
	void SetControllerVariable( const char* name, float value ) override;
	void HandleControllerEvent( const char* name ) override;
	void StartControllers() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster - overriding EveSpaceObject2 implementations
	bool GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables ) override;
	void GatherShadowRenderables( std::vector<std::vector<ShadowCasterInfo>>& shadowCasters, TriFrustum* splitCameraFrustums, TriFrustumOrtho* shadowFrustums, const size_t arraySize, const unsigned int shadowMapSize, const Vector3 sunDir );
	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;

	// re-positions all attached turrets to the corresponding locators
	void RebuildTurretPositions();
	// checks and counts the number of locators and/or granny-bones used to position turrets
	unsigned int GetTurretLocatorCount();
	// Asynch update for turret sets
	virtual void UpdateTurretsAsyncronous( EveUpdateContext& updateContext );

	void SetShaderOption(const BlueSharedString& name, const BlueSharedString& value) override;

	// Active turret info
	int GetActiveTurretCount() const;
	unsigned int m_activeTurretCount;

protected:
	/////////////////////////////////////////////////////////////////////////////////////
	// children
	bool DisplayChildren() const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// activation
	//float m_activationStrength;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// turrets
	PEveTurretSetVector m_turretSets;
	virtual const Matrix* GetTurretTransform( unsigned int turretSetIndex ) const;
private:
	/////////////////////////////////////////////////////////////////////////////////////
	// turrets
	// ship-internal data on turret locators
	struct TurretSetLocatorInfo
	{
		// how this turretset is attached
		EveSpaceObject2::LocatorType type;
		// all locator indices of this turretset
		std::vector<unsigned int> locatorIndices;
	};
	std::vector<TurretSetLocatorInfo> m_turretSetsLocatorInfo;
	size_t GetTurretLocatorIndex( size_t turretSetIdx, size_t slotIdx ) const;

	// turret locator counting
	struct TurretLocatorCountingInfo
	{
		unsigned int currentCount;
		unsigned int totalCount;
	};
	std::map<std::string, TurretLocatorCountingInfo> m_turretLocatorCountingInfo;
	void ResetTurretLocatorCounter( bool updateTotal );
	bool GetTurretLocatorCountingInfo( const char* name, unsigned int& current, unsigned int& total ) const;
	bool ValidateTurretLocatorName( const char* locatorName, unsigned int& locatorsFoundA, unsigned int& locatorsFoundB ) const;
};

TYPEDEF_BLUECLASS( EveMobile );

#endif // EveMobile_H
