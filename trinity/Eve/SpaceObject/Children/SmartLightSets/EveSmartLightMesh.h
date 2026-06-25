// Copyright © 2025 CCP ehf.

#pragma once
#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "Lights/Tr2PointLight.h"
#include "EveSmartLightBaseGroup.h"
#include "Eve/SpaceObject/Children/EveChildInstanceMeshRenderer.h"
#include "attributeModifiers/IEveSmartLightGroupAttributeModifier.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

BLUE_CLASS( EveSmartLightMesh ) :
	private EveChildInstanceMeshRenderer,
	public EveSmartLightBaseGroup
{
public:
	EXPOSE_TO_BLUE();

	EveSmartLightMesh( IRoot* lockobj = nullptr );

	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution ) override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override {};
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override {};
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	uint32_t GetNumberOfEntities() const override;

private:
	void SetMeshColorParameter( Vector4 meshColor );
	Color m_lastAreaColor;
	BlueSharedString m_shaderParamColorName;
};

TYPEDEF_BLUECLASS( EveSmartLightMesh );
