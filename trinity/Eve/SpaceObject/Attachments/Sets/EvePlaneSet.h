////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EvePlaneSet_H
#define EvePlaneSet_H

#include "IEveSpaceObjectAttachment.h"
#include "ITr2Renderable.h"
#include "Tr2GrannyAnimation.h"
#include "Utilities/BoundingBox.h"

#include "EvePlaneSetItem.h"

// forwards
BLUE_DECLARE( EvePlaneSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );
BLUE_DECLARE( TriTextureParameter );

struct ViewDistanceInfo;

class Tr2PerObjectData;

struct EvePlaneLight 
{
	EvePlaneLight();
	EvePlaneLight( const LightData& lightData, float saturation, uint32_t index, const std::wstring profilePath );

	LightData lightData;
	float saturation;
	Tr2LightProfileResPtr lightProfile;

	uint32_t index;
	Matrix boneMatrix;
};

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EvePlaneSet ):
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EvePlaneSet( IRoot* lockobj = NULL );
	~EvePlaneSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void UpdateLights( const granny_matrix_3x4* bones, size_t boneCount, float parentStrength, float boosterGain );
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void AddToQuadRenderer( Tr2QuadRenderer & quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount ) override;
	virtual void GetBatches( ITriRenderBatchAccumulator * accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL );
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );

	void AddLight( const EvePlaneLight& light ) ;
	void GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const override;

	void SetPrimaryTextureParameter( TriTextureParameterPtr primaryTextureParameter );

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	// access effect
	void SetEffect( Tr2EffectPtr effect );

	// access items
	void AddPlaneItem( EvePlaneSetItemPtr item );

	// access pickBufferID
	void SetPickBufferID( uint8_t pickBufferID );

	void SetIsSkinned( bool isSkinned );

	// rebuild the interal vertexbuffers etc.
	void Rebuild();

	EvePlaneSetItemVector* GetPlanes();
private:

	Color GetAverageColor() const;

	struct PlaneVertex
	{
		Vector4 transform1;
		Vector4 transform2;
		Vector4 transform3;
		Vector4 color;
		Vector4_16 layer1Transform;
		Vector4_16 layer2Transform;
		Vector4_16 layer1Scroll;
		Vector4_16 layer2Scroll;
		Vector4_16 blinkData;
		uint8_t index;
		uint8_t boneIndex;
		uint8_t pickBufferID;
		uint8_t maskMapAtlasIndex;
	};
	struct VolatileData
	{
		Matrix transform;
		Vector4 color;
	};

	// toggle visibility
	bool m_display;
	bool m_hideOnLowQuality;
	bool m_isSkinned;
	// pickbuffer ID
	uint8_t m_pickBufferID;
	// keep a name
	std::string m_name;

	// bounding box functions
	AxisAlignedBoundingBox GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const;
	void CreateBoundingBoxes();
	// bounding boxes that are static
	AxisAlignedBoundingBox m_aabb;
	// bounding boxes are grouped together by bone index
	std::vector<std::pair<int, AxisAlignedBoundingBox>> m_boundingBoxes;

	// the list of all them plane items
	PEvePlaneSetItemVector m_planes;

	unsigned m_effectHash;

	std::vector<PlaneVertex> m_items;
	std::vector<VolatileData> m_volatileData;

	// this shader renders or picks them all
	Tr2EffectPtr m_effect;
	std::vector<EvePlaneLight> m_lights;
	TriTextureParameterPtr m_primaryTextureParameter;
	float m_activationStrength;
};

TYPEDEF_BLUECLASS( EvePlaneSet );

#endif // EvePlaneSet_H