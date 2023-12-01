#include "StdAfx.h"

#include "Tr2KelvinColor.h"

Tr2KelvinColor::Tr2KelvinColor( IRoot* lockobj ) :
	m_temperature( 5500.0f ),
	m_tint( 0.5f ),
	m_whiteBalance( TR2STANDARDILLUMINANT_D55 )
{
}

Tr2KelvinColor::~Tr2KelvinColor()
{
}

bool Tr2KelvinColor::Initialize()
{
	return true;
}

Vector3d XYZToRGB( const Vector3d& xyz )
{
	// Using CIE 1931 (2-deg Standard Observer)
	return Vector3d(
		 0.41866 * xyz.x - 0.15866 * xyz.y - 0.08283 * xyz.z,
		-0.09117 * xyz.x + 0.25243 * xyz.y + 0.01571 * xyz.z,
		 0.00092 * xyz.x - 0.00255 * xyz.y + 0.17860 * xyz.z );
}

Vector2 Tr2StandardIlluminantToCCT( Tr2StandardIlluminant illuminant )
{
	// Returns x,y values using CIE 1931 2-deg Standard Observer
	// Values taken from table in http://en.wikipedia.org/wiki/Standard_illuminant
	switch( illuminant )
	{
	case TR2STANDARDILLUMINANT_A:
		return Vector2( 0.44757f, 0.40745f );
		break;
	case TR2STANDARDILLUMINANT_D50:
		return Vector2( 0.34567f, 0.35850f );
		break;
	case TR2STANDARDILLUMINANT_D55:
		return Vector2( 0.33242f, 0.34743f );
		break;
	case TR2STANDARDILLUMINANT_D65:
		return Vector2( 0.31271f, 0.32902f );
		break;
	case TR2STANDARDILLUMINANT_D75:
		return Vector2( 0.29902f, 0.31485f );
		break;
	case TR2STANDARDILLUMINANT_E:
		return Vector2( 0.33333f, 0.33333f );
		break;
	case TR2STANDARDILLUMINANT_F1:
		return Vector2( 0.31310f, 0.33727f );
		break;
	case TR2STANDARDILLUMINANT_F2:
		return Vector2( 0.37208f, 0.37529f );
		break;
	case TR2STANDARDILLUMINANT_F3:
		return Vector2( 0.40910f, 0.39430f );
		break;
	case TR2STANDARDILLUMINANT_F4:
		return Vector2( 0.44018f, 0.40329f );
		break;
	case TR2STANDARDILLUMINANT_F5:
		return Vector2( 0.31379f, 0.34531f );
		break;
	case TR2STANDARDILLUMINANT_F6:
		return Vector2( 0.37790f, 0.38835f );
		break;
	case TR2STANDARDILLUMINANT_F7:
		return Vector2( 0.31292f, 0.32933f );
		break;
	case TR2STANDARDILLUMINANT_F8:
		return Vector2( 0.34588f, 0.35875f );
		break;
	case TR2STANDARDILLUMINANT_F9:
		return Vector2( 0.37417f, 0.37281f );
		break;
	case TR2STANDARDILLUMINANT_F10:
		return Vector2( 0.34609f, 0.35986f );
		break;
	case TR2STANDARDILLUMINANT_F11:
		return Vector2( 0.38052f, 0.37713f );
		break;
	case TR2STANDARDILLUMINANT_F12:
		return Vector2( 0.43695f, 0.40441f );
		break;
	default: // Default to D55, which seems to be pretty standard
		return Vector2( 0.33242f, 0.34743f );
		break;
	}
}

Vector3d TriColorFromKelvin( float temperature, float tint, Tr2StandardIlluminant whitePoint )
{
	double xc, yc;
	const double T = temperature;
	const double Y = 1.0;

	if( T < 1000.0f || T > 25000.0f )
	{
		return Vector3d( 0.0, 0.0, 0.0 );
	}

	
	if( T >= 1000.0f && T <= 4000.0f )
	{
		xc = -0.2661239 * ( 1000000000.0 / ( T * T * T ) ) -
			  0.2343580 * ( 1000000.0 / ( T * T ) ) +
			  0.8776956 * ( 1000.0 / T ) +
			  0.179910;
	}
	else
	{
		xc = -3.0258469 * ( 1000000000.0 / ( T * T * T ) ) +
			  2.1070379 * ( 1000000.0 / ( T * T ) ) +
			  0.2226347 * ( 1000.0 / T ) +
			  0.24039;
	}

	if( T >= 1000.0f && T <= 2222.0f )
	{
		yc = -1.1063814 * ( xc * xc * xc ) -
			  1.3481102 * ( xc * xc ) +
			  2.1855583 * xc -
			  0.20219683;
	}
	else if( T > 2222.0f && T <= 4000.0 )
	{
		yc = -0.9549476 * ( xc * xc * xc ) -
			  1.3741859 * ( xc * xc ) +
			  2.09137015 * xc -
			  0.16748867;
	}
	else
	{
		yc = 3.081758 * ( xc * xc * xc ) -
			 5.8733867 * ( xc * xc ) +
			 3.75112997 * xc -
			 0.37001483;
	}

	// Compute base correlated color temperature
	const double X = ( Y / yc ) * xc;
	const double Z = ( Y / yc ) * ( 1.0 - xc - yc );
	const Vector3d cctXYZ( X, Y, Z );
	
	// Compute white balance
	const double wbY = 0.54;
	Vector2 wbCoeff = Tr2StandardIlluminantToCCT( whitePoint );
	const double wbX = ( wbY / (double)wbCoeff.y ) * (double)wbCoeff.x;
	const double wbZ = ( wbY / (double)wbCoeff.y ) * 
		(1.0 - (double)wbCoeff.x - (double)wbCoeff.y );
	const Vector3d wbXYZ( wbX, wbY, wbZ );

	// Convert to RGB
	const Vector3d cctRGB = XYZToRGB( cctXYZ );
	const Vector3d wbRGB = XYZToRGB( wbXYZ );

	// Apply white balance
	const Vector3d balancedRGB(
		cctRGB.x / wbRGB.x,
		cctRGB.y / wbRGB.y,
		cctRGB.z / wbRGB.z );

	// Apply tint
	const Vector3d tintedRGB(
		(1.0 - tint) * balancedRGB.x,
		tint * balancedRGB.y,
		(1.0 - tint) * balancedRGB.z );

	// Scale to (0.0,1.0) range
	const double componentMax = std::max( tintedRGB.x,
		std::max( tintedRGB.y, tintedRGB.z ) );

	// Final color
	const Vector3d finalColor(
		tintedRGB.x / componentMax,
		tintedRGB.y / componentMax,
		tintedRGB.z / componentMax );

	return finalColor;
}