/* 
	*************************************************************************************

	TriSequencer.h

	Author:    Hilmar Veigar Pétursson
	Created:   October 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Implementations for all sequencers for all types og curves



	Dependencies:

		None

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRISEQUENCER_H_
#define _TRISEQUENCER_H_

#include "include/ITriFunction.h"

#include "include/ITriCurveLength.h"

BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_IVECTOR( ITriVectorFunction );

BLUE_DECLARE_INTERFACE( ITriScalarFunction );
BLUE_DECLARE_IVECTOR( ITriScalarFunction );

BLUE_DECLARE_INTERFACE( ITriColorFunction );
BLUE_DECLARE_IVECTOR( ITriColorFunction );

BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );
BLUE_DECLARE_IVECTOR( ITriQuaternionFunction );


BLUE_CLASS( TriVectorSequencer ) :
	public ITriVectorFunction
{
public:
	EXPOSE_TO_BLUE();

	std::wstring  mName;
	Be::Time mStart;
	Vector3 mValue;	
	TRIOPERATOR	mOperator;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriFunction
	/////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Vector3 v; Update( &v, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////
	Vector3* Update(
		Vector3* in,
		Be::Time time
		);

	Vector3* Update(
		Vector3* in,
		double time
		);

	Vector3* GetValueAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		double time
		);
	Vector3d* InterpolatedPosition(Vector3d* out, Be::Time) {return out;};

	PITriVectorFunctionVector mFunctions;

	TriVectorSequencer(IRoot* lockobj = NULL);
	~TriVectorSequencer();
private:
	Vector3* GetValueAtMult(Vector3* in, double pos);
	Vector3* GetValueAtAdd(Vector3* in, double pos);
	Vector3* GetValueAtAverage(Vector3* in, double pos);
	Vector3* GetValueAtMult(Vector3* in, Be::Time now);
	Vector3* GetValueAtAdd(Vector3* in, Be::Time now);
	Vector3* GetValueAtAverage(Vector3* in, Be::Time now);
public:
#if BLUE_WITH_PYTHON
    PyObject* PyGetValueDoubleDotAt( PyObject* args );
#endif
};
TYPEDEF_BLUECLASS( TriVectorSequencer );



// Color



BLUE_CLASS( TriColorSequencer ) :
    public ITriColorFunction,
    public ITriCurveLength
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  mName;
    Be::Time mStart;
    Color mValue;  
    TRIOPERATOR mOperator;

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriFunction
    /////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Color c; Update( &c, time ); }

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriColorFunction
    /////////////////////////////////////////////////////////////////////////////////////
    Color* Update(
        Color* in,
        Be::Time time
        );

    Color* Update(
        Color* in,
        double time
        );

    Color* GetValueAt(
        Color* in,
        Be::Time time
        );

    Color* GetValueAt(
        Color* in,
        double time
        );

	float Length();

	PITriColorFunctionVector mFunctions;

    TriColorSequencer(IRoot* lockobj = NULL);
    ~TriColorSequencer();
private:
    Color* GetValueAtMult(Color* in, double pos);
    Color* GetValueAtAdd(Color* in, double pos);
    Color* GetValueAtMult(Color* in, Be::Time now);
    Color* GetValueAtAdd(Color* in, Be::Time now);
};
TYPEDEF_BLUECLASS( TriColorSequencer );




//////////////



BLUE_CLASS( TriPerlinCurve ) :
    public ITriScalarFunction
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  mName;
    double mStart;
    float mValue;
    float mSpeed;
    float mAlpha;
    float mBeta;
	float mOffset;
	float mScale;
	double mLastUpdated;
	long mStartOffset;

    int32_t mN;   

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriFunction
    /////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Update( time ); }

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriScalarFunction
    /////////////////////////////////////////////////////////////////////////////////////
    float Update(
        Be::Time time
        );

    float Update(
        double time
        );

    float GetValueAt(
        Be::Time time
        );

    float GetValueAt(
        double time
        );

	void ScaleTime(
		float s
		);


    TriPerlinCurve(IRoot* lockobj = NULL);
    ~TriPerlinCurve();

public:
};
TYPEDEF_BLUECLASS( TriPerlinCurve );


#endif

