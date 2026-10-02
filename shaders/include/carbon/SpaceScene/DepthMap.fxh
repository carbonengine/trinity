#ifndef CARBON_DEPTHMAP_FXH
#define CARBON_DEPTHMAP_FXH

// Contains SRV representation of the depth buffer for the current render pass. Note that not all render passes may use a depth buffer, 
// so this may be unbound in some cases. For example, opaque forward rendering passes may not use a depth buffer, while transparent 
// forward rendering passes may use the depth buffer from the opaque pass.
Texture2D DepthMap <bool AutoRegister = true; >;

#endif // CARBON_DEPTHMAP_FXH