/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* by grqd_xp                                                            */
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
#include <string.h>
#include "diskio.h"
//-#include "stm32f10x.h"
#include "COMMON.h"

extern void SST25_R_BLOCK(unsigned long addr, unsigned char *readbuff, unsigned int BlockSize);
extern void SST25_W_BLOCK(UINT32 addr, UINT8 *readbuff, UINT16 BlockSize);

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/* Note that Tiny-FatFs supports only single drive and always            */
/* accesses drive number 0.                                              */

#define SST25_SECTOR_SIZE 4096
#define SST25_BLOCK_SIZE 512 

//u32 buff2[512/4];
/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{	
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{

	if(count==1)
        {
          SST25_R_BLOCK(sector <<12,&buff[0],SST25_SECTOR_SIZE);
	}
	else
        {
          
	}
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	if(count==1)
        {
		  SST25_W_BLOCK(sector<<12 ,(UINT8 *)&buff[0],SST25_SECTOR_SIZE);
	}
	else
        {
	}
        
  return RES_OK;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{

	DRESULT res = RES_OK;


      if (drv){ return RES_PARERR;}
   
      switch(ctrl)
      {
       case CTRL_SYNC:
           break;
    case GET_BLOCK_SIZE:
           *(DWORD*)buff = SST25_BLOCK_SIZE;
           break;
    case GET_SECTOR_COUNT:
           *(DWORD*)buff = SST25_BLOCK_SIZE;
           break;
    case GET_SECTOR_SIZE:
           *(WORD*)buff = SST25_SECTOR_SIZE;
           break;
       default:
           res = RES_PARERR;
           break;
   }
      return res;


}

DWORD get_fattime(void){
	return 0;
}

