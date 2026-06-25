// Copyright © 2018 CCP ehf.

#pragma once

#include "IEveSpaceObjectAttachment.h"
#include "Utilities/BoundingBox.h"
#include "Resources/Tr2LodResource.h"
#include "Lights/ITr2LightOwner.h"



BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriTextureParameter );


struct EveBannerItem
{
	EveBannerItem();

	int32_t bone;
	Vector3 position;
	Quaternion rotation;
	Vector3 scaling;
	float angleX;
	float angleY;
	int32_t reference;
};

struct EveBannerLight
{
	EveBannerLight();
	EveBannerLight( const LightData& lightData, float saturation, uint32_t index, const std::wstring& profilePath );

	LightData lightData;
	float saturation;
	Tr2LightProfileResPtr lightProfile;

	uint32_t index;
	Matrix boneMatrix;
};

BLUE_DECLARE_STRUCTURE_LIST( EveBannerItem );


BLUE_CLASS( EveBannerSet ) :
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public IBlueStructureListNotify,
	public Tr2DeviceResource,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveBannerSet( IRoot* lockobj = nullptr );

	virtual bool Initialize() override;

	virtual bool UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	virtual void UpdateLights( const Matrix& parentTransform, const Float4x3* bones, size_t boneCount, float parentStrength, float boosterGain ) override;
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;

	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	void AddBanner( const EveBannerItem& banner );
	void SetEffect( Tr2Effect * effect );
	void SetPrimaryTextureParameter( TriTextureParameterPtr primaryTextureParameter );
	void AddLightFromSOF( const EveBannerLight& light );

	void SetKey( int32_t key );
	void Rebuild();

	unsigned int GetPickingID() const;

	int32_t GetReference( size_t index ) const;

	static float GetBannerAspectRatio( const EveBannerItem& banner );


	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	Color GetAverageColor() const;

protected:
	virtual void ReleaseResources( TriStorage s ) override;
	virtual bool OnPrepareResources() override;

	virtual void OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list ) override;

private:
	struct Vertex;

	AxisAlignedBoundingBox GetAabb( const Float4x3* bones, size_t boneCount ) const;
	void CreateBannerGeometry( std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const EveBannerItem& item ) const;
	void CreateFlatBannerGeometry( std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const EveBannerItem& item ) const;
	void CreateVerticalCurvedBannerGeometry( std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const EveBannerItem& item ) const;
	void CreateHorizontalCurvedBannerGeometry( std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const EveBannerItem& item ) const;
	void CreateCurvedBannerGeometry( std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const EveBannerItem& item ) const;

	std::string m_name;
	int32_t m_key;
	Tr2EffectPtr m_effect;
	PEveBannerItemStructureList m_banners;
	TriTextureParameterPtr m_primaryTextureParameter;

	Tr2SuballocatedBuffer::Allocation m_vertexBuffer;
	Tr2SuballocatedBuffer::Allocation m_indexBuffer;
	uint32_t m_vertexDeclaration;

	AxisAlignedBoundingBox m_aabb;
	std::vector<std::pair<int32_t, AxisAlignedBoundingBox>> m_skinnedBoxes;

	float m_maxBannerRadius;

	std::vector<EveBannerLight> m_lights;
	float m_colorSaturation;
	float m_activationStrength;

	bool m_display;
	bool m_isPickable;
	bool m_isVisible;

	bool m_renderToReflection;
};

TYPEDEF_BLUECLASS( EveBannerSet );