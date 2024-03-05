////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionChildEffect.h"
#include "Controllers/Tr2Controller.h"
#include "Eve/SpaceObject/Children/IEveEffectChildrenOwner.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/EveMultiEffect.h"
#include "Eve/EveMultiEffectParameter.h"
#include "Eve/Renderable/Stretch/EveStretch3.h"


namespace
{
void PrefetchResFile( void* pContext )
{
	std::wstring* path = static_cast<std::wstring*>( pContext );

	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr stream( resFileClsid );
	stream->OpenW( path->c_str(), true );
	delete path;
}

std::unordered_set<std::string> s_requestedPaths;

}

Tr2ActionChildEffect::Tr2ActionChildEffect( IRoot* ) :
	m_addOnStart( true ),
	m_removeOnStop( true ),
	m_targetAnotherOwner( "" )
{
}

void Tr2ActionChildEffect::Link( Tr2Controller& controller )
{
	if( m_path.empty() )
	{
		return;
	}

	if( s_requestedPaths.find( m_path ) != end( s_requestedPaths ) )
	{
		return;
	}
	s_requestedPaths.insert( m_path );

	std::wstring* wstrCopy = new std::wstring( m_path.begin(), m_path.end() );

	if( !BePaths->FileExistsLocally( wstrCopy->c_str() ) )
	{
		// make sure that the res file exists
		BeResMan->AddToQueue(
			BRMQ_BACKGROUND,
			PrefetchResFile,
			wstrCopy,
			IBlueCallbackMan::BCBF_URGENT,
			nullptr );
	}
	else
	{
		delete wstrCopy;
	}
}


void Tr2ActionChildEffect::Start( Tr2Controller& controller )
{
	IEveEffectChildrenOwnerPtr owner = BlueCastPtr( controller.GetOwner() );
	bool rebind = false;

	if( owner && !m_targetAnotherOwner.empty() )
	{
		owner = BlueCastPtr( owner->GetEffectChildByName( m_targetAnotherOwner.c_str() ) );
	}

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
			} else if (EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ))
            {
                if ( m_targetAnotherOwner == BlueSharedString("SourceSpaceObject") )
                {
                    IEveSpaceObject2Ptr source = stretch3->GetSourceSpaceObject();
                    owner = BlueCastPtr( source );
                }

                if ( m_targetAnotherOwner == BlueSharedString("DestSpaceObject") )
                {
                    IEveSpaceObject2Ptr dest = stretch3->GetDestSpaceObject();
                    owner = BlueCastPtr( dest );
                }
            }
		}
		if( !owner )
		{
			return;
		}
        rebind = true;
	}

	m_child = nullptr;
	if( !m_childName.empty() )
	{
		m_child = owner->GetEffectChildByName( m_childName.c_str() );
	}
	if( m_addOnStart && !m_child && !m_path.empty() )
	{
		m_child = BeResMan->LoadObject<IEveSpaceObjectChild>( m_path.c_str() );
		if( m_child )
		{
			if( !m_childName.empty() )
			{
				m_child->SetName( m_childName.c_str() );
			}
			owner->AddToEffectChildrenList( m_child );
			m_child->StartControllers();
		}
        if( rebind )
        {
            EveMultiEffectPtr effect = BlueCastPtr( controller.GetOwner() );
            if( effect )
            {
                effect->Rebind( true );
            }
        }
	}
}

void Tr2ActionChildEffect::Stop( Tr2Controller& controller )
{
	if( m_child && m_removeOnStop )
	{
		IEveEffectChildrenOwnerPtr owner = BlueCastPtr( controller.GetOwner() );

		if( owner && !m_targetAnotherOwner.empty() )
		{
			owner = BlueCastPtr( owner->GetEffectChildByName( m_targetAnotherOwner.c_str() ) );
		}

		if( owner )
		{
			owner->RemoveFromEffectChildrenList( m_child );
		}
		else
		{
			EveMultiEffectPtr effect = BlueCastPtr( controller.GetOwner() );
			if( effect && !m_targetAnotherOwner.empty() )
			{
				EveMultiEffectParameter* mep = effect->GetParameterByName( m_targetAnotherOwner );

				if( !mep )
				{
					return;
				}

				auto obj = mep->GetParameterObject();
				auto cast = dynamic_cast<IEveEffectChildrenOwner*>( obj );
				if( cast )
				{
					cast->RemoveFromEffectChildrenList( m_child );
				}
			}
            else if (EveStretch3Ptr stretch3 = BlueCastPtr( controller.GetOwner() ))
            {
                IEveEffectChildrenOwnerPtr cast;

                if ( m_targetAnotherOwner == BlueSharedString("SourceSpaceObject") )
                {
                    IEveSpaceObject2Ptr source = stretch3->GetSourceSpaceObject();
                    cast = BlueCastPtr( source );
                }

                if ( m_targetAnotherOwner == BlueSharedString("DestSpaceObject") )
                {
                    IEveSpaceObject2Ptr dest = stretch3->GetDestSpaceObject();
                    cast = BlueCastPtr( dest );
                }

                if( cast )
                {
                    cast->RemoveFromEffectChildrenList( m_child );
                }
            }
		}
	}
	m_child = nullptr;
}
