#pragma once
#ifndef ITr2PickableScene_H
#define ITr2PickableScene_H

// Forward declarations
struct ITr2Renderable;
class TriProjection;
class ITriRenderBatchAccumulator;
class TriView;
class TriViewport;
class Tr2PickBuffer;
BLUE_DECLARE_INTERFACE( ITr2Renderable );
BLUE_DECLARE( Tr2Material );

// --------------------------------------------------------------------------------------
// Description:
//   ITr2PickableScene is an interface for scenes that support GPU mouse picking. The 
//   interface has a few defined function helpers to do the actual work associated with
//   picking. ITr2PickableScene descendants only need to implement a few abstract
//   scene-dependant functions.
// See Also:
//   Tr2InteriorScene, Tr2PrimitiveScene
// --------------------------------------------------------------------------------------
struct ITr2PickableScene
{
public:
	// -------------------------------------------------------------
	// Description:
	//   Components reported for picking.
	// -------------------------------------------------------------
	enum PickComponent
	{
		// Object pointer (IRoot)
		PICK_OBJECT		= 1,
		// Mesh and area indexes
		PICK_AREA		= 2,
		// World position
		PICK_POSITION	= 4,
		// Object UV coordinates
		PICK_UV			= 8,
	};
	// Union of PickComponent
	typedef unsigned int PickComponents;

	// -------------------------------------------------------------
	// Description:
	//   Picking results.
	// -------------------------------------------------------------
	struct PickResults
	{
		// Bit field indicating which
		// components are valid
		PickComponents components;

		// "Picked" object (or NULL if no intersection)
		IRoot* object;
		// Area ID + 1 (lower 8 bits) and mesh ID
		unsigned int area;
		// World position
		Vector3 position;
		// Object texture UV coordinates
		Vector2 uv;
	};

	void PickObject( Tr2RenderContext& renderContext, int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, PickResults& results );
protected:
	// -------------------------------------------------------------
	// Description:
	//   Data to be decoded from picking buffer.
	// -------------------------------------------------------------
	struct BufferResults
	{
		// Object ID
		unsigned int objectId;
		// Mesh and area ID
		unsigned int areaId;
		// Depth (disance to the camera)
		float depth;
		// Object texture UV coordinates
		Vector2 uv;
	};

	// Maximum number of picking passes
	static const unsigned int MAX_PICK_PASSES = 4;
	
    typedef std::vector<ITr2Renderable*> ITr2RenderableArray;
    typedef unsigned short ObjectIdType;

    virtual void SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport );

	// -------------------------------------------------------------
	// Description:
	//   Sets per-frame data before picking rendering begins.
	// -------------------------------------------------------------
	virtual void SetPerFrameDataForPicking( Tr2RenderContext& renderContext ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns picking object candidates that need to be rendered
	//   into the picking buffer.
	// Arguments:
	//   dirWorld  - Picking ray direction (in world space)
	// Return Value:
	//   Array of picking object candidates
	// -------------------------------------------------------------
	virtual const ITr2RenderableArray& GetPickingObjectsToRender( const Vector3& dirWorld ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns picking object candidates that need to be rendered
	//   into the picking buffer.
	// Arguments:
	//   dirWorld  - Picking frusum direction (in world space)
	//   fov  - FOV of picking frustum
	//   aspect  - aspect ratio of picking frustum
	// Return Value:
	//   Array of picking object candidates
	// -------------------------------------------------------------
	virtual const ITr2RenderableArray& GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns a render batch accumulator to use for opaque object
	//   areas.
	// Return Value:
	//   Opaque render batch accumulator
	// -------------------------------------------------------------
	virtual ITriRenderBatchAccumulator* GetOpaquePickingBatchAccumulator( void ) = 0;
	
	// -------------------------------------------------------------
	// Description:
	//   Returns a render batch accumulator to use for picking object
	//   areas (for transparent objects).
	// Return Value:
	//   Picking render batch accumulator
	// -------------------------------------------------------------
	virtual ITriRenderBatchAccumulator* GetPickingBatchAccumulator( void ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Returns an array of passes that need to be rendered in order
	//   to get all picking components. 
	// Arguments:
	//   requestedComponents  - Components requested for picking operation
	//                          (union of PickComponent).
	//   passes - (out) Array components actually rendered during each
	//            picking pass (union of PickComponent). Maximum number
	//            of passes is MAX_PICK_PASSES.
	// Return Value:
	//   Number of passes required to query all requested picking
	//   components.
	// -------------------------------------------------------------
	virtual unsigned int GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes ) = 0;

	// -------------------------------------------------------------
	// Description:
	//   Checks if the picking areas need to be rendered during the
	//   given pass. 
	// Arguments:
	//   pass  - union of PickComponent that are to be queried during
	//		this pass.
	// Return Value:
	//   true If picking batches need to be rendered during the given
	//		pass.
	//   false Otherwise.
	// -------------------------------------------------------------
	virtual bool RenderPickingAreasForComponents( PickComponents pass ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Decodes results in the picking buffer after the given pass. 
	// Arguments:
	//   pBuffer - contents of the picking buffer.
	//   pass  - union of PickComponent that are to be queried during
	//		this pass.
	//   results  - (out) results decoded from the buffer (the 
	//		function only needs to set fields turned on in the pass
	//		argument).
	// -------------------------------------------------------------
	virtual void DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const = 0;


	// -------------------------------------------------------------
	// Description:
	//   Returns picking buffer to use for picking. 
	// Return Value:
	//   Picking buffer to use for picking
	// -------------------------------------------------------------
	virtual Tr2PickBuffer& GetPickBuffer( void ) = 0;

	virtual bool RenderPicking( ITriRenderBatchAccumulator* pOpaquePickingBatches,
						ITriRenderBatchAccumulator* pPickingBatches,
						PickComponents pass );
private:
    // Do-all combo method. If pickedUV is not none, this switches to UV 
    // picking and pickedObject and area are no longer filled in.
    void PickObject( int x, int y, TriProjection* proj, TriView* view, 
                     TriViewport* viewport, IRoot** pickedObject, 
                     Vector3* outWorldPosition, unsigned int* outArea, 
                     Vector2* pickedUV );

	void GetBatches( std::vector<ITr2Renderable*> const& pickableObjects,
					 ITriRenderBatchAccumulator*& pOpaquePickingBatches,
					 ITriRenderBatchAccumulator*& pPickingBatches );
};

#endif // ITr2PickableScene_H
