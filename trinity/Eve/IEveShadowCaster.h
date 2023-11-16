#ifndef IEveShadowCaster_h
#define IEveShadowCaster_h

struct ITr2Renderable;
class TriFrustumOrtho;
class TriFrustum;

struct ShadowCasterInfo
{
	ITr2Renderable* renderable;
	float shadowSize;
};

BLUE_INTERFACE( IEveShadowCaster ) :
	public IRoot
{
	// Warning: GetRenderablesCastingShadow can be called on different threads, so it needs to be thread-safe.
	virtual bool GetRenderablesCastingShadow( bool isSelf, const TriFrustumOrtho& frustum, std::vector<ITr2Renderable*>& renderables ) = 0;
	// Used for cascaded shadow map
	virtual void GatherShadowRenderables( std::vector<std::vector<ShadowCasterInfo>>& shadowCasters, TriFrustum* splitCameraFrustums, TriFrustumOrtho* shadowFrustums, const size_t arraySize, const unsigned int shadowMapSize, const Vector3 sunDir ) = 0;
	virtual bool IsShadowReceiveEnabled() = 0;
};

#endif
