// Copyright © 2017 CCP ehf.

#pragma once
#ifndef EveHazeSet_H
#define EveHazeSet_H

#include "IEveSpaceObjectAttachment.h"
#include "ITr2Renderable.h"
#include "Utilities/BoundingBox.h"
#include "EveHazeSetItem.h"
#include "Lights/ITr2LightOwner.h"

// forwards
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );
struct ViewDistanceInfo;

class Tr2PerObjectData;

struct EveHazeSetLight
{
	EveHazeSetLight();
	EveHazeSetLight( const LightData& lightData, uint32_t index, const std::wstring& profilePath, bool boosterGainInfluence );

	LightData lightData;
	Tr2LightProfileResPtr lightProfile;
	uint32_t index;
	bool boosterGainInfluence;

	Matrix boneMatrix;
};

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's haze sets.
// SeeAlso:
//   EveHazeSetItem
// --------------------------------------------------------------------------------
BLUE_CLASS( EveHazeSet ) :
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public Tr2DeviceResource,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveHazeSet( IRoot* lockobj = NULL );
	~EveHazeSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	virtual void UpdateLights( const Matrix& parentTransform, const Float4x3* bones, size_t boneCount, float parentStrength, float boosterGain ) override;
	virtual void GetBatches( ITriRenderBatchAccumulator * accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL ) override;
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentTransform, const Float4x3* bones, size_t boneCount ) override;
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	void AddLightFromSOF( const EveHazeSetLight& light );

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;

	// setup
	void Setup( Tr2EffectPtr effect );

	// access items
	void AddHazeItem( EveHazeSetItemPtr item );

	// rebuild the interal vertexbuffers etc.
	void Rebuild();

private:
	bool OnPrepareResources();

	// toggle visibility
	bool m_display;
	// keep a name
	std::string m_name;

	// the list of all them haze items
	PEveHazeSetItemVector m_hazes;
	// transforms for each of the hazes
	std::vector<Matrix> m_cachedTransforms;
	// this shader renders them all
	Tr2EffectPtr m_effect;

	// has it's own vertex handle and buffer
	unsigned int m_vertexDeclHandle;
	unsigned int m_vertexCount;
	Tr2SuballocatedBuffer::Allocation m_vertexBuffer;

	// bounding box around static items
	AxisAlignedBoundingBox m_aabb;
	std::vector<std::pair<int, CcpMath::AxisAlignedBox>> m_boundingBoxes;

	std::vector<EveHazeSetLight> m_lights;
	float m_activationStrength;
	float m_boosterGain;

	void CreateBoundingBox();
};

TYPEDEF_BLUECLASS( EveHazeSet );

#endif // EveHazeSet_H
