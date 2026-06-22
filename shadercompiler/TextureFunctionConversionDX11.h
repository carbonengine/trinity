// Copyright © 2023 CCP ehf.

#pragma once

class ParserState;
struct Symbol;

extern void ConvertTextureFunctionsDX11( ParserState& state );
extern void TransferSRGBToTexturesDX11( ParserState& state );

extern void ConvertTextureFunctionsToMetal( ParserState& state );

extern void MergeSamplers( ParserState& state );

int32_t BindlessTextureType( int type );
