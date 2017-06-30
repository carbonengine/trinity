////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef Tr2RenderContext_H
#define Tr2RenderContext_H

BLUE_DECLARE( Tr2RenderContext );

#include "TrinityAL/ITr2RenderContextEvents.h"
#include "Shader/Tr2EffectStateManager.h"

BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2RenderContext );
BLUE_DECLARE( Tr2RenderTarget );

// --------------------------------------------------------------------------------------
// Description:
//   Common part of Tr2RenderContext and Tr2PrimaryRenderContext. Contains 
//   Tr2EffectStateManager instance and local variable store.
// See Also:
//   Tr2RenderContext, Tr2PrimaryRenderContext
// --------------------------------------------------------------------------------------
struct Tr2RenderContextBase: public IRoot, public ITr2RenderContextEvents
{
	Tr2RenderContextBase( Tr2RenderContext& renderContext );

	void OnContextCreated( Tr2RenderContextAL& renderContext );
	void OnTextureUnset( const Tr2TextureAL& texture, Tr2RenderContextAL& renderContext );

	Tr2RenderTargetPtr GetBackBuffer();

	Tr2EffectStateManager m_esm;

	// Hints for RenderBatchesSortedByEffect method
	enum BatchesRenderHints
	{
		// Nothing special
		HINT_DEFAULT				= 0,
		// Batches effects don't have unique per-effect data,
		// so setting it can be skipped (see RenderLightBatches)
		HINT_NO_PER_EFFECT_DATA		= 1,
	};

	enum OverrideMode
	{
		OM_DO_NOTHING,				// use the override effect, but leave the PS data lone
		OM_APPLY_PS,				// use the override effect, and apply its PS data, replacing what was already there
		OM_DO_NOT_SET_ORIGINAL_PS,	// do not set original pixel shader inputs
	};

	TriVariable* GetObjectIdVariable();

	void RenderBatches( ITriRenderBatchAccumulator* batches );
	void RenderLightBatches( ITriRenderBatchAccumulator* batches );
	void RenderBatchesWithOverride( ITriRenderBatchAccumulator* batches, Tr2Material* overrideEffect, OverrideMode overrideMode );
	void RenderBatchesForPicking( Tr2Material* effect, TriRenderBatch* &p, int &objectNum );
	void RenderBatchesForPickingWithoutOverride( ITriRenderBatchAccumulator* batches, int &objectNum );

	// Render batches from an accumulator that was not set up for sorting by effect,
	// so they are rendered in whatever order they were added to the accumulator.
	// This is usually used for rendering transparent objects where the application
	// sorted by object.
	void RenderBatchesInOrder( ITriRenderBatchAccumulator* batches );

	// Render batches from an accumulator that was set up for sorting by effect.
	// This is normally used for opaque or additive batches. State settings can
	// be minimized by taking advantage of sorting that has been done.
	void RenderBatchesSortedByEffect( ITriRenderBatchAccumulator* batches, BatchesRenderHints hints = HINT_DEFAULT );

	Tr2ConstantBufferAL* GetConstantBuffer( int buffer ) { return &m_perObjectConstantBuffers[buffer]; }
protected:
	Tr2ConstantBufferAL		m_perObjectConstantBuffers[ Tr2RenderContextEnum::CBUFFER_COUNT ];
private:
	Tr2RenderTargetPtr m_backBuffer;

	TriVariable		*m_objectIdVariable;
	TriVariable		*m_areaIdVariable;
};

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed render context. Usually it is passed as a parameter. DX11 can have 
//   multiple secondary render contexts. In DX9 and OpenGL there is only one 
//   Tr2RenderContext which is the main render context.
// See Also:
//   Tr2RenderContextAL
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2RenderContext )
:	public Tr2RenderContextBase, 
	public Tr2RenderContextAL
{
public:
	Tr2RenderContext();

	EXPOSE_TO_BLUE();

	static void DestroyMainThreadRenderContext();
private:
};

TYPEDEF_BLUECLASS( Tr2RenderContext );

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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
BLUE_CLASS( Tr2PrimaryRenderContext ): public Tr2RenderContextBase, public Tr2PrimaryRenderContextAL
{
public:
	Tr2PrimaryRenderContext();

	EXPOSE_TO_BLUE();

	operator Tr2RenderContext&();
};
TYPEDEF_BLUECLASS( Tr2PrimaryRenderContext );

#else

#define	Tr2PrimaryRenderContext Tr2RenderContext

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
#define USE_MAIN_THREAD_RENDER_CONTEXT()	Tr2PrimaryRenderContext& renderContext = Tr2RenderContext_GetMainThreadRenderContext();

extern const Be::VarChooser Tr2RenderContextEnum_ConstantType_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_ClearFlags_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_DepthStencilFormat_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_TextureType_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_PixelFormat_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_ScanlineOrdering_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_DisplayScaling_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_SwapEffect_Chooser[];
extern const Be::VarChooser Tr2RenderContextEnum_PresentInterval_Chooser[];

#endif