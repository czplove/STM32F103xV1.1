/*
 * escore-i2c.c  --  I2C interface for Audience earSmart chips
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "escore.h"
#include "escore-i2c.h"


 /*i2c 写一个字节*/  
/* Byte Write */  
int I2C1_WriteByte(uint8_t DeviceAddress, uint8_t *Data, uint8_t len)  
{  
	int  cnt;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));      //等待I2C空闲   


    //start  
    I2C_GenerateSTART(I2C1, ENABLE);  
    while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS );   

    //device address  
    I2C_Send7bitAddress(I2C1, DeviceAddress, I2C_Direction_Transmitter); //写模式  
    while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS );     
   
  

    for( cnt = 0; cnt < len; cnt ++)          
    {
	    //data  
	    //printf("Data[%d] = %x", cnt, Data[cnt]);
	    I2C_SendData(I2C1, Data[cnt]);  
	    while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS );  
	 
	  // printf("Data[%d] = %x", cnt, Data[cnt]);

     }

    //stop  
    I2C_GenerateSTOP(I2C1, ENABLE);   

	return 1;
}  
  
/* i2c 读一个字节*/  
/* Random Read */  
uint8_t I2C1_ReadByte(uint8_t DeviceAddress, u8 *buf, u8 len)  
{  
    uint8_t Data;  
	int cnt;
      
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));      //等待I2C空闲    
    
       
                                                                            
    //start  
    I2C_GenerateSTART(I2C1, ENABLE);  
    while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS ) ;      
  
    //device address  
    I2C_Send7bitAddress(I2C1, DeviceAddress, I2C_Direction_Receiver); //读模式  
    while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS ) ;  

     for(cnt = 0; cnt < len; cnt ++)
    {
    /* 按照manual的图273，先读取数据，关闭ACK应答，最后发出STOP*/         
    	while( I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS );   /* 等待读取事件，再读取数据 */ 
    	buf[cnt] = I2C_ReceiveData(I2C1);
	

    }
     I2C_AcknowledgeConfig(I2C1, DISABLE); //关闭应答和停止条件产生    
    printf("received\r\n");  
    I2C_GenerateSTOP(I2C1, ENABLE);   
      
    I2C_AcknowledgeConfig(I2C1, ENABLE);   
  
    return Data;  
}  
static u32 escore_cpu_to_i2c(struct escore_priv *escore, u32 resp)
{
	return cpu_to_be32(resp);
}

static u32 escore_i2c_to_cpu(struct escore_priv *escore, u32 resp)
{
	return be32_to_cpu(resp);
}

int escore_i2c_read(struct escore_priv *escore, void *buf, int len)
{
	I2C1_ReadByte(0x7c, (uint8_t *)buf, len);
	return 0;
}

int escore_i2c_write(struct escore_priv *escore, const void *buf, int len)
{
	int max_xfer_len = ES_MAX_I2C_XFER_LEN;	
	int rc = 0, written = 0, xfer_len;	
	int retry = ES_MAX_RETRIES;
	uint8_t *pbuf;
	int i;
		pbuf = (uint8_t *)buf;

		while (written < len) {		
			xfer_len = min(len - written, max_xfer_len);		
				
			pbuf += written ;		
			do {			
	
				rc = I2C1_WriteByte(0x7c, pbuf, xfer_len);
				if (rc == 1)				
					break;			
				else				
					printf("failed, retrying, rc:%d\n",rc);			
				usleep_range(10000, 10000);			
				--retry;		
				} while (retry != 0);		
			retry = ES_MAX_RETRIES;		
			written += xfer_len;	
			}	
		return 0;
}

int escore_i2c_cmd(struct escore_priv *escore, u32 cmd, u32 *resp)
{
	int err = 0;
	int sr = cmd & BIT(28);
	u32 rv;
	int retry = ES_MAX_RETRIES + 1;



	cmd = cpu_to_be32(cmd);
	err = escore_i2c_write(escore, &cmd, sizeof(cmd));
	if (err || sr)
		return err;

	do {

	

		err = escore_i2c_read(escore, &rv, sizeof(rv));
		*resp = be32_to_cpu(rv);
		if (err) {
			printf(
				" escore_i2c_read() failure\n");
		} else if ((*resp & ES_ILLEGAL_CMD) == ES_ILLEGAL_CMD) {
			 printf(" illegal command 0x%08x\n",
				 cmd);
			err = -EINVAL;
			goto cmd_exit;
		} else if (*resp == ES_NOT_READY) {
		
				printf(" escore_i2c_read() not ready\n");
			err = -ETIMEDOUT;
		} else {
			goto cmd_exit;
		}

		--retry;
	} while (retry != 0 && escore->cmd_compl_mode != ES_CMD_COMP_INTR);

cmd_exit:
	return err;
}

int escore_i2c_boot_setup(struct escore_priv *escore)
{
	u16 boot_cmd = ES_I2C_BOOT_CMD;
	u16 boot_ack = 0;
	char msg[2];
	int rc;



	cpu_to_be16(&boot_cmd);
	memcpy(msg, (char *)&boot_cmd, 2);
	rc = escore_i2c_write(escore, msg, 2);
	if (rc < 0) {
		printf(": firmware load failed boot write\n");
		goto escore_bootup_failed;
	}
	usleep_range(1000, 1000);
	memset(msg, 0, 2);
	rc = escore_i2c_read(escore, msg, 2);
	if (rc < 0) {
		printf(": firmware load failed boot ack\n");
		goto escore_bootup_failed;
	}
	memcpy((char *)&boot_ack, msg, 2);
	printf(": boot_ack = 0x%04x\n",  boot_ack);
	if (boot_ack != ES_I2C_BOOT_ACK) {
		printf(": firmware load failed boot ack pattern");
		rc = -EIO;
		goto escore_bootup_failed;
	}

escore_bootup_failed:
	return rc;
}

int escore_i2c_boot_finish(struct escore_priv *escore)
{
	u32 sync_cmd = (ES_SYNC_CMD << 16) | ES_SYNC_POLLING;
	u32 sync_ack;
	int rc = 0;
	int sync_retry = ES_SYNC_MAX_RETRY;

	/* Use Polling method if gpio-a is not defined */

	/* sometimes earSmart chip sends success in second sync command */
	do {
		//printf(": write ES_SYNC_CMD = 0x%08x\n",
		//		 sync_cmd);
		rc = escore_i2c_cmd(escore, sync_cmd, &sync_ack);
		if (rc < 0) {
			printf(": firmware load failed sync write\n");
			continue;
		}
		printf(": sync_ack = 0x%08x\n",  sync_ack);
		if (sync_ack != ES_SYNC_ACK) {
			printf(": firmware load failed sync ack pattern");
			rc = -EIO;
		} else {

			printf(": firmware load success");
			break;
		}
	} while (sync_retry--);

	return rc;
}

static void escore_i2c_setup_pri_intf(struct escore_priv *escore)
{
	escore->bus.ops.read = escore_i2c_read;
	escore->bus.ops.write = escore_i2c_write;
	escore->bus.ops.cmd = escore_i2c_cmd;
}

static int escore_i2c_setup_high_bw_intf(struct escore_priv *escore)
{
	int rc = 0;

	escore->bus.ops.high_bw_write = escore_i2c_write;
	escore->bus.ops.high_bw_read = escore_i2c_read;
	escore->bus.ops.high_bw_cmd = escore_i2c_cmd;
	escore->boot_ops.setup = escore_i2c_boot_setup;
	escore->boot_ops.finish = escore_i2c_boot_finish;
	escore->bus.ops.cpu_to_bus = escore_cpu_to_i2c;
	escore->bus.ops.bus_to_cpu = escore_i2c_to_cpu;



	return rc;
}

int escore_i2c_init(void)
{
	struct escore_priv *escore = &escore_priv;
	int rc;




	if (escore->pri_intf == ES_I2C_INTF) {
		escore->bus.setup_prim_intf = escore_i2c_setup_pri_intf;
	}
	if (escore->high_bw_intf == ES_I2C_INTF)
		escore->bus.setup_high_bw_intf = escore_i2c_setup_high_bw_intf;


	rc = escore_probe(escore, ES_I2C_INTF);
	return rc;
}

