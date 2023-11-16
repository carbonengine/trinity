////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#pragma once

#include "IEveProceduralSelectionMethod.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "EveProceduralMethodAttributeMapParameter.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveProceduralMethodAttributeMapParameter );
BLUE_DECLARE_VECTOR( EveProceduralMethodAttributeMapParameter );
BLUE_DECLARE_INTERFACE( IEveVolume );


BLUE_CLASS( EveProceduralMethodAttributeMap ) :
	public IEveProceduralSelectionMethod,
    public INotify
{
public:
	EXPOSE_TO_BLUE();

    EveProceduralMethodAttributeMap( IRoot* lockobj = NULL );
	~EveProceduralMethodAttributeMap();

    void SelectParameter();

    //  IEveProceduralSelectionMethod
    void UpdateASyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
    bool IsSelectedChildModified() const override;
    EveChildRefPtr GetSelectedChild() override;
    IEveVolumeVector* GetDebugVolumes() override;

    //  INotify
    bool OnModified( Be::Var* value ) override;

protected:
    BlueSharedString m_mappedAttribute;
    BlueSharedString m_seed;
    int m_selectedChildIndex;
    PEveProceduralMethodAttributeMapParameterVector m_parameters;
private:
    bool m_selectedChildModified;
    PIEveVolumeVector m_debugVolumes;
};

TYPEDEF_BLUECLASS( EveProceduralMethodAttributeMap );