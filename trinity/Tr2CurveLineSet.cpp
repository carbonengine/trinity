// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveLineSet.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/BoundingSphere.h"
#include "TriRenderBatch.h"
#include "Tr2Renderer.h"


using namespace Tr2RenderContextEnum;

static const char* CURVE_LINE_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3D.fx";
static const char* CURVE_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3DPicking.fx";

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

Tr2CurveLineSet::Tr2CurveLineSet( IRoot* lockobj ) :
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_translation( 0.f, 0.f, 0.f ),
	m_additive( false ),
	m_lineWidthFactor( 1.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_currentSubmittedLineCount( 0 ),
	m_vertexBufferSize( 0 ),
	m_display( true ),
	m_dynamic( false ),
	m_boundsDirty( false ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_depthOffset( 0.f )
{
	m_lines.reserve( 100 );
	PrepareResources();
}

Tr2CurveLineSet::~Tr2CurveLineSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// -------------------------------------------------------------
// Description:
//   Implements INotify method. Is called when exposed member
//   variable is modified. If line width factor is modified -
//   recreate the line set.
// Arguments:
//   val - Value being modified
// -------------------------------------------------------------
bool Tr2CurveLineSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_lineWidthFactor ) )
	{
		// just re-fill the vertexbuffer
		FillVertexBuffer();
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Releases vertex buffer
//   and vertex declaration.
// Arguments:
//   s - Type of resources to release
// -------------------------------------------------------------
void Tr2CurveLineSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}

// -------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Creates vertex buffer
//   and vertex declaration.
// -------------------------------------------------------------
bool Tr2CurveLineSet::OnPrepareResources()
{
	// create vertex decl
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		static Tr2VertexDefinition s_curveLineDataVertexDecl;
		if( s_curveLineDataVertexDecl.empty() )
		{
			Tr2VertexDefinition& tvd = s_curveLineDataVertexDecl;

			tvd.Add( tvd.FLOAT32_3, tvd.POSITION );

			tvd.Add( tvd.FLOAT32_4, tvd.TEXCOORD, 0 );
			tvd.Add( tvd.FLOAT32_4, tvd.TEXCOORD, 1 );
			tvd.Add( tvd.FLOAT32_3, tvd.TEXCOORD, 2 );
			tvd.Add( tvd.FLOAT32_3, tvd.TEXCOORD, 3 );

			tvd.Add( tvd.UBYTE_4_NORM, tvd.COLOR, 0 );
			tvd.Add( tvd.UBYTE_4_NORM, tvd.COLOR, 1 );
			tvd.Add( tvd.UBYTE_4_NORM, tvd.COLOR, 2 );
		}

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_curveLineDataVertexDecl );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	// always fill it (this will also create a vertexbuffer)
	return FillVertexBuffer();
}

// -------------------------------------------------------------
// Description:
//   Returns number of line segments in this line set.
// Return value:
//   Total number of line segments in this line set.
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::GetNumOfLines() const
{
	// must iterate over complete list, cause each part of the list can contain different line numbers
	unsigned int res = 0;
	for( std::vector<LineData>::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it )
	{
		if( it->type != LINETYPE_INVALID )
		{
			res += it->numOfSegments;
		}
	}
	return res;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Performs several checks to see if this line segment ID is valid. First check for array-out-of-
//   bounds problems, then check if the type is right.
// Arguments:
//   id - the index into the list
// ------------------------------------------------------------------------------------------------------
bool Tr2CurveLineSet::isValidLineID( unsigned int id ) const
{
	// out of bounds?
	if( id >= m_lines.size() )
	{
		return false;
	}
	// not empty slot?
	if( m_lines[id].type == LINETYPE_INVALID )
	{
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Helper function to convert RGBA color to BGRA.
// Attributes:
//   color - color in RGBA format
// Return value:
//   color in ARGB
// ------------------------------------------------------------------------------------------------------
inline unsigned SwizzleColor( unsigned color )
{
	return ( ( color & 0xff0000 ) >> 16 ) | ( color & 0xff00ff00 ) | ( ( color & 0xff ) << 16 );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Populate vertex buffer with line data.
// Attributes:
//   pos1 - start position of line segment
//   col1 - color at start of line segment
//   length1 - how far along the line does this segment start
//   pos2 - end position of segment
//   col2 - end color of segment
//   length2 - ow far along the line does this segment end
//   lineID - the line's id
//   posPrev - start position of the previous segment
//   posNext - end position of the next segment
//   buffer - the vertex buffer
// ------------------------------------------------------------------------------------------------------
void Tr2CurveLineSet::WriteLineVerticesToBuffer(
	const Vector3& pos1,
	const Color& col1,
	float length1,
	const Vector3& pos2,
	const Color& col2,
	float length2,
	const Vector3& posPrev,
	const Vector3& posNext,
	unsigned int lineID,
	LineVertex* buffer )
{
	// line info
	const LineData* lineData = &m_lines[lineID];

	// direction position hint for the vertexshader
	Vector3 dirOffset = pos2 - pos1;

	uint32_t col1Swizzled = SwizzleColor( col1 );
	uint32_t col2Swizzled = SwizzleColor( col2 );
	uint32_t multiColorSwizzled = SwizzleColor( lineData->multiColor );
	uint32_t overlayColorSwizzled = SwizzleColor( lineData->overlayColor );

	Vector3 animationData( lineData->animationSpeed, lineData->animationScale, (float)lineID );

	// tri0 - v0
	LineVertex& v0 = buffer[0];
	v0.position = pos1;
	v0.lineDir = Vector4( dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	v0.beginEnd = Vector4( 0.f, length1, lineData->multiColorBorder, length2 - length1 );
	v0.nextLineDir = posPrev;
	v0.color = col1Swizzled;
	v0.overrideColor = multiColorSwizzled;
	v0.overlayColor = overlayColorSwizzled;
	v0.animationData = animationData;
	// tri0 - v1
	LineVertex& v1 = buffer[1];
	v1.position = pos1;
	v1.lineDir = Vector4( dirOffset, m_lineWidthFactor * lineData->width );
	v1.beginEnd = Vector4( 0.f, length1, lineData->multiColorBorder, length2 - length1 );
	v1.nextLineDir = posPrev;
	v1.color = col1Swizzled;
	v1.overrideColor = multiColorSwizzled;
	v1.overlayColor = overlayColorSwizzled;
	v1.animationData = animationData;
	// tri0 - v2
	LineVertex& v2 = buffer[2];
	v2.position = pos2;
	v2.lineDir = Vector4( -1.f * dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	v2.beginEnd = Vector4( 1.f, length2, lineData->multiColorBorder, length2 - length1 );
	v2.nextLineDir = posNext;
	v2.color = col2Swizzled;
	v2.overrideColor = multiColorSwizzled;
	v2.overlayColor = overlayColorSwizzled;
	v2.animationData = animationData;
	// tri1 - v0
	LineVertex& v3 = buffer[3];
	v3.position = pos1;
	v3.lineDir = Vector4( dirOffset, m_lineWidthFactor * lineData->width );
	v3.beginEnd = Vector4( 0.f, length1, lineData->multiColorBorder, length2 - length1 );
	v3.nextLineDir = posPrev;
	v3.color = col1Swizzled;
	v3.overrideColor = multiColorSwizzled;
	v3.overlayColor = overlayColorSwizzled;
	v3.animationData = animationData;
	// tri2 - v1
	LineVertex& v4 = buffer[4];
	v4.position = pos2;
	v4.lineDir = Vector4( -1.f * dirOffset, m_lineWidthFactor * lineData->width );
	v4.beginEnd = Vector4( 1.f, length2, lineData->multiColorBorder, length2 - length1 );
	v4.nextLineDir = posNext;
	v4.color = col2Swizzled;
	v4.overrideColor = multiColorSwizzled;
	v4.overlayColor = overlayColorSwizzled;
	v4.animationData = animationData;
	// tri2 - v2
	LineVertex& v5 = buffer[5];
	v5.position = pos2;
	v5.lineDir = Vector4( -1.f * dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	v5.beginEnd = Vector4( 1.f, length2, lineData->multiColorBorder, length2 - length1 );
	v5.nextLineDir = posNext;
	v5.color = col2Swizzled;
	v5.overrideColor = multiColorSwizzled;
	v5.overlayColor = overlayColorSwizzled;
	v5.animationData = animationData;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Populate vertex buffer with particle line data.
// Attributes:
//   pos1 - start position of line segment
//   col1 - color at start of line segment
//   length1 - how far along the line does this segment start
//   pos2 - end position of segment
//   col2 - end color of segment
//   length2 - ow far along the line does this segment end
//   lineID - the line's id
//   buffer - the vertex buffer
// ------------------------------------------------------------------------------------------------------
void Tr2CurveLineSet::WriteParticleVerticesToBuffer(
	const Vector3& pos1,
	const Color& col1,
	float length1,
	const Vector3& pos2,
	const Color& col2,
	float length2,
	unsigned int lineID,
	LineVertex* buffer )
{
	// line info
	const LineData* lineData = &m_lines[lineID];

	// direction position hint for the vertexshader
	Vector3 dirOffset = pos2 - pos1;

	// random
	float randomVal = (float)rand() / (float)RAND_MAX;

	// tri0 - v0
	buffer[0].position = pos1;
	buffer[0].lineDir = Vector4( dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	buffer[0].beginEnd = Vector4( -1.f * m_lineWidthFactor * lineData->width, length1, randomVal, length2 - length1 );
	buffer[0].color = SwizzleColor( col1 );
	buffer[0].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[0].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[0].animationData = lineData->intermediatePosition;
	// tri0 - v1
	buffer[1].position = pos1;
	buffer[1].lineDir = Vector4( dirOffset, m_lineWidthFactor * lineData->width );
	buffer[1].beginEnd = Vector4( -1.f * m_lineWidthFactor * lineData->width, length1, randomVal, length2 - length1 );
	buffer[1].color = SwizzleColor( col1 );
	buffer[1].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[1].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[1].animationData = lineData->intermediatePosition;
	// tri0 - v2
	buffer[2].position = pos1;
	buffer[2].lineDir = Vector4( dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	buffer[2].beginEnd = Vector4( m_lineWidthFactor * lineData->width, length2, randomVal, length2 - length1 );
	buffer[2].color = SwizzleColor( col2 );
	buffer[2].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[2].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[2].animationData = lineData->intermediatePosition;
	// tri1 - v0
	buffer[3].position = pos1;
	buffer[3].lineDir = Vector4( dirOffset, m_lineWidthFactor * lineData->width );
	buffer[3].beginEnd = Vector4( -1.f * m_lineWidthFactor * lineData->width, length1, randomVal, length2 - length1 );
	buffer[3].color = col1;
	buffer[3].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[3].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[3].animationData = lineData->intermediatePosition;
	// tri2 - v1
	buffer[4].position = pos1;
	buffer[4].lineDir = Vector4( dirOffset, m_lineWidthFactor * lineData->width );
	buffer[4].beginEnd = Vector4( m_lineWidthFactor * lineData->width, length2, randomVal, length2 - length1 );
	buffer[4].color = SwizzleColor( col2 );
	buffer[4].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[4].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[4].animationData = lineData->intermediatePosition;
	// tri2 - v2
	buffer[5].position = pos1;
	buffer[5].lineDir = Vector4( dirOffset, -1.f * m_lineWidthFactor * lineData->width );
	buffer[5].beginEnd = Vector4( m_lineWidthFactor * lineData->width, length2, randomVal, length2 - length1 );
	buffer[5].color = SwizzleColor( col2 );
	buffer[5].overrideColor = SwizzleColor( lineData->multiColor );
	buffer[5].overlayColor = SwizzleColor( lineData->overlayColor );
	buffer[5].animationData = lineData->intermediatePosition;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Populate line set's vertex buffer with line data.
// Return value:
//   Boolean value indicating success of vertex buffer creation.
// ------------------------------------------------------------------------------------------------------
bool Tr2CurveLineSet::FillVertexBuffer()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// collect this while looping through the list
	m_currentSubmittedLineCount = 0;

	// build bounding sphere while looping through list
	BoundingSphereInitialize( m_boundingSphere );
	BoundingBoxInitialize( m_minBounds, m_maxBounds );
	m_boundsDirty = true;

	// catch zero size, so we don't need to worry about empty arrays
	if( m_lines.size() )
	{
		// can the big vertexbuffer handle this?
		int currentNumOfLines = GetNumOfLines();
		if( !m_vertexBuffer.IsValid() || ( currentNumOfLines > m_vertexBufferSize ) )
		{
			// release old and create new
			USE_MAIN_THREAD_RENDER_CONTEXT();

			Tr2CpuUsage::Type cpuUsage = m_dynamic ? Tr2CpuUsage::WRITE_OFTEN : Tr2CpuUsage::WRITE;

			CR_RETURN_VAL(
				m_vertexBuffer.Create(
					sizeof( LineVertex ),
					currentNumOfLines * 6,
					Tr2GpuUsage::VERTEX_BUFFER,
					cpuUsage,
					nullptr,
					renderContext ),
				false );
			m_vertexBufferSize = currentNumOfLines;
		}

		size_t byteSize = m_vertexBufferSize * 6 * sizeof( LineVertex );
		auto buffer = Tr2Renderer::GetPoolAllocator()->Allocate( byteSize );
		LineVertex* vertexBuffer = reinterpret_cast<LineVertex*>( buffer );

		for( unsigned int i = 0; i < m_lines.size(); ++i )
		{
			// what type of line?
			switch( m_lines[i].type )
			{
			case LINETYPE_INVALID:
				break;

			case LINETYPE_STRAIGHT: {
				// put some verts into buffer
				WriteLineVerticesToBuffer(
					m_lines[i].position1,
					m_lines[i].color1,
					0.f,
					m_lines[i].position2,
					m_lines[i].color2,
					1.f,
					m_lines[i].position1 - ( m_lines[i].position2 - m_lines[i].position1 ),
					m_lines[i].position2 + ( m_lines[i].position2 - m_lines[i].position1 ),
					i,
					vertexBuffer );
				vertexBuffer += 6;

				// add to bounding sphere
				BoundingSphereUpdate( m_lines[i].position1, m_boundingSphere );
				BoundingSphereUpdate( m_lines[i].position2, m_boundingSphere );
				// add to bounding box
				BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].position1 );
				BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].position2 );

				// one straight line more
				m_currentSubmittedLineCount++;
			}
			break;

			case LINETYPE_SPHERED: {
				// create rotation matrix to rotate each segment further to target vector
				Vector3 rotationAxis, startDirNrm, endDirNrm;
				// directional vectors
				Vector3 startDir = m_lines[i].position1 - m_lines[i].intermediatePosition;
				Vector3 endDir = m_lines[i].position2 - m_lines[i].intermediatePosition;
				startDirNrm = Normalize( startDir );
				endDirNrm = Normalize( endDir );
				// matrix
				Matrix rotationMatrix;
				rotationAxis = Cross( startDir, endDir );
				float fullAngle = acosf( Dot( startDirNrm, endDirNrm ) );
				float segmentAngle = fullAngle / (float)m_lines[i].numOfSegments;

				// run through all segmens and create lines
				rotationMatrix = RotationMatrix( rotationAxis, -segmentAngle );

				Vector3 dir0 = TransformNormal( startDir, rotationMatrix );

				rotationMatrix = RotationMatrix( rotationAxis, segmentAngle );

				Vector3 dir1 = startDir;
				Vector3 dir2 = TransformNormal( dir1, rotationMatrix );
				;
				Vector3 dir3 = startDir;
				// also interpolate color across all the segments
				Color col1 = m_lines[i].color1;
				Color col2 = m_lines[i].color2;
				for( unsigned int s = 0; s < m_lines[i].numOfSegments; ++s )
				{
					float segmentFactor = (float)( s + 1 ) / (float)m_lines[i].numOfSegments;

					// rotate end dir of this segment
					dir3 = TransformNormal( dir2, rotationMatrix );

					// interpolate color
					col2 = Lerp( m_lines[i].color1, m_lines[i].color2, segmentFactor );

					// put some verts into buffer
					WriteLineVerticesToBuffer(
						dir1 + m_lines[i].intermediatePosition,
						col1,
						(float)s / (float)m_lines[i].numOfSegments,
						dir2 + m_lines[i].intermediatePosition,
						col2,
						segmentFactor,
						dir0 + m_lines[i].intermediatePosition,
						dir3 + m_lines[i].intermediatePosition,
						i,
						vertexBuffer );

					vertexBuffer += 6;

					// add to bounding sphere
					BoundingSphereUpdate( dir1 + m_lines[i].intermediatePosition, m_boundingSphere );
					BoundingSphereUpdate( dir2 + m_lines[i].intermediatePosition, m_boundingSphere );
					// add to bounding box
					BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].intermediatePosition );
					BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].intermediatePosition );

					// next segment
					dir0 = dir1;
					dir1 = dir2;
					dir2 = dir3;
					col1 = col2;
				}

				// several straight lines more
				m_currentSubmittedLineCount += m_lines[i].numOfSegments;
			}
			break;

			case LINETYPE_CURVED: {
				// use intermediate point to generate tangents for hermite spline
				Vector3 tangent1 = m_lines[i].intermediatePosition - m_lines[i].position1;
				Vector3 tangent2 = m_lines[i].intermediatePosition - m_lines[i].position2;
				tangent2 *= -1.f;
				// run through all segmens and create lines
				Vector3 pos0 = Hermite( m_lines[i].position1, tangent1, m_lines[i].position2, tangent2, -1.0f / m_lines[i].numOfSegments );
				Vector3 pos1 = m_lines[i].position1;
				Vector3 pos2 = Hermite( m_lines[i].position1, tangent1, m_lines[i].position2, tangent2, 1.0f / m_lines[i].numOfSegments );
				Vector3 pos3 = m_lines[i].position1;
				// also interpolate color across all the segments
				Color col1 = m_lines[i].color1;
				Color col2 = m_lines[i].color2;
				for( unsigned int s = 0; s < m_lines[i].numOfSegments; ++s )
				{
					float segmentFactor = (float)( s + 1 ) / (float)m_lines[i].numOfSegments;
					float segmentFactor2 = (float)( s + 2 ) / (float)m_lines[i].numOfSegments;
					pos3 = Hermite( m_lines[i].position1, tangent1, m_lines[i].position2, tangent2, segmentFactor2 );

					// interpolate color
					col2 = Lerp( m_lines[i].color1, m_lines[i].color2, segmentFactor );

					// put some verts into buffer
					WriteLineVerticesToBuffer(
						pos1,
						col1,
						(float)s / (float)m_lines[i].numOfSegments,
						pos2,
						col2,
						segmentFactor,
						pos0,
						pos3,
						i,
						vertexBuffer );
					vertexBuffer += 6;

					// add to bounding sphere
					BoundingSphereUpdate( pos1, m_boundingSphere );
					BoundingSphereUpdate( pos2, m_boundingSphere );
					// add to bounding box
					BoundingBoxUpdate( m_minBounds, m_maxBounds, pos1 );
					BoundingBoxUpdate( m_minBounds, m_maxBounds, pos2 );

					// next segment
					pos0 = pos1;
					pos1 = pos2;
					pos2 = pos3;
					col1 = col2;
				}

				// several straight lines more
				m_currentSubmittedLineCount += m_lines[i].numOfSegments;
			}
			break;

			case LINETYPE_PARTICLE: {
				// just add segements as particles
				for( unsigned int s = 0; s < m_lines[i].numOfSegments; ++s )
				{
					// length
					float segmentFactor = (float)s / (float)m_lines[i].numOfSegments;

					// put some verts into buffer
					WriteParticleVerticesToBuffer( m_lines[i].position1, m_lines[i].color1, segmentFactor, m_lines[i].position2, m_lines[i].color2, segmentFactor, i, vertexBuffer );
					vertexBuffer += 6;
				}

				// add to bounding sphere
				BoundingSphereUpdate( m_lines[i].position1, m_boundingSphere );
				BoundingSphereUpdate( m_lines[i].position2, m_boundingSphere );
				// add to bounding box
				BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].position1 );
				BoundingBoxUpdate( m_minBounds, m_maxBounds, m_lines[i].position2 );

				// several straight lines more
				m_currentSubmittedLineCount += m_lines[i].numOfSegments;
			}
			break;
			}
		}

		// lock and fill it
		CR_RETURN_VAL( m_vertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );

		memcpy( vertexBuffer, buffer, byteSize );

		m_vertexBuffer.UnmapForWriting( renderContext );
	}

	return true;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Implements ITr2Renderable interface.
//  Return Value:
//    Always true.
// --------------------------------------------------------------------------------------
bool Tr2CurveLineSet::HasTransparentBatches()
{
	return true;
}

// -------------------------------------------------------------
// Description:
//   Creates render batches for line set.
// Arguments:
//   batches - Render batch accumulator
//   batchType - Type of batches to gather
//   data - Per-object data
// -------------------------------------------------------------
void Tr2CurveLineSet::GetBatches( ITriRenderBatchAccumulator* accumulator,
								  TriBatchType batchType,
								  const Tr2PerObjectData* perObjectData,
								  Tr2RenderReason reason )
{
	if( !m_display )
	{
		return;
	}

	// If the lineset is not marked as additive, it is transparent
	if( batchType == TRIBATCHTYPE_TRANSPARENT && !m_additive )
	{
		GetBatchImpl( accumulator, perObjectData, m_lineEffect );
	}
	else if( batchType == TRIBATCHTYPE_ADDITIVE && m_additive )
	{
		GetBatchImpl( accumulator, perObjectData, m_lineEffect );
	}
	else if( batchType == TRIBATCHTYPE_PICKING && m_pickEffect )
	{
		GetBatchImpl( accumulator, perObjectData, m_pickEffect );
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Implements ITr2Renderable interface.
//  Return Value:
//    Line set's bounding sphere center distance from view position.
// --------------------------------------------------------------------------------------
float Tr2CurveLineSet::GetSortValue()
{
	// center of bounding sphere is "check point"
	Vector3 center = TransformCoord( BoundingSphereGetCenter( m_boundingSphere ), m_worldTransform );

	// distance from viewer to sort z
	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	float distance = Length( d );
	return distance + m_depthOffset;
}

// -------------------------------------------------------------
// Description:
//   Implements ITr2Renderable method. Does nothing. Derived
//   classes need to supply their own per-object data.
// Arguments:
//   accumulator - Render batch accumulator
// Return value:
//   NULL always
// -------------------------------------------------------------
Tr2PerObjectData* Tr2CurveLineSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return NULL;
}

// -------------------------------------------------------------
// Description:
//   Submits all changes of this line set to the video card.
// -------------------------------------------------------------
bool Tr2CurveLineSet::SubmitChanges()
{
	// fill vertex buffer (also does size checking...)
	FillVertexBuffer();
	return true;
}

// -------------------------------------------------------------
// Description:
//   Centralize adding a new line to the list into this function. Before we add the new line, we check
//   if their might be an empty slot which we can re-use.
// Arguments:
//   lineData - complete block of data on the new line
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::addLineData( const LineData* lineData )
{
	// do we have some empty line IDs?
	if( m_emptyLineID.empty() )
	{
		// no, no empty line slots left, so push it
		m_lines.push_back( *lineData );
		return (unsigned int)m_lines.size() - 1;
	}
	else
	{
		// re-use empty old, but empty line slot
		unsigned int slotID = m_emptyLineID.back();
		m_lines[slotID] = *lineData;
		// remove this id from list
		m_emptyLineID.pop_back();
		return slotID;
	}
}

// -------------------------------------------------------------
// Description:
//   Adds a straight line to the line set, but does not submit it
// Arguments:
//   startPosition - The start position of the line
//   nstartColor - The color of the line at start position
//   endPosition - The end position of the line
//   endColor - The color of the line at end position
//   width - The width of the line in pixels
// Return value:
//   The line's id
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::AddStraightLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, float width )
{
	LineData newLine;

	newLine.type = LINETYPE_STRAIGHT;
	newLine.position1 = position1;
	newLine.color1 = color1;
	newLine.position2 = position2;
	newLine.color2 = color2;
	newLine.intermediatePosition = Vector3( 0.f, 0.f, 0.f );
	newLine.width = width;
	newLine.multiColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.multiColorBorder = -1.f;
	newLine.overlayColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.animationSpeed = 0.f;
	newLine.animationScale = 1.f;
	newLine.numOfSegments = 1;

	// use helper function to put this line in list
	return addLineData( &newLine );
}

// -------------------------------------------------------------
// Description:
//   Adds a curved line (based on splines) to the line set using cartesian coordinates, but does not submit it
// Arguments:
//   startPosition - The start position of the line
//   startColor - The color of the line at start position
//   endPosition - The end position of the line
//   endColor - The color of the line at end position
//   middle - The center control point
//   width - The width of the line in pixels
// Return value:
//   The line's id
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::AddCurvedLineCrt( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& middle, float width, Be::OptionalWithDefaultValue<int, 20> segments )
{
	LineData newLine;

	newLine.type = LINETYPE_CURVED;
	newLine.position1 = position1;
	newLine.color1 = color1;
	newLine.position2 = position2;
	newLine.color2 = color2;
	newLine.intermediatePosition = middle;
	newLine.width = width;
	newLine.multiColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.multiColorBorder = -1.f;
	newLine.overlayColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.animationSpeed = 0.f;
	newLine.animationScale = 1.f;
	newLine.numOfSegments = segments > 0 ? segments.GetValue() : 1;

	// use helper function to put this line in list
	return addLineData( &newLine );
}

// -------------------------------------------------------------
// Description:
//   Adds a curved line (based on splines) to the line set using cartesian coordinates, but does not submit it
// Arguments:
//   startPosition - The start position of the line in 3d spherical coordinates (phi, theta, radius)
//   startColor - The color of the line at start position
//   endPosition - The end position of the line in 3d spherical coordinates (phi, theta, radius)
//   endColor - The color of the line at end position
//   center - The center of the sphere the spherical coordinates are based on
//   middle - The center control point
//   width - The width of the line in pixels
// Return value:
//   The line's id
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::AddCurvedLineSph( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, const Vector3& middle, float width )
{
	// sphericals stored in vec3
	float phi1 = position1.x;
	float theta1 = position1.y;
	float radius1 = position1.z;
	float phi2 = position2.x;
	float theta2 = position2.y;
	float radius2 = position2.z;
	float phiM = middle.x;
	float thetaM = middle.y;
	float radiusM = middle.z;
	// is given in spherical coords, so convert them into cartesian
	Vector3 startPnt( radius1 * sinf( phi1 ) * sinf( theta1 ), radius1 * cosf( theta1 ), radius1 * cosf( phi1 ) * sinf( theta1 ) );
	Vector3 endPnt( radius2 * sinf( phi2 ) * sinf( theta2 ), radius2 * cosf( theta2 ), radius2 * cosf( phi2 ) * sinf( theta2 ) );
	Vector3 middlePnt( radiusM * sinf( phiM ) * sinf( thetaM ), radiusM * cosf( thetaM ), radiusM * cosf( phiM ) * sinf( thetaM ) );
	// dont forget center!
	startPnt += center;
	endPnt += center;
	middlePnt += center;
	// add it
	return AddCurvedLineCrt( startPnt, color1, endPnt, color2, middlePnt, width );
}

// -------------------------------------------------------------
// Description:
//   Adds a sphered line (a straight line on a sphere) to the line set using cartesian coordinates, but does not submit it
// Arguments:
//   startPosition - The start position of the line on the surface of the sphere in 3d cartesian space
//   startColor - The color of the line at start position
//   endPosition - The end position of the line on the surface of the sphere in 3d cartesian space
//   endColor - The color of the line at end position
//   center - The center of the sphere
//   width - The width of the line in pixels
// Return value:
//   The line's id
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::AddSpheredLineCrt( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, float width )
{
	LineData newLine;

	newLine.type = LINETYPE_SPHERED;
	newLine.position1 = position1;
	newLine.color1 = color1;
	newLine.position2 = position2;
	newLine.color2 = color2;
	newLine.intermediatePosition = center;
	newLine.width = width;
	newLine.multiColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.multiColorBorder = -1.f;
	newLine.overlayColor = Color( 0.f, 0.f, 0.f, 0.f );
	newLine.animationSpeed = 0.f;
	newLine.animationScale = 1.f;
	newLine.numOfSegments = 20;

	// use helper function to put this line in list
	return addLineData( &newLine );
}

// -------------------------------------------------------------
// Description:
//   Adds a sphered line (a straight line on a sphere) to the line set using spherical coordinates, but does not submit it
// Arguments:
//   startPosition - The start position of the line in 3d spherical coordinates (phi, theta, radius)
//   startColor - The color of the line at start position
//   endPosition - The end position of the line in 3d spherical coordinates (phi, theta, radius)
//   endColor - The color of the line at end position
//   center - The center of the sphere the spherical coordinates are based on
//   width - The width of the line in pixels
// Return value:
//   The line's id
// -------------------------------------------------------------
unsigned int Tr2CurveLineSet::AddSpheredLineSph( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& center, float width )
{
	// sphericals stored in vec3
	float phi1 = position1.x;
	float theta1 = position1.y;
	float radius1 = position1.z;
	float phi2 = position2.x;
	float theta2 = position2.y;
	float radius2 = position2.z;
	// is given in spherical coords, so convert them into cartesian
	Vector3 startPnt( radius1 * sinf( phi1 ) * sinf( theta1 ), radius1 * cosf( theta1 ), radius1 * cosf( phi1 ) * sinf( theta1 ) );
	Vector3 endPnt( radius2 * sinf( phi2 ) * sinf( theta2 ), radius2 * cosf( theta2 ), radius2 * cosf( phi2 ) * sinf( theta2 ) );
	// dont forget center!
	startPnt += center;
	endPnt += center;
	// add it
	return AddSpheredLineCrt( startPnt, color1, endPnt, color2, center, width );
}

// -------------------------------------------------------------
// Description:
//   Changes the start and end color of a line. Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   color1 - The new color of the line at start position
//   color2 - The new color of the line at end position
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineColor( unsigned int id, const Vector4& color1, const Vector4& color2 )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.color1 = color1;
		line.color2 = color2;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes only the start and endpositions of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   position1 - The start position of the line on the surface of the sphere in 3d cartesian space
//   position2 - The end position of the line on the surface of the sphere in 3d cartesian space
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLinePositionCrt( unsigned int id, const Vector3& position1, const Vector3& position2 )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.position1 = position1;
		line.position2 = position2;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes only the start and endpositions of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   position1 - The start position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)
//   position2 - The end position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)
//   center - The center of the sphere the spherical coordinates are based on in 3d cartesian space
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLinePositionSph( unsigned int id, const Vector3& position1, const Vector3& position2, const Vector3& center )
{
	// sphericals stored in vec3
	float phi1 = position1.x;
	float theta1 = position1.y;
	float radius1 = position1.z;
	float phi2 = position2.x;
	float theta2 = position2.y;
	float radius2 = position2.z;
	// is given in spherical coords, so convert them into cartesian
	Vector3 startPnt( radius1 * sinf( phi1 ) * sinf( theta1 ), radius1 * cosf( theta1 ), radius1 * cosf( phi1 ) * sinf( theta1 ) );
	Vector3 endPnt( radius2 * sinf( phi2 ) * sinf( theta2 ), radius2 * cosf( theta2 ), radius2 * cosf( phi2 ) * sinf( theta2 ) );
	// dont forget center!
	startPnt += center;
	endPnt += center;
	// add it
	return ChangeLinePositionCrt( id, startPnt, endPnt );
}

// -------------------------------------------------------------
// Description:
//   Changes only the intermediate position of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card.eing updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   intermediatePosition - The new intermediate position of the line on the surface of the sphere in 3d cartesian space
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineIntermediateCrt( unsigned int id, const Vector3& intermediatePosition )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.intermediatePosition = intermediatePosition;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes only the intermediate position of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card.eing updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   intermediatePosition - The new intermediate position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
//   center - The center of the sphere the spherical coordinates are based on in 3d cartesian spac
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineIntermediateSph( unsigned int id, const Vector3& intermediatePosition, const Vector3& center )
{
	// sphericals stored in vec3
	float phi = intermediatePosition.x;
	float theta = intermediatePosition.y;
	float radius = intermediatePosition.z;
	// is given in spherical coords, so convert them into cartesian
	Vector3 intermediatePnt( radius * sinf( phi ) * sinf( theta ), radius * cosf( theta ), radius * cosf( phi ) * sinf( theta ) );
	// dont forget center!
	intermediatePnt += center;
	// add it
	return ChangeLineIntermediateCrt( id, intermediatePnt );
}

// -------------------------------------------------------------
// Description:
//   Changes only the intermediate position of a line, no matter what type of line.
//   Requires a call to SubmitChanges before being updated on the video card.eing updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   width - The new width of the line
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineWidth( unsigned int id, float width )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.width = width;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes the multicolor settings of a line, so it will have a seperate color until a border.
//   Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   color - The color along the line until the border value is reached
//   border - Border value along the line, goes from 0.0 to 1.0
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineMultiColor( unsigned int id, const Vector4& color, float border )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.multiColor = color;
		line.multiColorBorder = border;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes the animation settings of a line.
//   Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   color - The color along the line until the border value is reached
//   speed - The speed of the pattern/texture crawls along the line
//   scale - The scale of the pattern/texture across the the line
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineAnimation( unsigned int id, const Vector4& color, float speed, float scale )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		line.overlayColor = color;
		line.animationSpeed = speed;
		line.animationScale = scale;
	}
}

// -------------------------------------------------------------
// Description:
//   Changes the number of segments a curved line is made of.
//   Does not work with straight lines!
//   Requires a call to SubmitChanges before being updated on the video card.
// Arguments:
//   id - The line ID returned by a previous call to AddXXX()
//   numOfSegments - number of segments
// -------------------------------------------------------------
void Tr2CurveLineSet::ChangeLineSegmentation( unsigned int id, unsigned int numOfSegments )
{
	if( isValidLineID( id ) )
	{
		LineData& line = m_lines[id];
		// only update if not straight line
		if( line.type != LINETYPE_STRAIGHT )
		{
			line.numOfSegments = numOfSegments;
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Clears all lines.
//   Requires a call to SubmitChanges before being updated on the video card.
// -------------------------------------------------------------
void Tr2CurveLineSet::ClearLines()
{
	m_lines.clear();
	m_emptyLineID.clear();
}

// -------------------------------------------------------------
// Description:
//   Modify if batches should be additive
// -------------------------------------------------------------
void Tr2CurveLineSet::SetAdditiveFlag( bool b )
{
	m_additive = b;
}

// -------------------------------------------------------------
// Description:
//   Modify if batches should be dynamic. (Animated)
// -------------------------------------------------------------
void Tr2CurveLineSet::SetDynamicFlag( bool b )
{
	m_dynamic = b;
}

// -------------------------------------------------------------
// Description:
//   Creates render batches for line set.
// Arguments:
//   batches - Render batch accumulator
//   batchType - Type of batches to gather
//   data - Per-object data
// -------------------------------------------------------------
void Tr2CurveLineSet::GetBatchImpl( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect )
{
	if( !effect || !m_vertexBuffer.IsValid() || m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( effect );
	batch.SetPerObjectData( perObjectData );

	float maxDepth = Tr2Renderer::GetFrustumRadius();

	Vector3 center( m_boundingSphere.x, m_boundingSphere.y, m_boundingSphere.z );
	center -= Tr2Renderer::GetViewPosition();
	float z = std::min( std::max( ( Length( center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

	unsigned int depth = (unsigned int)( (float)0xFFFFFFF * ( 1.0f - z ) );
	batch.m_depth = depth;

	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, sizeof( LineVertex ) );
	batch.SetDrawInstanced( 6 * m_currentSubmittedLineCount, 1, 0, 0 );
	accumulator->Commit( batch );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Removes a line from the list. Does NOT erase it from list, cause that would mean all ID's are
//   no longer valid. We just flag this line to INVALID and remember it's id on a list, so we can
//   re-use this slot in later calls to ::AddXXX()
// Arguments:
//   id - the id of the line to be deleted
// ------------------------------------------------------------------------------------------------------
void Tr2CurveLineSet::RemoveLine( unsigned int id )
{
	if( isValidLineID( id ) )
	{
		// simply set this line segment to INVALID, and store this id for future use in ::AddXXX()
		m_lines[id].type = LINETYPE_INVALID;
		m_emptyLineID.push_back( id );
	}
}

void Tr2CurveLineSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_PICKING, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_OPAQUE ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_OPAQUE, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_TRANSPARENT ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_TRANSPARENT, perObjectData );
		GetBatches( batches, TRIBATCHTYPE_ADDITIVE, perObjectData );
	}
}