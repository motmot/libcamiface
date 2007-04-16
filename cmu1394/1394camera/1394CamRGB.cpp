//  1394CamRGB.cpp: source for YUV to RGB conversion routines in C1394Camera
//
//	Version 5.2 : 3/13/2002
//
//	Copyright 5/2000 - 10/2001
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//
//  Copyright 3/2002
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//
//  This file is part of the CMU 1394 Digital Camera Driver
//
//  The CMU 1394 Digital Camera Driver is free software; you can redistribute 
//  it and/or modify it under the terms of the GNU Lesser General Public License 
//  as published by the Free Software Foundation; either version 2.1 of the License,
//  or (at your option) any later version.
//
//  The CMU 1394 Digital Camera Driver is distributed in the hope that it will 
//  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with the CMU 1394 Digital Camera Driver; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include <mmsystem.h>
#include <stdio.h>
#include "1394Camera.h"
extern "C" {
#include "pch.h"
}


/*
 * getRGB
 *
 * essentially a giant switch statement so the user doesn't need one
 *
 * places the current image into pBitmap in top-down RGB format
 *
 */

void C1394Camera::getRGB(unsigned char *pBitmap)
{
	DllTrace(DLL_TRACE_ENTER,"ENTER getRGB (%08x)\n",pBitmap);
	if(pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,"getRGB: pBitmap is NULL, bailing out\n");
		DllTrace(DLL_TRACE_EXIT,"EXIT getRGB\n");
		return;
	}

	switch(m_videoFormat)
	{
		case 0:
			switch(m_videoMode)
			{
			case 0: 
				// 160x120 YUV444
				YUV444toRGB(pBitmap);
				break;
			case 1:
				// 320x240 YUV222
				YUV422toRGB(pBitmap);
				break;
			case 2:
				// 640x480 YUV411
				YUV411toRGB(pBitmap);
				break;
			case 3:
				// 640x480 YUV422
				YUV422toRGB(pBitmap);
				break;
			case 4:
				// 640x480 RGB
				memcpy(pBitmap,m_pData,640 * 480 * 3);
				break;
			case 5:
				// 640x480 MONO
				YtoRGB(pBitmap);
				break;
			case 6:
				// 640x480 MONO16
				Y16toRGB(pBitmap);
				break;
			default:
				DllTrace(DLL_TRACE_ERROR,"getRGB: invalid video mode/format combination\n");
				break;
			} break;

		case 1:
			switch(m_videoMode)
			{
			case 0:
				// 800x600 YUV422
				YUV422toRGB(pBitmap);
				break;
			case 1:
				// 800x600 RGB
				memcpy(pBitmap,m_pData,800 * 600 * 3);
				break;
			case 2:
				// 800x600 MONO
				YtoRGB(pBitmap);
				break;
			case 3:
				// 1024x768 YUV422
				YUV422toRGB(pBitmap);
				break;
			case 4:
				// 1024x768 RGB
				memcpy(pBitmap,m_pData,1024 * 768 * 3);
				break;
			case 5:
				// 1024x768 MONO
				YtoRGB(pBitmap);
				break;
			case 6:
				// 800x600 MONO16
			case 7:
				// 1024x768 MONO16
				Y16toRGB(pBitmap);
				break;
			default:
				DllTrace(DLL_TRACE_ERROR,"getRGB: invalid video mode/format combination\n");
				break;
			} break;

		case 2:
			switch(m_videoMode)
			{
			case 0:
				// 1280x960 YUV422
				YUV422toRGB(pBitmap);
				break;
			case 1:
				// 1280x960 RGB
				memcpy(pBitmap,m_pData,1280 * 960 * 3);
				break;
			case 2:
				// 1280x960 MONO
				YtoRGB(pBitmap);
				break;
			case 3:
				// 1600x1200 YUV422
				YUV422toRGB(pBitmap);
				break;
			case 4:
				// 1600x1200 RGB
				memcpy(pBitmap,m_pData,1600 * 1200 * 3);
				break;
			case 5:
				// 1600x1200 MONO
				YtoRGB(pBitmap);
				break;
			case 6:
				// 1280x1024 MONO16
			case 7:
				// 1600x1200 MONO16
				Y16toRGB(pBitmap);
				break;
			default:
				DllTrace(DLL_TRACE_ERROR,"getRGB: invalid video mode/format combination\n");
				break;
			} break;

		case 7:
			switch(m_controlSize.m_colorCode)
			{
			case 0:
				// Mono8
				YtoRGB(pBitmap);
				break;
			case 1:
				// YUV 411
				YUV411toRGB(pBitmap);
				break;
			case 2:
				// YUV 422
				YUV422toRGB(pBitmap);
				break;
			case 3:
				// YUV 444
				YUV444toRGB(pBitmap);
				break;
			case 4:
				// RGB8
				memcpy(pBitmap,m_pData,1280 * 960 * 3);
				break;
			case 5:
				// Mono16
				Y16toRGB(pBitmap);
				break;
			case 6:
				// RGB16
				RGB16toRGB(pBitmap);
				break;
			default:
				// unsupported
				DllTrace(DLL_TRACE_ERROR,
					"getRGB: invalid color code %d in m_controlSize\n",
					m_controlSize.m_colorCode);
				break;
			} break;

		default:
			DllTrace(DLL_TRACE_ERROR,"getRGB: unsupported format: %d\n",m_videoFormat);
			break;
	}
	DllTrace(DLL_TRACE_EXIT,"EXIT getRGB\n");
}

void C1394Camera::getDIB(unsigned char *pBitmap)
{
	getRGB(pBitmap);
	unsigned char *linebuf = (unsigned char *) malloc(m_width * 3);
	unsigned char *top, *bot, *pin, *pout, *pend;
	int ystep = m_width * 3;
	top = pBitmap;
	bot = top + (m_height - 1) * m_width * 3;

	while(top < bot)
	{
		// copy the top line into the linebuf, swapping R & B along the way
		pin = top;
		pend = pin + ystep;
		pout = linebuf;
		while(pin < pend)
		{
			*pout++ = *(pin + 2);
			*pout++ = *(pin + 1);
			*pout++ = *pin;
			pin += 3;
		}

		// copy the bottom line into the top line as before
		pin = bot;
		pend = pin + ystep;
		pout = top;
		while(pin < pend)
		{
			*pout++ = *(pin + 2);
			*pout++ = *(pin + 1);
			*pout++ = *pin;
			pin += 3;
		}

		// copy the linebuf into the bottom line
		RtlCopyMemory(bot,linebuf,ystep);

		bot -= ystep;
		top += ystep;
	}
	free(linebuf);
}

/**********************************************
 *  The actual conversion stuff starts here.  *
 *  This code is highly optimized, uses only  *
 *  integer math, and usually outperforms     *
 *  Intel's IPL by a 10-20% margin.           *
 **********************************************/

/*
 * Note on computation of deltaG:
 *
 * We are still using 0.1942 U + 0.5094 V, except
 * they have been multiplied by 65536 so we can use integer
 * multiplication, then a bitshift, instead of using the
 * slower FPU.  Because each channel is only 8 bits, the loss
 * in precision has negligible effects on the result.
 */

// I'm not overly fond of macros, but this gets used a LOT

#define CLAMP_TO_UCHAR(a) (unsigned char)((a) < 0 ? 0 : ((a) > 255 ? 255 : (a)))

// YUV 4:4:4 (24 bits per pixel)

void C1394Camera::YUV444toRGB(unsigned char* pBitmap)
{
	long Y, U, V, deltaG;
	unsigned char *srcptr, *srcend, *destptr;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "YUV444toRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	srcptr = m_pData;
	srcend = srcptr + (m_width * m_height * 3);
	destptr = pBitmap;

	// data pattern: UYV
	// unroll it to 4 pixels/round

	while(srcptr < srcend)
	{
		U = (*srcptr++) - 128;
		Y = (*srcptr++);
		V = (*srcptr++) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		U = (*srcptr++) - 128;
		Y = (*srcptr++);
		V = (*srcptr++) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		U = (*srcptr++) - 128;
		Y = (*srcptr++);
		V = (*srcptr++) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		U = (*srcptr++) - 128;
		Y = (*srcptr++);
		V = (*srcptr++) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );
	}
}

// YUV 4:2:2 (16 bits per pixel)

void C1394Camera::YUV422toRGB(unsigned char *pBitmap)
{
	long Y, U, V, deltaG;
	unsigned char *srcptr, *srcend, *destptr;

	srcptr = m_pData;
	srcend = srcptr + ((m_width * m_height) << 1);
	destptr = pBitmap;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "YUV422toRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	// data pattern: UYVY

	while(srcptr < srcend)
	{
		U = *srcptr;
		U -= 128;
		V = *(srcptr+2);
		V -= 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		Y = *(srcptr + 1);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		Y = *(srcptr + 3);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		srcptr += 4;

		// twice in the same loop... just like halving the loop overhead

		U = (*srcptr) - 128;
		V = (*(srcptr+2)) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		Y = *(srcptr + 1);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		Y = *(srcptr + 3);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		srcptr += 4;

	}
}

// YUV 4:1:1 (12 bits per pixel)

void C1394Camera::YUV411toRGB(unsigned char *pBitmap)
{
	long Y, U, V, deltaG;
	unsigned char *srcptr, *srcend, *destptr;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "YUV411toRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	// data pattern: UYYVYY

	srcptr = m_pData;
	srcend = srcptr + ((m_width * m_height * 3) >> 1);
	destptr = pBitmap;

	while(srcptr < srcend)
	{
		U = (*srcptr) - 128;
		V = (*(srcptr+3)) - 128;

		deltaG = (12727 * U + 33384 * V);
		deltaG += (deltaG > 0 ? 32768 : -32768);
		deltaG >>= 16;

		Y = *(srcptr + 1);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		Y = *(srcptr + 2);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		Y = *(srcptr + 4);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		Y = *(srcptr + 5);
		*destptr++ = CLAMP_TO_UCHAR( Y + V );
		*destptr++ = CLAMP_TO_UCHAR( Y - deltaG );
		*destptr++ = CLAMP_TO_UCHAR( Y + U );

		srcptr += 6;
	}
}

// 8-bit monochrome, duplicate into R,G,B

void C1394Camera::YtoRGB(unsigned char *pBitmap)
{
	unsigned char *srcptr, *srcend, *destptr;

	srcptr = m_pData;
	srcend = srcptr + (m_width * m_height);
	destptr = pBitmap;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "YtoRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	// just Y's (monochrome)

	// unroll it to 4 per cycle

	while(srcptr < srcend)
	{
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr++;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr++;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr++;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr++;
	}
}

// 16-bit monochrome (new to spec 1.3)
// the first of each pair of bytes is the high byte, so just copy those to RGB

void C1394Camera::Y16toRGB(unsigned char *pBitmap)
{
	unsigned char *srcptr, *srcend, *destptr;

	srcptr = m_pData;
	srcend = srcptr + 2 * (m_width * m_height);
	destptr = pBitmap;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "Y16toRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	// just Y's (monochrome, 16-bit little endian)

	// unroll it to 4 per cycle

	while(srcptr < srcend)
	{
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;
	}
}

// 16-bit RGB (new to spec 1.3)
// the first of each pair of bytes is the high byte, so just copy those to RGB
// near duplicate of Y16toRGB

void C1394Camera::RGB16toRGB(unsigned char *pBitmap)
{
	unsigned char *srcptr, *srcend, *destptr;

	srcptr = m_pData;
	srcend = srcptr + 6 * (m_width * m_height);
	destptr = pBitmap;

	// single-stage idiotproofing
	if(m_pData == NULL || pBitmap == NULL)
	{
		DllTrace(DLL_TRACE_ERROR,
				 "RGB16toRGB: NULL pointer exception (m_pData = %08x, pBitmap = %08x)\n",
				 m_pData,
				 pBitmap);
		return;
	}

	// R,G,B are 16-bit source, chop of the top 8 and feed 

	// unroll it to 3 per cycle

	while(srcptr < srcend)
	{
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;

		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		*destptr++ = *srcptr;
		srcptr += 2;
	}
}

