/*
 * escore-uart-common.c  --  Audience eS705 UART interface
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author: Matt Lupfer <mlupfer@cardinalpeak.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//-#include "user_conf.h"

#include "escore.h"
#include "escore-uart-common.h"

#define USARTz                   USART2   //-Õâ¸öÃ»ÓÃÐèÒª×¢ÒâÏÂ
#define USARTz_GPIO              GPIOA
  #define USARTz_CLK               RCC_APB1Periph_USART2
  #define USARTz_GPIO_CLK          RCC_APB2Periph_GPIOA
  #define USARTz_Tx_DMA_Channel    DMA1_Channel7
  #define USARTz_Tx_DMA_FLAG       DMA1_FLAG_TC7
  #define USARTz_DR_Base           0x40004404
  #define USARTz_IRQn              USART2_IRQn

 u8 buffer[TX_BUFF_SIZE];
 u8 RxBuf[RX_BUFF_SIZE];
  u8 RXCount = 0;


int escore_uart_read_internal(struct escore_priv *escore, void *buf, int len)
{
	int rc = 0;
	u8 *p;
	//int i;
	p = (u8 *)buf;

	



#if 0	
	printf("RXCount = %d" , RXCount);
	if(len != RXCount)
	{
		memset(RxBuf,0x00,sizeof(RxBuf));
		RXCount = 0;
		return -EIO;

	}
	
	memcpy((u8 *)buf,(u8 *)RxBuf, len);
	memset(RxBuf,0x00,sizeof(RxBuf));
	RXCount = 0;
#endif
 	fifo_get(p, len);

//for(i = 0 ; i < len;  i ++)
   //  memcpy((u8 *)buf,(u8 *)RxBuf, len);
//	printf( "#########p[%d] =  %x############\n",  i, p[i]);

	return rc;
}

u32 escore_cpu_to_uart(struct escore_priv *escore, u32 resp)
{
	return cpu_to_be32(resp);
	
}

u32 escore_uart_to_cpu(struct escore_priv *escore, u32 resp)
{
	return be32_to_cpu(resp);

}

int escore_uart_read(struct escore_priv *escore, void *buf, int len)
{
	int rc =0;

	rc = escore_uart_read_internal(escore, buf, len);



	return rc;
}

int escore_uart_write(struct escore_priv *escore, const void *buf, int len)
{
	int rc = 0;
	u8 * p;
	int i;
	
	p = (u8 *)buf;


	 //printf( "escore_uart_write() size %d\n",  len);

	
	 for ( i = 0; i  < len; i++) 
	 {
	 //	printf(" *p%x\n", *p);
        	USART_SendData(USART1,*p);
		p++;
		while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
     	 }


	// printf("escore_uart_write() returning %d\n",  rc);
	return rc;
}
int escore_uart_write_file(struct escore_priv *escore, const char * name)
{
	int rc = 0;

	
	 FILINFO finfo;
	 FIL fd;
	 UINT size;

	 
 
	 
	 finfo = OutPutFile(name);
	

	 rc = f_open(&fd, name, FA_OPEN_EXISTING | FA_READ); 	//ÒÔ¶ÁµÄ·½Ê½´ò¿ªÎÄ¼þ
	 if(rc < 0)
	 	printf("open file error %s" , name);

	for(;;)
	{	
	 	rc = f_read(&fd, buffer, sizeof(buffer), &size);
	 	if(rc < 0)
	 		printf("f_read fail %d" , rc);		
		//escore_dma_data_to_uart_port(buf, size);
		escore_uart_write(escore, buffer,size);
		if(size < sizeof(buffer))
			break;


	}

	f_close(&fd);
	 
	 
	 rc = 0;
	return rc;
}

int escore_dma_data_to_uart_port(u8* buf, int len)
{

	DMA_InitTypeDef DMA_InitStructure;

	printf("escore_dma_data_to_uart_port start\n");

	USART_Cmd(USARTz, DISABLE);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;


	/* USARTz TX DMA1 Channel (triggered by USARTz Tx event) Config */
	DMA_DeInit(USARTz_Tx_DMA_Channel);  
	DMA_InitStructure.DMA_PeripheralBaseAddr = USARTz_DR_Base;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = len;  
	DMA_Init(USARTz_Tx_DMA_Channel, &DMA_InitStructure);


	USART_DMACmd(USARTz, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(USARTz_Tx_DMA_Channel, ENABLE);
	USART_Cmd(USARTz, ENABLE);


	/* Wait until USARTz TX DMA1 Channel Transfer Complete */
	while (DMA_GetFlagStatus(USARTz_Tx_DMA_FLAG) == RESET)
	{

	}

	return 0;
}

int escore_uart_write_data_block(struct escore_priv *escore, const void *buf, int len)
{
	int rc = 0;
	
	

	/* USARTy TX DMA1 Channel (triggered by USARTy Tx event) Config */

	escore_uart_write(escore,buf, len);



	return rc;
}


int escore_uart_cmd(struct escore_priv *escore, u32 cmd, u32 *resp)
{
	int err = 0;
	int sr = cmd & BIT(28);
	u32 rv;
	int retry = ES_MAX_RETRIES + 1;

	cmd = cpu_to_be32(cmd);
	err = escore_uart_write(escore, &cmd, sizeof(cmd));
	

	
	if (err || sr)
		return err;

	do {
		
			usleep_range(10000,
					10000 + 500);


		
		err = escore_uart_read(escore, &rv, sizeof(rv));
		
		*resp = be32_to_cpu(rv);;
		
		if (err) {
			printf(" escore_uart_read() failure\n");
		} else if ((*resp & ES_ILLEGAL_CMD) == ES_ILLEGAL_CMD) {
			printf( " illegal command 0x%08x\n",
				 cmd);
			err = -EINVAL;
			goto cmd_exit;
		} else if (*resp == ES_NOT_READY) {
			printf(
				" escore_uart_read() not ready\n");
			err = -ETIMEDOUT;
		} else {
			goto cmd_exit;
		}

		--retry;
	} while (retry != 0 && escore->cmd_compl_mode != ES_CMD_COMP_INTR);

cmd_exit:
	return err;
}

int escore_configure_tty(u32 bps, int stop)
{
	int rc = 0;

	  
	  USART_InitTypeDef USART_InitStructure;
	  USART_Cmd(USART1, DISABLE);	

	 
	  RCC_APB1PeriphClockCmd( RCC_APB2Periph_USART1 , ENABLE);	 //Ê¹ÄÜ´®¿Ú1Ê±ÖÓ


	  USART_InitStructure.USART_BaudRate = bps;						//ËÙÂ
	  USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//Êý¾ÝÎ»8Î»
	  USART_InitStructure.USART_StopBits = USART_StopBits_2;			//Í£Ö¹Î»1Î»
	  USART_InitStructure.USART_Parity = USART_Parity_No;				//ÎÞÐ£ÑéÎ»
	  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //ÎÞÓ²¼þÁ÷¿Ø
	  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//ÊÕ·¢Ä£Ê½

	  /* Configure USART2 */
	  USART_Init(USART1, &USART_InitStructure);							//ÅäÖÃ´®¿Ú²ÎÊýº¯Êý   
	   /* Enable the USART2 */
	  USART_Cmd(USART1, ENABLE);	

	return rc;
}


int escore_uart_open(struct escore_priv *escore)
{


	if (escore->uart_ready) {
		printf("UART is already open\n");
		return 0;
	}


	escore->uart_ready = 1;
	return 0;
}

int escore_uart_close(struct escore_priv *escore)
{

	if (!escore->uart_ready) {
		printf("UART is already closed\n");
		return 0;
	}


	escore->uart_ready = 0;

	return 0;
}



struct es_stream_device es_uart_streamdev = {
	 escore_uart_open,
	 escore_uart_read_internal,
	 escore_uart_close,
	 ES_UART_INTF,
};

