////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#include "EveProceduralMethodCyclingParameter.h"


EveProceduralMethodCyclingParameter::EveProceduralMethodCyclingParameter( IRoot* lockobj ) :
    m_modified( false ),
    m_hasLoaded( false ),
    m_restartRequired( true ),
    m_reloadRequired( false ),
    m_playDuration( 1.f )
{
}

EveProceduralMethodCyclingParameter::~EveProceduralMethodCyclingParameter()
{
}

bool EveProceduralMethodCyclingParameter::Initialize()
{
    if( m_child == nullptr )
    {
        m_child.CreateInstance();
    }
    return true;
}

bool EveProceduralMethodCyclingParameter::OnModified( Be::Var* value )
{
    if( IsMatch( value, m_child ) )
    {
        if( m_child != nullptr )
        {
            m_child->SetAutoLoadBlocker(true);
        }
    }

    return true;
}

void EveProceduralMethodCyclingParameter::SetModified( bool isModified )
{
    m_modified = isModified;
}

bool EveProceduralMethodCyclingParameter::IsModified() const
{
    return m_modified;
}

const char* EveProceduralMethodCyclingParameter::GetName() const
{
    return m_name.c_str();
}

EveChildRefPtr EveProceduralMethodCyclingParameter::GetChild()
{
    return m_child;
}

float EveProceduralMethodCyclingParameter::GetDuration() const
{
    return m_playDuration;
}

void EveProceduralMethodCyclingParameter::Load()
{
    if( m_hasLoaded && !m_reloadRequired )
    {
        if( m_restartRequired )
        {
            m_child->StartControllers();
            m_child->PlayAllCurveSets();
        }
        return;
    }

    if( m_child != nullptr )
    {
        m_child->Reload(true);
        m_hasLoaded = true;
    }
}