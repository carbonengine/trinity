#include "StdAfx.h"
#include "TriSequencer.h"
#include "TriConstants.h"

BLUE_DEFINE( TriScalarSequencer );
BLUE_DEFINE( TriVectorSequencer );
BLUE_DEFINE( TriQuaternionSequencer );
BLUE_DEFINE( TriColorSequencer );
BLUE_DEFINE( TriXYZScalarSequencer );
BLUE_DEFINE( TriYPRSequencer );
BLUE_DEFINE( TriRGBAScalarSequencer );
BLUE_DEFINE( TriPerlinCurve );
BLUE_DEFINE( TriSineCurve );
BLUE_DEFINE( TriRandomConstantCurve );

const Be::ClassInfo* TriScalarSequencer::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriScalarSequencer, "no comment")
		MAP_INTERFACE(ITriFunction)
		MAP_INTERFACE(ITriScalarFunction)
		MAP_INTERFACE(ITriCurveLength)
		
		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE
		(  
			"name",          
			mName,     
			"Yes you can name your sequencer", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               start
		MAP_ATTRIBUTE
		( 
			"start",         
			mStart,        
			"The time at which the sequence should begin", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               value
		MAP_ATTRIBUTE
		(  
			"value",         
			mValue,         
			"The last value of the curve, can be set externally, wont be changed on Update() if lenght is 0", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               operator
		MAP_ATTRIBUTE_WITH_CHOOSER
		(  
			"operator",         
			mOperator,         
			"na", 
			Be::READWRITE | Be::PERSIST | Be::ENUM, 
			TriOperator
		)

		////////////////////////////////////////////////////////////////////////////
		//               functions
		MAP_ATTRIBUTE
		( 
			"functions",      
			mFunctions,
			"These are the functions, whose values are added together", 
			Be::READ | Be::PERSIST
		)	

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(  
			"inMinClamp",         
			mInMinClamp,         
			"Input functions are min clamped to this value if clamping is on", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(  
			"inMaxClamp",         
			mInMaxClamp,         
			"Input functions are max clamped to this value if clamping is on", 
			Be::READWRITE | Be::PERSIST 
		)		
		
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(  
			"outMinClamp",         
			mOutMinClamp,         
			"Output value is min clamped to this value if clamping is on", 
			Be::READWRITE | Be::PERSIST 
		)			

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(  
			"outMaxClamp",         
			mOutMaxClamp,         
			"Output value is max clamped to this value if clamping is on", 
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(  
			"clamping",         
			mClamping,         
			"Use in and out clamping parameters to constrain the input and output range of values.", 
			Be::READWRITE | Be::PERSIST
		)

	EXPOSURE_END()
}

const Be::ClassInfo* TriVectorSequencer::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriVectorSequencer, "no comment")
		MAP_INTERFACE(ITriFunction)
		MAP_INTERFACE(ITriVectorFunction)

		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE
		(  
			"name",          
			mName,     
			"Yes you can name your sequencer", 
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               start
		MAP_ATTRIBUTE
		( 
			"start",         
			mStart,        
			"The time at which the sequence should begin", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               value
		MAP_ATTRIBUTE
		(  
			"value",         
			mValue,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)
		////////////////////////////////////////////////////////////////////////////
		//               operator
		MAP_ATTRIBUTE_WITH_CHOOSER
		(  
			"operator",         
			mOperator,         
			"na", 
			Be::READWRITE | Be::PERSIST | Be::ENUM, 
			TriOperator
		)
		////////////////////////////////////////////////////////////////////////////
		//               functions
		MAP_ATTRIBUTE
		( 
			"functions",      
			mFunctions,
			"These are the functions, whose values are added together", 
			Be::READ | Be::PERSIST
		)

	EXPOSURE_END()

}

const Be::ClassInfo* TriQuaternionSequencer::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriQuaternionSequencer, "no comment")
		MAP_INTERFACE(ITriFunction)
		MAP_INTERFACE(ITriQuaternionFunction)
        MAP_INTERFACE(ITriCurveLength)

		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE
		(  
			"name",          
			mName,     
			"Yes you can name your sequencer", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               start
		MAP_ATTRIBUTE
		( 
			"start",         
			mStart,        
			"The time at which the sequence should begin", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               value
		MAP_ATTRIBUTE
		(  
			"value",         
			mValue,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               functions
		MAP_ATTRIBUTE
		( 
			"functions",      
			mFunctions,
			"These are the functions, whose values are added together", 
			Be::READ | Be::PERSIST
		)	

	EXPOSURE_END()
}

const Be::ClassInfo* TriColorSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriColorSequencer, "Add or Multiply Color Curves")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriColorFunction)
        MAP_INTERFACE(ITriCurveLength)

        ////////////////////////////////////////////////////////////////////////////
        //               name
        MAP_ATTRIBUTE
        (  
            "name",          
            mName,     
            "Yes you can name your sequencer", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               start
        MAP_ATTRIBUTE
        ( 
            "start",         
            mStart,        
            "The time at which the sequence should begin", 
            Be::READWRITE | Be::PERSIST 
        )       

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "value",         
            mValue,         
            "na", 
            Be::READWRITE | Be::PERSIST
        )
        ////////////////////////////////////////////////////////////////////////////
        //               operator
		MAP_ATTRIBUTE_WITH_CHOOSER
        (  
            "operator",         
            mOperator,         
            "na", 
            Be::READWRITE | Be::PERSIST | Be::ENUM, 
            TriOperator
        )
        ////////////////////////////////////////////////////////////////////////////
        //               functions
        MAP_ATTRIBUTE
        ( 
            "functions",      
            mFunctions,
            "These are the functions, whose values are added together", 
            Be::READ | Be::PERSIST
        )   

	EXPOSURE_END()

}

const Be::ClassInfo* TriXYZScalarSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriXYZScalarSequencer, "Use the XYZ Scalar Sequencer to create a 3d vector function from three 1d scalar functions.")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriVectorFunction)
        MAP_INTERFACE(ITriCurveLength)

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"The name of the sequencer",
			Be::READWRITE | Be::PERSIST
		)
        
		
		////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (
            "value",
            mValue,
            "na",
            Be::READWRITE | Be::PERSIST
        )
		


		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"XCurve",
			mXCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)


		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"YCurve",
			mYCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"ZCurve",
			mZCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)

    EXPOSURE_END()

}

const Be::ClassInfo* TriYPRSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriYPRSequencer, "Yaw Pitch Roll Sequencer returns a Quaternion from three scalar curves representing yaw, pitch and roll. Note that this uses degrees, not radians.")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriQuaternionFunction)

		MAP_ATTRIBUTE
		(  
			"name",          
			mName,     
			"na", 
			Be::READWRITE | Be::PERSIST 
		)	
        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (
            "value",
            mValue,
            "na",
            Be::READWRITE | Be::PERSIST
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (
            "YawPitchRoll",
            mYawPitchRoll,
            "na",
            Be::READWRITE | Be::PERSIST
        )

		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"YawCurve",
			mYawCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"PitchCurve",
			mPitchCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               mXCurve
		MAP_ATTRIBUTE
		(
			"RollCurve",
			mRollCurve,
			"na",
			Be::READWRITE | Be::PERSIST
		)

    EXPOSURE_END()
}

const Be::ClassInfo* TriRGBAScalarSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriRGBAScalarSequencer, "Use the RGBA Scalar Sequencer to create a RGBA function from four 1d scalar functions.")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriColorFunction)
        MAP_INTERFACE(ITriCurveLength)

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (
            "value",
            mValue,
            "na",
            Be::READWRITE | Be::PERSIST
        )



        ////////////////////////////////////////////////////////////////////////////
        //               mRedCurve
        MAP_ATTRIBUTE
        (
            "RedCurve",
            mRedCurve,
            "na",
            Be::READWRITE | Be::PERSIST
        )


        ////////////////////////////////////////////////////////////////////////////
        //               mRedCurve
        MAP_ATTRIBUTE
        (
            "GreenCurve",
            mGreenCurve,
            "na",
            Be::READWRITE | Be::PERSIST
        )

        ////////////////////////////////////////////////////////////////////////////
        //               mRedCurve
        MAP_ATTRIBUTE
        (
            "BlueCurve",
            mBlueCurve,
            "na",
            Be::READWRITE | Be::PERSIST
        )


        ////////////////////////////////////////////////////////////////////////////
        //               mRedCurve
		MAP_ATTRIBUTE
        (
            "AlphaCurve",
            mAlphaCurve,
            "na",
            Be::READWRITE | Be::PERSIST
        )

    EXPOSURE_END()

}

const Be::ClassInfo* TriPerlinCurve::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriPerlinCurve, "A scalar function that generates Perlin noise")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriScalarFunction)

        ////////////////////////////////////////////////////////////////////////////
        //               name
		MAP_ATTRIBUTE
        (  
            "name",          
            mName,     
            "Yes you can name your sequencer", 
            Be::READWRITE | Be::PERSIST
        )
	
        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "value",         
            mValue,         
            "The last value of the curve, can be set externally, wont be changed on Update() if lenght is 0", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "offset",         
            mOffset,         
            "The result of the perlin function is offset by this value. The default value is 0.0.", 
            Be::READWRITE | Be::PERSIST
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "scale",         
            mScale,         
            "The result of the perlin function is multiplied by this value. The default value is 1.0.", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "alpha",         
            mAlpha,         
            "Alpha value. 1.0 is rough but larger numbers yield smoother curves. Values lower than 1.0 will give scalar values over beyond the range of 0.0 to 1.0, which is the intended range of the function.", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "speed",         
            mSpeed,         
            "Time multiplier. Higher numbers are faster. 1.0 is default", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "beta",         
            mBeta,         
            "beta is the harmonic scaling/spacing, typically 2", 
            Be::READWRITE | Be::PERSIST 
        )
        
        ////////////////////////////////////////////////////////////////////////////
        //               value
		MAP_ATTRIBUTE
        (  
            "N",         
            mN,         
            "N is the iteration count ( grain resolution ). Keep values in the range 1 to 4. Values higher than 4 are hardly noticable under regular circumstances.", 
            Be::READWRITE | Be::PERSIST 
        )
        
    EXPOSURE_END()
}

const Be::ClassInfo* TriSineCurve::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriSineCurve, "A scalar function that generates sine waves")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriScalarFunction)

        ////////////////////////////////////////////////////////////////////////////
        //               name
        MAP_ATTRIBUTE
        (  
            "name",          
            mName,     
            "Yes you can name your sequencer", 
            Be::READWRITE | Be::PERSIST 
        )

        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "value",         
            mValue,         
            "The last value of the curve, can be set externally, wont be changed on Update() if lenght is 0", 
            Be::READWRITE | Be::PERSIST 
        )



        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "offset",         
            mOffset,         
            "The result of the Sine function is offset by this value. The default value is 0.0.", 
            Be::READWRITE | Be::PERSIST 
        )


        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "scale",         
            mScale,         
            "The result of the Sine function is multiplied by this value. The default value is 1.0.", 
            Be::READWRITE | Be::PERSIST 
        )


        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "speed",         
            mSpeed,         
            "Time multiplier. Higher numbers are faster. 1.0 is default", 
            Be::READWRITE | Be::PERSIST 
        )

    EXPOSURE_END()
}

const Be::ClassInfo* TriRandomConstantCurve::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriRandomConstantCurve, "A scalar function that sets a random constant upon initialization")
        MAP_INTERFACE(ITriFunction)
        MAP_INTERFACE(ITriScalarFunction)

        ////////////////////////////////////////////////////////////////////////////
        //               name
        MAP_ATTRIBUTE
        (  
            "name",          
            mName,     
            "Yes you can name your sequencer", 
            Be::READWRITE | Be::PERSIST 
        )
    


        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "value",         
            mValue,         
            "The last value of the curve, can be set externally, wont be changed on Update() if lenght is 0", 
            Be::READWRITE 
        )



        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "min",         
            mMin,         
            "The minimum random value", 
            Be::READWRITE | Be::PERSIST 
        )


        ////////////////////////////////////////////////////////////////////////////
        //               value
        MAP_ATTRIBUTE
        (  
            "max",         
            mMax,         
            "The maximum random value", 
            Be::READWRITE | Be::PERSIST 
        )


		MAP_ATTRIBUTE
		(  
			"hold",         
			mHold,         
			"If this is turned off, the random value is set at every frame. Good for checking if the range of random numbers meets your expectations", 
			Be::READWRITE | Be::PERSIST 
		)
        
    EXPOSURE_END()
}

