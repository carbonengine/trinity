#pragma once

class ParserState;
struct Symbol;

extern void ConvertTextureFunctionsDX11( ParserState& state );
extern void TransferSRGBToTexturesDX11( ParserState& state );