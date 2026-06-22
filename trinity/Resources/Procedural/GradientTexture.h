// Copyright © 2025 CCP ehf.

////////////////////////////////////////////////////////////
//
//    Created:   May 2025
//

#pragma once

void RasterizeGradient( const std::string_view& path, ImageIO::HostBitmap& bitmap );
bool IsGradientTexturePath( const wchar_t* path );