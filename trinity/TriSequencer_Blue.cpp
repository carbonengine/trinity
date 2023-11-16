#include "StdAfx.h"
#include "TriSequencer.h"
#include "TriConstants.h"

BLUE_DEFINE( TriVectorSequencer );
BLUE_DEFINE( TriColorSequencer );
BLUE_DEFINE( TriPerlinCurve );

const Be::ClassInfo* TriVectorSequencer::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriVectorSequencer, "no comment\n:jessica-deprecated:")
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

const Be::ClassInfo* TriColorSequencer::ExposeToBlue()
{
    EXPOSURE_BEGIN(TriColorSequencer, "Add or Multiply Color Curves \n:jessica-deprecated: True")
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
