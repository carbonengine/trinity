// Copyright © 2025 CCP ehf.

#pragma once

namespace Base64
{

std::vector<uint8_t> Decode( const std::string_view& str );
std::string Encode( const uint8_t* data, size_t size );

}