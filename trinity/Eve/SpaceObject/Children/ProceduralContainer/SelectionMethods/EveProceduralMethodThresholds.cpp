////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#include "EveProceduralMethodThresholds.h"


EveProceduralMethodThresholds::EveProceduralMethodThresholds( IRoot* lockobj ) :
    PARENTLOCK( m_parameters ),
    PARENTLOCK( m_debugVolumes ),
    m_seed( -1.f ),
    m_selectedChildIndex( -1 ),
    m_selectedChildModified( false )
{
    m_parameters.SetNotify( this );
}

EveProceduralMethodThresholds::~EveProceduralMethodThresholds()
{
}

bool EveProceduralMethodThresholds::Initialize()
{
    SortParameters();
    return true;
}

bool EveProceduralMethodThresholds::OnModified( Be::Var* value )
{
    if( IsMatch( value, m_seed ) )
    {
        SelectParameter();
    }

    return true;
}

void EveProceduralMethodThresholds::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* list )
{
    if( list == &m_parameters && ( event & BELIST_LOADING ) == 0 )
    {
        SortParameters();
    }
}

void EveProceduralMethodThresholds::SelectParameter()
{
    int currentChild = m_selectedChildIndex;

    int index = 0;
    for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
    {
        if( m_seed <= ( *it )->GetThreshold() )
        {
            m_selectedChildIndex = index;
            break;
        }
        index++;
    }

    if( currentChild != m_selectedChildIndex)
    {
        m_selectedChildModified = true;
    }
}

//  A Struct declaration to use for sorting in function SortParameters()
struct SelectiveParameterCompare
{
    inline bool operator() (const EveProceduralMethodThresholdParameter* param1, const EveProceduralMethodThresholdParameter* param2)
    {
        return ( param1->GetThreshold() < param2->GetThreshold() );
    }
};

void EveProceduralMethodThresholds::SortParameters()
{
    if(m_parameters.size() < 2 )
    {
        return;
    }

    std::sort(begin( m_parameters ), end( m_parameters ), SelectiveParameterCompare() );
}

bool EveProceduralMethodThresholds::IsSelectedChildModified() const
{
    return m_selectedChildModified;
}

EveChildRefPtr EveProceduralMethodThresholds::GetSelectedChild()
{
    if ( m_selectedChildIndex < 0 || m_selectedChildIndex > (m_parameters.size() - 1) )
    {
        return nullptr;
    }

    m_selectedChildModified = false;
    EveProceduralMethodThresholdParameterPtr param = BlueCastPtr( m_parameters.GetAt( m_selectedChildIndex ) );

    if( param != nullptr )
    {
        auto child = param->GetChild();
        if( child != nullptr && strlen(child->GetResPath()) != 0 )
        {
            param->Load();
            return child;
        }
    }
    return nullptr;
}

void EveProceduralMethodThresholds::UpdateASyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
    bool reselect = false;
    for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
    {
        if( ( *it )->IsModified() )
        {
            reselect = true;
            ( *it )->SetModified( false );
        }
    }
    if( reselect )
    {
        SortParameters();
        SelectParameter();
    }
}

IEveVolumeVector* EveProceduralMethodThresholds::GetDebugVolumes()
{
    return &m_debugVolumes;
}

void EveProceduralMethodThresholds::SetProceduralMethodVariable( const char* name, float value )
{
    if( strcmp( name, m_thresholdAttribute.c_str() ) == 0)
    {
        if( m_seed != value)
        {
            m_seed = value;
            SelectParameter();
        }
    }
}