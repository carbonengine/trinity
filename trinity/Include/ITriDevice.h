/* 
	*************************************************************************

	ITriDevice.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/

#ifndef _ITRIDEVICE_H_
#define _ITRIDEVICE_H_

// forwards
BLUE_DECLARE_INTERFACE( ITriDevice );
struct Tr2PresentParametersAL;

// --------------------------------------------------------------------------------------
// Description:
//   Types of device callbacks.
// See also:
//   ITriDevice
// --------------------------------------------------------------------------------------
enum Tr2DeviceCallbackTime
{
	DEVICE_CALLBACK_FRAME_BEGIN,
	DEVICE_CALLBACK_FRAME_END,
};

// --------------------------------------------------------------------------------------
// Description:
//   Per-frame callback function. Called by device right before Present.
// See also:
//   ITriDevice
// --------------------------------------------------------------------------------------
typedef void (*Tr2DeviceCallback)( ITriDevice* device, void* userData );

BLUE_INTERFACE(ITriDevice) : public IRoot
{
	// !!! NOTE, all objects are returned
	// without any addrefing
	
	// D3D rendering objects
	
	virtual float AspectRatio() = 0;

	virtual void ScreenToProjection(
		int x,
		int y,
		float* fx,
		float* fy
		) = 0;

	virtual bool SetPresentation (
		int adapter,
		const Tr2PresentParametersAL* d3dpp
		) = 0;

	virtual bool ChangeDevice(
		uint32_t adapter, Tr2WindowHandle hWnd, const Tr2PresentParametersAL *pp) = 0;
};

#endif