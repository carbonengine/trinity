////////////////////////////////////////////////////////////
//
//    Created:   August 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorCurveLineSet.h"

#include "Tr2LitPerObjectData.h"
#include "TriRenderBatch.h"
#include "Tr2InteriorLightSet.h"
#include "Tr2ShaderMaterial.h"

static const char* CURVE_LINE_SHADER = "Line3D";
static const char* CURVE_PICK_SHADER = "Line3DPicking";

Tr2InteriorCurveLineSet::Tr2InteriorCurveLineSet(IRoot* lockobj /*= NULL*/ ) :
    m_perspectiveCorrection( true ),
	m_isVisible( true )
{
	// create line draw effect
	Tr2ShaderSituation situation;
    if( m_perspectiveCorrection )
    {
        situation.AddSituationString( "PerspectiveCorrection" );
    }

	m_lineEffectMaterial.CreateInstance();
	m_lineEffectMaterial->SetHighLevelShaderName( CURVE_LINE_SHADER );
	m_lineEffectMaterial->PopulateDefaultParameters();
	m_lineEffectMaterial->BindLowLevelShader( situation );
	m_lineEffect = m_lineEffectMaterial;

	m_pickEffectMaterial.CreateInstance();
	m_pickEffectMaterial->SetHighLevelShaderName( CURVE_PICK_SHADER );
	m_pickEffectMaterial->PopulateDefaultParameters();
	m_pickEffectMaterial->BindLowLevelShader( situation );
	m_pickEffect = m_pickEffectMaterial;
}

Tr2InteriorCurveLineSet::~Tr2InteriorCurveLineSet()
{
}

// -------------------------------------------------------------
// Description:
//   Implements INotify method. Is called when exposed member 
//   variable is modified. 
// Arguments:
//   val - Value being modified
// -------------------------------------------------------------
bool Tr2InteriorCurveLineSet::OnModified( Be::Var* value )
{
    Tr2CurveLineSet::OnModified( value );
	if( IsMatch( value, m_perspectiveCorrection ) )
	{
	    Tr2ShaderSituation situation;
		if( m_perspectiveCorrection )
        {
            situation.AddSituationString( "PerspectiveCorrection" );
        }
        m_lineEffectMaterial->BindLowLevelShader( situation );
        m_pickEffectMaterial->BindLowLevelShader( situation );
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Assign SH probe data to attached object. Does nothing.
// Arguments:
//   redProbeMatrix - Red SH probe coefficients
//   greenProbeMatrix - Green SH probe coefficients
//   blueProbeMatrix - Blue SH probe coefficients
// -------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetSHProbeMatrices( const Matrix &redProbeMatrix, 
								 const Matrix &greenProbeMatrix, 
								 const Matrix &blueProbeMatrix )
{
}

// -------------------------------------------------------------
// Description:
//   Assign a world trasform of a parent dynamic object.
// Arguments:
//   worldTransform - Local to world space transform
// -------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetWorldTransform( const Matrix &worldTransform )
{
	m_worldTransform = worldTransform;
}
// -------------------------------------------------------------
// Description:
//   Return if bounding box of the attached object has changed.
// Return Value:
//   true If bounding box of the attached object has changed
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorCurveLineSet::IsDirty() const
{
	return m_boundsDirty;
}

// -------------------------------------------------------------
// Description:
//   Clears object dirty flag.
// -------------------------------------------------------------
void Tr2InteriorCurveLineSet::ClearDirtyFlag()
{
	m_boundsDirty = false;
}
// -------------------------------------------------------------
// Description:
//   Return bounding box (in parent coordinate space).
// Arguments:
//   minBounds (out) - Min bounds of the attached object
//   maxBounds (out) - Max bounds of the attached object
// Return Value:
//   true If bounds returned are valid
//   false If bounds are not ready or the object does not need/have
//         a bounding box
// -------------------------------------------------------------
bool Tr2InteriorCurveLineSet::GetBoundingBox( Vector3 &minBounds, Vector3 &maxBounds )
{
	if( !m_currentSubmittedLineCount )
	{
		return false;
	}
	minBounds = m_minBounds;
	maxBounds = m_maxBounds;

	return true;
}


// --------------------------------------------------------------------------------------
// Description:
//   Sets visibility flag for the object.
// Arguments:
//   bVisible - Visibility flag: true if the object should render, false if the object
//              should be hidden
// --------------------------------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns visibility flag.
// Return Value:
//   true If the object should render
//   false if the object should be hidden
// --------------------------------------------------------------------------------------
bool Tr2InteriorCurveLineSet::IsVisible( void ) const
{
	return m_isVisible;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Does nothing.
// Arguments:
//   visibleLights - Number of lights affecting this object
// --------------------------------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetVisibleLightCount( int visibleLights ) 
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Does nothing.
// Arguments:
//   visibleLightSet - Set of lights affecting this object
// --------------------------------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet )
{
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object using a per-instance light-set override and 
//    an arbitrary object-to-world matrix.  
//  Arguments:
//    accumulator - The batch accumulator used to allocate memory for per-object data
//    lightSet - The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorCurveLineSet::GetPerObjectDataWithPerInstanceLighting( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet, 
	const Matrix& objectToWorldMatrix, 
	const Matrix& mirrorToWorldMatrix 
)
{
	Tr2LitPerObjectData* data = accumulator->Allocate<Tr2LitPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	// Pixel Shader Light information
	Tr2InteriorPerObjectPSData perObjectPSBuffer;
	// standard vertex shader data
	Tr2PerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
		data->SetLightsActive( lightSet->GetNumOfActiveLights(), lightSet->GetNumOfActiveLights() );
	}

	// Copy the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object using a per-instance light-set override and 
//    an arbitrary object-to-world matrix.  
//  Arguments:
//    accumulator - The batch accumulator used to allocate memory for per-object data
//    lightSet - The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorCurveLineSet::GetPerObjectDataForPrePass(
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix
)
{
	Tr2PerObjectDataPrePass* data = accumulator->Allocate<Tr2PerObjectDataPrePass>();

	if( !data )
	{
		return NULL;
	}

	// Standard vertex shader data
	Tr2PerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// Do the copy
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object.  
//  Arguments:
//    accumulator - The batch accumulator used to allocate memory for per-object data
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorCurveLineSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectDataWithPerInstanceLighting( accumulator, NULL, m_worldTransform, Tr2Renderer::GetIdentityTransform() );
}

// -------------------------------------------------------------
// Description:
//   Assigns an SH lighting solver to an interior object. Interior
//   scene assigns a solver right before quering for per object
//   dataand gathering batches for an object in prepass forward or 
//   full forward passes. Having a pointer to SH lighting solver
//   allows an object to register transparent areas that are supposed
//   to receive SH lighting with the solver. The pointer is guaranteed
//   to stay alive during next call to GetBatches.
//   This implementation does nothing.
// Arguments:
//   solver - Pointer to the solver object
// -------------------------------------------------------------
void Tr2InteriorCurveLineSet::SetSHLightingSolver( ITr2InteriorSHLightingSolver* solver )
{ 
}

#endif
