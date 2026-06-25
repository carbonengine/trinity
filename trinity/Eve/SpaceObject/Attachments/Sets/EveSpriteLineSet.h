// Copyright © 2016 CCP ehf.

#pragma once
#ifndef EveSpriteLineSet_H
#define EveSpriteLineSet_H

#include "IEveSpaceObjectAttachment.h"
#include "EveSpriteLineSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Utilities/BoundingBox.h"
#include "Lights/ITr2LightOwner.h"

// forwards
BLUE_DECLARE( EveSpriteLineSet );
BLUE_DECLARE( Tr2QuadRenderer );
BLUE_DECLARE( Tr2Effect );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering a line made of sprite set blinkies
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpriteLineSet ) :
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	EveSpriteLineSet( IRoot* lockobj = NULL );
	~EveSpriteLineSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

public:
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount );
	virtual void UpdateLights( const Matrix& parentTransform, const Float4x3* bones, size_t boneCount, float parentStrength, float boosterGain );
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	virtual void AddToQuadRenderer( Tr2QuadRenderer & quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const Float4x3* bones, size_t boneCount );
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount );

	// setup
	void Setup( Tr2EffectPtr effect, bool isSkinned );
	void Add( EveSpriteLineSetItemPtr newItem );

	void AddLightFromSOF( const EveSpriteLight& light );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	// rebuild resources
	void Rebuild();

private:
	bool ReallocateResources();

	// general data
	std::string m_name;
	bool m_display;
	bool m_skinned;

	std::vector<EveSpriteLight> m_lights;
	float m_activationStrength;

	// shader
	unsigned int m_effectHash;
	Tr2EffectPtr m_effect;

	// buffers for globals quadrenderer
	TrackableStdVector<EveSpriteSet::PoolVertex> m_buffer;
	TrackableStdVector<EveSpriteSet::SpriteData> m_spriteData;

	// all the line set
	PEveSpriteLineSetItemVector m_spriteLines;

	// bounding box functions
	AxisAlignedBoundingBox GetAabb( const Float4x3* bones, size_t boneCount ) const;
	// bounding boxes that are static
	AxisAlignedBoundingBox m_aabb;
	// bounding boxes are grouped together by bone index
	std::vector<std::pair<int, AxisAlignedBoundingBox>> m_boundingBoxes;
};

TYPEDEF_BLUECLASS( EveSpriteLineSet );

#endif // EveSpriteLineSet_H