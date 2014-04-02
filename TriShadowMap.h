#ifndef TRISHADOWMAP_H
#define TRISHADOWMAP_H

#include "Tr2DeviceResource.h"

#include "TriFrustum.h"
#include "Utilities/Obb.h"

class TriFrustumOrtho;

BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( Tr2Effect );

// --------------------------------------------------------------------------------
// Description:
//   This class holds a simple shadow map
// SeeAlso:
//   
// --------------------------------------------------------------------------------
class TriShadowMap :
	public INotify,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	TriShadowMap( IRoot* lockobj = 0 );
	~TriShadowMap();

    using IRoot::Lock;
	using IRoot::Unlock;

	void ClearVariableStore();

	static void CalculateShadowFrustum( 
		Vector3 lightDirection,
		float lightDistance,
		Obb receiverBoundingBox,
		TriFrustumOrtho& frustum );

	void SetLightDirection( const Vector3& dir );
	void SetLightDistance( float dist );
	
	// Set the local bounding box for the shadow receivers
	void SetReceiverObb( const Obb& obb );

	bool BeginShadowRendering( Vector3& lightViewPosition, Matrix& lightView, Matrix& lightViewProj, Tr2RenderContext& renderContext );
	void GetBounds( Vector3& minBounds, Vector3& maxBounds );
	void GetReceiverLightAabb( Vector3& min, Vector3& max );
	void EndShadowRendering();
	void SetShadowTexture( bool useBlankTexture = false );

	Vector4 GetShadowMapSettings() const;

	void DrawDebugInfo();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:
#if TRINITYDEV
	void GetDescription( std::string& desc );
#endif

	Tr2TextureAL& GetTexture();

private:
	bool m_useMips;			// True if we want shadow map to be mipped
	bool m_generateMips;	// True if we generate mips manually
	bool m_filterVsm;		// True if we want a Gaussian blur on the VSM

	// of this is disabled this module is nearly empty
	bool m_enabled;

	// Width and height of shadow map
	unsigned int m_size;
	// Depth test bias
	float m_depthBias;
	float m_lightLeakStep;

	// bounds of the frustum of the light, ie with Z pulled in to make sure
	// we include every possible shadow caster.
	Vector3 m_boundsMin;
	Vector3 m_boundsMax;
	// bounds of the frustum for just the shadow receiver, ie the AABB of the
	// input OBB transformed to light space, and no further changes.
	Vector3 m_receiverLightAabbMin;
	Vector3 m_receiverLightAabbMax;

	Vector3 m_lightDirection;
	float m_lightDistance;
	Obb		m_receiverObb;

	bool	m_debugFreezeObb;

	TriFrustum m_frustum;

	// debug drawer helpers
	TriLineSetPtr m_lineSet;

	// the handle to the texture var
	TriVariable* m_shadowMapHandle;
	TriVariable* m_shadowSizeHandle;
	TriVariable* m_invInputSizeHandle;

	Tr2TextureAL				m_noShadowTexture;
	Tr2RenderTargetAL			m_shadowMapRT;
	Tr2DepthStencilAL			m_shadowMapDS;

	Tr2RenderTargetAL			m_filterBlurRT;
	Tr2EffectPtr				m_filterBlurEffect;

	typedef std::vector<Vector3> VectorArray;
};
TYPEDEF_BLUECLASS( TriShadowMap );

#endif
