///////////////////////////////////////////////////////////////////////////////
//
// 1394CamCap.cpp
//
// source for C1394Camera::
//  - StartImageCapture
//  - CaptureImage
//  - StopImageCapture
//
// Based on the following functions originally by Iwan Ulrich from isochapi.c:
//
//		IsochStartImageCapture
//		IsochAttachBufferCapture
//		IsochWaitImageCapture
//		IsochStopImageCapture
//
//	Copyright 1/2000
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//
//  Copyright 3/2002 - 7/2002
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
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include <mmsystem.h>
#include <stdio.h>
#include "1394Camera.h"
#include "pch.h"

/* Image Capture has been deprecated since 6.3, use
 * Image Acquisition with nBuffers = 1 to emulate this
 * functionality
 */
/*
 * StartImageCapture
 *
 * Arguments:
 *   <none>
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Comments:
 *   Wraps StartImageAcquisitionEx
 */

int C1394Camera::StartImageCapture()
{
	return StartImageAcquisitionEx(1,10,0);
}


/*
 * CaptureImage
 *
 * Arguments:
 *   <none>
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Comments:
 *   Wraps AcquireImageEx
 */

int C1394Camera::CaptureImage()
{
	return AcquireImageEx(TRUE,NULL);
}


/*
 * StopImageCapture
 *
 * Arguments:
 *   <none>
 *
 * Return Values:
 *   0 : success
 *   nonzero : some error ;)
 *
 * Comments:
 *   Wraps StopImageAcquisition
 */

int C1394Camera::StopImageCapture()
{
	return StopImageAcquisition();
}
