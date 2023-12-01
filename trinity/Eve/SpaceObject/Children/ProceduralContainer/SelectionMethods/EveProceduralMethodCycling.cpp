////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#include "EveProceduralMethodCycling.h"


EveProceduralMethodCycling::EveProceduralMethodCycling( IRoot* lockobj ) :
    PARENTLOCK( m_parameters ),
    PARENTLOCK( m_debugVolumes ),
    m_selectedChildIndex( -1 ),
    m_selectedChildModified( false ),
    m_startTime( 0 ),
    m_startTimeOffset( 0.0 ),
	m_randomizeOrder( false )
{
    m_parameters.SetNotify( this );
}

EveProceduralMethodCycling::~EveProceduralMethodCycling()
{
}


bool EveProceduralMethodCycling::OnModified( Be::Var* value )
{
    return true;
}

void EveProceduralMethodCycling::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* list )
{
    if( list == &m_parameters && ( event & BELIST_LOADING ) == 0 )
    {
        SelectParameter();
    }
}

void EveProceduralMethodCycling::SelectParameter()
{
    if (m_parameters.empty())
    {
        return;
    }

	if ( m_randomizeOrder && m_parameters.size() > 2 )
	{
		int oldIndex = m_selectedChildIndex;
		m_selectedChildIndex =  rand() % ( (int)(m_parameters.size()) - 1 );
		if ( m_selectedChildIndex >= oldIndex )
		{
			m_selectedChildIndex++;
		}
	}
	else
	{
		m_selectedChildIndex++;
		m_selectedChildIndex %= (int)m_parameters.size();
	}

    m_startTime = BeOS->GetCurrentFrameTime() - TimeFromDouble((double) m_startTimeOffset);
    m_selectedChildModified = true;
}

bool EveProceduralMethodCycling::IsSelectedChildModified() const
{
    return m_selectedChildModified;
}

EveChildRefPtr EveProceduralMethodCycling::GetSelectedChild()
{
    if ( m_selectedChildIndex < 0 || m_selectedChildIndex > (m_parameters.size() - 1) )
    {
        return nullptr;
    }

    m_selectedChildModified = false;
    EveProceduralMethodCyclingParameterPtr param = BlueCastPtr( m_parameters.GetAt( m_selectedChildIndex ) );

    if( param != nullptr )
    {
        auto child = param->GetChild();
        if( child != nullptr && strlen( child->GetResPath()) != 0 )
        {
            param->Load();
            return child;
        }
    }
    return nullptr;
}

void EveProceduralMethodCycling::UpdateASyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
    if ( m_selectedChildIndex < 0 || m_selectedChildIndex > (m_parameters.size() - 1) )
    {
        SelectParameter();
        return;
    }
    EveProceduralMethodCyclingParameterPtr param = BlueCastPtr( m_parameters.GetAt( m_selectedChildIndex ) );

    if( param != nullptr )
    {
        float elapsedTime = TimeAsFloat( BeOS->GetCurrentFrameTime() - m_startTime );
        if (elapsedTime >= param->GetDuration())
        {
            SelectParameter();
        }
    }
}

IEveVolumeVector* EveProceduralMethodCycling::GetDebugVolumes()
{
    return &m_debugVolumes;
}