// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionOverlay.h"
#include "Controllers/Tr2Controller.h"
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

void Tr2ActionOverlay::Start( ITr2ActionController& controller )
{
	EveSpaceObject2Ptr owner = BlueCastPtr( controller.GetOwner() );
	bool rebind = false;

	if( !owner )
	{
		if( !m_targetAnotherOwner.empty() )
		{
			if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) )
			{
				if( EveMultiEffectParameterPtr mep = multiEffect->GetParameterByName( m_targetAnotherOwner ) )
				{
					owner = BlueCastPtr( mep->GetParameterObject() );
				}
			}
			else if( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) )
			{
				IEveSpaceObject2Ptr obj;

				if( m_targetAnotherOwner == BlueSharedString( "SourceSpaceObject" ) )
				{
					obj = stretch3->GetSourceSpaceObject();
				}

				if( m_targetAnotherOwner == BlueSharedString( "DestSpaceObject" ) )
				{
					obj = stretch3->GetDestSpaceObject();
				}

				if( obj )
				{
					owner = BlueCastPtr( obj );
				}
			}
		}

		if( !owner )
		{
			return;
		}
		rebind = true;
	}

	LoadOverlay( owner );

	if( rebind )
	{
		if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) )
		{
			multiEffect->Rebind( true );
		}
		else if( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) )
		{
			stretch3->Rebind( true );
		}
	}
}

void Tr2ActionOverlay::Stop( ITr2ActionController& controller )
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
				owner = BlueCastPtr( mep->GetParameterObject() );
			}
		}
		else if( EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ) )
		{
			IEveSpaceObject2Ptr obj;

			if( m_targetAnotherOwner == BlueSharedString( "SourceSpaceObject" ) )
			{
				obj = stretch3->GetSourceSpaceObject();
			}
			if( m_targetAnotherOwner == BlueSharedString( "DestSpaceObject" ) )
			{
				obj = stretch3->GetDestSpaceObject();
			}

			if( obj )
			{
				owner = BlueCastPtr( obj );
			}
		}
	}

	if( owner && m_removeOnStop )
	{
		owner->RemoveOverlayEffect( m_overlay );
	}
	m_overlay = nullptr;
}

void Tr2ActionOverlay::LoadOverlay( EveSpaceObject2* owner )
{
	m_overlay = nullptr;

	// check if it exists on the owner
	if( !m_overlayName.empty() )
	{
		m_overlay = owner->GetOverlayEffectByName( m_overlayName.c_str() );
	}
	if( m_addOnStart && !m_overlay && !m_path.empty() )
	{
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

		bool urgent = BeResMan->IsUrgentResourceLoads();
		BeResMan->SetUrgentResourceLoads( true );
		m_overlay = BeResMan->LoadObject<EveMeshOverlayEffect>( path.c_str() );
		BeResMan->SetUrgentResourceLoads( urgent );
		if( m_overlay )
		{
			if( !m_overlayName.empty() )
			{
				m_overlay->m_name = m_overlayName;
			}
			if( m_addOnStart )
			{
				owner->AddOverlayEffect( m_overlay );
				m_overlay->StartControllers();
			}
		}
	}
}
