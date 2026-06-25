// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "EveLensflare.h"

#include "include/TriMath.h"
#include "TriFrustum.h"
#include "Tr2VariableStore.h"
#include "Tr2GpuBuffer.h"

#include "EveOccluder.h"
#include "EveTransform.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Mesh.h"
#include "Controllers/ITr2Controller.h"
#include "Shader/Tr2Effect.h"

#include <ITriFunction.h>



class EveLensflarePerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		FillAndSetConstants( *buffers[Tr2RenderContextEnum::VERTEX_SHADER],
							 &m_data,
							 sizeof( Data ),
							 Tr2RenderContextEnum::VERTEX_SHADER,
							 Tr2Renderer::GetPerObjectVSStartRegister(),
							 renderContext );
		FillAndSetConstants( *buffers[Tr2RenderContextEnum::PIXEL_SHADER],
							 &m_data,
							 sizeof( Data ),
							 Tr2RenderContextEnum::PIXEL_SHADER,
							 Tr2Renderer::GetPerObjectPSStartRegister(),
							 renderContext );
	}

	struct Data
	{
		Vector4 directionScale;
		uint32_t indices[4];
	};
	Data m_data;
};


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members and register some variable store handles
// --------------------------------------------------------------------------------
EveLensflare::EveLensflare( IRoot* lockobj ) :
	PARENTLOCK( m_flares ),
	PARENTLOCK( m_occluders ),
	PARENTLOCK( m_backgroundOccluders ),
	PARENTLOCK( m_distanceToEdgeCurves ),
	PARENTLOCK( m_distanceToCenterCurves ),
	PARENTLOCK( m_radialAngleCurves ),
	PARENTLOCK( m_xDistanceToCenter ),
	PARENTLOCK( m_yDistanceToCenter ),
	PARENTLOCK( m_bindings ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_controllers ),
	m_display( true ),
	m_update( true ),
	m_isVisible( false ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_cameraFactor( 20.f ),
	m_sunSize( 0.f ),
	m_directionVar( "LensflareFxDirectionScale", Vector4( 0.f, 0.f, 0.f, 1.f ) ),
	m_occScaleVar( "LensflareFxOccScale", Vector4( 1.f, 0.f, 0.f, 0.f ) ),
	m_transform( IdentityMatrix() )
{
	m_controllers.SetNotify( this );
}

bool EveLensflare::Initialize()
{
	for( auto& controller : m_controllers )
	{
		if( !controller->IsLinked() )
		{
			controller->Link( *GetRawRoot() );
		}
	}
	return true;
}

void EveLensflare::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_controllers && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( ITr2ControllerPtr controller = BlueCastPtr( value ) )
			{
				controller->Link( *GetRawRoot() );
				for( auto it = begin( m_controllerVariables ); it != end( m_controllerVariables ); ++it )
				{
					controller->SetVariable( it->first.c_str(), it->second );
				}
			}
			break;
		case BELIST_REMOVED:
			if( ITr2ControllerPtr controller = BlueCastPtr( value ) )
			{
				controller->Unlink();
			}
			break;
		case BELIST_UNLOADSTART:
			for( auto& controller : m_controllers )
			{
				controller->Unlink();
			}
			break;
		default:
			break;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Clean up controllers by unlinking them
// --------------------------------------------------------------------------------
EveLensflare::~EveLensflare()
{
	for( auto& controller : m_controllers )
	{
		controller->Unlink( UnlinkReason::DELETING );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Updates the lensflare's position by updating the translation curve. Also
//   calulates the overall size of the lensflare based on a magic formula and
//   updates the variable store values
// Arguments:
//   time - time delta since last frame
// --------------------------------------------------------------------------------
void EveLensflare::Update( Be::Time realTime, Be::Time simTime )
{
	Vector3 dirVec;

	if( !m_update )
	{
		return;
	}

	// update the position of this lensflare
	if( m_translationCurve )
	{
		m_translationCurve->Update( &m_position, simTime );
	}

	// calc sun size depending on it's position in the client and some very strange, very old magic numbers
	m_sunSize = 1.f;
	if( m_translationCurve )
	{
		float distanceToCenter = Length( m_position ) / 0.1495978707e12f;
		m_sunSize = 1.5f / logf( distanceToCenter + 2.71f );
	}

	// pass important data to shader
	uint32_t fg = m_occlusionOffset ? *m_occlusionOffset : 0;
	uint32_t bg = m_backgroundOcclusionOffset ? *m_backgroundOcclusionOffset : 0;
	m_occScaleVar = Vector4( *reinterpret_cast<float*>( &fg ), *reinterpret_cast<float*>( &bg ), 0.f, 0.f );

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( realTime, simTime );
	}

	for( auto it = m_controllers.begin(); it != m_controllers.end(); ++it )
	{
		( *it )->Update( 0.5f );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Is a lot like ::Update but needs the current frame's frustum! Basiclly
//   calculates the transform for the billboard geomtry to become a 2d sprite.
//   Calculation itself is a little bit strange, but totally EVE legacy!
// Arguments:
//   frustum - the current view frustum of the current frame
// --------------------------------------------------------------------------------
void EveLensflare::PrepareRender( const TriFrustum& frustum )
{
	// debug
	if( !m_display )
	{
		return;
	}

	// calc direction to the sun (usually normalized m_position)
	if( LengthSq( m_position ) == 0.f )
	{
		Vector3 curPos( 0.f, 0.f, 0.f );
		curPos -= frustum.m_viewPos;
		m_direction = Normalize( curPos );
	}
	else
	{
		Vector3 invPos = -m_position;
		m_direction = Normalize( invPos );
	}

	Vector3 cameraSpacePos = frustum.m_viewDir * ( -1.f * m_cameraFactor );
	cameraSpacePos += frustum.m_viewPos;

	// build the matrix that will rotate the flares into position
	// by using the position of the camera and the direction to the world pos
	Vector3 negDirVec = -m_direction;
	TriMatrixArcFromForward( &m_transform, &negDirVec );
	m_transform._41 = cameraSpacePos.x;
	m_transform._42 = cameraSpacePos.y;
	m_transform._43 = cameraSpacePos.z;
	m_transform._44 = 1.0f;

	// pass important data to shader
	m_directionVar = Vector4( m_direction, m_sunSize );

	Vector4 direction( m_direction.x, m_direction.y, m_direction.z, 0 );
	direction = Transform( direction, Tr2Renderer::GetViewTransform() );
	direction = Transform( direction, frustum.m_projectionMatrix );
	direction.x /= direction.w;
	direction.y /= direction.w;
	float distanceToEdge = 1 - std::min( 1 - std::abs( direction.x ), 1 - std::abs( direction.y ) );
	float distanceToCenter = Length( *reinterpret_cast<Vector2*>( &direction ) );
	float radialAngle = atan2( direction.y, direction.x ) + TRI_PI;

	for( auto it = m_distanceToEdgeCurves.begin(); it != m_distanceToEdgeCurves.end(); ++it )
	{
		( *it )->UpdateValue( distanceToEdge );
	}

	for( auto it = m_distanceToCenterCurves.begin(); it != m_distanceToCenterCurves.end(); ++it )
	{
		( *it )->UpdateValue( distanceToCenter );
	}

	for( auto it = m_radialAngleCurves.begin(); it != m_radialAngleCurves.end(); ++it )
	{
		( *it )->UpdateValue( radialAngle );
	}

	for( auto it = m_xDistanceToCenter.begin(); it != m_xDistanceToCenter.end(); ++it )
	{
		( *it )->UpdateValue( direction.x + 10.0f );
	}

	for( auto it = m_yDistanceToCenter.begin(); it != m_yDistanceToCenter.end(); ++it )
	{
		( *it )->UpdateValue( direction.y + 10.0f );
	}

	for( auto it = m_bindings.begin(); it != m_bindings.end(); ++it )
	{
		( *it )->CopyValue();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Pass this call down to all the flares,
//   which are EveTransforms.
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveTransform
// --------------------------------------------------------------------------------
void EveLensflare::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display || !m_isVisible )
	{
		return;
	}

	// add all the single flares, which are renderbales
	for( EveTransformVector::const_iterator it = m_flares.begin(); it != m_flares.end(); ++it )
	{
		( *it )->GetRenderables( renderables, nullptr );
	}

	if( m_mesh )
	{
		renderables.push_back( this );
	}
}

void EveLensflare::UpdateVisibility( const EveUpdateContext& updateContext )
{
	m_isVisible = false;
	auto& frustum = updateContext.GetFrustum();
	// visibility?
	float viewDotDir = Dot( frustum.m_viewDir, m_direction );
	m_isVisible = viewDotDir >= 0.f;

	// update visibility for all the flares
	for( EveTransformVector::const_iterator it = m_flares.begin(); it != m_flares.end(); ++it )
	{
		( *it )->UpdateVisibility( updateContext, m_transform );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Run the 1st visibility determination by rendering small sprites and use
//   EveOccluder
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
// SeeAlso:
//   EveOccluder
// --------------------------------------------------------------------------------
void EveLensflare::RunOcclusionQueries( Tr2RenderContext& renderContext, const EveUpdateContext& updateContext )
{
	// debug
	if( !m_display )
	{
		return;
	}
	if( !m_occlusionOffset )
	{
		m_occlusionOffset = Tr2OcclusionBuffer::GetInstance().AllocateOffset();
	}
	if( !m_backgroundOcclusionOffset )
	{
		m_backgroundOcclusionOffset = Tr2OcclusionBuffer::GetInstance().AllocateOffset();
	}

	bool bufferCleared = false;

	// pass to occlusion modules
	uint32_t index = 0;
	for( auto& occluder : m_occluders )
	{
		occluder->RunQuery( renderContext, updateContext, m_transform, Tr2OcclusionBuffer::GetOccluderOffset( m_occlusionOffset, index++ ), 1 );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Run the 2nd visibility determination by rendering small sprites and use
//   EveOccluder
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
// SeeAlso:
//   EveOccluder
// --------------------------------------------------------------------------------
void EveLensflare::RunBackgroundOcclusionQueries( Tr2RenderContext& renderContext, const EveUpdateContext& updateContext )
{
	// debug
	if( !m_display )
	{
		return;
	}
	if( !m_backgroundOcclusionOffset )
	{
		m_backgroundOcclusionOffset = Tr2OcclusionBuffer::GetInstance().AllocateOffset();
	}

	bool bufferCleared = true;

	// pass to occlusion modules
	uint32_t index = 0;
	for( auto& occluder : m_backgroundOccluders )
	{
		occluder->RunQuery( renderContext, updateContext, m_transform, Tr2OcclusionBuffer::GetOccluderOffset( m_backgroundOcclusionOffset, index++ ), 0 );
	}
}

void EveLensflare::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

bool EveLensflare::HasTransparentBatches()
{
	return false;
}

float EveLensflare::GetSortValue()
{
	return 1.f;
}

Tr2PerObjectData* EveLensflare::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	auto data = accumulator->Allocate<EveLensflarePerObjectData>();
	if( data )
	{
		data->m_data.directionScale = Vector4( m_direction, m_sunSize );
		data->m_data.indices[0] = m_occlusionOffset ? *m_occlusionOffset : 0;
		data->m_data.indices[1] = m_backgroundOcclusionOffset ? *m_backgroundOcclusionOffset : 0;
	}
	return data;
}

// -------------------------------CurveSets----------------------------------------------
void EveLensflare::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			if( rangeName.empty() )
			{
				( *it )->ResetTimeRange();
				( *it )->Play();
			}
			else
			{
				( *it )->PlayTimeRange( rangeName.c_str() );
			}
		}
	}
}

void EveLensflare::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Stop();
		}
	}
}

void EveLensflare::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Update( time, time );
		}
	}
}

float EveLensflare::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetMaxCurveDuration() );
		}
	}
	return maxDuration;
}

float EveLensflare::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetRangeDuration( rangeName.c_str() ) );
		}
	}
	return maxDuration;
}

// --------------------------Controllers-----------------------------------------
void EveLensflare::SetControllerVariable( const char* name, float value )
{
	auto found = find_if( begin( m_controllerVariables ), end( m_controllerVariables ), [name]( auto& x ) { return x.first == name; } );
	if( found == end( m_controllerVariables ) )
	{
		m_controllerVariables.push_back( { name, value } );
	}
	else
	{
		found->second = value;
	}
	for( auto it = m_controllers.begin(); it != m_controllers.end(); ++it )
	{
		( *it )->SetVariable( name, value );
	}
}

void EveLensflare::HandleControllerEvent( const char* name )
{
	for( auto it = m_controllers.begin(); it != m_controllers.end(); ++it )
	{
		( *it )->HandleEvent( name );
	}
}

void EveLensflare::StartControllers()
{
	for( auto it = m_controllers.begin(); it != m_controllers.end(); ++it )
	{
		( *it )->Start();
	}
}