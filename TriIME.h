/* 
	*************************************************************************************

	TriIME.h

	Created:   July 2005
	OS:        Win32
	Project:   Trinity

	Description:   

		Windows IME shim

	Dependencies:

		Windows

	(c) CCP 2005

	*************************************************************************************
*/


#ifndef _TRIIME_H_
#define _TRIIME_H_

#define TRIIME_Description \
"na"

#include "include/ITriIME.h"
#include "TriIMEWrapper.h"

#if BLUE_WITH_PYTHON
class TriIME :
	public ITriIME,
	public IPythonMethods
{
public:
	EXPOSE_TO_BLUE();

	TriIME(IRoot* lockobj = NULL);
	~TriIME(void);


	/////////////////////////////////////////////////////////////////////////////////////
	// ITriIME
	/////////////////////////////////////////////////////////////////////////////////////
	/*void SetPivots(
		float _11, float _12, float _13,
		float _21, float _22, float _23,
		float _31, float _32, float _33
		);	 
	
	const D3DXCONVOLUTIONMATRIX3* GetMatrix(
		) const;*/
	//void SetupImeApi();


	/////////////////////////////////////////////////////////////////////////////////////
	// IPythonMethods
	/////////////////////////////////////////////////////////////////////////////////////
	void Destroy(
		);

	PyObject* GetAttr( 
		const char* name, 
		bool* handled
		);

	bool SetAttr(
		const char* name,
		PyObject* v,
		bool* handled
		);

	PyObject* Repr(
		bool* handled
		);

public:
	PyObject* Py__init__( PyObject* args );
	PyObject* PySetHWND( PyObject* args );
	PyObject* PyAssociateContext( PyObject* args );
	PyObject* PyOnLanguageChanged( PyObject* args );
	PyObject* PyGetCompositionString( PyObject* args );
	PyObject* PyGetCandidateList( PyObject* args );
	PyObject* PyGetConversionStatus( PyObject* args );
	PyObject* PySetConversionStatus( PyObject* args );
	PyObject* PyGetConversionList( PyObject* args );
	PyObject* PyIsIME( PyObject* args );
	PyObject* PyGetImeId( PyObject* args );
	PyObject* PyGetReadingString( PyObject* args );
	PyObject* PyActivateKeyboardLayout( PyObject* args );
	PyObject* PyGetKeyboardLayout( PyObject* args );
	PyObject* PyGetKeyboardLayoutList( PyObject* args );
	PyObject* PyShowReadingWindow( PyObject* args );
	PyObject* PyNotifyIME( PyObject* args );
	PyObject* PyDisableTextFrameService( PyObject* args );
	PyObject* PySimulateHotKey( PyObject* args );
	PyObject* PyGetOpenStatus( PyObject* args );
	PyObject* PySetOpenStatus( PyObject* args );
	PyObject* PyGetGuideLine( PyObject* args );
	PyObject* PySetCompositionString( PyObject* args );
	PyObject* PyBackspace( PyObject* args );
	PyObject* PyGetIMEFileName( PyObject* args );

protected:
	HWND			m_hWnd;
	static HIMC		s_hImcDef;

	class CHIMC
	{
		TriIME* m_pParent;
		HIMC	m_hIMC;
	public:
		CHIMC(TriIME* pParent)
		{
			m_hIMC	  = NULL;
			m_pParent = pParent;
		}

		operator HIMC()
		{
			if (m_hIMC == NULL)
				m_hIMC = TriIMEWrapper::ImmGetContext(m_pParent->m_hWnd);
			return m_hIMC;
		}

		~CHIMC()
		{
			if (m_hIMC)
			{
				TriIMEWrapper::ImmReleaseContext(m_pParent->m_hWnd, m_hIMC);
				m_hIMC = NULL;
			}
		}

	};
};
TYPEDEF_BLUECLASS(TriIME);

#endif
#endif // _TRIMIME_H_
