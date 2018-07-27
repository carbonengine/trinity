////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#pragma once

#include "IEveSpaceObjectChildSet.h"
#include "Utilities/BoundingBox.h"


BLUE_DECLARE( Tr2Effect );


struct EveBannerItem
{
	EveBannerItem();

	int32_t bone;
	Vector3 position;
	Quaternion rotation;
	Vector3 scaling;
	float angleX;
	float angleY;
};

BLUE_DECLARE_STRUCTURE_LIST( EveBannerItem );


BLUE_CLASS( EveBannerSet )
	: public IEveSpaceObjectChildSet,
	public IInitialize,
	public IBlueStructureListNotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveBannerSet( IRoot* lockobj = nullptr );

	virtual bool Initialize();

	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );

	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );

	void AddBanner( const EveBannerItem& banner );
	void SetEffect( Tr2Effect* effect );
	void Rebuild();

	void Render( Tr2RenderContext& renderContext ) const;
	unsigned int GetPickingID() const;
protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

	virtual void OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list );
private:
	struct Vertex;

	AxisAlignedBoundingBox GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const;
	void CreateBannerGeometry( std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, const EveBannerItem& item ) const;

	std::string m_name;
	int32_t m_key;
	Tr2EffectPtr m_effect;
	PEveBannerItemStructureList m_banners;

	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_indexBuffer;
	uint32_t m_vertexDeclaration;

	AxisAlignedBoundingBox m_aabb;
	std::vector<std::pair<int32_t, AxisAlignedBoundingBox>> m_skinnedBoxes;

	bool m_display;
	bool m_isVisible;
};

TYPEDEF_BLUECLASS( EveBannerSet );