////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "TriStepSetVariableStore.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"

TriStepSetVariableStore::TriStepSetVariableStore( IRoot* lockobj )
:	m_type( TRIVARIABLE_INVALID )
{
	memset( m_data, 0, sizeof( m_data ) );
}

TriStepSetVariableStore::~TriStepSetVariableStore(void)
{

}

TriStepResult TriStepSetVariableStore::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	switch( m_type )
	{
	case TRIVARIABLE_TEXTURE_RES:
		GlobalStore().RegisterVariable( m_variableName.c_str(), m_texture );
		break;
	case TRIVARIABLE_GPUBUFFER:
		GlobalStore().RegisterVariable( m_variableName.c_str(), m_gpuBuffer );
		break;
	case TRIVARIABLE_INT:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<int*>( m_data ) );
		break;
	case TRIVARIABLE_FLOAT:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<float*>( static_cast<void*>( m_data ) ) );
		break;
	case TRIVARIABLE_FLOAT2:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<Vector2*>( m_data ) );
		break;
	case TRIVARIABLE_FLOAT3:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<Vector3*>( m_data ) );
		break;
	case TRIVARIABLE_FLOAT4:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<Vector4*>( m_data ) );
		break;
	case TRIVARIABLE_FLOAT4X4:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<Matrix*>( m_data ) );
		break;
	case TRIVARIABLE_COLOR:
		GlobalStore().RegisterVariable( m_variableName.c_str(), *reinterpret_cast<Color*>( m_data ) );
		break;

	default:
		break;
	}
	return RS_OK;
}