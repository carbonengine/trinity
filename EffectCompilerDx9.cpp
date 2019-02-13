#include "stdafx.h"
#include "EffectCompilerDx9.h"
#include "EffectData.h"
#include "CompileMessageQueue.h"
#include "YamlOutput.h"
#include <regex>

extern CompileMessageQueue g_messages;
extern StringTable g_stringTable;
extern bool g_printWarnings;
extern unsigned g_optimizationLevel;
extern bool g_avoidFlowControl;

IDirect3DDevice9Ex* g_device9 = nullptr;
CRITICAL_SECTION g_analyzeEffectDx9CS;


extern void PrintStageInfo( YamlOutput& listing, const StageInput& stage, const EffectData& result );


template<class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT> regex_replacecb( BidirIt first, BidirIt last,
	const std::basic_regex<CharT, Traits>& re, UnaryFunction f )
{
	std::basic_string<CharT> s;

	typename std::match_results<BidirIt>::difference_type
		positionOfLastMatch = 0;
	auto endOfLastMatch = first;

	auto callback = [&]( const std::match_results<BidirIt>& match )
	{
		auto positionOfThisMatch = match.position( 0 );
		auto diff = positionOfThisMatch - positionOfLastMatch;

		auto startOfThisMatch = endOfLastMatch;
		std::advance( startOfThisMatch, diff );

		s.append( endOfLastMatch, startOfThisMatch );
		s.append( f( match ) );

		auto lengthOfMatch = match.length( 0 );

		positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

		endOfLastMatch = startOfThisMatch;
		std::advance( endOfLastMatch, lengthOfMatch );
	};

	std::regex_iterator<BidirIt> begin( first, last, re ), end;
	std::for_each( begin, end, callback );

	s.append( endOfLastMatch, last );

	return s;
}

template<class Traits, class CharT, class UnaryFunction>
std::string regex_replacecb( const std::string& s,
	const std::basic_regex<CharT, Traits>& re, UnaryFunction f )
{
	return regex_replacecb( s.cbegin(), s.cend(), re, f );
}


class SamplerSetupFullDx9
{
public:
	SamplerSetupFullDx9()
	{
		SetState( D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
		SetState( D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
		SetState( D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP );
		SetState( D3DSAMP_BORDERCOLOR, 0 );
		SetState( D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		SetState( D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		SetState( D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		SetState( D3DSAMP_MIPMAPLODBIAS, 0 );
		SetState( D3DSAMP_MAXMIPLEVEL, 0 );
		SetState( D3DSAMP_MAXANISOTROPY, 4 );
		SetState( D3DSAMP_SRGBTEXTURE, 0 );
		SetState( D3DSAMP_ELEMENTINDEX, 0 );
	}

	void SetState( D3DSAMPLERSTATETYPE type, DWORD value )
	{
		unsigned int ix = (unsigned int)type - SAMPLER_STATE_MIN;
		m_states[ix] = value;
	}
	DWORD GetState( D3DSAMPLERSTATETYPE type )
	{
		return m_states[type - SAMPLER_STATE_MIN];
	}

	std::string m_name;
	std::string m_textureName;

	static const unsigned int SAMPLER_STATE_MIN = 1;
	static const unsigned int SAMPLER_STATE_COUNT = 12;
private:
	DWORD m_states[SAMPLER_STATE_COUNT];
};

typedef std::map<int, SamplerSetupFullDx9> SamplerSetupFullDx9Map;

class EffectAnalyzerDx9 : public ID3DXEffectStateManager
{
public:
	// Constructor
	EffectAnalyzerDx9() :
		m_pixelConstantValueCount( 0 ),
		m_vertexConstantValueCount( 0 )
	{
	}

	bool AnalyzeEffect( EffectData& effectData, ID3DXEffect* fx, YamlOutput& listing, ID3DXBuffer* shaderText );
	void Reset();

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID *ppv) ;
	virtual ULONG STDMETHODCALLTYPE AddRef() ;
	virtual ULONG STDMETHODCALLTYPE Release() ;

	// ID3DXEffectStateManager
	virtual HRESULT STDMETHODCALLTYPE SetTransform( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix) ;
	virtual HRESULT STDMETHODCALLTYPE SetMaterial( CONST D3DMATERIAL9 *pMaterial) ;
	virtual HRESULT STDMETHODCALLTYPE SetLight( DWORD Index, CONST D3DLIGHT9 *pLight) ;
	virtual HRESULT STDMETHODCALLTYPE LightEnable( DWORD Index, BOOL Enable) ;
	virtual HRESULT STDMETHODCALLTYPE SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) ;
	virtual HRESULT STDMETHODCALLTYPE SetTexture( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture) ;
	virtual HRESULT STDMETHODCALLTYPE SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) ;
	virtual HRESULT STDMETHODCALLTYPE SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) ;
	virtual HRESULT STDMETHODCALLTYPE SetNPatchMode( FLOAT NumSegments) ;
	virtual HRESULT STDMETHODCALLTYPE SetFVF( DWORD FVF) ;
	virtual HRESULT STDMETHODCALLTYPE SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader) ;
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount) ;
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount) ;
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount) ;
	virtual HRESULT STDMETHODCALLTYPE SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader) ;
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount) ;
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount) ;
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount) ;

	// A collection of render states set by the effect in the current pass
	RenderStates m_renderStates;

	// Vertex shader set for current pass
	CComPtr<IDirect3DVertexShader9> m_vertexShader;

	// Pixel shader set for current pass
	CComPtr<IDirect3DPixelShader9> m_pixelShader;

	// Vertex and pixel shader constant values set for the current pass.
	// This captures any constants that are given values in the .fx file.
	static const int CONSTANTS_MAX = 256;
	unsigned int m_pixelConstantValueCount;
	Vector4 m_pixelConstantValues[CONSTANTS_MAX];
	unsigned int m_vertexConstantValueCount;
	Vector4 m_vertexConstantValues[CONSTANTS_MAX];

	// Dummy textures are created for each texture parameter found in the
	// effect. This allows us to capture the mapping of textures parameters
	// to samplers.
	typedef std::vector<CComPtr<IDirect3DBaseTexture9>> TextureVector;
	typedef std::map<IDirect3DBaseTexture9*, std::string> TextureNameMap;
	TextureNameMap m_textureToName;
	TextureVector m_textures;
	SamplerSetupFullDx9Map m_samplers;

	void ExtractConstantTable( void* func, std::vector<Constant>& constants, ID3DXEffect* fx, int samplerOffset );
};






HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::QueryInterface( REFIID iid, LPVOID *ppv )
{
	if( iid == IID_ID3DXEffectStateManager )
	{
		*ppv = (void*)(ID3DXEffectStateManager*)this;
	}
	return S_FALSE;
}

ULONG STDMETHODCALLTYPE EffectAnalyzerDx9::AddRef()
{
	return 1;
}

ULONG STDMETHODCALLTYPE EffectAnalyzerDx9::Release()
{
	return 1;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetTransform( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetTransform( %d, <matrix> ) encountered when analyzing DX9 effect", State );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetMaterial( CONST D3DMATERIAL9 *pMaterial )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetMaterial( <material> ) encountered when analyzing DX9 effect" );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetLight( DWORD Index, CONST D3DLIGHT9 *pLight )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetLight( %d, <light> ) encountered when analyzing DX9 effect", Index );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::LightEnable( DWORD Index, BOOL Enable )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: LightEnable( %d, %d ) encountered when analyzing DX9 effect", Index, Enable );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetRenderState( D3DRENDERSTATETYPE state, DWORD value )
{
	switch( state )
	{
	case D3DRS_LOCALVIEWER:
		// We've hijacked this flag temporarily for identifying character shaders
		// Cullmode for character shader is the same for left and right handed mode.
		// This is a dirty hack that should go away once character rendering is
		// consistent with the rest of the world.
		//m_renderStates.SetState( D3DRS_CULLMODE, D3DCULL_CW );
		break;
	case D3DRS_VERTEXBLEND:
		// This realy is BlendOpAlpha
		m_renderStates[D3DRS_BLENDOPALPHA] = value;
		break;
	case D3DRS_ALPHATESTENABLE:
	case D3DRS_SRCBLEND:
	case D3DRS_DESTBLEND:
	case D3DRS_ALPHAREF:
	case D3DRS_ALPHAFUNC:
	case D3DRS_ALPHABLENDENABLE:
	case D3DRS_BLENDOP:
	case D3DRS_ZENABLE:
	case D3DRS_ZWRITEENABLE:
	case D3DRS_ZFUNC:
	case D3DRS_FILLMODE:
	case D3DRS_COLORWRITEENABLE:
	case D3DRS_DEPTHBIAS:
	case D3DRS_SLOPESCALEDEPTHBIAS:
	case D3DRS_SRGBWRITEENABLE:
	case D3DRS_SEPARATEALPHABLENDENABLE:
	case D3DRS_BLENDOPALPHA:
	case D3DRS_SRCBLENDALPHA:
	case D3DRS_DESTBLENDALPHA:
	case D3DRS_COLORWRITEENABLE1:
	case D3DRS_COLORWRITEENABLE2:
	case D3DRS_COLORWRITEENABLE3:
		m_renderStates[state] = value;
		break;
	case D3DRS_CULLMODE:
		if( value == D3DCULL_CCW )
		{
			value = D3DCULL_CW;
		}
		else if( value == D3DCULL_CW )
		{
			value = D3DCULL_CCW;
		}
		m_renderStates[state] = value;
		break;

	default:
		g_messages.AddMessage( "\\memory(0): warning X0000: SetRenderState( %d, %d ) encountered when analyzing DX9 effect", state, value );
		break;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetTexture( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture )
{
	SamplerSetupFullDx9& sampler = m_samplers[Stage];

	std::string name = m_textureToName[pTexture];
	sampler.m_textureName = name;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetTextureStageState( %d, %d, %d ) encountered when analyzing DX9 effect", Stage, Type, Value );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value )
{
	SamplerSetupFullDx9& sampler = m_samplers[Sampler];
	sampler.SetState( Type, Value );

	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetNPatchMode( FLOAT NumSegments )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetNPatchMode( %d ) encountered when analyzing DX9 effect", NumSegments );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetFVF( DWORD FVF )
{
	g_messages.AddMessage( "\\memory(0): warning X0000: SetFVF( %d ) encountered when analyzing DX9 effect", FVF );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader )
{
	m_vertexShader = pShader;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetVertexShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount )
{
	size_t size = RegisterCount * sizeof( Vector4 );
	if( RegisterIndex + RegisterCount > CONSTANTS_MAX )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetVertexShaderConstantF: Too many constants being set (%d, %d) when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	else
	{
		memcpy( m_vertexConstantValues + RegisterIndex, pConstantData, size );
		m_vertexConstantValueCount = RegisterIndex + RegisterCount;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetVertexShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount )
{
	if( RegisterCount > 0 )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetVertexShaderConstantI( %d, <data>, %d ) encountered when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetVertexShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount )
{
	if( RegisterCount > 0 )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetVertexShaderConstantB( %d, <data>, %d ) encountered when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader )
{
	m_pixelShader = pShader;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetPixelShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount )
{
	size_t size = RegisterCount*sizeof( Vector4 );
	if( RegisterIndex + RegisterCount > CONSTANTS_MAX )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetPixelShaderConstantF: Too many constants being set (%d, %d) encountered when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	else
	{
		memcpy( m_pixelConstantValues + RegisterIndex, pConstantData, size );
		m_pixelConstantValueCount = RegisterIndex + RegisterCount;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetPixelShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount )
{
	// We don't care about the default values for integer constants.
	// As far as I know we're only using an int constant in one case, for number
	// of pointlights and we rely on the engine properly setting it.
	if( false && RegisterCount > 0 )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetPixelShaderConstantI( %d, <data>, %d ) encountered when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE EffectAnalyzerDx9::SetPixelShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount )
{
	if( RegisterCount > 0 )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: SetPixelShaderConstantB( %d, <data>, %d ) encountered when analyzing DX9 effect", RegisterIndex, RegisterCount );
	}
	return S_OK;
}

void EffectAnalyzerDx9::Reset()
{
	m_renderStates.clear();
	m_samplers.clear();
	m_pixelConstantValueCount = 0;
	m_vertexConstantValueCount = 0;
}

static void ExtractShaderInputs( DWORD* shaderCode, StageInput& stage )
{
	D3DXSEMANTIC inputs[256];
	UINT count = 0;
	if( FAILED( D3DXGetShaderInputSemantics( (DWORD*)shaderCode, inputs, &count ) ) )
	{
		g_messages.AddMessage( "\\memory(0): warning X0000: Failed to get shader input semantics" );
	}
	for( unsigned k = 0; k < count; ++k )
	{
		InputDescription input;
		switch( inputs[k].Usage )
		{
		case D3DDECLUSAGE_POSITION:
			input.name = UC_POSITION;
			break;
		case D3DDECLUSAGE_BLENDWEIGHT:
			input.name = UC_BLENDWEIGHTS;
			break;
		case D3DDECLUSAGE_BLENDINDICES:
			input.name = UC_BLENDINDICES;
			break;
		case D3DDECLUSAGE_NORMAL:
			input.name = UC_NORMAL;
			break;
		case D3DDECLUSAGE_TEXCOORD:
			input.name = UC_TEXCOORD;
			break;
		case D3DDECLUSAGE_TANGENT:
			input.name = UC_TANGENT;
			break;
		case D3DDECLUSAGE_BINORMAL:
			input.name = UC_BITANGENT;
			break;
		case D3DDECLUSAGE_COLOR:
			input.name = UC_COLOR;
			break;
		default:
			g_messages.AddMessage( "\\memory(0): warning X0000: Vertex shader uses unsupported input semantics #%i", inputs[k].Usage );
			continue;
		}
		input.index = inputs[k].UsageIndex;
		input.usedMask = 0xff;
		input.registerIndex = 0;
		stage.inputs.push_back( input );
	}
}

static void StripComments( void* code, unsigned& codeSize, void*& strippedCode )
{
    // Calculates the new size (without comments)
    int* codeData = static_cast<int*>( code );
    unsigned int sizeInWords = codeSize / 4;
    unsigned int strippedSizeInWords = sizeInWords;
 
    for( unsigned int i = 0; i < sizeInWords; i++ )
    {
        if( ( codeData[i] & 0xffff ) == D3DSIO_COMMENT )
        {
            int commentSize = codeData[i] >> 16;
            strippedSizeInWords -= 1 + commentSize;
            i += commentSize;
        }
		else if( ( codeData[i] & 0xffff ) == D3DSIO_DEF )
        {
            i += 5;
        }
    }
 
    // Creates a new buffer with the original code but omitting the comments
	strippedCode = new char[strippedSizeInWords * 4];
 
    int* strippedCodeData = static_cast<int*>( strippedCode );
    size_t offset = 0;
 
    for( unsigned int i = 0; i < sizeInWords; i++ )
    {
        if( ( codeData[i] & 0xffff ) == D3DSIO_COMMENT )
        {
            int commentSize = codeData[i] >> 16;
            i += commentSize;
        }
		else if( ( codeData[i] & 0xffff ) == D3DSIO_DEF )
        {
            strippedCodeData[offset++] = codeData[i++];
            strippedCodeData[offset++] = codeData[i++];
            strippedCodeData[offset++] = codeData[i++];
            strippedCodeData[offset++] = codeData[i++];
            strippedCodeData[offset++] = codeData[i++];
            strippedCodeData[offset++] = codeData[i];
        }
        else
        {
            strippedCodeData[offset++] = codeData[i];
        }
    }
 
    codeSize = strippedSizeInWords * 4;
}

static unsigned GetInstructionCount( const void* shaderCode )
{
	unsigned instructionCount = -1;
	CComPtr<ID3DXBuffer> disassembly;
	D3DXDisassembleShader( (const DWORD*)shaderCode, FALSE, NULL, &disassembly);
	char* asmCode = (char*)disassembly->GetBufferPointer();
	const char* approximately = "approximately ";
	char* found = strstr( asmCode, approximately );
	if( found )
	{
		sscanf_s( found + strlen( approximately ), "%u", &instructionCount );
	}
	return instructionCount;
}

bool EffectAnalyzerDx9::AnalyzeEffect( EffectData& effectData, ID3DXEffect* fx, YamlOutput& listing, ID3DXBuffer* shaderText )
{
	// Need to set the FFP to default values in texture stage mapping to
	// coordindices.  This is some peculiar Direct3D interference with the
	// new Programmable Pipeline.  Documented in some old info doc from Microsoft. 
	// Only causes problems in some version of D3D/device drivers/cards. 
	// We need to do this hear because callbacks triggered by this update can
	// invoke calls back to the D3D effect system which needs to be in a good state.
	// <halldor 2009-11-18>
	if( g_device9 )
	{
		for( int i = 0; i < 16; ++i )
		{
			g_device9->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
		}
	}


	D3DXEFFECT_DESC effectDesc;
	fx->GetDesc( &effectDesc );
	int n = effectDesc.Parameters;

	D3DXEFFECT_DESC	effectDescription;
	D3DXPARAMETER_DESC parameterDescription;
	D3DXPARAMETER_DESC annotationDescription;

	if( FAILED( fx->GetDesc( &effectDescription ) ) )
	{
		g_messages.AddMessage( "\\memory(0): error X0000: Failed to get effect description when analyzing DX9 effect - no annotations will be saved" );
		return true;
	}

	for (unsigned x = 0; x < effectDescription.Parameters; x++)
	{
		D3DXHANDLE effectParam = fx->GetParameter( NULL, x );

		bool isUsed = false;
		for( UINT t = 0; t < effectDesc.Techniques; ++t )
		{
			if( fx->IsParameterUsed( effectParam, fx->GetTechnique( t ) ) )
			{
				isUsed = true;
				break;
			}
		}
		if( !isUsed )
		{
			continue;
		}
		if( FAILED( fx->GetParameterDesc( effectParam, &parameterDescription ) ) )
		{
			g_messages.AddMessage( "\\memory(0): warning X0000: Getting parameter description failed when analyzing DX9 effect" );
			continue;
		}

		StringReference parameterName;
		if( parameterDescription.Annotations > 0 )
		{
			parameterName = g_stringTable.AddString( parameterDescription.Name );
		}

		for( unsigned a = 0; a < parameterDescription.Annotations; ++a )
		{
			D3DXHANDLE annotation = fx->GetAnnotation( effectParam, a );

			if( FAILED( fx->GetParameterDesc( annotation, &annotationDescription ) ) )
			{
				g_messages.AddMessage( "\\memory(0): warning X0000: Getting parameter description failed when analyzing DX9 effect" );
				continue;
			}

			if( annotationDescription.Type == D3DXPT_BOOL )
			{
				BOOL boolValue;
				if( SUCCEEDED( fx->GetBool( annotation, &boolValue ) ) )
				{
					Annotation value;
					value.type = ANNOTATION_TYPE_BOOL;
					value.intValue = boolValue ? 1 : 0;
					effectData.annotations[parameterName].annotations[g_stringTable.AddString( annotationDescription.Name )] = value;
				}
			}
			else if( annotationDescription.Type == D3DXPT_INT )
			{
				int intValue;
				if( SUCCEEDED( fx->GetInt( annotation, &intValue ) ) )
				{
					Annotation value;
					value.type = ANNOTATION_TYPE_INT;
					value.intValue = intValue;
					effectData.annotations[parameterName].annotations[g_stringTable.AddString( annotationDescription.Name )] = value;
				}
			}
			else if( annotationDescription.Type == D3DXPT_FLOAT )
			{
				float floatValue;
				if( SUCCEEDED( fx->GetFloat( annotation, &floatValue ) ) )
				{
					Annotation value;
					value.type = ANNOTATION_TYPE_FLOAT;
					value.floatValue = floatValue;
					effectData.annotations[parameterName].annotations[g_stringTable.AddString( annotationDescription.Name )] = value;
				}
			}
			else if( annotationDescription.Type == D3DXPT_STRING )
			{
				LPCSTR stringValue;
				if( SUCCEEDED( fx->GetString( annotation, &stringValue ) ) )
				{
					Annotation value;
					value.type = ANNOTATION_TYPE_STRING;
					value.stringValue = g_stringTable.AddString( stringValue );
					effectData.annotations[parameterName].annotations[g_stringTable.AddString( annotationDescription.Name )] = value;
				}
			} 
			else
			{
				g_messages.AddMessage( "\\memory(0): warning X0000: Annotation type %d was unknown when analyzing DX9 effect", annotationDescription.Type );
			}
		}
	}

	std::vector<D3DXHANDLE> texturesSet;

	for( int paramIx = 0; paramIx < n; ++paramIx )
	{
		D3DXHANDLE paramHandle = fx->GetParameter( 0, paramIx );
		D3DXPARAMETER_DESC desc;
		fx->GetParameterDesc( paramHandle, &desc );
		if( desc.Class == D3DXPC_OBJECT )
		{
			switch( desc.Type )
			{
			case D3DXPT_TEXTURE:
			case D3DXPT_TEXTURE1D:
			case D3DXPT_TEXTURE2D:
				{
					CComPtr<IDirect3DTexture9> tex;
					g_device9->CreateTexture( 32, 32, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL );
					CComPtr<IDirect3DBaseTexture9> baseTex( tex );
					fx->SetTexture( paramHandle, baseTex );
					texturesSet.push_back( paramHandle );
					m_textureToName[baseTex] = desc.Name;
					m_textures.push_back( baseTex );
				}
				break;

			case D3DXPT_TEXTURE3D:
				{
					CComPtr<IDirect3DVolumeTexture9> tex;
					g_device9->CreateVolumeTexture( 32, 32, 2, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT , &tex, NULL );
					CComPtr<IDirect3DBaseTexture9> baseTex( tex );
					fx->SetTexture( paramHandle, baseTex );
					texturesSet.push_back( paramHandle );
					m_textureToName[baseTex] = desc.Name;
					m_textures.push_back( baseTex );
				}
				break;

			case D3DXPT_TEXTURECUBE:
				{
					CComPtr<IDirect3DCubeTexture9> tex;
					g_device9->CreateCubeTexture( 32, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT , &tex, NULL );
					CComPtr<IDirect3DBaseTexture9> baseTex( tex );
					fx->SetTexture( paramHandle, baseTex );
					texturesSet.push_back( paramHandle );
					m_textureToName[baseTex] = desc.Name;
					m_textures.push_back( baseTex );
				}
				break;
			}
		}
	}

	fx->SetStateManager( this );

	effectData.techniques.clear();
	effectData.techniques.resize( effectDesc.Techniques );

	for( UINT t = 0; t < effectDesc.Techniques; ++t )
	{
		fx->SetTechnique( fx->GetTechnique( t ) );

		Technique& technique = effectData.techniques[t];
		D3DXTECHNIQUE_DESC techniqueDesc;
		fx->GetTechniqueDesc( fx->GetTechnique( t ), &techniqueDesc );
		technique.name = g_stringTable.AddString( techniqueDesc.Name );

		listing.dict()
			.literal( "name" ).literal( techniqueDesc.Name )
			.literal( "passes" ).list();

		UINT passCount;
		if( SUCCEEDED( fx->Begin( &passCount, 0 ) ) )
		{
			technique.passes.resize( passCount );

			for( unsigned int i = 0; i < passCount; ++i )
			{
				if( FAILED( fx->BeginPass( i ) ) )
				{
					g_messages.AddMessage( "\\memory(0): error X0000: Failed to apply effect" );
					return false;
				}

				fx->EndPass();

				Pass& pass = technique.passes[i];

				pass.states = m_renderStates;
				pass.stages.resize( 2 );
				pass.stages[0].type = VERTEX_STAGE;
				pass.stages[1].type = PIXEL_STAGE;

				// This is ONLY to ensure that SM2 hardware continues to work before releasing CQ.
				// Some Incarna shaders get loaded by the login screen, and the default shader model has increased.
				// Rather than fail loudly as we will after dropping SM2 support, simply log a warning and continue.

				pass.stages[0].shadowShaderSize = 0;
				pass.stages[0].shadowShaderData = nullptr;
				pass.stages[0].shadowShaderDataStr = -1;
				pass.stages[1].shadowShaderSize = 0;
				pass.stages[1].shadowShaderData = nullptr;
				pass.stages[1].shadowShaderDataStr = -1;
				pass.stages[0].threadGroupSize[0] = 0;
				pass.stages[0].threadGroupSize[1] = 0;
				pass.stages[0].threadGroupSize[2] = 0;
				pass.stages[1].threadGroupSize[0] = 0;
				pass.stages[1].threadGroupSize[1] = 0;
				pass.stages[1].threadGroupSize[2] = 0;

				if( m_vertexShader == NULL )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Vertex shader NULL encountered when analyzing DX9 effect" );
					pass.stages[0].shaderSize = 0;
					pass.stages[0].shaderData = nullptr;
					pass.stages[0].shaderDataStr = -1;
				}
				else
				{

					if( FAILED( m_vertexShader->GetFunction( nullptr, &pass.stages[0].shaderSize ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Getting vertex shader data when analyzing DX9 effect" );
						return false;
					}
					char* buffer = new char[pass.stages[0].shaderSize];
					if( FAILED( m_vertexShader->GetFunction( buffer, &pass.stages[0].shaderSize ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Getting vertex shader data when analyzing DX9 effect" );
						return false;
					}
					StripComments( buffer, pass.stages[0].shaderSize, pass.stages[0].shaderData );
					pass.stages[0].shaderDataStr = g_stringTable.AddString( pass.stages[0].shaderData, pass.stages[0].shaderSize );
					delete[] buffer;
				}

				if( m_pixelShader == NULL )
				{
					g_messages.AddMessage( "\\memory(0): warning X0000: Pixel shader NULL encountered when analyzing DX9 effect" );
					pass.stages[1].shaderSize = 0;
					pass.stages[1].shaderData = nullptr;
					pass.stages[1].shaderDataStr = -1;
				}
				else
				{
					if( FAILED( m_pixelShader->GetFunction( nullptr, &pass.stages[1].shaderSize ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Getting pixel shader data when analyzing DX9 effect" );
						return false;
					}
					char* buffer = new char[pass.stages[1].shaderSize];
					if( FAILED( m_pixelShader->GetFunction( buffer, &pass.stages[1].shaderSize ) ) )
					{
						g_messages.AddMessage( "\\memory(0): error X0000: Getting pixel shader data when analyzing DX9 effect" );
						return false;
					}
					StripComments( buffer, pass.stages[1].shaderSize, pass.stages[1].shaderData );
					delete[] buffer;
					pass.stages[1].shaderDataStr = g_stringTable.AddString( pass.stages[1].shaderData, pass.stages[1].shaderSize );
				}

				static char buffer[64 * 1024];
				UINT bufferSize = sizeof( buffer );

				if( m_pixelShader )
				{
					m_pixelShader->GetFunction( buffer, &bufferSize );
					ExtractConstantTable( buffer, pass.stages[1].constants, fx, 0 );
					ExtractShaderInputs( (DWORD*)buffer, pass.stages[1] );
				}
				if( m_vertexShader )
				{
					bufferSize = sizeof( buffer );
					m_vertexShader->GetFunction( buffer, &bufferSize );
					ExtractConstantTable( buffer, pass.stages[0].constants, fx, D3DVERTEXTEXTURESAMPLER0 );
					ExtractShaderInputs( (DWORD*)buffer, pass.stages[0] );
				}

				for( auto it = m_samplers.begin(); it != m_samplers.end(); ++it )
				{
					Sampler ss;
					ss.name = g_stringTable.AddString( it->second.m_name.c_str() );
					ss.comparison = 0;
					ss.minLOD = FLT_MAX;
					ss.maxLOD = float( it->second.GetState( D3DSAMP_MAXMIPLEVEL ) );

					ss.minFilter = BYTE( it->second.GetState( D3DSAMP_MINFILTER ) );
					ss.magFilter = BYTE( it->second.GetState( D3DSAMP_MAGFILTER ) );
					ss.mipFilter = BYTE( it->second.GetState( D3DSAMP_MIPFILTER ) );
					ss.addressU = BYTE( it->second.GetState( D3DSAMP_ADDRESSU ) );
					ss.addressV = BYTE( it->second.GetState( D3DSAMP_ADDRESSV ) );
					ss.addressW = BYTE( it->second.GetState( D3DSAMP_ADDRESSW ) );
					DWORD lodBias = it->second.GetState( D3DSAMP_MIPMAPLODBIAS );
					ss.mipLODBias = *(float*)&lodBias;
					ss.maxAnisotropy = BYTE( it->second.GetState( D3DSAMP_MAXANISOTROPY ) );
					ss.srgbTexture = it->second.GetState( D3DSAMP_SRGBTEXTURE ) ? 1 : 0;
					ss.comparisonFunc = D3D11_COMPARISON_NEVER;
					D3DXCOLOR borderColor = it->second.GetState( D3DSAMP_BORDERCOLOR );
					ss.borderColor.x = borderColor.r;
					ss.borderColor.y = borderColor.g;
					ss.borderColor.z = borderColor.b;
					ss.borderColor.w = borderColor.a;

					Texture tex;
					tex.name = g_stringTable.AddString( it->second.m_textureName.c_str() );
					tex.type = TEX_TYPE_TYPELESS;
					tex.isSRGB = it->second.GetState( D3DSAMP_SRGBTEXTURE ) != 0;
					tex.isAutoregister = false;

					D3DXHANDLE effectParam = fx->GetParameterByName( NULL, it->second.m_textureName.c_str() );
					if( effectParam )
					{
						D3DXPARAMETER_DESC desc;
						if( SUCCEEDED( fx->GetParameterDesc( effectParam, &desc ) ) )
						{
							switch( desc.Type )
							{
							case D3DXPT_TEXTURE1D:
								tex.type = TEX_TYPE_1D;
								break;
							case D3DXPT_TEXTURE2D:
								tex.type = TEX_TYPE_2D;
								break;
							case D3DXPT_TEXTURE3D:
								tex.type = TEX_TYPE_3D;
								break;
							case D3DXPT_TEXTURECUBE:
								tex.type = TEX_TYPE_CUBE;
								break;
							}
						}
					}

					D3DXPARAMETER_DESC paramDesc;
					D3DXHANDLE paramHandle = fx->GetParameterByName( nullptr, it->second.m_textureName.c_str() );
					if( paramHandle )
					{
						if( SUCCEEDED( fx->GetParameterDesc( paramHandle, &paramDesc ) ) )
						{
							for( unsigned a = 0; a < paramDesc.Annotations; ++a )
							{
								D3DXHANDLE annotation = fx->GetAnnotation( paramHandle, a );
								D3DXPARAMETER_DESC annotationDescription;
								if( annotation && SUCCEEDED( fx->GetParameterDesc( annotation, &annotationDescription ) ) )
								{
									if( annotationDescription.Type == D3DXPT_BOOL )
									{
										BOOL boolValue;
										if( SUCCEEDED( fx->GetBool( annotation, &boolValue ) ) )
										{
											if( strcmp( annotationDescription.Name, "Tr2sRGB" ) == 0 )
											{
												tex.isSRGB = boolValue != 0;
											}
											else if( strcmp( annotationDescription.Name, "AutoRegister" ) == 0 )
											{
												tex.isAutoregister = boolValue != 0;
											}
										}
									}
								}
							}
						}
					}
					if( it->first >= D3DVERTEXTEXTURESAMPLER0 )
					{
						pass.stages[0].samplers[it->first - D3DVERTEXTEXTURESAMPLER0] = ss;
						pass.stages[0].textures[it->first - D3DVERTEXTEXTURESAMPLER0] = tex;
					}
					else
					{
						pass.stages[1].samplers[it->first] = ss;
						pass.stages[1].textures[it->first] = tex;
					}
				}

				if( listing.enabled() )
				{
					listing.list();

					if( m_vertexShader )
					{
						m_vertexShader->GetFunction( buffer, &bufferSize );
						unsigned version = D3DXGetShaderVersion( (const DWORD*)buffer );

						char profileStr[64];
						sprintf_s( profileStr, "vs_%i_%i", ( version >> 8 ) & 0xff, version & 0xff );

						listing.dict()
							.literal( "profile" ).literal( profileStr )
							.literal( "original" ).dict();

						if( shaderText )
						{
							listing.literal( "source" ).literal( shaderText );
						}

						CComPtr<ID3DXBuffer> disassembly;
						if( SUCCEEDED( D3DXDisassembleShader( (const DWORD*)buffer, FALSE, NULL, &disassembly ) ) )
						{
							char* asmCode = (char*)disassembly->GetBufferPointer();
							listing.literal( "asm" ).literal( asmCode );
							const char* approximately = "approximately ";
							char* found = strstr( asmCode, approximately );
							if( found )
							{
								unsigned instructionCount = -1;
								sscanf_s( found + strlen( approximately ), "%u", &instructionCount );
								listing.literal( "stats" ).dict().literal( "instructionCount" ).literal( instructionCount ).end();
							}
						}

						listing.end();
						PrintStageInfo( listing, pass.stages[0], effectData );
						listing.end();
					}
					if( m_pixelShader )
					{
						m_pixelShader->GetFunction( buffer, &bufferSize );
						unsigned version = D3DXGetShaderVersion( (const DWORD*)buffer );

						char profileStr[64];
						sprintf_s( profileStr, "ps_%i_%i", ( version >> 8 ) & 0xff, version & 0xff );

						listing.dict()
							.literal( "profile" ).literal( profileStr )
							.literal( "original" ).dict();
						CComPtr<ID3DXBuffer> disassembly;

						if( shaderText )
						{
							listing.literal( "source" ).literal( shaderText );
						}

						if( SUCCEEDED( D3DXDisassembleShader( (const DWORD*)buffer, FALSE, NULL, &disassembly ) ) )
						{
							char* asmCode = (char*)disassembly->GetBufferPointer();
							listing.literal( "asm" ).literal( asmCode );
							const char* approximately = "approximately ";
							char* found = strstr( asmCode, approximately );
							if( found )
							{
								unsigned instructionCount = -1;
								sscanf_s( found + strlen( approximately ), "%u", &instructionCount );
								listing.literal( "stats" ).dict().literal( "instructionCount" ).literal( instructionCount ).end();
							}
						}
						listing.end();
						PrintStageInfo( listing, pass.stages[1], effectData );
						listing.end();
					}
					listing.end();
				}

				if( m_pixelConstantValueCount )
				{
					pass.stages[1].defaultValues.resize( m_pixelConstantValueCount * sizeof( Vector4 ) );
					memcpy( &pass.stages[1].defaultValues[0], m_pixelConstantValues, m_pixelConstantValueCount * sizeof( Vector4 ) );
					pass.stages[1].defaultValuesStr = g_stringTable.AddString( &pass.stages[1].defaultValues[0], pass.stages[1].defaultValues.size() );
				}
				else
				{
					pass.stages[1].defaultValuesStr = -1;
				}
				if( m_vertexConstantValueCount )
				{
					pass.stages[0].defaultValues.resize( m_vertexConstantValueCount * sizeof( Vector4 ) );
					memcpy( &pass.stages[0].defaultValues[0], m_vertexConstantValues, m_vertexConstantValueCount * sizeof( Vector4 ) );
					pass.stages[0].defaultValuesStr = g_stringTable.AddString( &pass.stages[0].defaultValues[0], pass.stages[0].defaultValues.size() );
				}
				else
				{
					pass.stages[0].defaultValuesStr = -1;
				}

				Reset();


			}

			fx->End();
		}

		listing.end();
		listing.end();
	}

	fx->SetStateManager( NULL );

	for( std::vector<D3DXHANDLE>::const_iterator it = texturesSet.begin(); it != texturesSet.end(); ++it )
	{
		fx->SetTexture( *it, NULL );
	}

	return true;
}

void EffectAnalyzerDx9::ExtractConstantTable( void* func, std::vector<Constant>& constants, ID3DXEffect* fx, int samplerOffset )
{
	static const char* registerSet[] = { "B", "I", "c", "s" };
	ID3DXConstantTable* dxConstants;
	D3DXGetShaderConstantTable( (DWORD*)func, &dxConstants );

	constants.clear();

	if( dxConstants )
	{
		bool trimValues = false;
		unsigned defaultValueCount = 0;

		D3DXCONSTANTTABLE_DESC tbDesc;
		dxConstants->GetDesc( &tbDesc );
		for( UINT i = 0; i < tbDesc.Constants; ++i )
		{
			D3DXHANDLE ch = dxConstants->GetConstant( NULL, i );
			D3DXCONSTANT_DESC desc;
			UINT count = 0;
			if( FAILED( dxConstants->GetConstantDesc( ch, &desc, &count ) ) )
			{
				continue;
			}

			if( desc.RegisterSet == D3DXRS_SAMPLER )
			{
				m_samplers[desc.RegisterIndex + samplerOffset].m_name = desc.Name;
			}
			else
			{
				if( samplerOffset && desc.RegisterIndex + desc.RegisterCount == m_vertexConstantValueCount && strcmp( desc.Name, "g_uiTransforms" ) == 0 )
				{
					m_vertexConstantValueCount = desc.RegisterIndex;
					trimValues = true;
				}
				else
				{
					defaultValueCount = max( defaultValueCount, desc.RegisterIndex + desc.RegisterCount );
				}

				Constant constant;
				constant.name = g_stringTable.AddString( desc.Name );
				constant.offset = desc.RegisterIndex * sizeof( Vector4 );
				constant.size = desc.RegisterCount * sizeof( Vector4 );
				//constant.cbIndex = 0;
				switch( desc.Type )
				{
				case D3DXPT_FLOAT:
					constant.type = CONSTANT_TYPE_FLOAT;
					break;
				case D3DXPT_INT:
					constant.type = CONSTANT_TYPE_INT;
					break;
				case D3DXPT_BOOL:
					constant.type = CONSTANT_TYPE_BOOL;
					break;
				default:
					constant.type = CONSTANT_TYPE_OTHER;
				}
				switch( desc.Class )
				{
				case D3DXPC_SCALAR:
					constant.dimension = 1;
					break;
				case D3DXPC_VECTOR:
					constant.dimension = 4;
					break;
				case D3DXPC_MATRIX_COLUMNS:
				case D3DXPC_MATRIX_ROWS:
					constant.dimension = 16;
					break;
				default:
					constant.dimension = 1;
				}
				constant.elements = desc.Elements;
				constant.isSRGB = false;
				constant.isAutoregister = false;

				D3DXPARAMETER_DESC paramDesc;
				D3DXHANDLE paramHandle = fx->GetParameterByName( nullptr, desc.Name );
				if( paramHandle )
				{
					if( SUCCEEDED( fx->GetParameterDesc( paramHandle, &paramDesc ) ) )
					{
						for( unsigned a = 0; a < paramDesc.Annotations; ++a )
						{
							D3DXHANDLE annotation = fx->GetAnnotation( paramHandle, a );
							D3DXPARAMETER_DESC annotationDescription;
							if( annotation && SUCCEEDED( fx->GetParameterDesc( annotation, &annotationDescription ) ) )
							{
								if( annotationDescription.Type == D3DXPT_BOOL )
								{
									BOOL boolValue;
									if( SUCCEEDED( fx->GetBool( annotation, &boolValue ) ) )
									{
										if( strcmp( annotationDescription.Name, "Tr2sRGB" ) == 0 )
										{
											constant.isSRGB = boolValue != 0; 
										}
										else if( strcmp( annotationDescription.Name, "AutoRegister" ) == 0 )
										{
											constant.isAutoregister = boolValue != 0;
										}
									}
								}
							}
						}
					}
				}
				constants.push_back( constant );
			}
		}
		if( trimValues )
		{
			m_vertexConstantValueCount = defaultValueCount;
		}
	}
}






bool EffectCompilerDX9::Create()
{
	CComPtr<IDirect3D9Ex> direct3D9;
	HRESULT hr;
	if( FAILED( hr = Direct3DCreate9Ex( D3D_SDK_VERSION, &direct3D9 ) ) )
	{
		printf_s( "ShaderCompiler: error X0000: Could not create DX9 object (code 0x%x)\n", hr );
		return false;
	}

	WNDCLASS wc; 
    wc.style = CS_OWNDC; 
    wc.lpfnWndProc = (WNDPROC)&DefWindowProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
	wc.hInstance = GetModuleHandle( nullptr ); 
    wc.hIcon = nullptr; 
    wc.hCursor = nullptr; 
    wc.hbrBackground = nullptr; 
    wc.lpszMenuName =  nullptr; 
    wc.lpszClassName = "dx9"; 
    if (!RegisterClass(&wc)) 
	{
		DWORD error = GetLastError();
		char* lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		if( lpMsgBuf )
		{
			printf( "Error initializing DirectX9: RegisterClass returned FALSE: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);
		}
		else
		{
			printf( "Error initializing DirectX9: RegisterClass returned FALSE\n" );
		}
		return false; 
	}
	HWND dx9Wnd = CreateWindow( "dx9", "dx9", WS_OVERLAPPED, 0, 0, 16, 16, nullptr, nullptr, GetModuleHandle( nullptr ), 0 );
	if( dx9Wnd == nullptr )
	{
		DWORD error = GetLastError();
		char* lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		if( lpMsgBuf )
		{
			printf( "Error initializing DirectX9: CreateWindow returned NULL: %s\n", lpMsgBuf );
			LocalFree(lpMsgBuf);
		}
		else
		{
			printf( "Error initializing DirectX9: CreateWindow returned NULL\n" );
		}
		return false; 
	}


	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed   = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	if( FAILED( hr = direct3D9->CreateDeviceEx( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dx9Wnd,
										D3DCREATE_SOFTWARE_VERTEXPROCESSING,
										&d3dpp, nullptr, &g_device9 ) ) )
	{
		printf_s( "ShaderCompiler: error X0000: Could not create DX9 device (code 0x%x)\n", hr );
		return false;
	}
	InitializeCriticalSection( &g_analyzeEffectDx9CS );
	return true;
}

namespace
{

enum CompileStatus
{
	COMPILE_STATUS_SUCCESS,
	COMPILE_STATUS_ERROR,
	COMPILE_STATUS_CRASH,
};

CompileStatus SafeCompileEffect( ID3DXEffectCompiler* effectCompiler, DWORD flags, LPD3DXBUFFER* effect, LPD3DXBUFFER* errorMsgs )
{
	__try
	{
		return SUCCEEDED( effectCompiler->CompileEffect( flags, effect, errorMsgs ) ) ? COMPILE_STATUS_SUCCESS : COMPILE_STATUS_ERROR;
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{ 
		return COMPILE_STATUS_CRASH;
	}
}

const std::regex s_premutationPragma( "^[[:space:]]*#[[:space:]]*pragma[[:space:]]*permutation.*" );
const std::regex s_blendOpAlpha( "[^[:alnum:]]BlendOpAlpha[[:space:]]*=[[:space:]]*([[:alnum:]]+)[[:space:]]*;" );

}

bool EffectCompilerDX9::CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result, bool disableListing )
{
	auto src = std::regex_replace( std::string( source, sourceLength ), s_premutationPragma, std::string( "" ) );

	// We need to replace BlendOpAlpha render state assignment because FXC bug will
	// replace BlendOpAlpha with BendOp. We highjack VertexBlend state for that.
	src = regex_replacecb( src, s_blendOpAlpha, []( const std::smatch& m ) -> std::string {
		auto value = m.str( 1 );
		if( _stricmp( value.c_str(), "add" ) == 0 )
		{
			return " VERTEXBLEND=1;";
		}
		else if( _stricmp( value.c_str(), "subtract" ) == 0 )
		{
			return " VERTEXBLEND=2;";
		}
		else if( _stricmp( value.c_str(), "revsubtract" ) == 0 )
		{
			return " VERTEXBLEND=3;";
		}
		else if( _stricmp( value.c_str(), "min" ) == 0 )
		{
			return " VERTEXBLEND=4;";
		}
		else if( _stricmp( value.c_str(), "max" ) == 0 )
		{
			return " VERTEXBLEND=5;";
		}
		else
		{
			return m.str( 0 ).c_str();
		}
	} );

	source = src.c_str();
	sourceLength = src.length();


	CComPtr<ID3DXEffectCompiler> effectCompiler;
	CComPtr<ID3DXBuffer> errors;
	CComPtr<ID3DXBuffer> effectData;

	DWORD optimizationLevel;
	switch( g_optimizationLevel )
	{
	case 0:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL0;
		break;
	case 1:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL1;
		break;
	case 2:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL2;
		break;
	default:
		optimizationLevel = D3DXSHADER_OPTIMIZATION_LEVEL3;
	}

	if( FAILED( D3DXCreateEffectCompiler( 
		source, 
		sourceLength, 
		defines, 
		include, 
		D3DXSHADER_PACKMATRIX_COLUMNMAJOR | D3DXSHADER_NO_PRESHADER | optimizationLevel | ( g_avoidFlowControl ? D3DXSHADER_AVOID_FLOW_CONTROL : 0 ), 
		&effectCompiler, 
		&errors ) ) )
	{
		if( errors )
		{
			g_messages.AddMessages( errors );
		}
		return false;
	}
	if( g_printWarnings && errors )
	{
		g_messages.AddMessages( errors );
	}
	errors = nullptr;

	switch( SafeCompileEffect( effectCompiler,
		D3DXSHADER_PACKMATRIX_COLUMNMAJOR | D3DXSHADER_NO_PRESHADER | optimizationLevel | ( g_avoidFlowControl ? D3DXSHADER_AVOID_FLOW_CONTROL : 0 ), 
		&effectData, 
		&errors ) )
	{
	case COMPILE_STATUS_ERROR:
		if( errors )
		{
			g_messages.AddMessages( errors );
		}
		return false;
	case COMPILE_STATUS_CRASH:
		g_messages.AddMessage( "\\memory(0): error C0000: WTF? D3D compiler has crashed(" );
		return false;
	}
	if( g_printWarnings && errors )
	{
		g_messages.AddMessages( errors );
	}
	errors = nullptr;

	YamlListing listing( !disableListing );
	listing.dict()
		.literal( "permutation" ).dict();
	listing
		.literal( "platform" ).literal( "DX9" )
		.literal( "id" ).literal( "000" )
		.literal( "defines" ).dict();
	for( int i = 0; defines[i].Name; ++i )
	{
		listing.literal( defines[i].Name ).literal( defines[i].Definition );
	}
	listing.end();
	listing.literal( "techniques" ).list();


	CComPtr<ID3DXBuffer> shaderText;
	if( listing.enabled() )
	{
		D3DXPreprocessShader( source, sourceLength, defines, include, &shaderText, nullptr );
	}

	EnterCriticalSection( &g_analyzeEffectDx9CS );

	CComPtr<ID3DXEffect> effect;

	if( FAILED( D3DXCreateEffectEx(
		g_device9,
		effectData->GetBufferPointer(),
		effectData->GetBufferSize(),
		NULL,
		NULL,
		"PerFramePS;PerObjectPS;PerFrameVS;PerObjectVS;",
		0,
		NULL,
		&effect,
		&errors
		) ) )
	{
		if( errors )
		{
			g_messages.AddMessages( errors );
		}
		LeaveCriticalSection( &g_analyzeEffectDx9CS );
		return false;
	}
	bool success = false;
	{
		EffectAnalyzerDx9 analyzer;
		success = analyzer.AnalyzeEffect( result, effect, listing, shaderText );
	}
	listing.end();
	listing.end();
	listing.end();
	effect = nullptr;
	LeaveCriticalSection( &g_analyzeEffectDx9CS );
	return success;
}
