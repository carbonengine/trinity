////////////////////////////////////////////////////////////
//
//    Created:   May 2025
//    Copyright: CCP 2025
//

#pragma once

void RasterizeGradient( const std::string_view& path, ImageIO::HostBitmap& bitmap );
bool IsGradientTexturePath( const wchar_t* path );