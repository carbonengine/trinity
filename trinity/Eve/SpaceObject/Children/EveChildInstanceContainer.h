////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//
#pragma once

#include "IEveSpaceObjectChild.h"
#include "IEveEffectChildrenOwner.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "ITr2CurveSetOwner.h"
#include "Controllers/ITr2ControllerOwner.h"
#include "Shader/IShaderConfigurer.h"
#include "TransformModifiers/IEveChildTransformModifier.h"
#include "Eve/EveEntity.h"


struct EveChildInstanceTransform
{
	EveChildInstanceTransform();

	Vector3 scale;
	Quaternion rotation;
	Vector3 translation;
	int32_t boneIndex;
};

BLUE_DECLARE_STRUCTURE_LIST( EveChildInstanceTransform );
BLUE_DECLARE( EveChildInheritProperties );

// --------------------------------------------------------------------------------------
// Description:
//   A child container that copies a source IEveSpaceObjectChild across locatorsets
//   and or transforms
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveChildInstanceContainer ) :
	public IEveSpaceObjectChild,
	public ITr2CurveSetOwner,
	public INotify,
	public IEveEffectChildrenOwner,
	public ITr2DebugRenderable,
	public IShaderConfigurer,
	public ITr2ControllerOwner,
	public EveChildTransform,
	public IBlueStructureListNotify,
	public IListNotify,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();
	EveChildInstanceContainer( IRoot* lockobj = NULL );
	~EveChildInstanceContainer();

	void DistributeAcrossLocatorset( const BlueSharedString locatorSetName );
	float GetOwnerMaxSpeed() const;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, Matrix& parentTransform );
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void ChangeLOD( Tr2Lod lod );
	void GetLights( Tr2LightManager& lightManager ) const;
	void SetOrigin( Origin origin );
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	bool IsAlwaysOn() const override;
	void SetInheritProperties( const Color* colorSet );
	void GetWorldVelocity( Vector3& velocity ) const;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void AddTransformModifier( IEveChildTransformModifier* modifier ) override;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// IEveEffectChildrenOwner
	IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const;
	void AddToEffectChildrenList( IEveSpaceObjectChild* child );
	void RemoveFromEffectChildrenList( IEveSpaceObjectChild* child );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ControllerOwner
	void SetControllerVariable( const char* name, float value );
	void HandleControllerEvent( const char* name ) override;
	void StartControllers();
	void SetControllerVariableForInstance( const uint32_t index, const char* name, float value );
	void HandleControllerEventForInstance( const uint32_t index, const char* name );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2CurveSetOwner
	void PlayCurveSet( const std::string& name, const std::string& rangeName );
	void StopCurveSet( const std::string& name );
	void UpdateCurveSet( const std::string& name, Be::Time time );
	float GetCurveSetDuration( const std::string& name ) const;
	float GetRangeDuration( const std::string& name, const std::string& rangeName ) const;
	void PlayAllCurveSets() override;
	void StopAllCurveSets() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value ) override;	

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueStructureListNotify
	void OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	void CreateInstance(const Vector3& scale, const Quaternion& rotation, const Vector3& translation, const int32_t boneIndex = -1);
	void ClearInstanceList();
	void PopFront();
	void DisableEditMode( bool disable );

	void SetSourceEffect( IEveSpaceObjectChildPtr sourceEffect );
	void AddInstanceTransform( const Vector3& scale, const Quaternion& rotation, const Vector3& translation, int32_t boneIndex = -1 );
	void UpdateInstance( const uint32_t index, const Vector3& scale, const Quaternion& rotation, const Vector3& translation );

protected:
	void CreateInstances( IEveSpaceObject2* parent );
	void RunOnInstances( std::function<void( IEveSpaceObjectChild* )> func ) const;

protected:

	BlueSharedString m_name;
	Vector3 m_worldVelocity;
	float m_ownerMaxSpeed;
	bool m_display;

	bool m_disableEditMode;
	bool m_isAlwaysOn;
	EveChildInheritPropertiesPtr m_inheritProperties;
	Origin m_origin;

	IEveSpaceObjectChildPtr m_source;
	PIEveSpaceObjectChildVector m_instances;

	bool m_reset;
	BlueSharedString m_locatorSetName;

	PEveChildInstanceTransformStructureList m_transforms;
	PIEveChildTransformModifierVector m_transformModifiers;
};

TYPEDEF_BLUECLASS( EveChildInstanceContainer );