/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * netcrypt.c
 *
 * 1999 pumpkin Studios
 * secure netplay services.
 *
 * Feistel cipher with XOR & addition as nonlinear mixing funcs.
 * Based on Tiny Encryption Algorithm (TEA) by David Wheeler & Roger Needham
 * of the cambridge computer laboratory.
 * delta = delta = (sqrt(5) - 1).2^31
 *
 * This cipher is secure enough for time critical data, but probably
 * not secure enough for long term storage. eg, find for encrypting network packets
 * but not good enough for storing files securely on harddisk. Try encryptstrength=32;
 */

#include <physfs.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "netplay.h"
#include "netlog.h"

#define			ENCRYPTSTRENGTH 16		// 32=ample, 16=sufficient, 8=maybe ok, good dispersion after 6.
#define			NIBBLELENGTH	8		// bytes done per encrypt step.

// ////////////////////////////////////////////////////////////////////////
// Prototypes
UDWORD	NEThashFile(char *pFileName);
UDWORD	NEThashBuffer(char *pData, UDWORD size);

BOOL	NETsetKey			(UDWORD c1,UDWORD c2,UDWORD c3, UDWORD c4);
NETMSG*	NETmanglePacket		(NETMSG *msg);
void	NETunmanglePacket	(NETMSG *msg);

BOOL	NETmangleData		( UDWORD *input, UDWORD *result, UDWORD dataSize);
BOOL	NETunmangleData		( UDWORD *input, UDWORD *result, UDWORD dataSize);

// ////////////////////////////////////////////////////////////////////////
// make a hash value from an exe name.
UDWORD	NEThashFile(char *pFileName)
{
	UDWORD	hashval,c,*val;
	PHYSFS_file	*pFileHandle;
	char	fileName[255];

	UBYTE	inBuff[2048];		// must be multiple of 4 bytes.

	strcpy(fileName,pFileName);

	hashval =0;

	debug( LOG_WZ, "NEThashFile: Hashing File\n" );

	// open the file.
	pFileHandle = PHYSFS_openRead(fileName);									// check file exists
	if (pFileHandle == NULL)
	{
		debug( LOG_WZ, "NEThashFile: Failed\n" );
		return 0;															// failed
	}

	// multibyte/buff version
	while(PHYSFS_read( pFileHandle, &inBuff, sizeof(inBuff), 1 ) == 1)				// get number of droids in force
	{
		for(c=0;c<2048 ;c+=4)
		{	val = (UDWORD*)&inBuff[c];
			hashval = hashval ^ *val;
		}

	}

#if 0 // 1 byte version
	// xor the file with it's self.
	while((c=getc(pFileHandle))!=EOF)
	{
		hashval = hashval ^ c;
	}
#endif
	PHYSFS_close(pFileHandle);

	debug( LOG_WZ, "NEThashFile: Hash Complete :   *****  %u  ***** is todays magic number.\n",hashval );
//	DBERROR(("%d",hashval));

	return hashval;
}


// ////////////////////////////////////////////////////////////////////////
// return a hash from a data buffer.

UDWORD	NEThashBuffer(char *pData, UDWORD size)
{
	UDWORD hashval,*val;
	UDWORD pt;

	hashval =0;
	pt =0;

	while(pt <= size-4) // remember that we read 4 bytes at once
	{
		val = (UDWORD*)(pData+pt);
		hashval = hashval ^ *val;
		pt += 4;
	}

	return hashval;
}



// ////////////////////////////////////////////////////////////////////////
// return a ubyte hash from a UDWORD value.
UBYTE NEThashVal(UDWORD value)
{
	return (value^13416564)%246;
}

// ////////////////////////////////////////////////////////////////////////
// set the key for the encrypter.
BOOL NETsetKey(UDWORD c1,UDWORD c2,UDWORD c3, UDWORD c4)
{
	if(c1)
	{
		NetPlay.cryptKey[0] = c1;
	}
	if(c2)
	{
		NetPlay.cryptKey[1] = c2;
	}
	if(c3)
	{
		NetPlay.cryptKey[2] = c3;
	}
	if(c4)
	{
		NetPlay.cryptKey[3] = c4;
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// encrypt a byte sequence of nibblelength
static BOOL mangle	( UDWORD *v,  UDWORD *w)
{
	UDWORD	y=v[0],
					z=v[1],
					sum=0,
					delta=0x9E3779B9,
					n=ENCRYPTSTRENGTH;
	while(n-- > 0)
	{
		sum+=delta;
		y += ((z << 4) + NetPlay.cryptKey[0]) ^ (z + sum) ^ ((z >> 5) + NetPlay.cryptKey[1]);
		z += ((y << 4) + NetPlay.cryptKey[2]) ^ (y + sum) ^ ((y >> 5) + NetPlay.cryptKey[3]);
	}
	w[0] = y;
	w[1] = z;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// decrypt a byte sequence of nibblelength
static BOOL unmangle(UDWORD *v, UDWORD *w)
{
	UDWORD	y=v[0],
					z=v[1],
					sum,
					delta=0x9E3779B9,
					n=ENCRYPTSTRENGTH;

	sum = delta * n;/* (generally sum =delta*n )*/
	while(n-- > 0)
	{
		z -= ((y << 4) + NetPlay.cryptKey[2]) ^ (y + sum) ^ ((y >> 5) + NetPlay.cryptKey[3]);
		y -= ((z << 4) + NetPlay.cryptKey[0]) ^ (z + sum) ^ ((z >> 5) + NetPlay.cryptKey[1]);
		sum -= delta;
	}
	w[0] = y;
	w[1] = z;
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// encrypt a netplay packet

NETMSG *NETmanglePacket(NETMSG *msg)
{
	NETMSG result;
	UDWORD pos=0;

#if 0
	return msg;
#endif

	if(msg->size > MaxMsgSize-NIBBLELENGTH)
	{
		debug( LOG_ERROR, "NETmanglePacket: can't encrypt huge packets. returning unencrypted packet" );
		abort();
		return msg;
	}

	msg->paddedBytes	= 0;
	while( msg->size%NIBBLELENGTH != 0)	//need to pad out msg.
	{
		msg->body[msg->size] = 0;
		msg->size++;
		msg->paddedBytes++;
	}

	result.type			= msg->type + ENCRYPTFLAG;
	result.size			= msg->size;
	result.paddedBytes		= msg->paddedBytes;
	result.destination		= msg->destination;

	while(msg->size)
	{
		mangle((UDWORD*)&msg->body[pos],(UDWORD*)&result.body[pos]);
		pos			+=NIBBLELENGTH;
		msg->size	-=NIBBLELENGTH;
	}

	memcpy( msg,&result,sizeof(NETMSG) );
	return msg;
}

// ////////////////////////////////////////////////////////////////////////
// decrypt a netplay packet
// messages SHOULD be 8byte multiples, not required tho. will return padded out..

void NETunmanglePacket(NETMSG *msg)
{
	NETMSG result;
	UDWORD pos=0;

	if(msg->size%NIBBLELENGTH !=0)
	{
		debug( LOG_ERROR, "NETunmanglePacket: Incoming msg wrong length" );
		abort();
		NETlogEntry("NETunmanglePacket failure",msg->type,msg->size);
		return;
	}

	result.type = msg->type - ENCRYPTFLAG;
	result.size = 0;
	result.paddedBytes = msg->paddedBytes;

	while(msg->size)
	{
		unmangle((UDWORD*)&msg->body[pos],(UDWORD*)&result.body[pos]);
		pos			+=NIBBLELENGTH;
		msg->size	-=NIBBLELENGTH;
		result.size	+=NIBBLELENGTH;
	}
	result.size -= msg->paddedBytes;

	memcpy(msg,&result,sizeof(NETMSG));
	return;
}

// ////////////////////////////////////////////////////////////////////////
// encrypt any datastream.
BOOL NETmangleData(UDWORD *input, UDWORD *result, UDWORD dataSize)
{
	UDWORD	offset;

	offset = 0;

	if(dataSize%8 != 0)		//if message not multiple of 8 bytes,
	{
		debug( LOG_ERROR, "NETmangleData: msg not a multiple of 8 bytes" );
		abort();
		return FALSE;
	}

	//  /4's are UDWORD form. since nibblelength is in char form
	while(offset!=(UDWORD)(dataSize/4) )
	{
		mangle( (input+offset),(result+offset) );
		offset		+= NIBBLELENGTH/4 ;
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// decrypt any datastream.
BOOL NETunmangleData(UDWORD *input, UDWORD *result, UDWORD dataSize)
{
	UDWORD	offset;

	memset(result,0,dataSize);
	offset = 0;

	if(dataSize%8 != 0)		//if message not multiple of 8 bytes,
	{
		debug( LOG_ERROR, "NETunmangleData: msg not a multiple of 8 bytes" );
		abort();
		return FALSE;
	}

	//  /4's are UDWORD form. since nibblelength is in char form
	while(offset!= (UDWORD)(dataSize/4))
	{
		unmangle( (input+offset) ,(result+offset));
		offset	+= NIBBLELENGTH/4;
	}
	return TRUE;
}

