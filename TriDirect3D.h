/* 
	*************************************************************************************

	TriDirect3D.h

	Author:    Hilmar Veigar Pétursson
	Created:   October 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIDIRECT3D_Description


	Dependencies:

		Blue, DirectX 9.0

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRIDIRECT3D_H_
#define _TRIDIRECT3D_H_

#if BLUE_WITH_PYTHON
//typedefs for passing data opaquely around.
typedef BluePySWrap<Structify<Tr2WindowHandle> > WrappedHWND;
#endif


//A class that takes care of wrapping presentation parameters back and forth from a dict.
class PresentationParameters : public Tr2PresentParametersAL
{
public:
	PresentationParameters();
	PresentationParameters( const Tr2PresentParametersAL& o ): Tr2PresentParametersAL( o ) {}
#if BLUE_WITH_PYTHON
	BluePyDict Get() const;
	bool Set(PyObject *dict);
#endif
};

#endif