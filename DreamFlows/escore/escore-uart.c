/*
 * escore-uart.c  --  Audience eS705 UART interface
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


#include "escore.h"
#include "escore-uart.h"
#include "escore-uart-common.h"


static int escore_uart_boot_setup(struct escore_priv *escore);
static int escore_uart_boot_finish(struct escore_priv *escore);


static int escore_uart_abort_config(struct escore_priv *escore);
struct escore_uart_device escore_uart;
#ifdef ESCORE_FW_LOAD_BUF_SZ
#undef ESCORE_FW_LOAD_BUF_SZ
#endif
#define ESCORE_FW_LOAD_BUF_SZ 1024
#define ES_MAX_READ_RETRIES 100

#define ES705_MAX_READ_RETRIES 100

#define CONFIG_SND_SOC_ES_UARTHS_BAUD  3000000



struct cycle_buffer tx;
struct cycle_buffer *fifo;
static u32 escore_default_uart_baud = 460800;

static u32 escore_uarths_baud[UART_RATE_MAX] = {
	460800, 921600, 1000000, 1024000, 1152000,
	2000000, 2048000, 3000000, 3072000 };

static int set_sbl_baud_rate(struct escore_priv *escore, u32 sbl_rate_req_cmd)
{
	char msg[4] = {0};
	char *buf;
	u8 bytes_to_read;
	int rc;
	u32 baudrate_change_resp;



	sbl_rate_req_cmd = cpu_to_be32(sbl_rate_req_cmd);

	rc = escore_uart_write(escore, &sbl_rate_req_cmd,
			sizeof(sbl_rate_req_cmd));
	if (rc) {
		printf("Baud rate setting for UART failed\n");
		rc = -EIO;
		return rc;
	}

	baudrate_change_resp = 0;

	/* Sometimes an extra byte (0x00) is received over UART
	 * which should be discarded.
	 */
	usleep_range(10000, 10500);

	rc = escore_uart_read(escore, msg, sizeof(baudrate_change_resp));

	baudrate_change_resp |= *(u32 *)msg;

	printf("baudrate_change_resp %x" , baudrate_change_resp);


	if (baudrate_change_resp != sbl_rate_req_cmd) {
		printf("Invalid response to Rate Request :%x, %x\n",baudrate_change_resp,sbl_rate_req_cmd);
		return -EINVAL;
	}

	return rc;
}

static int escore_uart_wakeup(struct escore_priv *escore)
{
	char wakeup_byte = 'A';
	int ret = 0;

	
	ret = escore_uart_open(&escore_priv);
	if (ret) {
		printf( " escore_uart_open() failed %d\n",
				 ret);
		goto escore_uart_open_failed;
	}
	ret = escore_uart_write(&escore_priv, &wakeup_byte, 1);
	if (ret < 0) {
		printf( "UART wakeup failed:%d\n",
				 ret);
		goto escore_uart_wakeup_exit;
	}

	/* Read an extra byte to flush UART read buffer. If this byte
	 * is not read, an extra byte is received in next UART read
	 * because of above write. No need to check response here.
	 */
	escore_uart_read(&escore_priv, &wakeup_byte, 1);

escore_uart_wakeup_exit:
	escore_uart_close(&escore_priv);
escore_uart_open_failed:
	return ret;
}

static int escore_uart_abort_config(struct escore_priv *escore)
{
	u8 sbl_sync_cmd = ESCORE_SBL_SYNC_CMD;
	int rc;

	rc = escore_configure_tty(
			escore_uart.baudrate_bootloader, UART_TTY_STOP_BITS);
	if (rc) {
		printf(" config UART failed, rc = %d\n",  rc);
		return rc;
	}

	rc = escore_uart_write(escore, &sbl_sync_cmd, 1);
	if (rc) {
		printf(" UART SBL sync write failed, rc = %d\n",
			 rc);
		return rc;
	}

	return rc;
}

int escore_uart_boot_setup(struct escore_priv *escore)
{
	u8 sbl_sync_cmd = ESCORE_SBL_SYNC_CMD;
	u8 sbl_boot_cmd = ESCORE_SBL_BOOT_CMD;
	u32 sbl_rate_req_cmd = (u32)ESCORE_SBL_SET_RATE_REQ_CMD << 16;
	char msg[4] = {0};
	int uart_tty_baud_rate = UART_TTY_BAUD_RATE_460_8_K;
	int rc;
	u8 i;
	int iRetry=0;
	u8 read_cnt = 0;

	/* set speed to bootloader baud */
	escore_configure_tty(
		escore_uart.baudrate_bootloader, UART_TTY_STOP_BITS);

	


write_retry:

	/* SBL SYNC BYTE 0x00 */
	
	printf(" write ESCORE_SBL_SYNC_CMD = %x\n", 
		sbl_sync_cmd);
	memcpy(msg, (char *)&sbl_sync_cmd, 1);

	rc = escore_uart_write(escore, msg, 1);


	if (rc) {
		iRetry++;
		printf(" sbl sync write\n");
		if (iRetry < 10)
			goto write_retry;
		rc = -EIO;
		goto escore_bootup_failed;
	}
	iRetry = 0;
	printf(" firmware load sbl sync write rc=%d\n",  rc);
	memset(msg, 0, 4);
synca_retry:
	usleep_range(10000, 10500);

	rc = escore_uart_read(escore, msg, 1);
	if (rc) {
		printf(" firmware load failed sync ack rc = %d\n",
			 rc);
		iRetry++;
		printf(" sync ack rc = %d  retry=%d\n",
				 rc, iRetry);
		if (iRetry < 10)
			goto synca_retry;
		goto escore_bootup_failed;
	}
	printf(" sbl sync ack = %x\n",  msg[0]);
	if (msg[0] != ESCORE_SBL_SYNC_ACK&& read_cnt < ES705_MAX_READ_RETRIES) {

		printf( " sync ack 0x%02x\n",
				 msg[0]);
		read_cnt++;
		goto synca_retry;

	}
	else if(read_cnt == ES705_MAX_READ_RETRIES){

		rc = -EIO;
		goto escore_bootup_failed;
	}
	/* UART Baud rate and Clock setting:-
	 *
	 * Default baud rate will be 460.8Khz and external clock
	 * will be to 6MHz unless high speed firmware download
	 * is enabled.
	 */
	


		escore_default_uart_baud = 3000000;//CONFIG_SND_SOC_ES_UARTHS_BAUD;

		/* Set Baud rate and external clock */
		uart_tty_baud_rate = ES_UARTHS_BAUD_RATE_3000K;
		for (i = 0; i < ARRAY_SIZE(escore_uarths_baud); i++) {
			if (escore_uarths_baud[i] == escore_default_uart_baud)
				uart_tty_baud_rate = i;
		}

		sbl_rate_req_cmd |= uart_tty_baud_rate << 8;
		sbl_rate_req_cmd |= (escore->pdata->ext_clk_rate & 0xff);

		rc = set_sbl_baud_rate(escore, sbl_rate_req_cmd);
		if (rc < 0)
			goto escore_bootup_failed;

		escore_configure_tty(
				escore_uarths_baud[uart_tty_baud_rate],
				UART_TTY_STOP_BITS);
	

	msleep(20);
	read_cnt = 0;

	iRetry=0;

	/* SBL BOOT BYTE 0x01 */
	memset(msg, 0, 4);
	//printf(" write ESCORE_SBL_BOOT_CMD = 0x%02x\n", 
		//sbl_boot_cmd);
	memcpy(msg, (char *)&sbl_boot_cmd, 1);
	rc = escore_uart_write(escore, msg, 1);
	if (rc) {
		printf(" firmware load failed sbl boot write\n");
		rc = -EIO;
		goto escore_bootup_failed;
	}
bootb_retry:
	/* SBL BOOT BYTE ACK 0x01 */
	msleep(20);
	memset(msg, 0, 4);
	rc = escore_uart_read(escore, msg, 1);
	if (rc) {
		iRetry++;

		printf( " sbl boot nack rc %d   retry=%d\n",
				 rc, iRetry);
		if (iRetry < 10)
			goto bootb_retry;
		goto escore_bootup_failed;
	}
	//printf(" sbl boot ack = 0x%02x\n",  msg[0]);

	if (msg[0] != ESCORE_SBL_BOOT_ACK&&read_cnt < ES_MAX_READ_RETRIES ) {
		printf( " sbl boot nack 0x%02x\n",
				 msg[0]);
		read_cnt++;
		goto bootb_retry;

	}
	else if(read_cnt == ES_MAX_READ_RETRIES)
	{
		rc = -EIO;
		printf( " Invalid response, maxout\n");
		goto escore_bootup_failed;


	}
	rc = 0;

escore_bootup_failed:
	return rc;
}

int escore_uart_boot_finish(struct escore_priv *escore)
{
	char msg[4];
	char len;
	int rc;
	u32 sync_cmd;
	u32 sync_ack;
	int sync_retry = ES_SYNC_MAX_RETRY;

	/* Use Polling method if gpio-a is not defined */
	/*
	 * Give the chip some time to become ready after firmware
	 * download. (FW is still transferring)
	 */


	msleep(200);

	/* Discard extra bytes from escore during firmware load. Host gets
	 * one more extra bytes after VS firmware download as per Bug 19441.
	 * As per bug #22191, in case of eS755 four extra bytes are received
	 * instead of 3 extra bytes. To make it generic, host reads 4 bytes.
	 * There is no need to check for response because host may get less
	 * bytes.
	 */
	 len = fifo_check();
	if(len)
	{
		escore_uart_read(escore, msg, len);
		printf("got extra data ");

	}	
	sync_cmd = ((u32)ES_SYNC_CMD << 16) | ES_SYNC_POLLING;

	memset(msg, 0, 4);
	//rc = escore_uart_read(escore, msg, 4);

	/* now switch to firmware baud to talk to chip */

	escore_configure_tty(
		3000000, UART_TTY_STOP_BITS);


	/* sometimes earSmart chip sends success in second sync command */
	do {
		printf(" write ES_SYNC_CMD = 0x%08x\n",
			 sync_cmd);
		rc = escore_uart_cmd(escore, sync_cmd, &sync_ack);
		if (rc) {
			printf(" firmware load failed sync write - %d\n",
				 rc);
			continue;
		}
		//printf(" sync_ack = 0x%08x\n",  sync_ack);
		if (sync_ack != ES_SYNC_ACK) {
			printf(" firmware load failed sync ack pattern");
			rc = -EIO;
		} else {
			printf(" firmware load success\n");
			break;
		}
	} while (sync_retry--);

	return rc;
}



static void escore_uart_setup_pri_intf(struct escore_priv *escore)
{
	
	escore->bus.ops.read = escore_uart_read;
	escore->bus.ops.write = escore_uart_write;
	escore->bus.ops.cmd = escore_uart_cmd;
	escore->escore_uart_wakeup = escore_uart_wakeup;
	
}

static int escore_uart_setup_high_bw_intf(struct escore_priv *escore)
{
	int rc = 0;


	escore->bus.ops.high_bw_write_file = escore_uart_write_file;
	escore->bus.ops.high_bw_write = escore_uart_write_data_block;
	escore->bus.ops.high_bw_read = escore_uart_read;
	escore->bus.ops.high_bw_cmd = escore_uart_cmd;
	escore->bus.ops.high_bw_open = escore_uart_open;
	escore->bus.ops.high_bw_close = escore_uart_close;
	escore->boot_ops.escore_abort_config = escore_uart_abort_config;
	escore->boot_ops.setup = escore_uart_boot_setup;
	escore->boot_ops.finish = escore_uart_boot_finish;
	escore->bus.ops.cpu_to_bus = escore_cpu_to_uart;
	escore->bus.ops.bus_to_cpu = escore_uart_to_cpu;
	escore->escore_uart_wakeup = escore_uart_wakeup;



	escore_uart.baudrate_bootloader = 460800;


	escore_uart.baudrate_fw = 3000000;

	


	return rc;
}




  static int init_cycle_buffer(void)  
 {
     int size = RX_BUFF_SIZE;
     int ret;  
  
    ret = size & (size - 1);  
     if (ret)  
         return ret;
     fifo = &tx;  
     if (!fifo)  
         return -1;  
 
    memset(fifo, 0, sizeof(struct cycle_buffer));  
     fifo->size = size;  
     fifo->in = fifo->out = 0;  

     fifo->buf = (unsigned char *) RxBuf;  

     memset(fifo->buf, 0, size);  
     return 0;  
 }  

  unsigned int fifo_get(unsigned char *buf, unsigned int len)  
 {  
     unsigned int l;  
     len = min(len, fifo->in - fifo->out);  
     l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));  
     memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);  
     memcpy(buf + l, fifo->buf, len - l);  
     fifo->out += len;  
     
     return len;  
 }  

  int fifo_check()
  {

	if(fifo->in > fifo->out)
		return fifo->in -fifo->out;
	else
		return 0;

  }


  unsigned int fifo_put(unsigned char *buf, unsigned int len)  
 {  
     unsigned int l;  
     len = min(len, fifo->size - fifo->in + fifo->out);  
     l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));  
     memcpy(fifo->buf + (fifo->in & (fifo->size - 1)), buf, l);  
     memcpy(fifo->buf, buf + l, len - l);  
     fifo->in += len;  
     return len;  
 }  


 int debug_fifo(void)
 {
	int i ;
	printf(" fifo->size %d, fifo->in %d,  fifo->out %d", fifo->size,fifo->in, fifo->out);

	for(i = 0; i < 20; i++)
		
	printf("fifo->buf[%d] =%x",i, fifo->buf[i]);
	

 }
  
int escore_uart_probe(void)
{
	int rc = 0;
	
	struct escore_priv *escore = &escore_priv;
	int probe_intf = ES_INVAL_INTF;

	init_cycle_buffer();

	rc = escore_uart_open(escore);
	if (rc) {
		printf( "es705_uart_open() failed %d\n", rc);
		return rc;
	}

	/* device node found, continue */


	if (escore->pri_intf == ES_UART_INTF) {
		escore->bus.setup_prim_intf = escore_uart_setup_pri_intf;
		probe_intf = ES_UART_INTF;
	}
	if (escore->high_bw_intf == ES_UART_INTF) {
		escore->bus.setup_high_bw_intf = escore_uart_setup_high_bw_intf;
		probe_intf = ES_UART_INTF;
	}

	rc = escore_probe(escore, probe_intf);

	if (rc) {
		printf( "UART common probe failed\n");
		goto bootup_error;
	}
	return rc;

bootup_error:
	
	escore_uart_close(escore);
	
	printf("exit with error\n");
	return rc;
}

