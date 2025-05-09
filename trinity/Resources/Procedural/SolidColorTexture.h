////////////////////////////////////////////////////////////
//
//    Created:   May 2025
//    Copyright: CCP 2025
//

#pragma once

void RasterizeSolidColor( const std::string_view& path, ImageIO::HostBitmap& bitmap );
bool IsSolidColorTexturePath( const wchar_t* path );