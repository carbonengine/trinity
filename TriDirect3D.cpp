#include "StdAfx.h"
#include "TriDirect3D.h"

//////////////////////////////////////////////////////////////////
// PresentationParams class members.


PresentationParameters::PresentationParameters()
{
	memset(this, 0, sizeof(*this));
}

#if BLUE_WITH_PYTHON
bool PresentationParameters::Set(PyObject *_dict)
{
	BluePyDict dict(BluePy(_dict,true));
	if (!dict.Check()) {
		PyErr_SetString(PyExc_TypeError, "dict expected");
		return false;
	}

	// okay, fill it out
	memset(this, 0, sizeof(*this));
#define GETINT(_n) i = dict.Get(#_n); if (i && !i.Check()) return 0; if (i)
	BluePyInt i(0);
	GETINT(BackBufferWidth) mode.width = unsigned( i.Int() );
	GETINT(BackBufferHeight) mode.height = unsigned( i.Int() );
	GETINT(BackBufferFormat) mode.format = Tr2RenderContextEnum::PixelFormat( i.Int() );
	GETINT(BackBufferCount) backBufferCount = unsigned( i.Int() );
	GETINT(MultiSampleType) msaaType = unsigned( i.Int() );
	GETINT(MultiSampleQuality) msaaQuality = unsigned( i.Int() );
	GETINT(SwapEffect) swapEffect = Tr2RenderContextEnum::SwapEffect( i.Int() );

	outputWindow = NULL;
	BluePy py = dict.Get("hDeviceWindow");
	if (py && py != Py_None) {
		WrappedHWND *wrapped = WrappedHWND::Unwrap(py);
		if (!wrapped) {
			PyErr_SetString(PyExc_TypeError, "hDeviceWindow must be a handle, or None");
			return false;
		}
		outputWindow = *wrapped;
	}

	py = dict.Get("Windowed");
	if (py)
		windowed = py.True();
	py = dict.Get("EnableAutoDepthStencil");
	if (py && py.True())
	{
		GETINT(AutoDepthStencilFormat) depthStencilFormat = Tr2RenderContextEnum::DepthStencilFormat( i.Int() );
	}
	else
	{
		depthStencilFormat = Tr2RenderContextEnum::DSFMT_UNKNOWN;
	}
	GETINT(FullScreen_RefreshRateInHz) mode.refreshRateDenominator = unsigned( i.Int() );
	GETINT(PresentationInterval) presentInterval = Tr2RenderContextEnum::PresentInterval( i.Int() );

	GETINT(scaling) mode.scaling = Tr2RenderContextEnum::DisplayScaling( i.Int() );
	GETINT(scanlineOrdering) mode.scanlineOrdering = Tr2RenderContextEnum::ScanlineOrdering( i.Int() );
#undef GETINT
	return true;
}

BluePyDict PresentationParameters::Get() const
{
	BluePy pTrue(Py_True, true); //stock objects
	BluePy pFalse(Py_False, true);
	BluePyDict dict(1);

#define PUTINT(_n) dict.Set(#_n, BluePyInt(_n))
	dict.Set( "BackBufferWidth", BluePyInt( mode.width ) );
	dict.Set( "BackBufferHeight", BluePyInt( mode.height ) );
	dict.Set( "BackBufferFormat", BluePyInt( mode.format ) );
	dict.Set( "BackBufferCount", BluePyInt( backBufferCount ) );
	dict.Set( "MultiSampleType", BluePyInt( msaaType ) );
	dict.Set( "MultiSampleQuality", BluePyInt( msaaQuality ) );
	dict.Set( "SwapEffect", BluePyInt( swapEffect ) );
	PyObject *handle = WrappedHWND::Wrap( outputWindow );
	dict.Set( "hDeviceWindow", handle );
	Py_XDECREF( handle );
	dict.Set( "Windowed", windowed ? pTrue : pFalse );
	dict.Set( "EnableAutoDepthStencil", depthStencilFormat == Tr2RenderContextEnum::DSFMT_UNKNOWN ? pFalse : pTrue );

	dict.Set( "AutoDepthStencilFormat", BluePyInt( depthStencilFormat ) );
	dict.Set( "Flags", BluePyInt( 0 ) );
	dict.Set( "FullScreen_RefreshRateInHz", BluePyInt( mode.refreshRateDenominator ) );
	dict.Set( "PresentationInterval", BluePyInt( presentInterval ) );
#undef PUTINT
	return dict;
}

#endif
