/*
** Copyright (c) 1996, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
**
** $Revision: 22 $ 
** $Date: 3/03/98 5:13p $ 
**
*/

#ifndef __SST1INIT_H__
#define __SST1INIT_H__

/*
** Copyright (c) 1996, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
** $Revision: 22 $
** $Date: 3/03/98 5:13p $
**
** SST-1 Initialization routine protypes
**
** If all initialization routines are called, it is assumed they are called
** in the following order:
**   1. sst1InitMapBoard();
**   2. sst1InitRegisters();
**   3. sst1InitGamma();
**   4. sst1InitVideo();
**
** sst1InitShutdown() is called at the end of an application to turn off
**  the SST-1 graphics subsystem
**
*/

/* sst1init.h assumes "glide.h" and "sst.h" are already included */

/* Init code debug print routine */
#ifdef INIT_DOS /* Glide version... */
#define INIT_OUTPUT
#define INIT_PRINTF(a) sst1InitPrintf a
#define INIT_INFO(A)
#endif

#ifndef DIRECTX
#undef GETENV
#undef ATOI
#undef ATOF
#undef SSCANF
#undef POW
#define GETENV(A) sst1InitGetenv A
#define ATOI(A) atoi(A)
#define ATOF(A) atof(A)
#define SSCANF( A, B, C ) sscanf( A, B, C )
#define POW( A, B ) pow( A, B )
#define FTOL( X ) ((FxU32)(X))

/* Video resolution declarations */
#include "sst1vid.h"

#else /* DIRECTX */
#include "ddglobal.h"
#pragma optimize ("",off)   /* ddglobal.h tuns this on for retail builds */
#undef INIT_PRINTF
#undef INIT_INFO
#undef GETENV
#undef ATOI
#undef ATOF
#undef FTOL
#undef ITOF_INV
#undef SSCANF
#undef POW
/* #define INIT_PRINTF(a) */
#ifdef FXTRACE
  #define INIT_PRINTF DDPRINTF
#else
  #define INIT_PRINTF 1 ? (void) 0 : (void)
#endif
#define INIT_INFO(A)
#define GETENV(A)  ddgetenv(A)
#define ATOI(A) ddatoi(A)
#define ATOF(A) ddatof(A)
#define FTOL(A) ddftol(A)
#define ITOF_INV(A) dd_itof_inv(A)
#define SSCANF( A, B, C ) ddsscanf( A, B, C )
#define POW( A, B ) ddpow( A, B )

#endif /* DIRECTX */

/* Defines to writing to/reading from SST-1 */
#if 0
#define IGET(A)    A
#define ISET(A,D)  A = (D)
#else
#define IGET(A)    sst1InitRead32((FxU32 *) &(A))
#define ISET(A,D)  sst1InitWrite32((FxU32 *) &(A), D)  
#endif

/*
**  P6 Fence
** 
**  Here's the stuff to do P6 Fencing.  This is required for the
**  certain things on the P6
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
  FxU32 p6FenceVar;
#else
  extern FxU32 p6FenceVar;
#endif

#if defined(__WATCOMC__)
void
p6Fence(void);
#pragma aux p6Fence = \
  "xchg eax, p6FenceVar" \
  modify [eax];

#define P6FENCE p6Fence()

#elif defined(__MSC__)

#define P6FENCE {_asm xchg eax, p6FenceVar}

#else
#error "P6 Fencing in-line assembler code needs to be added for this compiler"
#endif  

#ifdef __cplusplus
}
#endif

#ifndef _FXPCI_H_
#include <fxpci.h>
#endif

#define MAX_NUM_TMUS 3

/* Following defines need to go in "sst.h" eventually */
/*--------- SST PCI Configuration Command bits --------------*/
#define SST_PCIMEM_ACCESS_EN               BIT(1)

/*------- SST PCI Configuration Register defaults -----------*/
#define SST_PCI_INIT_ENABLE_DEFAULT        0x0
#define SST_PCI_BUS_SNOOP_DEFAULT          0x0

/*--- SST PCI Init Enable Configuration Register defaults ---*/
#define SST_SLI_OWNPCI                     SST_SCANLINE_SLV_OWNPCI
#define SST_SLI_MASTER_OWNPCI              0x0
#define SST_SLI_SLAVE_OWNPCI               SST_SCANLINE_SLV_OWNPCI

/*----------------- SST fbiinit0 bits -----------------------*/
#define SST_FBIINIT0_DEFAULT               0x00000410
#define SST_GRX_RESET                      BIT(1)
#define SST_PCI_FIFO_RESET                 BIT(2)
#define SST_EN_ENDIAN_SWAPPING             BIT(3)

/*----------------- SST fbiinit1 bits -----------------------*/
#define SST_FBIINIT1_DEFAULT               0x00201102
#define SST_SLI_DETECT                     BIT(2)

/*----------------- SST fbiinit2 bits -----------------------*/
#define SST_FBIINIT2_DEFAULT               0x80000040
#define SST_DRAM_REFRESH_16MS              (0x30 << SST_DRAM_REFRESH_CNTR_SHIFT)

/*----------------- SST fbiinit3 bits -----------------------*/
#define SST_FBIINIT3_DEFAULT               0x001E4000
#define SST_TEXMAP_DISABLE                 BIT(6)
#define SST_FBI_MEM_TYPE_SHIFT             8
#define SST_FBI_MEM_TYPE                   (0x7<<SST_FBI_MEM_TYPE_SHIFT)
#define SST_FBI_VGA_PASS_POWERON           BIT(12)
#define SST_FT_CLK_DEL_ADJ_SHIFT           13
#define SST_FT_CLK_DEL_ADJ                 (0xF<<SST_FT_CLK_DEL_ADJ_SHIFT)
#define SST_TF_FIFO_THRESH_SHIFT           17
#define SST_TF_FIFO_THRESH                 (0x1F<<SST_TF_FIFO_THRESH_SHIFT)

/*----------------- SST fbiinit4 bits -----------------------*/
#define SST_FBIINIT4_DEFAULT               0x00000001
#define SST_PCI_RDWS_1                     0x0
#define SST_PCI_RDWS_2                     BIT(0)
#define SST_EN_LFB_RDAHEAD                 BIT(1)
#define SST_MEM_FIFO_LWM_SHIFT             2
#define SST_MEM_FIFO_LWM                   (0x3F<<SST_MEM_FIFO_LWM_SHIFT)
#define SST_MEM_FIFO_ROW_BASE_SHIFT        8
#define SST_MEM_FIFO_ROW_BASE              (0x3FF<<SST_MEM_FIFO_ROW_BASE_SHIFT)
#define SST_MEM_FIFO_ROW_ROLL_SHIFT        18
#define SST_MEM_FIFO_ROW_ROLL              (0x3FF<<SST_MEM_FIFO_ROW_ROLL_SHIFT)

/*----------------- SST trexInit0 bits -----------------------*/
#define SST_EN_TEX_MEM_REFRESH             BIT(0)
#define SST_TEX_MEM_REFRESH_SHIFT          1
#define SST_TEX_MEM_REFRESH                (0x1FF<<SST_TEX_MEM_REFRESH_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_SHIFT        10
#define SST_TEX_MEM_PAGE_SIZE_8BITS        (0x0<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_9BITS        (0x1<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_10BITS       (0x2<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_SHIFT   12
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT17   (0x0<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT18   (0x1<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_EN_TEX_MEM_SECOND_RAS          BIT(14)
#define SST_TEX_MEM_TYPE_SHIFT             15
#define SST_TEX_MEM_TYPE_EDO               (0x0<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_TYPE_SYNC              (0x1<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_DATA_SIZE_16BIT        0x0
#define SST_TEX_MEM_DATA_SIZE_8BIT         BIT(18)
#define SST_TEX_MEM_DO_EXTRA_CAS           BIT(19)
#define SST_TEX_MEM2                       BIT(20)

#define SST_TREXINIT0_DEFAULT \
  ( (SST_EN_TEX_MEM_REFRESH)  \
  | (0x020 << SST_TEX_MEM_REFRESH_SHIFT) \
  | (SST_TEX_MEM_PAGE_SIZE_9BITS) \
  | (SST_TEX_MEM_SECOND_RAS_BIT_BIT18) \
  | (SST_EN_TEX_MEM_SECOND_RAS) \
  | (SST_TEX_MEM_TYPE_EDO) \
  | (SST_TEX_MEM_DATA_SIZE_16BIT) \
  | (0 & SST_TEX_MEM_DO_EXTRA_CAS) \
  | (0 & SST_TEX_MEM2)  )

#define SST_TREX0INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX1INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX2INIT0_DEFAULT             SST_TREXINIT0_DEFAULT

/*----------------- SST trexInit1 bits -----------------------*/
#define SST_TEX_SCANLINE_INTERLEAVE_MASTER 0x0
#define SST_TEX_SCANLINE_INTERLEAVE_SLAVE  BIT(0)
#define SST_EN_TEX_SCANLINE_INTERLEAVE     BIT(1)
#define SST_TEX_FT_FIFO_SIL_SHIFT          2
#define SST_TEX_FT_FIFO_SIL                (0x1F<<SST_TEX_FT_FIFO_SIL_SHIFT)
#define SST_TEX_TT_FIFO_SIL_SHIFT          7
#define SST_TEX_TT_FIFO_SIL                (0xF<<SST_TEX_TT_FIFO_SIL_SHIFT)
#define SST_TEX_TF_CLK_DEL_ADJ_SHIFT       12
#define SST_TEX_TF_CLK_DEL_ADJ             (0xF<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT)
#define SST_TEX_RG_TTCII_INH               BIT(16)
#define SST_TEX_USE_RG_TTCII_INH           BIT(17)
#define SST_TEX_SEND_CONFIG                BIT(18)
#define SST_TEX_RESET_FIFO                 BIT(19)
#define SST_TEX_RESET_GRX                  BIT(20)
#define SST_TEX_PALETTE_DEL_SHIFT          21
#define SST_TEX_PALETTE_DEL                (0x3<<SST_TEX_PALETTE_DEL_SHIFT)
#define SST_TEX_SEND_CONFIG_SEL_SHIFT      23
#define SST_TEX_SEND_CONFIG_SEL            (0x7<<SST_TEX_SEND_CONFIG_SEL_SHIFT)

/* After things stabilize, the fifo stall inputs levels should be backed off
   from the max. conservative values that are being used now for better
        performance.
   SST_TEX_FT_FIFO_SIL =  ??
   SST_TEX_TT_FIFO_SIL =  ??  (effects multi-trex only)
        */

/* for trex ver. 1 bringup, SST_TEX_PALETTE_DEL should be set to it's max
   (== 3) for <50 MHz bringup */

/* TF Clock delays for (FBI Revision, TMU Revision) */
#define SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R10  0xf
#define SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R11  0x6
#define SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R20  0xf
#define SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R21  0x5

/* FT Clock delays for (FBI Revision, TMU Revision) */
#define SST_FT_CLK_DEL_ADJ_DEFAULT_R10  0xa
#define SST_FT_CLK_DEL_ADJ_DEFAULT_R11  0xa
#define SST_FT_CLK_DEL_ADJ_DEFAULT_R20  0x5
#define SST_FT_CLK_DEL_ADJ_DEFAULT_R21  0x5

#define SST_TREXINIT1_DEFAULT \
  ( (SST_TEX_SCANLINE_INTERLEAVE_MASTER) \
  | (0 & SST_EN_TEX_SCANLINE_INTERLEAVE) \
  | (0x8 << SST_TEX_FT_FIFO_SIL_SHIFT) \
  | (0x8 << SST_TEX_TT_FIFO_SIL_SHIFT) \
  | (0xf << SST_TEX_TF_CLK_DEL_ADJ_SHIFT) \
  | (0 & SST_TEX_RG_TTCII_INH) \
  | (0 & SST_TEX_USE_RG_TTCII_INH) \
  | (0 & SST_TEX_SEND_CONFIG) \
  | (0 & SST_TEX_RESET_FIFO) \
  | (0 & SST_TEX_RESET_GRX) \
  | (0 << SST_TEX_PALETTE_DEL_SHIFT) \
  | (0 << SST_TEX_SEND_CONFIG_SEL_SHIFT) )

#define SST_TREX0INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX1INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX2INIT1_DEFAULT              SST_TREXINIT1_DEFAULT

/*----------------- SST clutData bits -----------------------*/
#define SST_CLUTDATA_INDEX_SHIFT           24
#define SST_CLUTDATA_RED_SHIFT             16
#define SST_CLUTDATA_GREEN_SHIFT           8
#define SST_CLUTDATA_BLUE_SHIFT            0

/*----------------- SST video setup shifts ------------------*/
#define SST_VIDEO_HSYNC_OFF_SHIFT          16
#define SST_VIDEO_HSYNC_ON_SHIFT           0
#define SST_VIDEO_VSYNC_OFF_SHIFT          16
#define SST_VIDEO_VSYNC_ON_SHIFT           0
#define SST_VIDEO_HBACKPORCH_SHIFT         0
#define SST_VIDEO_VBACKPORCH_SHIFT         16
#define SST_VIDEO_XDIM_SHIFT               0
#define SST_VIDEO_YDIM_SHIFT               16

/*----------------- SST dacData constants -------------------*/
#define SST_DACREG_WMA                     0x0
#define SST_DACREG_LUT                     0x1
#define SST_DACREG_RMR                     0x2
#define SST_DACREG_RMA                     0x3
#define SST_DACREG_ICS_PLLADDR_WR          0x4 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_RD          0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_DATA        0x5 /* ICS only */
#define SST_DACREG_ICS_CMD                 0x6 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_16BPP     0x50 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_24BPP     0x70 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK0       0x0 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1       0x1 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7       0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1_DEFAULT 0x55 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7_DEFAULT 0x71 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK0       0xa /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1       0xb /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1_DEFAULT 0x79 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_CTRL        0xe /* ICS only */
#define SST_DACREG_ICS_PLLCTRL_CLK1SEL     BIT(4)
#define SST_DACREG_ICS_PLLCTRL_CLK0SEL     BIT(5)
#define SST_DACREG_ICS_PLLCTRL_CLK0FREQ    0x7
#define SST_DACREG_INDEXADDR               SST_DACREG_WMA
#define SST_DACREG_INDEXDATA               SST_DACREG_RMR
#define SST_DACREG_INDEX_RMR               0x0
#define SST_DACREG_INDEX_CR0               0x1
#define SST_DACREG_INDEX_MIR               0x2
#define SST_DACREG_INDEX_MIR_ATT_DEFAULT   0x84   /* AT&T */
#define SST_DACREG_INDEX_MIR_TI_DEFAULT    0x97   /* TI */
#define SST_DACREG_INDEX_DIR               0x3
#define SST_DACREG_INDEX_DIR_ATT_DEFAULT   0x9    /* AT&T */
#define SST_DACREG_INDEX_DIR_TI_DEFAULT    0x9    /* TI */
#define SST_DACREG_INDEX_TST               0x4
#define SST_DACREG_INDEX_CR1               0x5
#define SST_DACREG_INDEX_CC                0x6
#define SST_DACREG_INDEX_AA0               0xff  /* can't access */
#define SST_DACREG_INDEX_AA1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB0               0xff  /* can't access */
#define SST_DACREG_INDEX_AB1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB2               0xff  /* can't access */
#define SST_DACREG_INDEX_AC0               0x48
#define SST_DACREG_INDEX_AC1               0x49
#define SST_DACREG_INDEX_AC2               0x4a
#define SST_DACREG_INDEX_AD0               0x4c
#define SST_DACREG_INDEX_AD1               0x4d
#define SST_DACREG_INDEX_AD2               0x4e
#define SST_DACREG_INDEX_BA0               0xff  /* can't access */
#define SST_DACREG_INDEX_BA1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB0               0xff  /* can't access */
#define SST_DACREG_INDEX_BB1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB2               0xff  /* can't access */
#define SST_DACREG_INDEX_BC0               0xff  /* can't access */
#define SST_DACREG_INDEX_BC1               0xff  /* can't access */
#define SST_DACREG_INDEX_BC2               0xff  /* can't access */
#define SST_DACREG_INDEX_BD0               0x6c
#define SST_DACREG_INDEX_BD1               0x6d
#define SST_DACREG_INDEX_BD2               0x6e

#define SST_DACREG_CR0_INDEXED_ADDRESSING  BIT(0)
#define SST_DACREG_CR0_8BITDAC             BIT(1)
#define SST_DACREG_CR0_SLEEP               BIT(3)
#define SST_DACREG_CR0_COLOR_MODE_SHIFT    4
#define SST_DACREG_CR0_COLOR_MODE         (0xF<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_16BPP   (0x3<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_24BPP   (0x5<<SST_DACREG_CR0_COLOR_MODE_SHIFT)

#define SST_DACREG_CR1_BLANK_PEDASTAL_EN  BIT(4)

#define SST_DACREG_CC_BCLK_SEL_SHIFT       0
#define SST_DACREG_CC_BCLK_SELECT_BD       BIT(3)
#define SST_DACREG_CC_ACLK_SEL_SHIFT       4
#define SST_DACREG_CC_ACLK_SELECT_AD       BIT(7)

#define SST_DACREG_CLKREG_MSHIFT           0
#define SST_DACREG_CLKREG_PSHIFT           6
#define SST_DACREG_CLKREG_NSHIFT           0
#define SST_DACREG_CLKREG_LSHIFT           4
#define SST_DACREG_CLKREG_IBSHIFT          0

#define SST_FBI_DACTYPE_ATT                0
#define SST_FBI_DACTYPE_ICS                1
#define SST_FBI_DACTYPE_TI                 2

/* Definitions for parsing voodoo.ini file */
#define DACRDWR_TYPE_WR                    0
#define DACRDWR_TYPE_RDMODWR               1
#define DACRDWR_TYPE_RDNOCHECK             2
#define DACRDWR_TYPE_RDCHECK               3
#define DACRDWR_TYPE_RDPUSH                4
#define DACRDWR_TYPE_WRMOD_POP             5

/* Other useful defines */
#define PCICFG_WR(ADDR, DATA)                                         \
    n = DATA;                                                         \
    if(pciSetConfigData(ADDR, sst1InitDeviceNumber, &n) == FXFALSE)   \
        return(FXFALSE)
#define PCICFG_RD(ADDR, DATA)                                          \
    if(pciGetConfigData(ADDR, sst1InitDeviceNumber, &DATA) == FXFALSE) \
        return(FXFALSE)
#define DAC_INDEXWRADDR(ADDR)                                         \
    sst1InitDacWr(sstbase, SST_DACREG_INDEXADDR, ADDR)
#define DAC_INDEXWR(DATA)                                             \
    sst1InitDacWr(sstbase, SST_DACREG_INDEXDATA, (DATA))
#define DAC_INDEXRD()                                                 \
    sst1InitDacRd(sstbase, SST_DACREG_INDEXDATA)
#define SLI_DRIVEOK()                                                 \
      (!(sst1CurrentBoard->numBoardInsystem & 0x1) ||                 \
       ((sst1CurrentBoard->numBoardInsystem & 0x1) &&                 \
        (sst1InitSliDetect(sstbase) == 0)))

/*-----------------------------------------------------------*/

/*
** SST-1 Initialization typedefs
**
*/

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    float freq;
    FxU32 clkTiming_M;
    FxU32 clkTiming_P;
    FxU32 clkTiming_N;
    FxU32 clkTiming_L;
    FxU32 clkTiming_IB;
} sst1ClkTimingStruct;

typedef struct {
    FxU32 hSyncOn;
    FxU32 hSyncOff;
    FxU32 vSyncOn;
    FxU32 vSyncOff;
    FxU32 hBackPorch;
    FxU32 vBackPorch;
    FxU32 xDimension;
    FxU32 yDimension;
    FxU32 memOffset;
    FxU32 memFifoEntries_1MB;
    FxU32 memFifoEntries_2MB;
    FxU32 memFifoEntries_4MB;
    FxU32 tilesInX_Over2;
    FxU32 vFifoThreshold;
    FxBool video16BPPIsOK;
    FxBool video24BPPIsOK;
    float clkFreq16bpp;
    float clkFreq24bpp;
} sst1VideoTimingStruct;

typedef struct {
    FxU32 *virtAddr;        /* virtual memory base address */
    FxU32 physAddr;         /* physical memory base address */
    FxU32 deviceNumber;     /* PCI device number */
    FxU32 vendorID;         /* PCI vendor ID */
    FxU32 deviceID;         /* PCI device ID */
    FxU32 fbiRevision;      /* FBI revision number */
    FxU32 fbiBoardID;       /* FBI board ID (poweron strapping bits) */
    FxU32 fbiVideo16BPP;    /* FBI video display mode */
    FxU32 fbiVideoWidth;    /* FBI video display X-resolution */
    FxU32 fbiVideoHeight;   /* FBI video display Y-resolution */
    FxU32 fbiMemoryFifoEn;  /* FBI memory fifo enabled */
    FxU32 sstSliDetect;     /* Scanline interleave detected */
    FxU32 tmuRevision;      /* TMU revision number (for all TMUs) */
    FxU32 numberTmus;       /* number of TMUs installed */
    FxU32 tmuConfig;        /* TMU configuration bits */
    FxU32 fbiMemSize;       /* FBI frame buffer memory (in MBytes) */
    FxU32 tmuMemSize[MAX_NUM_TMUS]; /* TMU texture memory (in MBytes) */

    /* These cannot be read from the hardware, so we shadow them here */
    FxU32 tmuInit0[MAX_NUM_TMUS];
    FxU32 tmuInit1[MAX_NUM_TMUS];

    /* Misc */
    FxU32 initGrxClkDone;
    FxU32 vgaPassthruDisable;
    FxU32 vgaPassthruEnable;
    FxU32 fbiDacType;
    FxU32 memFifoStatusLwm;
    FxU32 numBoardInsystem;
    FxU32 grxClkFreq;
    FxU32 fbiMemSpeed;
    FxU32 tmuMemSpeed;
    FxU32 tmuClkFixed;
    FxU32 fbiConfig;        /* FBI strapping pins */
    FxU32 fbiVideoRefresh;  /* FBI video refresh rate */
    FxU32 fbiRegulatorPresent;  /* FBI voltage regulator detected */
    FxBool fbiTripleBufferingEnabled;  /* Specify video triple buffering */
} sst1DeviceInfoStruct;

typedef struct {
    unsigned char type;
    unsigned char addr;
    FxU32 data;
    FxU32 mask;
    void *nextRdWr;
} sst1InitDacRdWrStruct;

typedef struct {
    FxU32 width;
    FxU32 height;
    FxU32 refresh;
    FxU32 video16BPP;
    sst1InitDacRdWrStruct *setVideoRdWr;
    void *nextSetVideo;
} sst1InitDacSetVideoStruct;

typedef struct {
    FxU32 frequency;
    sst1InitDacRdWrStruct *setMemClkRdWr;
    void *nextSetMemClk;
} sst1InitDacSetMemClkStruct;

typedef struct {
    FxU32 video16BPP;
    sst1InitDacRdWrStruct *setVideoModeRdWr;
    void *nextSetVideoMode;
} sst1InitDacSetVideoModeStruct;

typedef struct {
    char dacManufacturer[100];
    char dacDevice[100];
    sst1InitDacRdWrStruct *detect;
    sst1InitDacSetVideoStruct *setVideo;
    sst1InitDacSetMemClkStruct *setMemClk;
    sst1InitDacSetVideoModeStruct *setVideoMode;
    void *nextDac;
} sst1InitDacStruct;

typedef struct {
    char envVariable[100];
    char envValue[256];
    void *nextVar;
} sst1InitEnvVarStruct;

FX_ENTRY FxU32 * FX_CALL sst1InitMapBoard(FxU32);
FX_ENTRY FxU32 FX_CALL sst1InitNumBoardsInSystem(void);
FX_ENTRY FxBool FX_CALL sst1InitRegisters(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGamma(FxU32 *, double);
FX_ENTRY FxBool FX_CALL sst1InitGammaRGB(FxU32 *, double, double, double);
FX_ENTRY FxBool FX_CALL sst1InitGammaTable(FxU32 *, FxU32, FxU32 *, FxU32 *, FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitVideo(FxU32 *, GrScreenResolution_t,
  GrScreenRefresh_t, sst1VideoTimingStruct *);
FX_ENTRY sst1VideoTimingStruct * FX_CALL sst1InitFindVideoTimingStruct(GrScreenResolution_t screenResolution, 
                                                     GrScreenRefresh_t screenRefresh);
FX_ENTRY FxBool FX_CALL sst1InitShutdown(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitShutdownSli(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitSli(FxU32 *, FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetDeviceInfo(FxU32 *, sst1DeviceInfoStruct *);

/* Miscellaneous routines */
FX_ENTRY void FX_CALL sst1InitWrite32(FxU32 *, FxU32);
FX_ENTRY FxU32 FX_CALL sst1InitRead32(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdle(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdleFBI(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitIdleFBINoNOP(FxU32 *);
FX_ENTRY FxU32  FX_CALL sst1InitReturnStatus(FxU32 *);
FX_ENTRY FxU32 FX_CALL sst1InitDacRd(FxU32 *, FxU32);
FX_ENTRY void FX_CALL sst1InitDacWr(FxU32 *, FxU32, FxU32);
FxBool sst1InitExecuteDacRdWr(FxU32 *, sst1InitDacRdWrStruct *);
FX_ENTRY void FX_CALL sst1InitSetResolution(FxU32 *, sst1VideoTimingStruct *,
    FxU32);
FX_ENTRY FxBool FX_CALL sst1InitDacIndexedEnable(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitGrxClk(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitCalcGrxClk(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitDacDetect(FxU32 *);
FxBool sst1InitDacDetectATT(FxU32 *);
FxBool sst1InitDacDetectTI(FxU32 *);
FxBool sst1InitDacDetectICS(FxU32 *);
FxBool sst1InitDacDetectINI(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitSetGrxClk(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitComputeClkParams(float, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkATT(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkICS(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetGrxClkINI(FxU32 *, sst1ClkTimingStruct *);
FX_ENTRY FxBool FX_CALL sst1InitSetVidClk(FxU32 *, float);
FxBool sst1InitSetVidClkATT(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetVidClkICS(FxU32 *, sst1ClkTimingStruct *);
FxBool sst1InitSetVidClkINI(FxU32 *, FxU32, FxU32, FxU32, FxU32);
FxBool sst1InitSetVidMode(FxU32 *, FxU32);
FxBool sst1InitSetVidModeATT(FxU32 *, FxU32);
FxBool sst1InitSetVidModeICS(FxU32 *, FxU32);
FxBool sst1InitSetVidModeINI(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitCheckBoard(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetFbiInfo(FxU32 *, sst1DeviceInfoStruct *);
FX_ENTRY FxBool FX_CALL sst1InitGetTmuInfo(FxU32 *, sst1DeviceInfoStruct *);
FX_ENTRY void FX_CALL sst1InitRenderingRegisters(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitGetTmuMemory(FxU32 *sstbase,
    sst1DeviceInfoStruct *info, FxU32 tmu, FxU32 *TmuMemorySize);
FX_ENTRY FxBool FX_CALL sst1InitClearSwapPending(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitVgaPassCtrl(FxU32 *, FxU32);
FxBool sst1InitResetTmus(FxU32 *);
FX_ENTRY FxU32 FX_CALL sst1InitSliDetect(FxU32 *);
FX_ENTRY FxBool FX_CALL sst1InitSliPciOwner(FxU32 *, FxU32);
FX_ENTRY FxBool FX_CALL sst1InitVoodooFile(void);
FX_ENTRY char * FX_CALL sst1InitGetenv(char *);
FX_ENTRY FxU32 * FX_CALL sst1InitGetBaseAddr(FxU32);
FxBool sst1InitFillDeviceInfo(FxU32 *, sst1DeviceInfoStruct *);
void sst1InitIdleLoop(FxU32 *);
void sst1InitClearBoardInfo(void);
FX_ENTRY FxBool FX_CALL sst1InitSetTripleBuffering(FxU32 *, FxBool);
FX_ENTRY void FX_CALL sst1InitCachingOn(void);

#ifdef __cplusplus
}
#endif

/* Info/Print routines */
#ifdef INIT_OUTPUT

#ifdef __cplusplus
extern "C" {
#endif
FX_ENTRY void FX_CALL sst1InitPrintf(const char *, ...);
#ifdef __cplusplus
}
#endif

#ifndef _FILE_DEFINED
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
FILE *sst1InitMsgFile = stdout;
#else
extern FILE *sst1InitMsgFile;
#endif

#ifdef __cplusplus
}
#endif

#endif

/* Maximum number of SST-1 boards supported in system */
#define SST1INIT_MAX_BOARDS 16

/* Maximum number of read pushes in "voodoo.ini" file */
#define DACRDWR_MAX_PUSH    16

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SST1INIT_ALLOCATE
  static char headersIdent[] = "@#%InitHeaders $Revision: 22 $";
  FxBool sst1InitUseVoodooFile = FXFALSE;
  sst1InitEnvVarStruct *envVarsBase = (sst1InitEnvVarStruct *) NULL;
  sst1InitDacStruct *dacStructBase = (sst1InitDacStruct *) NULL;
  sst1InitDacStruct *iniDac = (sst1InitDacStruct *) NULL;
  sst1InitDacSetVideoStruct *iniVideo = (sst1InitDacSetVideoStruct *) NULL;
  sst1InitDacSetMemClkStruct *iniMemClk = (sst1InitDacSetMemClkStruct *) NULL;
  FxU32 iniStack[DACRDWR_MAX_PUSH];
  int iniStackPtr = 0;
  sst1DeviceInfoStruct *sst1CurrentBoard;
  FxU32 sst1InitDeviceNumber;
  sst1DeviceInfoStruct sst1BoardInfo[SST1INIT_MAX_BOARDS];
  FxU32 boardsInSystem;
  FxU32 sst1InitSliEnabled = 0;
  FxU32 *sst1InitSliSlaveVirtAddr;
  const PciRegister SST1_PCI_INIT_ENABLE   = { 0x40, 4, READ_WRITE };
  const PciRegister SST1_PCI_BUS_SNOOP0    = { 0x44, 4, WRITE_ONLY };
  const PciRegister SST1_PCI_BUS_SNOOP1    = { 0x48, 4, WRITE_ONLY };
  const PciRegister SST1_PCI_CFG_STATUS    = { 0x4c, 4, READ_ONLY };
  const PciRegister SST1_PCI_VCLK_ENABLE   = { 0xc0, 4, WRITE_ONLY };
  const PciRegister SST1_PCI_VCLK_DISABLE  = { 0xe0, 4, WRITE_ONLY };

#else
  extern FxBool sst1InitUseVoodooFile;
  extern sst1InitEnvVarStruct *envVarsBase;
  extern sst1InitDacStruct *dacStructBase;
  extern sst1InitDacStruct *iniDac;
  extern sst1InitDacSetVideoStruct *iniVideo;
  extern sst1InitDacSetMemClkStruct *iniMemClk;
  extern FxU32 iniStack[];
  extern int iniStackPtr;
  extern sst1DeviceInfoStruct *sst1CurrentBoard;
  extern FxU32 sst1InitDeviceNumber;
  extern sst1DeviceInfoStruct sst1BoardInfo[SST1INIT_MAX_BOARDS];
  extern FxU32 boardsInSystem;
  extern FxU32 sst1InitSliEnabled;
  extern FxU32 *sst1InitSliSlaveVirtAddr;
  extern PciRegister SST1_PCI_INIT_ENABLE;
  extern PciRegister SST1_PCI_BUS_SNOOP0;
  extern PciRegister SST1_PCI_BUS_SNOOP1;
  extern PciRegister SST1_PCI_CFG_STATUS;
  extern PciRegister SST1_PCI_VCLK_ENABLE;
  extern PciRegister SST1_PCI_VCLK_DISABLE;
#endif /* SST1INIT_ALLOCATE */

#ifdef __cplusplus
}
#endif

#ifdef SST1INIT_VIDEO_ALLOCATE
/* SST1INIT_VIDEO_ALLOCATE is only #defined in video.c
  
   Define useful clock and video timings
   Clocks generated are follows:
     Clock Freq. (MHz) =
       [14.318 * (clkTiming_M+2)] / [(clkTiming_N+2) * (2^clkTiming_P)]
  
   Solving for clkTiming_M yields:
     clkTiming_M =
       [ [(Clock Freq (Mhz)) * (clkTiming_N+2) * (2^clkTiming_P)] / 14.318 ] - 2
     
   NOTE: [14.318 * (clkTiming_M+2)] / (clkTiming_N+2) should be between
    120 and 240
   NOTE: Max. M is 127
   NOTE: Max. N is 31
   NOTE: Max. P is 3
   NOTE: Max. L is 15
   NOTE: Max. IB is 15
*/

/* 512x256@60 only syncs to Arcade-style monitors */
sst1VideoTimingStruct SST_VREZ_512X256_60 = {
    41,        /* hSyncOn */
    626,       /* hSyncOff */
    4,         /* vSyncOn */
    286,       /* vSyncOff */
    65,        /* hBackPorch */
    24,        /* vBackPorch */
    512,       /* xDimension */
    256,       /* yDimension */
    64,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXFALSE,   /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    23.334F,   /* clkFreq16bpp */
    23.334F    /* clkFreq24bpp */
};

/* 512x384@60 only syncs to Arcade-style monitors */
sst1VideoTimingStruct SST_VREZ_512X384_60 = {
    23,        /* hSyncOn */
    640,       /* hSyncOff */
    3,         /* vSyncOn */
    411,       /* vSyncOff */
    90,        /* hBackPorch */
    24,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    96,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXFALSE,   /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    33.0F,     /* clkFreq16bpp */
    33.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_512X384_72 = {
    51,        /* hSyncOn */
    591,       /* hSyncOff */
    3,         /* vSyncOn */
    430,       /* vSyncOff */
    55,        /* hBackPorch */
    25,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    96,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    20.093F,   /* clkFreq16bpp .. 72 Hz refresh rate */
    40.186F    /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_512X384_75 = {
    44,        /* hSyncOn */
    616,       /* hSyncOff */
    3,         /* vSyncOn */
    415,       /* vSyncOff */
    55,        /* hBackPorch */
    25,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    96,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    20.8F,     /* clkFreq16bpp .. 72 Hz refresh rate */
    41.6F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_512X384_85 = {
    55,        /* hSyncOn */
    599,       /* hSyncOff */
    3,         /* vSyncOn */
    401,       /* vSyncOff */
    70,        /* hBackPorch */
    16,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    96,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    22.527F,   /* clkFreq16bpp .. 72 Hz refresh rate */
    45.054F    /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_512X384_120 = {
    25,        /* hSyncOn */
    650,       /* hSyncOff */
    3,         /* vSyncOn */
    409,       /* vSyncOff */
    110,       /* hBackPorch */
    25,        /* vBackPorch */
    512,       /* xDimension */
    384,       /* yDimension */
    96,        /* memOffset */
    0x410,     /* memFifoEntries_1MB  ... 32256 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_2MB  ... 57344 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    8,         /* tilesInX_Over2 */
    20,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    33.5F,     /* clkFreq16bpp .. 72 Hz refresh rate */
    67.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_70 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    447,       /* vSyncOff */
    48,        /* hBackPorch */
    35,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... memory fifo not supported */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    21,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_75 = {
    99,        /* hSyncOn */
    733,       /* hSyncOff */
    3,         /* vSyncOn */
    429,       /* vSyncOff */
    52,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... memory fifo not supported */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    19,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    27.0F,     /* clkFreq16bpp */
    54.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_85 = {
    63,        /* hSyncOn */
    767,       /* hSyncOff */
    3,         /* vSyncOn */
    442,       /* vSyncOff */
    94,        /* hBackPorch */
    41,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... memory fifo not supported */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    18,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X400_120 = {
    67,        /* hSyncOn */
    798,       /* hSyncOff */
    3,         /* vSyncOn */
    424,       /* vSyncOff */
    94,        /* hBackPorch */
    16,        /* vBackPorch */
    640,       /* xDimension */
    400,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... memory fifo not supported */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    18,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    44.47F,    /* clkFreq16bpp */
    88.94F     /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_60 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_75 = {
    63,        /* hSyncOn */
    775,       /* hSyncOff */
    3,         /* vSyncOn */
    497,       /* vSyncOff */
    118,       /* hBackPorch */
    16,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    21,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    31.5F,     /* clkFreq16bpp */
    63.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_85 = {
    55,        /* hSyncOn */
    776,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    78,        /* hBackPorch */
    25,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    21,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    36.0F,     /* clkFreq16bpp */
    72.0F      /* clkFreq24bpp */
};

/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_640X480_120 = {
    45,        /* hSyncOn */
    785,       /* hSyncOff */
    3,         /* vSyncOn */
    506,       /* vSyncOff */
    100,       /* hBackPorch */
    18,        /* vBackPorch */
    640,       /* xDimension */
    480,       /* yDimension */
    150,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    17,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    50.82F,    /* clkFreq16bpp */
    101.64F    /* clkFreq24bpp */
};

#ifdef H3D
sst1VideoTimingStruct SST_VREZ_640X502_60 = {
    96,        /* hSyncOn */
    704,       /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    38,        /* hBackPorch */
    15,        /* vBackPorch */
    640,       /* xDimension */
    502,       /* yDimension */
    //    150,       /* memOffset */
    160,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x610,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    10,        /* tilesInX_Over2 */
    23,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    25.175F,   /* clkFreq16bpp */
    50.350F    /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_800X620_60 = {
    127,       /* hSyncOn */
    927,       /* hSyncOff */
    4,         /* vSyncOn */
    624,       /* vSyncOff */
    86,        /* hBackPorch */
    13,        /* vBackPorch */
    800,       /* xDimension */
    620,       /* yDimension */
    254,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x0,       /* memFifoEntries_2MB  ... 9152 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    13,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    40.0F,     /* clkFreq16bpp */
    80.0F      /* clkFreq24bpp */
};

#endif

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_800X600_60 = {
    127,       /* hSyncOn */
    927,       /* hSyncOff */
    4,         /* vSyncOn */
    624,       /* vSyncOff */
    86,        /* hBackPorch */
    23,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    247,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x6e2,     /* memFifoEntries_2MB  ... 9152 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    13,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    40.0F,     /* clkFreq16bpp */
    80.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_800X600_75 = {
    79,        /* hSyncOn */
    975,       /* hSyncOff */
    3,         /* vSyncOn */
    622,       /* vSyncOff */
    158,       /* hBackPorch */
    21,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    247,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x6e2,     /* memFifoEntries_2MB  ... 9152 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    13,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    49.5F,     /* clkFreq16bpp */
    99.0F      /* clkFreq24bpp */
};

/* VESA Standard */
/* Verified 10/21/96 */
sst1VideoTimingStruct SST_VREZ_800X600_85 = {
    63,        /* hSyncOn */
    983,       /* hSyncOff */
    3,         /* vSyncOn */
    628,       /* vSyncOff */
    150,       /* hBackPorch */
    27,        /* vBackPorch */
    800,       /* xDimension */
    600,       /* yDimension */
    247,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x6e2,     /* memFifoEntries_2MB  ... 9152 entries in memory fifo (no Z) */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    13,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.25F,    /* clkFreq16bpp */
    112.5F     /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_856X480_60 = {
    136,       /* hSyncOn */
    1008,      /* hSyncOff */
    2,         /* vSyncOn */
    523,       /* vSyncOff */
    100,       /* hBackPorch */
    23,        /* vBackPorch */
    856,       /* xDimension */
    480,       /* yDimension */
    210,       /* memOffset */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x430,     /* memFifoEntries_2MB  ... 31232 entries in memory fifo */
    0x100,     /* memFifoEntries_4MB  ... 57344 entries in memory fifo */
    14,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    36.0F,     /* clkFreq16bpp */
    72.0F      /* clkFreq24bpp */
};

sst1VideoTimingStruct SST_VREZ_960X720_60 = {
    200,       /* hSyncOn */
    1013,      /* hSyncOff */
    4,         /* vSyncOn */
    750,       /* vSyncOff */
    30,        /* hBackPorch */
    23,        /* vBackPorch */
    960,       /* xDimension */
    720,       /* yDimension */
    338,       /* memOffset (really 337.5) */
    0x0,       /* memFifoEntries_1MB  ... unsupported resolution */
    0x0,       /* memFifoEntries_2MB  ... unsupported resolution */
    0x765,     /* memFifoEntries_4MB  ... 4959 entries in memory fifo */
    15,        /* tilesInX_Over2 */
    14,        /* vFifoThreshold */
    FXTRUE,    /* video16BPPIsOK */
    FXTRUE,    /* video24BPPIsOK */
    56.0F,     /* clkFreq16bpp */
    112.0F     /* clkFreq24bpp */
};

#else /* SST1INIT_VIDEO_ALLOCATE */


#ifdef __cplusplus
extern "C" {
#endif

extern sst1VideoTimingStruct SST_VREZ_640X480_60;
extern sst1VideoTimingStruct SST_VREZ_800X600_60;

#ifdef __cplusplus
}
#endif

#endif /* SST1INIT_VIDEO_ALLOCATE */

#endif /* !__SST1INIT_H__ */
