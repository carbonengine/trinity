#pragma once
#ifndef Tr2GrannyVector3Curve_h
#define Tr2GrannyVector3Curve_h

#include "include/ITriFunction.h"
#include "include/ITriCurveLength.h"
#include "Tr2GRNMemoryPersist.h"


BLUE_CLASS( Tr2GrannyVector3Curve ):
	public ITriFunction,
	public ITriCurveLength,
	public ICustomPersist
{
public:
    EXPOSE_TO_BLUE();
    Tr2GrannyVector3Curve( IRoot* lockobj = NULL );
	~Tr2GrannyVector3Curve();
	void CreateFromPoints( float sampleDelta, std::vector<granny_real32>& knots );


	// ICustomPersist
	void GetWriteBufferAndSize( const char* propertyName, unsigned char**buffer, size_t* bufferSize );
	void ReleaseWriteBuffer( unsigned char* buffer );
	void SetBufferAndSize( const char* propertyName, unsigned char* buffer, size_t bufferSize );
	unsigned char* AllocateReadBuffer( const char* memberName, size_t bufferSize );


	// ITriFunction
	virtual void UpdateValue( double time );

	// ITriCurveLength
	float Length() { return m_duration; }
	
private:
	bool				m_cycle;
	Vector3				m_value;
	float				m_timeOffset;
	granny_real32		m_duration;
	granny_curve2*		m_grannyCurve;
};

TYPEDEF_BLUECLASS( Tr2GrannyVector3Curve );
#endif //Tr2GrannyVector3Curve_h
