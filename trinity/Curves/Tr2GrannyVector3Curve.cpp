#include "StdAfx.h"
#include "Tr2GrannyVector3Curve.h"
#include "Tr2GrannyTrack.h"

// --------------------------------------------------------------------------------------
// Description:
//	Tr2GrannyVector3Curve default constructor
// --------------------------------------------------------------------------------------	
Tr2GrannyVector3Curve::Tr2GrannyVector3Curve( IRoot* lockobj ):
	m_grannyCurve(NULL),
	m_duration(0.0f),
	m_timeOffset(0.0f),
	m_cycle(false),
	m_value( 0.0f, 0.0f, 0.0f )
{
}

// --------------------------------------------------------------------------------------
// Description:
//	Tr2GrannyVector3Curve destructor. Needs to free the granny curve created
// --------------------------------------------------------------------------------------	
Tr2GrannyVector3Curve::~Tr2GrannyVector3Curve()
{ 
	GrannyFreeCurve( m_grannyCurve ); 
}

// --------------------------------------------------------------------------------------
// Description:
//		Update the internal value that might be bound
//		Implements ITriFunction
// Arguments:
//		time	- The current time seconds
// --------------------------------------------------------------------------------------	
void Tr2GrannyVector3Curve::UpdateValue( double time )
{
	if( m_grannyCurve == NULL )
	{
		return;
	}
	// We might want to delay the start of the evaluation
	time -= m_timeOffset;

	// We just repeat ourselves
	if ( m_cycle )
	{
		time = fmod( time, (double)m_duration );
	}
	else
	{
		// Make sure that our time does not go below zero or above the duration
		if( time < 0.0 )
		{
			time = 0.0f;
		}
		else if( time > m_duration )
		{
			time = m_duration;
		}
	}

	GrannyEvaluateCurveAtT( 3, false, false, m_grannyCurve, false, m_duration, (float)time,(float*)&m_value, GrannyCurveIdentityPosition );
}

// --------------------------------------------------------------------------------------
// Description:
//		For writing the curve to a file we need to get the data as a raw buffer. 
//		
//		Implements ICustomPersist::GetWriteBufferAndSize
// Arguments:
//		memberName		-	Which member is the exporter asking for
//		buffer			-	The raw buffer to return
//		bufferSize		-	The size of the raw buffer
// --------------------------------------------------------------------------------------	
void Tr2GrannyVector3Curve::GetWriteBufferAndSize(const char* memberName, unsigned char** buffer, size_t* bufferSize )
{
	// This will allocate a buffer for the data to be written.
	// You need to clean that up in ReleaseWriteBuffer
	Tr2GRNMemoryPersist grannyMemoryPersist;
	if( grannyMemoryPersist.ConvertGrannyTypeToContiguousBuffer( GrannyCurve2Type, m_grannyCurve ) )
	{ 
		*buffer = grannyMemoryPersist.m_buffer;
		*bufferSize = grannyMemoryPersist.m_bufferSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//		The buffer used for writing out the data needs to be released
//		
//		Implements ICustomPersist::ReleaseWriteBuffer
// Arguments:
//		buffer			-	The raw buffer to release
// --------------------------------------------------------------------------------------	
void Tr2GrannyVector3Curve::ReleaseWriteBuffer( unsigned char* buffer )
{
	// Release the temp buffer used for writing out the data
	Tr2GRNMemoryPersist::ReleaseWriteBuffer( buffer );
}

// --------------------------------------------------------------------------------------
// Description:
//		We need to allocate memory correctly for the buffer we will get
//		
//		Implements ICustomPersist::AllocateReadBuffer
// Arguments:
//		memberName			-	What member are we allocating for
//		bufferSize			-	The buffer size
// --------------------------------------------------------------------------------------	
unsigned char* Tr2GrannyVector3Curve::AllocateReadBuffer( const char* memberName, size_t bufferSize )
{
	// Get the memory to use for your object.
	// Depending on your data, like GRANNY in our case it needs to be aligned
	return (unsigned char*)CCP_ALIGNED_MALLOC( "Tr2GrannyVector3Curve::curve", bufferSize, sizeof(int));
}

// --------------------------------------------------------------------------------------
// Description:
//		When the exporter has loaded our data it will give us back the buffer
//		
//		Implements ICustomPersist::SetBufferAndSize
// Arguments:
//		memberName			-	What member are we getting
//		buffer				-	The raw buffer to use
//		bufferSize			-	The buffer size
// --------------------------------------------------------------------------------------	
void Tr2GrannyVector3Curve::SetBufferAndSize( const char* memberName, unsigned char* buffer, size_t bufferSize )
{
	// The type has some pointers so we need to adjust the pointers relative to the base object loaded
	m_grannyCurve = (granny_curve2*)Tr2GRNMemoryPersist::RebaseBuffer( GrannyCurve2Type, buffer );
	CCP_ALIGNED_FREE( buffer );
}

// --------------------------------------------------------------------------------------
// Description:
//		Create a granny position curve from sample points
//		
//		Extends: ICustomPersist
// Arguments:
//		sampleDelta		-	The time difference between each knot
//		knots			-	The list of knots
// --------------------------------------------------------------------------------------	
void Tr2GrannyVector3Curve::CreateFromPoints( float sampleDelta, std::vector<granny_real32>& knots )
{
	// If the user calls this function again we need to clean up the previous allocation
	if(m_grannyCurve != NULL )
	{
		GrannyFreeCurve( m_grannyCurve ); 
	}

	// How many segments do we have
	m_duration = sampleDelta*(knots.size()-1);
	m_grannyCurve = CompressCurve( 
		PositionTolerance,					// error tolerance		
		sampleDelta,						// the time step between frames
		false,								// solve as quaternions
		PositionCurveFormats,				// possible compression formats
		ArrayLength(PositionCurveFormats),	// number of compression formats
		GrannyCurveDataD3Constant32fType,	// constant compression type
		GrannyCurveIdentityPosition,		// 
		(int)knots.size(),						//
		3,									// Dimensions
		knots);								//
}