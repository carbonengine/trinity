// Copyright © 2026 CCP ehf.

#include "Solids.fxh"

technique Main
{
    pass P0
    {
        ZWriteEnable = TRUE;
        ZEnable = TRUE;
        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        AlphaTestEnable = FALSE;
        DepthBias = 0.0001;
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader = compile ps_3_0 SpritePS();
    }
}
