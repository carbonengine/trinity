/* 
	*************************************************************************************

	ITriDeviceResource.h

	Created by:	Halldor Fannar
	Created on:	2006-10-25
	Project:	Trinity

	Note: This is a platform independent file and cannot expose any special code that is
	not shared between D3D9 and D3D10.  If any Python methods are to be exposed for
	this class this is the place to do it.

	(c) CCP 2006

	*************************************************************************************
*/

#pragma once
#ifndef _ITriDeviceResource_H_
#define _ITriDeviceResource_H_

typedef unsigned int TriStorage;

enum TriStorageFlags
{
	TRISTORAGE_VIDEOMEMORY = 1 << 0,  // release all resources created on video card memory (D3DPOOL_DEFAULT) for device->Reset
	TRISTORAGE_MANAGEDMEMORY = 1 << 1, // release all resources created in device memory (D3DPOOL_MANAGED)
	TRISTORAGE_ALL = (1 << 2) - 1,

	TRISTORAGE_FORCE_UINT = 0xFFFFFFFFu
};


// This class is for objects that manage device resources.  Note that multiple D3D resources can be
// encapsulated by one implementation of this interface.  For instance, TriDevice contains multiple D3D
// resources that need management.  The most elegant solution is to have TriDevice implement this
// interface and thus handle multiple D3D resources within each interface method.  The complications from
// this are minimal and are easily outweighed by the convenience.  
// When D3D resources need to be released, the TriDevice will use the interface below to properly handle it.
// Note that this interface is not a proper Blue interface - it does not inherit from IRoot. Do not put
// this as the first interface to inherit from in a class - Blue won't like it.
class Tr2DeviceResource
{
public:
	Tr2DeviceResource();
	virtual ~Tr2DeviceResource();

#if TRINITYDEV
	virtual void GetDescription( std::string& desc ) { desc = "<Unknown>"; }
#endif

	// This method will be called by the device when it needs to release resources.  
	// - TRISTORAGE_DEVICEMEMORY | TRISTORAGE_VIDEOMEMORY is used when the device itself is about to be 
	// released.  In this case all D3D resources should be released and their pointers set to NULL.
	// - TRISTORAGE_VIDEOMEMORY is used when the device is being reset and all its resources in video 
	// memory need to be evicted.  
	virtual void ReleaseResources( TriStorage s ) = 0;

	// This method is called to prepare all D3D resources for usage.  
	// - It should be used even the first time the Trinity class is created; to prepare the D3D resources.  
	// - If a Trinity class needs other 'child' objects to be prepared _before_ being able to _prepare_ 
	// itself then it should call PrepareResource on these child objects. This is always safe since 
	// preparation is only done for non-NULL D3D resources and repeatedly calling PrepareResource is 
	// thus non-lethal.
	bool PrepareResources(); 

private:
	// This function should prepare any resources that are in the D3DPOOL_DEFAULT (or would prevent a device reset)
	// It should NEVER be called directly, other than from PrepareResources
	// - When implementing this method only NULL valued D3D resources should be created and initialized.  
	// This is because a) a previous call to ReleaseResource might only have released resources in 
	// VIDEOMEMORY, leaving other resources intact (and thus non-NULL), b) multiple call sites in one
	// frame.
	virtual bool OnPrepareResources() = 0;
};


#endif
