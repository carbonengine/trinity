#pragma once
#ifndef Tr2UmbraScene_H
#define Tr2UmbraScene_H

#include "umbraTypes.h"

// Forward declarations
class TriFrustum;

// Blue forward declarations
BLUE_DECLARE( Tr2UmbraScene );

// TODO this could be renamed to Tr2UmbraCommander
class Tr2UmbraScene : 
	public Umbra::Commander
{
public:
	virtual ~Tr2UmbraScene() {}

	//////////////////////////////////////////////////////////////////////////////////////
	// Umbra::Commander
    virtual void command( Command c );

protected:
	// Handlers for specific Umbra commands
	// To be overridden by derived classes
	virtual void OnQueryBegin( void )					{ /* Do nothing */ }
	virtual void OnQueryEnd( void )						{ /* Do nothing */ }
	virtual void OnPortalEnter( void )					{ /* Do nothing */ }
	virtual void OnPortalExit( void )					{ /* Do nothing */ }
	virtual void OnPortalPreExit( void )				{ /* Do nothing */ }
	virtual void OnCellImmediateReport( void )			{ /* Do nothing */ }
	virtual void OnViewParametersChanged( void )		{ /* Do nothing */ }
	virtual void OnInstanceVisible( void )				{ /* Do nothing */ }
	virtual void OnRemovalSuggested( void )				{ /* Do nothing */ }
	virtual void OnInstanceImmediateReport( void )		{ /* Do nothing */ }
	virtual void OnRegionOfInfluenceActive( void )		{ /* Do nothing */ }
	virtual void OnRegionOfInfluenceInactive( void )	{ /* Do nothing */ }
	virtual void OnStencilMask( void )					{ /* Do nothing */ }
	virtual void OnTextMessage( void )					{ /* Do nothing */ }
	virtual void OnDrawLine2D( void )					{ /* Do nothing */ }
	virtual void OnDrawLine3D( void )					{ /* Do nothing */ }
	virtual void OnDrawBuffer( void )					{ /* Do nothing */ }
	virtual void OnOcclusionQueryBegin( void )			{ /* Do nothing */ }
	virtual void OnOcclusionQueryEnd( void )			{ /* Do nothing */ }
	virtual void OnOcclusionQueryGetResult( void )		{ /* Do nothing */ }
	virtual void OnOcclusionQueryDrawTestDepth( void )	{ /* Do nothing */ }
	virtual void OnInstanceDrawDepth( void )			{ /* Do nothing */ }
	virtual void OnFlushDepth( void )					{ /* Do nothing */ }
	virtual void OnDepthPassBegin( void )				{ /* Do nothing */ }
	virtual void OnDepthPassEnd( void )					{ /* Do nothing */ }
	virtual void OnColorPassBegin( void )				{ /* Do nothing */ }
	virtual void OnColorPassEnd( void )					{ /* Do nothing */ }
	virtual void OnTileBegin( void )					{ /* Do nothing */ }
	virtual void OnTileEnd( void )						{ /* Do nothing */ }
	virtual void OnFlushGPUCommandBuffer( void )		{ /* Do nothing */ }
};

class Tr2UmbraCamera
{
public:
	Tr2UmbraCamera();
	~Tr2UmbraCamera();

	void SetScreenSize( unsigned int width, unsigned int height );

	void EnableVirtualPortals( bool enable );
	void EnableOcclusionCulling( bool enable );
    void EnableFrustumCulling( bool enable );
	void EnableDepthPass( bool enable );

	void ResolveVisibility( Umbra::Commander* commander, Umbra::Cell* cell, int recursionDepth );

	void SetViewParameters( const Matrix& viewToCell, const TriFrustum* frustum );
	void SetViewParameters( const Matrix& viewToCell, const Matrix& projection );

	void SetProperties( unsigned int width, unsigned int height, unsigned int properties );
	unsigned int GetProperties( void ) const;

	unsigned int GetWidth( void ) const { return m_width; }
	unsigned int GetHeight( void ) const { return m_height; }

	float GetUmbraCrashWorkaroundScaling( unsigned int size ) const;

private:
	void UpdateParameters();

private:
	// Basic camera properties
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_properties;

	// Pointer to Umbra camera
	Umbra::Camera* m_camera;

	// Is the camera ready?
	bool m_isCameraReady;
};

#endif