///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for segments.
 *	\file		IceSegment.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Segment class.
 *	A segment is defined by S(t) = mP0 * (1 - t) + mP1 * t, with 0 <= t <= 1
 *	Alternatively, a segment is S(t) = Origin + t * Direction for 0 <= t <= 1.
 *	Direction is not necessarily unit length. The end points are Origin = mP0 and Origin + Direction = mP1.
 *
 *	\class		Segment
 *	\author		Pierre Terdiman
 *	\version	1.0
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"


using namespace IceMaths;

float Segment::SquareDistance(const Point& point, float* t)	const
{
	Point Diff = point - mP0;
	Point Dir = mP1 - mP0;
	float fT = Diff | Dir;

	if(fT<=0.0f)
	{
		fT = 0.0f;
	}
	else
	{
		float SqrLen= Dir.SquareMagnitude();
		if(fT>=SqrLen)
		{
			fT = 1.0f;
			Diff -= Dir;
		}
		else
		{
			fT /= SqrLen;
			Diff -= fT*Dir;
		}
	}

	if(t)	*t = fT;

	return Diff.SquareMagnitude();
}

