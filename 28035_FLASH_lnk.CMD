/*==================================================================================*/
/*	User specific Linker command file for running from FLASH						*/
/*==================================================================================*/
/*	FILE:			F28035_FLASH_HVACI_Sensored.CMD                                 */
/*                                                                                  */
/*	Description:	Linker command file for User custom sections targetted to run   */
/*					from FLASH.  			                                        */
/*                                                                                  */
/*  Target:  		TMS320F28035          					                        */
/*                                                                                  */
/*	Version: 		1.00                                							*/
/*                                                                                  */
/*----------------------------------------------------------------------------------*/
/*  Copyright Texas Instruments � 2009                                			    */
/*----------------------------------------------------------------------------------*/
/*  Revision History:                                                               */
/*----------------------------------------------------------------------------------*/
/*  Date	  | Description                                                         */
/*----------------------------------------------------------------------------------*/
/*  4/13/09  | Release 1.0  		New release.                                    */
/*----------------------------------------------------------------------------------*/
 

/* Define the memory block start/length for the F28035
   PAGE 0 will be used to organize program sections
   PAGE 1 will be used to organize data sections

   Notes:
         Memory blocks on F2803x are uniform (ie same
         physical memory) in both PAGE 0 and PAGE 1.
         That is the same memory region should not be
         defined for both PAGE 0 and PAGE 1.
         Doing so will result in corruption of program
         and/or data.

         L0 memory block is mirrored - that is
         it can be accessed in high memory or low memory.
         For simplicity only one instance is used in this
         linker file.

         Contiguous SARAM memory blocks or flash sectors can be
         be combined if required to create a larger memory block.
*/

MEMORY
{
/* Note that the memory allocation below does not create sections as necessary for
   the CLA on the F2803x. 
*/

PAGE 0:
	progRAM	    : origin = 0x008000, length = 0x001000 /* on-chip RAM (L0-L2)*/
	OTP         : origin = 0x3D7800, length = 0x000400     /* on-chip OTP */
	FLASHCODE   : origin = 0x3E8000, length = 0x008000     /* on-chip FLASH */
	FLASHC      : origin = 0x3F0000, length = 0x004000     /* on-chip FLASH */
	FLASHA      : origin = 0x3F6000, length = 0x001F80     /* on-chip FLASH */
	CSM_RSVD    : origin = 0x3F7F80, length = 0x000076     /* Part of FLASHA.  Program with all 0x0000 when CSM is in use. */
	BEGIN       : origin = 0x3F7FF6, length = 0x000002     /* Part of FLASHA.  Used for "boot to Flash" bootloader mode. */
	CSM_PWL     : origin = 0x3F7FF8, length = 0x000008     /* Part of FLASHA.  CSM password locations in FLASHA */

	/* FPU Math Tables in Boot ROM */
	FPUTABLES 	: origin = 0x3FD860, length = 0x0006A0

	IQTABLES    : origin = 0x3FE000, length = 0x000B50     /* IQ Math Tables in Boot ROM */
	IQTABLES2   : origin = 0x3FEB50, length = 0x00008C     /* IQ Math Tables in Boot ROM */
	IQTABLES3   : origin = 0x3FEBDC, length = 0x0000AA     /* IQ Math Tables in Boot ROM */

	BOOTROM     : origin = 0x3FF27C, length = 0x000D44     /* Boot ROM */
	RESET       : origin = 0x3FFFC0, length = 0x000002     /* part of boot ROM  */
	VECTORS     : origin = 0x3FFFC2, length = 0x00003E     /* part of boot ROM  */

PAGE 1 : 
	BOOT_RSVD   : origin = 0x000000, length = 0x000050     /* Part of M0, BOOT rom will use this for stack */
	RAMM0       : origin = 0x000050, length = 0x0003B0     /* on-chip RAM block M0 */
   	RAMM1       : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
	dataRAM	    : origin = 0x009000, length = 0x001000     /* data RAM (L3) */
	FLASHB      : origin = 0x3F5000, length = 0x002000     /* on-chip FLASH */

	CLA_CPU_MSGRAM  : origin = 0x001480, length = 0x000080 /* CLA-R/W, CPU-R message RAM */
	CPU_CLA_MSGRAM  : origin = 0x001500, length = 0x000080 /* CPU-R/W, CLA-R message RAM */
}
 
 
SECTIONS
{
   /* Allocate program areas: */
   .cinit            : > FLASHA,     PAGE = 0
   .pinit            : > FLASHA,     PAGE = 0
   .text             : > FLASHCODE,  PAGE = 0

   codestart         : > BEGIN       PAGE = 0
   ramfuncs          : LOAD = FLASHA,
                       RUN = progRAM, 
                       LOAD_START(_RamfuncsLoadStart),
                       LOAD_END(_RamfuncsLoadEnd),
                       RUN_START(_RamfuncsRunStart),
                       PAGE = 0

                       {
                        --library=rts2800_ml.lib<fs_mpy.obj>
						--library=SFRA_IQ_Lib.lib<SFRA_IQ_INJECT.obj>
						--library=SFRA_IQ_Lib.lib<SFRA_IQ_COLLECT.obj>
						}

   csmpasswds        : > CSM_PWL     PAGE = 0
   csm_rsvd          : > CSM_RSVD    PAGE = 0
   
   /* Allocate uninitalized data sections: */
   .stack            : > RAMM0,      PAGE = 1
   .ebss             : > dataRAM,    PAGE = 1
   .esysmem          : > dataRAM,    PAGE = 1

   /* Initalized sections go in Flash */
   /* For SDFlash to program these, they must be allocated to page 0 */
   .econst           : > FLASHA      PAGE = 0
   .switch           : > FLASHA      PAGE = 0      

	/* Allocate FPU math areas: */
	FPUmathTables : > FPUTABLES, PAGE = 0, TYPE = NOLOAD

   /* Allocate IQ math areas: */
//   IQmath            : > FLASHA      PAGE = 0                  /* Math Code */
   IQmath              : LOAD = FLASHA,
                         RUN = progRAM,
                         LOAD_START(_IQmathLoadStart),
                         LOAD_END(_IQmathLoadEnd),
                         RUN_START(_IQmathRunStart),
                         PAGE = 0
   IQmathTables        : > IQTABLES		PAGE = 0  , TYPE = NOLOAD   /* Math Tables In ROM */
   IQmathTablesRam     : > dataRAM		PAGE = 1


   Cla1ToCpuMsgRAM   : > CLA_CPU_MSGRAM PAGE = 1
   CpuToCla1MsgRAM   : > CPU_CLA_MSGRAM PAGE = 1

   .reset            : > RESET,      PAGE = 0, TYPE = DSECT
   vectors           : > VECTORS     PAGE = 0, TYPE = DSECT


 /* Uncomment the section below if calling the IQNexp() or IQexp()
      functions from the IQMath.lib library in order to utilize the
      relevant IQ Math table in Boot ROM (This saves space and Boot ROM
      is 1 wait-state). If this section is not uncommented, IQmathTables2
      will be loaded into other memory (SARAM, Flash, etc.) and will take
      up space, but 0 wait-state is possible.
   */
   /*
   IQmathTables2    : > IQTABLES2, PAGE = 0, TYPE = NOLOAD
   {

		IQmath.lib<IQNexpTable.obj> (IQmathTablesRam)

   }
   */
   /* Uncomment the section below if calling the IQNasin() or IQasin()
      functions from the IQMath.lib library in order to utilize the
      relevant IQ Math table in Boot ROM (This saves space and Boot ROM
      is 1 wait-state). If this section is not uncommented, IQmathTables2
      will be loaded into other memory (SARAM, Flash, etc.) and will take
      up space, but 0 wait-state is possible.
   */
   /*
   IQmathTables3    : > IQTABLES3, PAGE = 0, TYPE = NOLOAD
   {

              IQmath.lib<IQNasinTable.obj> (IQmathTablesRam)

   }
   */

}
    

    
SECTIONS
{

	/* CNTL_2P2Z/CNTL_3P3Z section */
	cntl_var_RAM				: > dataRAM				PAGE = 1
	cntl_coeff_RAM				: > dataRAM				PAGE = 1

	cntl_var_RAM2				: > dataRAM				PAGE = 1
	cntl_coeff_RAM2				: > dataRAM				PAGE = 1

	SINTBL						: > dataRAM				PAGE = 1

    SFRA_IQ_Data				: > dataRAM, ALIGN = 64, PAGE = 1

 	SFRA_Data					: > dataRAM, ALIGN = 64, PAGE = 1

	Net_terminals:	> dataRAM,PAGE = 1
	DLOG: > dataRAM,PAGE=1
}



 
