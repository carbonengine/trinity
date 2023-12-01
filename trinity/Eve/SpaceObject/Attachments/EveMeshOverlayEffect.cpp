////////////////////////////////////////////////////////////
//
//    Created:   January 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveMeshOverlayEffect.h"
#include "Shader/Tr2Effect.h"
#include "Curves/TriCurveSet.h"
#include "Controllers/ITr2Controller.h"


// --------------------------------------------------------------------------------------
// Description:
//   EveMeshOverlayEffect destructor
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::~EveMeshOverlayEffect()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   EveMeshOverlayEffect constructor
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::EveMeshOverlayEffect( IRoot* lockobj ):
	m_display( true ),
	m_update( true ),
	PARENTLOCK( m_opaqueEffects ),
	PARENTLOCK( m_decalEffects ),
	PARENTLOCK( m_transparentEffects ),
	PARENTLOCK( m_additiveEffects ),
	PARENTLOCK( m_distortionEffects ),
    PARENTLOCK( m_controllers )
{
    m_controllers.SetNotify( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   IInitialize
// --------------------------------------------------------------------------------------
bool EveMeshOverlayEffect::Initialize()
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

// --------------------------------------------------------------------------------------
// Description:
//   IListNotify
// --------------------------------------------------------------------------------------
void EveMeshOverlayEffect::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
    if (list == &m_controllers && (event & BELIST_LOADING) == 0) {
        switch (event & BELIST_EVENTMASK) {
        case BELIST_INSERTED:
            if (ITr2ControllerPtr controller = BlueCastPtr(value)) {
                controller->Link(*GetRawRoot());
            }
            break;
        case BELIST_REMOVED:
            if (ITr2ControllerPtr controller = BlueCastPtr(value)) {
                controller->Unlink();
            }
            break;
        default:
            break;
        }
    }
}

// --------------------------------------------------------------------------------------
// Description:
//   For each batch type we gove back the appropriate overlay type!
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::OverlayType EveMeshOverlayEffect::GetType( TriBatchType batchType ) const
{
	switch( batchType )
	{
	case TRIBATCHTYPE_OPAQUE:
		return TYPE_OPAQUEONLY;
	default:
		return TYPE_ALL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Tell if this overlay effect holds any effects for transparent areas
// --------------------------------------------------------------------------------------
bool EveMeshOverlayEffect::HasTransparentArea() const
{
	return !m_transparentEffects.empty();
}

inline void SetIndividualShaderOption( const PTr2EffectVector& effectVector, const BlueSharedString& name, const BlueSharedString& value )
{
	for (auto it = effectVector.begin(); it != effectVector.end(); ++it)
	{
		Tr2Effect *effect = *it;
		effect->SetOption( name, value );
	}
}

void EveMeshOverlayEffect::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	SetIndividualShaderOption( m_opaqueEffects, name, value );
	SetIndividualShaderOption( m_decalEffects, name, value );
	SetIndividualShaderOption( m_transparentEffects, name, value );
	SetIndividualShaderOption( m_additiveEffects, name, value );
	SetIndividualShaderOption( m_distortionEffects, name, value );
}


// --------------------------------------------------------------------------------------
// Description:
//   GetEffect. 
// Return Value:
//   A Tr2EffectVector of effects.
// --------------------------------------------------------------------------------------
const PTr2EffectVector& EveMeshOverlayEffect::GetEffects(TriBatchType batchType, bool& success) const
{
	if ( m_display )
	{
		if ( batchType == TRIBATCHTYPE_OPAQUE )
		{
			success = true;
			return m_opaqueEffects;
		}
		else if ( batchType == TRIBATCHTYPE_DECAL )
		{
			success = true;
			return m_decalEffects;
		}
		else if ( batchType == TRIBATCHTYPE_TRANSPARENT )
		{
			success = true;
			return m_transparentEffects;
		}
		else if ( batchType == TRIBATCHTYPE_ADDITIVE )
		{
			success = true;
			return m_additiveEffects;
		}
		else if ( batchType == TRIBATCHTYPE_DISTORTION )
		{
			success = true;
			return m_distortionEffects;
		}
	}
	success = false;
	return m_opaqueEffects;
}

// --------------------------------------------------------------------------------
// ITr2ControllerOwner

void EveMeshOverlayEffect::SetControllerVariable( const char* name, float value )
{
    for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
    {
        ( *it )->SetVariable( name, value );
    }
}


void EveMeshOverlayEffect::HandleControllerEvent( const char* name )
{
    for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
    {
        ( *it )->HandleEvent( name );
    }
}

void EveMeshOverlayEffect::StartControllers()
{
    for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
    {
        ( *it )->Start();
    }
}

// --------------------------------------------------------------------------------
// ITr2CurveSetOwner

void EveMeshOverlayEffect::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
    if( !m_curveSet )
    {
        return;
    }

    if( m_curveSet->GetName() == name )
    {
        if( rangeName.empty() )
        {
            m_curveSet->ResetTimeRange();
            m_curveSet->Play();
        }
        else
        {
            m_curveSet->PlayTimeRange( rangeName.c_str() );
        }
    }
}

void EveMeshOverlayEffect::StopCurveSet( const std::string& name )
{
    if( !m_curveSet )
    {
        return;
    }

    if( m_curveSet->GetName() == name )
    {
        m_curveSet->Stop();
    }
}

float EveMeshOverlayEffect::GetCurveSetDuration( const std::string& name ) const
{
    float maxDuration = 0.f;

    if( !m_curveSet )
    {
        return maxDuration;
    }

    if( m_curveSet->GetName() == name )
    {
        maxDuration = max( maxDuration, m_curveSet->GetMaxCurveDuration() );
    }
    return maxDuration;
}

float EveMeshOverlayEffect::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
    float maxDuration = 0.f;

    if( !m_curveSet )
    {
        return maxDuration;
    }

    if( m_curveSet->GetName() == name )
    {
        maxDuration = max( maxDuration, m_curveSet->GetRangeDuration( rangeName.c_str() ) );
    }

    return maxDuration;
}

// --------------------------------------------------------------------------------------
// Description:
//   Update
// --------------------------------------------------------------------------------------
void EveMeshOverlayEffect::Update( Be::Time realTime, Be::Time simTime )
{
	if( !m_update || !m_curveSet )
	{
		return;
	}

	m_curveSet->Update( realTime, simTime );

    for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
    {
        ( *it )->Update();
    }
}