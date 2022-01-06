////////////////////////////////////////////////////////////
//
//    Created:   August 2011
//    Copyright: CCP 2011
//
//    Refactored from EveCurveLineSet.h

#pragma once
#ifndef Tr2CurveLineSet_H
#define Tr2CurveLineSet_H

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2CurveLineSet );
BLUE_DECLARE( Tr2Material );

// -------------------------------------------------------------
// Description:
//   This class manages one or more thick lines.
//   Derived classes need to implement their own and
//   GetPerObjectData methods.
// -------------------------------------------------------------
class Tr2CurveLineSet : 
	public ITr2Renderable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public ITr2Pickable,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2CurveLineSet(IRoot* lockobj = NULL);
	~Tr2CurveLineSet();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// exposed
	bool SubmitChanges();
	// add
	unsigned int AddStraightLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, float width );
	unsigned int AddCurvedLineCrt( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& middle, float width );
	unsigned int AddCurvedLineSph( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, const Vector3& middle, float width );
	unsigned int AddSpheredLineCrt( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, float width );
	unsigned int AddSpheredLineSph( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, float width );
	// remove
	void RemoveLine( unsigned int id );
	void ClearLines();
	// modify
	void ChangeLineColor( unsigned int id, const Vector4& color1, const Vector4& color2 );
	void ChangeLinePositionCrt( unsigned int id, const Vector3& position1, const Vector3& position2 );
	void ChangeLinePositionSph( unsigned int id, const Vector3& position1, const Vector3& position2, const Vector3& center );
	void ChangeLineIntermediateCrt( unsigned int id, const Vector3& intermediatePosition );
	void ChangeLineIntermediateSph( unsigned int id, const Vector3& intermediatePosition, const Vector3& center );
	void ChangeLineWidth( unsigned int id, float width );
	void ChangeLineMultiColor( unsigned int id, const Vector4& color, float border );
	void ChangeLineAnimation( unsigned int id, const Vector4& color, float speed, float scale );
	void ChangeLineSegmentation( unsigned int id, unsigned int numOfSegments );

	void SetLineEffect( Tr2Material* shader )
	{
		m_lineEffect = shader;
	}

	void SetPickEffect( Tr2Material* shader )
	{
		m_pickEffect = shader;
	}

	void SetAdditiveFlag( bool b );
	void SetDynamicFlag( bool b );

protected:
	// line types (straight, curved, sphered, ...)
	enum LineType
	{
		LINETYPE_INVALID = 0,
		LINETYPE_STRAIGHT,
		LINETYPE_SPHERED,
		LINETYPE_CURVED,
		LINETYPE_PARTICLE,
	};

	// base incoming line data
	struct LineData
	{
		LineType type;					// straight, curved, etc.
		Vector3 position1;				// start position
		Color color1;					// color at start
		Vector3 position2;				// end position
		Color color2;					// color at end
		Vector3 intermediatePosition;	// special position for cruved lines: gives "curve"
		float width;					// width of lin in screen pixels
		Color multiColor;				// "override" color
		float multiColorBorder;			// "override" color until here
		Color overlayColor;				// color of the overlay animation layer
		float animationSpeed;			// speed of animation crawling along line
		float animationScale;			// scale of texture along line
		unsigned int numOfSegments;		// number of straight lines forming a curved line
	};

	// line draw shader
	Tr2MaterialPtr m_lineEffect;

	// line picking shader
	Tr2MaterialPtr m_pickEffect;

	// transforms of this set
	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;
	
	// name
	std::string m_name;

	bool m_display;

	// resulting transform
	Matrix m_worldTransform;

	// bounding sphere
	Vector4 m_boundingSphere;

	// bounding box
	Vector3 m_minBounds;
	Vector3 m_maxBounds;

	// indicates if line bounds have changed
	bool m_boundsDirty;
	
	unsigned int m_currentSubmittedLineCount;
private:

	// vertex
	struct LineVertex
	{
		Vector3 position;				// position in 3D
		Vector4 lineDir;				// [xyz] = offset to next/previous line segment, [w] = width (incl. -1.f to indicate direction)
		Vector4 beginEnd;				// [x] = 0 or 1 for begin/end, [y] = uniform scalar from 0 to 1 across length of line segment, [z] = override color border, [w] = length of vert in uniform scalar
		Vector3 animationData;			// [x] = animation speed, [y] = animation scale, [z] = lineID
		Vector3 nextLineDir;			// one after next/previous line segment position
		unsigned int color;				// [xyzw] = main color
		unsigned int overrideColor;		// [xyzw] = override color
		unsigned int overlayColor;		// [xyzw] = overlay color
	};

	// Helper function to allocate batches and set the correct effect
	void GetBatchImpl( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect );

	// check
	bool isValidLineID( unsigned int id ) const;
	// add a line segment to internal list
	unsigned int addLineData( const LineData* lineData );
	// calculate the amount of lines needed for the current linedata
	unsigned int GetNumOfLines() const;
	// fill vertex buffer
	bool FillVertexBuffer();
	// helper
	void WriteLineVerticesToBuffer( 
		const Vector3& pos1, 
		const Color& col1, 
		float length1, 
		const Vector3& pos2, 
		const Color& col2, 
		float length2, 
		const Vector3& posPrev, 
		const Vector3& posNext, 
		unsigned int lineID, 
		LineVertex* buffer );
	void WriteParticleVerticesToBuffer( const Vector3& pos1, const Color& col1, float length1, const Vector3& pos2, const Color& col2, float length2, unsigned int lineID, LineVertex* buffer );

	bool m_additive;
	bool m_dynamic;

	// line properties
	float m_lineWidthFactor;

	// vertex buffer
	unsigned int m_vertexDeclHandle;
	Tr2BufferAL m_vertexBuffer;
	int m_vertexBufferSize;

	// all the lines
	std::vector<LineData> m_lines;

	// empty line ids
	std::vector<unsigned int> m_emptyLineID;

	// Depth offset for transparency sorting
	float m_depthOffset;
};

TYPEDEF_BLUECLASS( Tr2CurveLineSet );
BLUE_DECLARE_VECTOR( Tr2CurveLineSet );

#endif
