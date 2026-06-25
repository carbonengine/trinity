// Copyright © 2025 CCP ehf.

#pragma once

#include "../EveChildTransform.h"
#include "../EveChildInheritProperties.h"
#include "../IEveInheritPropertiesOwner.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "IEveSmartLightGroup.h"
#include "Eve/SpaceObject/Utils/EveDistributionMethods/IEveDistributionMethod.h"

BLUE_DECLARE_INTERFACE( IEveDistributionMethod );
BLUE_DECLARE_INTERFACE( IEveSmartLightGroup );
BLUE_DECLARE_IVECTOR( IEveSmartLightGroup );

BLUE_CLASS( EveChildSmartLightSet ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2DebugRenderable,
	public IEveInheritPropertiesOwner,
	public INotify,
	public IListNotify,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveChildSmartLightSet( IRoot* lockobj = nullptr );

	// ITr2DebugRenderable
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;

	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const
	{
		return false;
	};
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) {};
	void ChangeLOD( Tr2Lod lod ) {};
	void SetControllerVariable( const char* name, float value ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	// IEveInheritPropertiesOwner
	void SetInheritProperties( const Color* colorSet );

	// INotify
	bool OnModified( Be::Var * value );

	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList );

private:
	bool m_display;
	std::string m_name;
	IEveDistributionMethodPtr m_distribution;
	PIEveSmartLightGroupVector m_lightGroups;

	EveChildInheritPropertiesPtr m_inheritProperties;
};

TYPEDEF_BLUECLASS( EveChildSmartLightSet );