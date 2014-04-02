////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef TriStepSetVariableStore_H
#define TriStepSetVariableStore_H


#include "TriRenderStep.h"
#include "Tr2VariableStore.h"

#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Include/ITriTextureRes.h"

BLUE_DECLARE( TriTextureRes );

// -------------------------------------------------------------
// Description:
//   A render step to set a variable in a global variable store.
// SeeAlso:
//   TriRenderStep, Tr2VariableStore
// -------------------------------------------------------------
BLUE_CLASS( TriStepSetVariableStore ): 
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepSetVariableStore( IRoot* lockobj = 0);
	~TriStepSetVariableStore(void);

#if BLUE_WITH_PYTHON
	void py__init__( Be::Optional<std::string> name, PyObject* value );
#endif

	TriStepResult Execute( Be::Time time, Tr2RenderContext& renderContext );

	void SetName( const std::string& newName )
	{
		m_variableName = newName;
	}

#if BLUE_WITH_PYTHON
	PyObject* GetValue();
	void SetValue( PyObject* val );
#endif

private:
	std::string m_variableName;
	TriVariableContentType m_type;
	IRootPtr m_object;
	ITriTextureResPtr m_texture;
	TriTextureResPtr m_textureRes;
	Tr2DepthStencilPtr	m_depthStencil;
	Tr2RenderTargetPtr	m_renderTarget;
	char m_data[ sizeof(float) * 16 ];
};

TYPEDEF_BLUECLASS( TriStepSetVariableStore );

#endif // TriStepSetVariableStore_H
