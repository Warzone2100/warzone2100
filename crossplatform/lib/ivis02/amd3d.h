/***************************************************************************
 *
 * Copyright (c) 1997, 1998 ADVANCED MICRO DEVICES, INC.  All Rights Reserved.
 *      
 * This software is unpublished and contains the trade secrets and 
 * confidential proprietary information of AMD. Unless otherwise
 * provided in the software Agreement associated herewith, it is 
 * licensed in confidence "AS IS" and is not to be reproduced in
 * whole or part by any means except for backup. Use, duplication,
 * or disclosure by the Government is subject to the restrictions
 * in paragraph(b)(3)(B) of the Rights in Technical Data and 
 * Computer Software clause in DFAR 52.227-7013(a)(Oct. 1988).
 * Software owned by Advanced Micro Devices Inc., One AMD Place
 * P.O. Box 3453, Sunnyvale, CA 94088-3453
 ***************************************************************************
 *      
 * AMD3D.H
 *
 * MACRO FORMAT
 * ============
 * This file contains inline assembly macros that
 * generate AMD-3D instructions in binary format.
 * Therefore, C or C++ programmer can use AMD-3D instructions
 * without any penalty in their C or C++ source code.
 * 
 * The macro's name and format conventions are as follow:
 *
 *
 * 	1. First argument of macro is a destination and
 * 	   second argument is a source operand.
 * 		ex) _asm PFCMPEQ (m3, m4)
 *      				  |    |
 *   	 		         dst  src
 *
 *	2. The destination operand can be m0 to m7 only. 
 *     The source operand can be any one of the register
 *     m0 to m7 or _eax, _ecx, _edx, _ebx, _esi, or _edi 
 *     that contains effective address.
 *    	ex) _asm PFRCP    (M7, M6)
 *		ex) _asm PFRCPIT2 (m0, m4)
 *		ex) _asm PFMUL    (m3, _edi)
 *	
 *  3. The prefetch(w) takes one src operand _eax, ecx, _edx,
 *     _ebx, _esi, or _edi that contains effective address.
 *      ex) _asm PREFETCH (_edi)
 *  			    
 * EXAMPLE
 * =======
 * Following program doesn't do anything but it shows you
 * how to use inline assembly AMD-3D instructions in C.
 * Note that this will only work in flat memory model which
 * segment registers cs, ds, ss and es point to the same 
 * linear address space total less than 4GB.
 * 
 * Used Microsoft VC++ 5.0
 *
 * #include <stdio.h>                    
 * #include "amd3d.h"                   
 *                                       
 * void main ()                          
 * {                                     
 *	float x = (float)1.25;            
 * 	float y = (float)1.25;            
 * 	float z, zz;                      
 *
 *	_asm {                            
 * 		movd mm1, x
 * 		movd mm2, y                  
 * 		pfmul (m1, m2)               
 * 		movd z, mm1
 *			femms
 * 	}                                 
 *                                       
 * 	printf ("value of z = %f\n", z);  
 *        
 * 	// 
 *	// Demonstration of using the memory instead of
 *	// multimedia register
 *	//
 * 	_asm {        
 * 		movd mm3, x
 *	   	lea esi, y   // load effective address of y
 * 		pfmul (m3, _esi)
 * 		movd zz, mm3                  
 * 		femms
 * 	}
 *
 *  	printf ("value of zz = %f\n", zz);
 *  }                                     
 ***************************************************************************/
#define	M0   0xc0
#define	M1   0xc1
#define	M2   0xc2
#define	M3   0xc3
#define	M4   0xc4
#define	M5   0xc5
#define	M6   0xc6
#define	M7   0xc7

#define	m0   0xc0
#define	m1   0xc1
#define	m2   0xc2
#define	m3   0xc3
#define	m4   0xc4
#define	m5   0xc5
#define	m6   0xc6
#define	m7   0xc7
#define _EAX 0x00
#define _ECX 0x01
#define _EDX 0x02
#define _EBX 0x03
#define _ESI 0x06
#define _EDI 0x07
#define _eax 0x00
#define _ecx 0x01
#define _edx 0x02
#define _ebx 0x03
#define _esi 0x06
#define _edi 0x07

#define PF2ID(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src   \
   _asm _emit 0x1d \
}

#define PFACC(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xae \
}

#define PFADD(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x9e \
}

#define PFCMPEQ(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb0 \
}

#define PFCMPGE(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x90 \
}

#define PFCMPGT(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa0 \
}

#define PFMAX(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa4 \
}

#define PFMIN(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x94 \
}

#define PFMUL(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb4 \
}

#define PFRCP(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x96 \
}

#define PFRCPIT1(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa6 \
}

#define PFRCPIT2(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb6 \
}

#define PFRSQRT(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x97 \
}

#define PFRSQIT1(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa7 \
}

#define PFSUB(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x9a \
}

#define PFSUBR(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xaa \
}

#define PI2FD(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x0d \
}

#define FEMMS \
{\
	_asm _emit 0x0f	\
	_asm _emit 0x0e	\
}

#define PAVGUSB(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xbf \
}

#define PMULHRW(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb7 \
}

#define PREFETCH(src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0d				\
   _asm _emit 0x00 | src	\
}

#define PREFETCHW(src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0d				\
   _asm _emit 0x08 | src	\
}

//
// Exactly same as above except macro names are all 
// lower case latter.
//
#define pf2id(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x1d \
}

#define pfacc(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xae \
}

#define pfadd(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x9e \
}

#define pfcmpeq(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb0 \
}

#define pfcmpge(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x90 \
}

#define pfcmpgt(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa0 \
}

#define pfmax(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa4 \
}

#define pfmin(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x94 \
}

#define pfmul(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb4 \
}

#define pfrcp(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x96 \
}

#define pfrcpit1(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa6 \
}

#define pfrcpit2(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb6 \
}

#define pfrsqrt(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x97 \
}

#define pfrsqit1(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xa7 \
}

#define pfsub(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x9a \
}

#define pfsubr(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xaa \
}

#define pi2fd(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0x0d \
}

#define femms \
{\
	_asm _emit 0x0f	\
	_asm _emit 0x0e	\
}

#define pavgusb(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xbf \
}

#define pmulhrw(dst, src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0f				\
   _asm _emit ((dst & 0x3f) << 3) | src	\
   _asm _emit 0xb7 \
}

#define prefetch(src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0d				\
   _asm _emit 0x00 | src	\
}

#define prefetchw(src)	\
{\
   _asm _emit 0x0f				\
   _asm _emit 0x0d				\
   _asm _emit 0x08 | src	\
}


