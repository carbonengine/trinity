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

BLUE_DECLARE( TriScalarSequencer );
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_IVECTOR( ITriVectorFunction );

BLUE_DECLARE_INTERFACE( ITriScalarFunction );
BLUE_DECLARE_IVECTOR( ITriScalarFunction );

BLUE_DECLARE_INTERFACE( ITriColorFunction );
BLUE_DECLARE_IVECTOR( ITriColorFunction );

BLUE_DECLARE_INTERFACE( ITriQuaternionFunction );
BLUE_DECLARE_IVECTOR( ITriQuaternionFunction );

BLUE_CLASS( TriScalarSequencer ) :
	public ITriScalarFunction,
	public ITriCurveLength
{
public:
	EXPOSE_TO_BLUE();

	std::wstring  mName;
	Be::Time mStart;
	float mValue;	

	float mInMinClamp;
	float mInMaxClamp;
	float mOutMinClamp;
	float mOutMaxClamp;
	bool  mClamping;

	TRIOPERATOR	mOperator;
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

	PITriScalarFunctionVector  mFunctions;

	TriScalarSequencer(IRoot* lockobj = NULL);
	~TriScalarSequencer();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriCurveLength
	/////////////////////////////////////////////////////////////////////////////////////
	float Length();
private:
	float GetValueAtMult(double pos);
	float GetValueAtAdd(double pos);
	float GetValueAtMult(Be::Time now);
	float GetValueAtAdd(Be::Time now);

	float ClampIn(float v);
	float ClampOut(float v);
 
public:
};
TYPEDEF_BLUECLASS( TriScalarSequencer );


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



/// Quaternion



BLUE_CLASS( TriQuaternionSequencer ) :
	public ITriQuaternionFunction,
	public ITriCurveLength
{
public:
	EXPOSE_TO_BLUE();

	std::wstring  mName;
	Be::Time mStart;
	Quaternion mValue;	

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriFunction
	/////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Quaternion q; Update( &q, time ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriQuaternionFunction
	/////////////////////////////////////////////////////////////////////////////////////
	Quaternion* Update(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* Update(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		double time
		);

	PITriQuaternionFunctionVector  mFunctions;

	TriQuaternionSequencer(IRoot* lockobj = NULL);
	~TriQuaternionSequencer();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITriCurveLength
	/////////////////////////////////////////////////////////////////////////////////////
	float Length();
public:
};
TYPEDEF_BLUECLASS( TriQuaternionSequencer );



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





BLUE_CLASS( TriXYZScalarSequencer ) :
    public ITriVectorFunction,
    public ITriCurveLength
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  m_name;
    //Be::Time mStart;
    Vector3 mValue;
    
	ITriScalarFunctionPtr mXCurve;
	ITriScalarFunctionPtr mYCurve;
	ITriScalarFunctionPtr mZCurve;

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
		){return in;};

    Vector3* GetValueDotAt(
        Vector3* in,
        double time
        ){return in;};

    Vector3* GetValueDoubleDotAt(
        Vector3* in,
        Be::Time time
        ){return in;};

    Vector3* GetValueDoubleDotAt(
        Vector3* in,
        double time
        ){return in;};

	

    Vector3d* InterpolatedPosition(Vector3d* out, Be::Time) {return out;};


    TriXYZScalarSequencer(IRoot* lockobj = NULL);
    ~TriXYZScalarSequencer();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriCurveLength
	/////////////////////////////////////////////////////////////////////////////////////
	float Length();
};
TYPEDEF_BLUECLASS( TriXYZScalarSequencer );




BLUE_CLASS( TriYPRSequencer ) :
    public ITriQuaternionFunction
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  mName;
    Be::Time mStart;
    Quaternion mValue;
    Vector3 mYawPitchRoll;

	ITriScalarFunctionPtr mYawCurve;
	ITriScalarFunctionPtr mPitchCurve;
	ITriScalarFunctionPtr mRollCurve;

	TriYPRSequencer(IRoot* lockobj = NULL);

    ~TriYPRSequencer();

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriFunction
    /////////////////////////////////////////////////////////////////////////////////////
	void UpdateValue( double time ) { Quaternion q; Update( &q, time ); }

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriQuaternionFunction
    /////////////////////////////////////////////////////////////////////////////////////
    Quaternion* Update(
        Quaternion* in,
        Be::Time time
        );

    Quaternion* Update(
        Quaternion* in,
        double time
        );

    Quaternion* GetValueAt(
        Quaternion* in,
        Be::Time time
        );

    Quaternion* GetValueAt(
        Quaternion* in,
        double time
        );

    Quaternion* GetValueDotAt(
        Quaternion* in,
        Be::Time time
        )
        {return in;};

    Quaternion* GetValueDotAt(
        Quaternion* in,
        double time)
        {return in;};

    Quaternion* GetValueDoubleDotAt(
        Quaternion* in,
        Be::Time time)
        {return in;};

    Quaternion* GetValueDoubleDotAt(
        Quaternion* in,
        double time)
        {return in;};



public:
};
TYPEDEF_BLUECLASS( TriYPRSequencer );


//////////////////////////////////////////


BLUE_CLASS( TriRGBAScalarSequencer ) :
    public ITriColorFunction,
    public ITriCurveLength
{
public:
	EXPOSE_TO_BLUE();

    Color mValue;
    
    ITriScalarFunctionPtr mRedCurve;
    ITriScalarFunctionPtr mGreenCurve;
    ITriScalarFunctionPtr mBlueCurve;
    ITriScalarFunctionPtr mAlphaCurve;
    

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

    
    Color* GetValueDotAt(
        Color* in,
        Be::Time time
        ){return in;};

    Color* GetValueDotAt(
        Color* in,
        double time
        ){return in;};

    Color* GetValueDoubleDotAt(
        Color* in,
        Be::Time time
        ){return in;};

    Color* GetValueDoubleDotAt(
        Color* in,
        double time
        ){return in;};


    TriRGBAScalarSequencer(IRoot* lockobj = NULL);
    ~TriRGBAScalarSequencer();

    /////////////////////////////////////////////////////////////////////////////////////
    // ITriCurveLength
    /////////////////////////////////////////////////////////////////////////////////////
    float Length();
};
TYPEDEF_BLUECLASS( TriRGBAScalarSequencer );



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

    int mN;   

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



BLUE_CLASS( TriSineCurve ) :
    public ITriScalarFunction
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  mName;
    Be::Time mStart;
    float mValue;
    float mSpeed;
    float mOffset;
    float mScale;


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

    TriSineCurve(IRoot* lockobj = NULL);
    ~TriSineCurve();
};
TYPEDEF_BLUECLASS( TriSineCurve );


BLUE_CLASS( TriRandomConstantCurve ) :
    public ITriScalarFunction
{
public:
	EXPOSE_TO_BLUE();

    std::wstring  mName;
    float mValue;
    float mMin;
    float mMax;
	bool  mHold;

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

	void Randomize();

    TriRandomConstantCurve(IRoot* lockobj = NULL);
    ~TriRandomConstantCurve();
};
TYPEDEF_BLUECLASS( TriRandomConstantCurve );

#endif

