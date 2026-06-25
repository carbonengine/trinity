// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "EveSmartLightBaseGroup.h"
#include "Eve/SpaceObject/Children/EveChildTransform.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

BLUE_DECLARE_INTERFACE( IEveSmartLightGroup );
BLUE_DECLARE_IVECTOR( IEveSmartLightGroup );

BLUE_CLASS( EveSmartLightColorShareGroup ) :
	public EveSmartLightBaseGroup,
	public INotify,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightColorShareGroup( IRoot* lockobj = nullptr );

	void AddQuadsToQuadRenderer( const PlacementDataWithIdentifierStructureList& placements, size_t size, const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void SetControllerVariable( const char* name, float value ) override;
	void SetInheritProperties( const Color* colorSet ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;

	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;
	// INotify
	bool OnModified( Be::Var * value ) override;
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

private:
	bool m_display;
	std::string m_name;

	PIEveSmartLightGroupVector m_lightGroups;
};

TYPEDEF_BLUECLASS( EveSmartLightColorShareGroup );
