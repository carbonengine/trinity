// Copyright © 2026 CCP ehf.

#include "Solids.fxh"

technique Main
{
    pass P0
    {
        ZWriteEnable = TRUE;
        ZEnable = FALSE;
        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        AlphaTestEnable = FALSE;
        
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader = compile ps_3_0 SpritePS();
    }
}