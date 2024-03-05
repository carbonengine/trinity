////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#pragma once

#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveProceduralMethodCyclingParameter ) :
    public INotify,
    public IInitialize
{
public:
    EXPOSE_TO_BLUE();
    EveProceduralMethodCyclingParameter( IRoot* lockobj = NULL );
	~EveProceduralMethodCyclingParameter();

    const char* GetName() const;
    void SetModified( bool isModified );
    bool IsModified() const;
    EveChildRefPtr GetChild();
    void Load();
    float GetDuration() const;

    //  INotify
    bool OnModified( Be::Var* value ) override;

    //  IInitialize
    bool Initialize() override;
protected:
	BlueSharedString m_name;
    EveChildRefPtr m_child;

private:
    bool m_modified;
    bool m_hasLoaded;
    bool m_restartRequired;
    bool m_reloadRequired;
    float m_playDuration;
};

TYPEDEF_BLUECLASS( EveProceduralMethodCyclingParameter );