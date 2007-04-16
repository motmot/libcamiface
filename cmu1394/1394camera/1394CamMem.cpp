///////////////////////////////////////////////////////////////////////////////
//
// 1394CamMem.cpp
//
// source for C1394Camera::
//  - MemGetNumChannels
//  - MemGetCurrentChannel
//  - MemLoadChannel
//  - MemSaveChannel
//
//  implements the camera memory (eeprom) channel load/save functionality 
//  introduced in rev 1.20 of the IIDC spec
//
//  Copyright 6/2003
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//  tallbaker@cmu.edu
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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
 * MemGetNumChannels
 *
 * Public
 *
 * Reads from the BASIC_FUNC_INQ register in search of the maximum memory channel,
 * which is bits 28-31 (the last four)
 *
 * Returns:
 *  -1 : Error on WriteQuadlet, GetLastError() may help
 *   n : maximum memory channel (0 indicates no user memory is available)
 */

int C1394Camera::MemGetNumChannels()
{
	ULONG ulRet, ulData;

	DllTrace(DLL_TRACE_ENTER,"ENTER MemGetNumChannels\n");
	if(CAM_SUCCESS != (ulRet = ReadQuadlet(0x400,&ulData)))
	{
		DllTrace(DLL_TRACE_ERROR,"MemGetNumChannels: error %08x on ReadQuadlet(0x400)\n",ulRet);
		return CAM_ERROR;
	}

	ulData &= 0xF;

	DllTrace(DLL_TRACE_EXIT,"EXIT MemGetNumChannels (%d)\n",ulData);

	return ulData;
}


/*
 * MemGetCurrentChannel
 *
 * Public
 *
 * Reads from the CUR_MEM_CH = 0x624 register in search of the currently
 * loaded channel in bits 0-3.
 * which is bits 28-31 (the last four)
 *
 * Returns:
 *  -1 : Error on ReadQuadlet, GetLastError() may help
 *   n : current memory channel (0 indicates factory defaults)
 */

int C1394Camera::MemGetCurrentChannel()
{
	ULONG ulRet, ulData;

	DllTrace(DLL_TRACE_ENTER,"ENTER MemGetCurrentChannel\n");
	if(CAM_SUCCESS != (ulRet = ReadQuadlet(0x624,&ulData)))
	{
		DllTrace(DLL_TRACE_ERROR,"MemGetCurrentChannel: error %08x on ReadQuadlet(0x624)\n",ulRet);
		return -1;
	}

	ulData = (ulData >> 28) & 15;

	DllTrace(DLL_TRACE_EXIT,"EXIT MemGetCurrentChannel (%d)\n",ulData);

	return (int) ulData;
}


/*
 * MemLoadChannel
 *
 * Public
 *
 * Writes <channel> to the CUR_MEM_CH = 0x624 register, causing the contents thereof
 * to be loaded into the camera's control and status registers
 *
 * Arguments
 *  channel : The channel to write to (1 <= channel <= MAX_CHANNEL)
 *
 * Returns:
 *  CAM_SUCCESS: success
 *  CAM_ERROR_PARAM_OUT_OF_RANGE : <channel> is invalid
 *  Other Errors : originate from Read/WriteQuadlet, check GetLastError()
 */

int C1394Camera::MemLoadChannel(int channel)
{
	ULONG ulRet, ulData;

	DllTrace(DLL_TRACE_ENTER,"ENTER MemLoadChannel (%d)\n",channel);

	if(channel < 0 || channel > MemGetNumChannels())
	{
		DllTrace(DLL_TRACE_ERROR,"MemLoadChannel: Invalid chanel: %d\n",channel);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	ulData = (unsigned long)(channel & 0xF) << 28;

	if(CAM_SUCCESS != (ulRet = WriteQuadlet(0x624, ulData)))
	{
		DllTrace(DLL_TRACE_ERROR,"MemLoadChannel: Error %08x on WriteQuadlet(0x624)\n");
		return ulRet;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT MemLoadChannel (%d)\n",channel);

	return CAM_SUCCESS;
}


/*
 * MemSaveChannel
 *
 * Public
 *
 * Writes <channel> to the MEM_SAVE_CH = 0x620 register, then writes to MEMORY_SAVE = 0x618
 * causing the contents of the control and status registers to be saved to <channel> in the
 * camera's EEPROM.
 *
 * Arguments
 *  channel : The channel to write to (1 <= channel <= MAX_CHANNEL)
 *
 * Returns:
 *  CAM_SUCCESS: success
 *  CAM_ERROR_PARAM_OUT_OF_RANGE : <channel> is invalid
 *  Other Errors : originate from Read/WriteQuadlet, check GetLastError()
 */

int C1394Camera::MemSaveChannel(int channel)
{
	ULONG ulRet, ulData;

	DllTrace(DLL_TRACE_ENTER,"ENTER MemSaveChannel (%d)\n",channel);

	if(channel <= 0 || channel > MemGetNumChannels())
	{
		DllTrace(DLL_TRACE_ERROR,"MemSaveChannel: Invalid chanel: %d\n",channel);
		return CAM_ERROR_PARAM_OUT_OF_RANGE;
	}

	ulData = (unsigned long)(channel & 0xF) << 28;

	if(CAM_SUCCESS != (ulRet = WriteQuadlet(0x620, ulData)))
	{
		DllTrace(DLL_TRACE_ERROR,"MemSaveChannel: Error %08x on WriteQuadlet(0x620)\n");
		return ulRet;
	}

	ulData = 0x80000000ul;

	if(CAM_SUCCESS != (ulRet = WriteQuadlet(0x618, ulData)))
	{
		DllTrace(DLL_TRACE_ERROR,"MemSaveChannel: Error %08x on WriteQuadlet(0x618)\n");
		return ulRet;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT MemSaveChannel (%d)\n",channel);

	return CAM_SUCCESS;
}

