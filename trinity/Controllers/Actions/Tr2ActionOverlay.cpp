////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionOverlay.h"
#include "Controllers/Tr2Controller.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Utilities/StringUtils.h"
#include "Eve/EveMultiEffect.h"
#include "Eve/EveMultiEffectParameter.h"
#include "Eve/Renderable/Stretch/EveStretch3.h"

Tr2ActionOverlay::Tr2ActionOverlay( IRoot* ) :
        m_addOnStart( true ),
        m_removeOnStop( true ),
        m_targetAnotherOwner( "" )
{
}

void Tr2ActionOverlay::Start( Tr2Controller& controller )
{
	EveSpaceObject2Ptr owner = BlueCastPtr( controller.GetOwner() );
    bool rebind = false;

	if( !owner )
	{
		if( !m_targetAnotherOwner.empty() )
		{
			if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) ) {
                if( EveMultiEffectParameterPtr mep = multiEffect->GetParameterByName( m_targetAnotherOwner ) )
                {
                    owner = BlueCastPtr( mep->GetParameterObject() );
                }
            } else if ( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) ) {
                IEveSpaceObject2Ptr obj;

                if (m_targetAnotherOwner == BlueSharedString("SourceSpaceObject" ) ) {
                    obj = stretch3->GetSourceSpaceObject();
                }

                if ( m_targetAnotherOwner == BlueSharedString("DestSpaceObject" ) ) {
                    obj = stretch3->GetDestSpaceObject();
                }

                if ( obj ) {
                    owner = BlueCastPtr(obj);
                }
            }
        }

		if( !owner )
		{
			return;
		}
        rebind = true;
	}
	
	auto path = m_path;
	std::transform( path.begin(), path.end(), path.begin(), ::tolower );

	if( owner->IsAnimated() && !StringFind( path.c_str(), "_skinned" ) )
	{
		StringInsertStubBefore( path, ".red", "_skinned" );
	}
	else if( !owner->IsAnimated() && StringFind( path.c_str(), "_skinned" ) )
	{
		StringRemove( path, "_skinned" );
	}

	BeResMan->SetUrgentResourceLoads( true );
	m_overlay = BeResMan->LoadObject<EveMeshOverlayEffect>( path.c_str() );
	BeResMan->SetUrgentResourceLoads( false );
	if( m_overlay && m_addOnStart)
	{
		owner->AddOverlayEffect( m_overlay );
        m_overlay->StartControllers();
	}

    if( rebind )
    {
        if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) )
        {
            multiEffect->Rebind( true );
        } else if( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) )
        {
            stretch3->Rebind( true );
        }
    }
}

void Tr2ActionOverlay::Stop( Tr2Controller& controller )
{
	if( !m_overlay )
	{
		return;
	}


    EveSpaceObject2Ptr owner = BlueCastPtr( controller.GetOwner() );

    if( !owner && !m_targetAnotherOwner.empty() )
    {
        if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) )
        {
            if( EveMultiEffectParameterPtr mep = multiEffect->GetParameterByName( m_targetAnotherOwner ) )
            {
                owner = BlueCastPtr(mep->GetParameterObject());
            }
        } else if ( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) ){

            IEveSpaceObject2Ptr obj;

            if (m_targetAnotherOwner == BlueSharedString("SourceSpaceObject")) {
                obj = stretch3->GetSourceSpaceObject();
            }
            if (m_targetAnotherOwner == BlueSharedString("DestSpaceObject")) {
                obj = stretch3->GetDestSpaceObject();
            }

            if (obj) {
                owner = BlueCastPtr(obj);
            }
        }
    }

	if( owner && m_removeOnStop)
	{
		owner->RemoveOverlayEffect( m_overlay );
	}
}
