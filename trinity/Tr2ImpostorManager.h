////////////////////////////////////////////////////////////
//
//    Created:   September 2016
//    Copyright: CCP 2016
//

#pragma once
#ifndef Tr2ImpostorManager_H
#define Tr2ImpostorManager_H

#include "Tr2QuadRenderer.h"


class TriFrustum;
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( Tr2Effect );


// --------------------------------------------------------------------------------------
// Description:
//   An interface for objects turning into impostors.
// See Also:
//   Tr2ImpostorManager
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2ImpostorSource )
{
	// Hash data for rendered impostor
	struct ImpostorHash
	{
		Vector3 viewDir;
		Vector3 upDir;
	};

	// Returns local to world transform
	virtual void GetLocalToWorldTransform( Matrix &transform ) const = 0;
	// Returns render batches to render the object into billboard
	virtual void GetImpostorBatches( const TriFrustum& frustum, std::map<TriBatchType, ITriRenderBatchAccumulator*>& batches ) = 0;
	// Evaluates rendering priority based on current and old hashes
	virtual float GetRenderPriority( const ImpostorHash& oldHash, const ImpostorHash& newHash ) const = 0;
	// Returns object bounding sphere
	virtual bool GetImpostorBoundingSphere( Vector4& sphere ) const = 0;
};


// --------------------------------------------------------------------------------------
// Description:
//   Tr2ImpostorManager class manages impostor billboards for objects. It uses quad 
//   renderer to render billboards. Its usage is roughly the following:
//   BeginUpdate()
//     Add() ... Add() ...
//   EndUpdate()
//   BeginUpdateAtlas()
//   for (i in 0 ... GetRenderQueueLength())
//     BeginImpostorUpdate()
//     render object
//     EndImpostorUpdate()
//   EndUpdateAtlas()
// See Also:
//   Tr2QuadRenderer
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ImpostorManager ): public IInitialize, public INotify, public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2ImpostorManager();

	virtual bool Initialize();

	virtual bool OnModified( Be::Var* value );

	virtual void ReleaseResources( TriStorage s );

	void Create( 
		Be::OptionalWithDefaultValue<uint32_t, 1024> width, 
		Be::OptionalWithDefaultValue<uint32_t, 1024> height, 
		Be::OptionalWithDefaultValue<uint32_t, 16> itemWidth, 
		Be::OptionalWithDefaultValue<uint32_t, 16> itemHeigh );

	void SetItemSize( uint32_t width, uint32_t height );

	bool Add( ITr2ImpostorSource* object, const ITr2ImpostorSource::ImpostorHash& hash );

	void BeginUpdate();
	void EndUpdate();

	void Reset();

	void Render() const;

	void BeginUpdateAtlas( Tr2RenderContext& renderContext );
	void EndUpdateAtlas( Tr2RenderContext& renderContext );

	ITr2ImpostorSource* BeginImpostorUpdate( size_t index, Tr2RenderContext& renderContext );
	void EndImpostorUpdate( size_t index, Tr2RenderContext& renderContext );

	size_t GetRenderQueueLength() const;

	Tr2DepthStencil* GetItemDepthStencil() const;

	size_t GetImpostorCount() const;
	
private:
	// Per-impostor data
	struct Impostor
	{
		// top-left corned of the billboard image in the atlas
		Vector2_16 texcoord;
		// current impostor hash
		ITr2ImpostorSource::ImpostorHash hash;
		// impostor hash the billboard was rendered with
		ITr2ImpostorSource::ImpostorHash oldHash;
		// computed rendering priority
		float renderPriority;
		// if the impostor is used this frame
		bool used;
		// is the impostor supposed to be re-rendered this frame
		bool render;
	};

	// Container for atlas item texcoords
	struct ImpostorAtlas
	{
		ImpostorAtlas();

		void Resize( uint32_t width, uint32_t height, uint32_t itemWidth, uint32_t itemHeight );
		bool Reserve( Vector2_16& coord );
		void Drop( Vector2_16 coord );
	private:
		// Free (unoccupied) texcoords
		TrackableStdVector<Vector2_16> m_free;
	};

	virtual bool OnPrepareResources();

	static bool CompareImpostors( const std::pair<ITr2ImpostorSource*, Impostor*>& item1, const std::pair<ITr2ImpostorSource*, Impostor*>& item2 );

	// Atlas width
	uint32_t m_width;
	// Atlas height
	uint32_t m_height;
	// Billboard width
	uint32_t m_itemWidth;
	// Billboard height
	uint32_t m_itemHeight;
	// Maximum number of re-renderings per frame
	uint32_t m_maxUpdates;

	// Atlas render target
	Tr2RenderTargetPtr m_rt;
	// Billboard render target
	Tr2RenderTargetPtr m_itemRt;
	// Billboard depth buffer
	Tr2DepthStencilPtr m_ds;

	// Effect for rendering billboards
	Tr2EffectPtr m_effect;
	// Quad renderer key for the m_effect
	Tr2QuadRenderer::EffectKey m_effectKey;
	// Billboard texcoord atlas
	ImpostorAtlas m_atlas;

	// Impostor objects
	std::unordered_map<ITr2ImpostorSource*, Impostor> m_objects;
	// Queue of objects to re-render in current frame
	std::vector<ITr2ImpostorSource*> m_renderQueue;
	
	bool m_atlasDirty;
};

TYPEDEF_BLUECLASS( Tr2ImpostorManager );

#endif
