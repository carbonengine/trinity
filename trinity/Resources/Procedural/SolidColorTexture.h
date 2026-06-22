// Copyright © 2025 CCP ehf.

////////////////////////////////////////////////////////////
//
//    Created:   May 2025
//

#pragma once

void RasterizeSolidColor( const std::string_view& path, ImageIO::HostBitmap& bitmap );
bool IsSolidColorTexturePath( const wchar_t* path );