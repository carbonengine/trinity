// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2RenderContext_H
#define Tr2RenderContext_H

BLUE_DECLARE( Tr2RenderContext );

#include "../trinityal/ITr2RenderContextEvents.h"
#include "Shader/Tr2EffectStateManager.h"
#include "Shader/Tr2EffectDescription.h"

BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2RenderContext );
BLUE_DECLARE( Tr2RenderTarget );
struct Tr2RenderBatch;

// --------------------------------------------------------------------------------------
// Description:
//   Common part of Tr2RenderContext and Tr2PrimaryRenderContext. Contains
//   Tr2EffectStateManager instance and local variable store.
// See Also:
//   Tr2RenderContext, Tr2PrimaryRenderContext
// --------------------------------------------------------------------------------------
struct Tr2RenderContextBase : public IRoot, public ITr2RenderContextEvents
{
	Tr2RenderContextBase( Tr2RenderContext& renderContext );

	void OnContextCreated( Tr2PrimaryRenderContextAL& renderContext );

#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	Tr2RenderTargetPtr GetBackBuffer();
#endif

	Tr2EffectStateManager m_esm;

	TriVariable* GetObjectIdVariable();

	void RenderBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName = DEFAULT_TECHNIQUE );
	void RenderBatchesWithOverride( ITriRenderBatchAccumulator* batches, Tr2Material* overrideEffect, const BlueSharedString& techniqueName = DEFAULT_TECHNIQUE );
	void RenderBatchesForPicking( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName );

	// Render batches from an accumulator that was not set up for sorting by effect,
	// so they are rendered in whatever order they were added to the accumulator.
	// This is usually used for rendering transparent objects where the application
	// sorted by object.
	void RenderBatchesInOrder( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName = DEFAULT_TECHNIQUE );

	// Render batches from an accumulator that was set up for sorting by effect.
	// This is normally used for opaque or additive batches. State settings can
	// be minimized by taking advantage of sorting that has been done.
	void RenderBatchesSortedByEffect( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName = DEFAULT_TECHNIQUE );

	static bool TechniqueInBatch( const std::vector<Tr2RenderBatch>& batches, const BlueSharedString& techniqueName = DEFAULT_TECHNIQUE );

	Tr2ConstantBufferAL* GetConstantBuffer( int buffer )
	{
		return &m_perObjectConstantBuffers[buffer];
	}

protected:
	Tr2ConstantBufferAL m_perObjectConstantBuffers[Tr2RenderContextEnum::CBUFFER_COUNT];

private:
	void RenderGdprBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName );
	void RenderBatchGroup( std::vector<Tr2RenderBatch>::const_iterator startBatch, const BlueSharedString& techniqueName, Tr2ConstantBufferAL** buffers, Tr2RenderContext& renderContext );
	void RenderSortedBatches( const std::vector<Tr2RenderBatch>& batches, const BlueSharedString& techniqueName, Tr2RenderContext& renderContext );


#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	Tr2RenderTargetPtr m_backBuffer;
#endif
	TriVariable* m_objectIdVariable;
	TriVariable* m_areaIdVariable;
};

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed render context. Usually it is passed as a parameter. DX11 can have
//   multiple secondary render contexts. In DX9 and OpenGL there is only one
//   Tr2RenderContext which is the main render context.
// See Also:
//   Tr2RenderContextAL
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2RenderContext ) :
	public Tr2RenderContextBase,
	public Tr2RenderContextAL
{
public:
	Tr2RenderContext();

	EXPOSE_TO_BLUE();

	static void DestroyMainThreadRenderContext();

	uint32_t BeginParallelEncoding( uint32_t requestedEncodersCount );
	Tr2RenderContext* Fork();
	void Join( Tr2RenderContext * context );
	void EndParallelEncoding();

private:
	void PrepareParallelContext( uint32_t index, Tr2RenderContext& context );

	// persistent list of created parallel contexts
	std::vector<Tr2RenderContextPtr> m_parallelContexts;
	// pool of available parallel contexts (is reset every time Begin/End parallel rendering is called)
	std::vector<Tr2RenderContextPtr> m_parallelContextsPool;
	CcpMutex m_parallelContextMutex;
	CcpSemaphore m_parallelContextSemaphore;
};

TYPEDEF_BLUECLASS( Tr2RenderContext );

#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT

// --------------------------------------------------------------------------------------
// Description:
//   DX11-only blue-exposed primary render context. Primary render context is used for
//   creating GPU resources. There is only one primary render context. The way to get it
//   is through USE_MAIN_THREAD_RENDER_CONTEXT() macro.
//   This class contains a casting operator to Tr2RenderContext (which does a plain
//   C-style cast). In order for it to work Tr2PrimaryRenderContext and Tr2RenderContext
//   should be identical except for the last base class.
// See Also:
//   Tr2RenderContext, Tr2PrimaryRenderContextAL
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2PrimaryRenderContext ) :
	public Tr2RenderContextBase, public Tr2PrimaryRenderContextAL
{
public:
	Tr2PrimaryRenderContext();

	EXPOSE_TO_BLUE();

	void OnContextCreated( Tr2PrimaryRenderContextAL & renderContext );
	operator Tr2RenderContext&();

	Tr2RenderTargetPtr GetBackBuffer();

private:
	Tr2RenderTargetPtr m_backBuffer;
};
TYPEDEF_BLUECLASS( Tr2PrimaryRenderContext );

#else

#define Tr2PrimaryRenderContext Tr2RenderContext

#endif


// Helper function to access the main thread primary rendering context from anywhere in the code.
// If you're tempted to use this, then...
// - make sure you don't have access to a renderContext from higher up the callstack, and if you
//   do, pass it down until you can use that one;
// - use the USE_MAIN_THREAD_RENDER_CONTEXT() macro instead, so it clearly indicates a problem spot and
//   the fact that your code will not be multi-threaded-rendering-ready;
Tr2PrimaryRenderContext& Tr2RenderContext_GetMainThreadRenderContext();

// This macro defines a renderContext variable that points to GetMainThreadRenderContext; see that
// function for details.
// The name 'renderContext' is intentionally looking like a local, not the global that it really points at,
// to make it easy to upgrade the code later on to take a Tr2RenderContext& renderContext by parameter.
#define USE_MAIN_THREAD_RENDER_CONTEXT() Tr2PrimaryRenderContext& renderContext = Tr2RenderContext_GetMainThreadRenderContext();

extern const Be::VarChooser Tr2RenderContextEnum_ConstantType_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_ClearFlags_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_DepthStencilFormat_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_TextureType_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_PixelFormat_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_ScanlineOrdering_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_DisplayScaling_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_SwapEffect_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_PresentInterval_Chooser[];

extern bool g_renderContextIsBeingDestroyed;

#endif
