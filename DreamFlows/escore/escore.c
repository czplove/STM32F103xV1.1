/*
 * escore.c  --  Audience earSmart Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "escore.h"
#include "escore-uart.h"

struct escore_macro cmd_hist[ES_MAX_ROUTE_MACRO_CMD] = { {0} };

int cmd_hist_index;
/* History struture, log route commands to debug */
/* Send a single command to the chip.
 *
 * If the SR (suppress response bit) is NOT set, will read the
 * response and cache it the driver object retrieve with escore_resp().
 *
 * Returns:
 * 0 - on success.
 * EITIMEDOUT - if the chip did not respond in within the expected time.
 * E* - any value that can be returned by the underlying HAL.
 */


 uint32_t TimingDelay;

extern u8 buffer[TX_BUFF_SIZE];

u16 cpu_to_be16(u16 data)
{
	u16 temp = 0;

	temp |= (data >> 8)&0xFF;
	temp |= (data &0xFF) << 8;


	return temp;
	

}

u32 cpu_to_be32(u32 data)
{
	u32 temp = 0;

	temp |= (data >> 24)&0xFF;
	temp |= ((data >> 16)&0xFF) << 8;
	temp |= ((data >> 8)&0xFF) << 16;
	temp |= (data&0xFF) << 24;

	return temp;
	

}

u32 be32_to_cpu(u32 data)
{
	u32 temp = 0;
	temp |= (data &0xFF) << 24;
	temp |= ((data >> 8)&0xFF) << 16;
	temp |= ((data >> 16)&0xFF) << 8;
	temp |= ((data >> 24)&0xFF);
	return temp;
}

 void TimingDelay_Decrement(void)
 {
   if (TimingDelay != 0x00)
   { 
     TimingDelay--;
   }
 }


int msleep(int  msec)
{

   TimingDelay = msec;
 
   while(TimingDelay != 0);


   return 0;
}

int usleep_range(int usec1,int usec2)
{
   TimingDelay = (usec1 + usec2)/2000;
 
   while(TimingDelay != 0);
   
   return 0;
}
void escore_pm_put_autosuspend(void)
{

}
int escore_pm_get_sync(void)
{

	return 1;
	
}
static int _escore_cmd(struct escore_priv *escore, u32 cmd, u32 *resp)
{
	int sr;
	int err;

	sr = cmd & BIT(28);
	cmd_hist[cmd_hist_index].cmd = cmd;
	if (cmd_hist_index == ES_MAX_ROUTE_MACRO_CMD-1)
		cmd_hist_index = 0;
	else
		cmd_hist_index++;
	
	err = escore->bus.ops.cmd(escore, cmd, resp);
	if (err || sr)
		goto cmd_err;

	if (resp == 0) {
		err = -ETIMEDOUT;
		printf("no response to command 0x%08x\n", cmd);
	} else {
		escore->bus.last_response = *resp;
		
	}
cmd_err:
	return err;
}

int escore_cmd(struct escore_priv *escore, u32 cmd, u32 *resp)
{
	int ret;
	ret = escore_pm_get_sync();
	if (ret > -1) {
		ret = _escore_cmd(escore, cmd, resp);
		escore_pm_put_autosuspend();
	}
	return ret;
}
int escore_write_block(struct escore_priv *escore, const u32 *cmd_block)
{
	int ret = 0;
	u32 resp;
	ret = escore_pm_get_sync();
	if (ret > -1) {
		while (*cmd_block != 0xffffffff) {
			_escore_cmd(escore, *cmd_block, &resp);
			usleep_range(1000, 1000);
			cmd_block++;
		}
		escore_pm_put_autosuspend();
	}
	return ret;
}


int escore_prepare_msg(struct escore_priv *escore, unsigned int reg,
		       unsigned int value, char *msg, int *len, int msg_type)
{
	struct escore_api_access *api_access;
	u32 api_word[2] = {0};
	int msg_len;

	if (reg > escore->api_addr_max) {
		printf(" invalid address = 0x%04x\n",  reg);
		return -EINVAL;
	}

	

	api_access = &escore->api_access[reg];
	//val_mask = (1 << get_bitmask_order(api_access->val_max)) - 1;

	if (msg_type == ES_MSG_WRITE) {
		msg_len = api_access->write_msg_len;
		memcpy((char *)api_word, (char *)api_access->write_msg,
				msg_len);

		switch (msg_len) {
		case 8:
			api_word[1] |= (value);
			break;
		case 4:
			api_word[0] |= ( value);
			break;
		}
	} else {
		msg_len = api_access->read_msg_len;
		memcpy((char *)api_word, (char *)api_access->read_msg,
				msg_len);
	}

	*len = msg_len;
	memcpy(msg, (char *)api_word, *len);
	
	return 0;

}

static unsigned int _escore_read( unsigned int reg)
{
	struct escore_priv *escore = &escore_priv;
	u32 api_word[2] = {0};
	unsigned int msg_len;
	unsigned int value = 0;
	u32 resp;
	int rc;

	rc = escore_prepare_msg(escore, reg, value, (char *) api_word,
			(int *)&msg_len, ES_MSG_READ);
	if (rc) {
		printf(" Failed to prepare read message 0x%x\n", reg);
		goto out;
	}

	rc = _escore_cmd(escore, api_word[0], &resp);
	if (rc < 0) {
		printf(" escore_cmd() api_word[0] 0x%x", api_word[0]);
		return rc;
	}
	api_word[0] = escore->bus.last_response;

	value = api_word[0] & 0xffff;
out:
	return value;
}

unsigned int escore_read( unsigned int reg)
{
	unsigned int ret = 0;
	int rc;
	rc = escore_pm_get_sync();
	if (rc > -1) {
		ret = _escore_read( reg);
		escore_pm_put_autosuspend();
	}
	return ret;
}

static int _escore_write( unsigned int reg,
		       unsigned int value)
{
	struct escore_priv *escore = &escore_priv;
	u32 api_word[2] = {0};
	int msg_len;
	u32 resp;
	int rc;
	int i;

	rc = escore_prepare_msg(escore, reg, value, (char *) api_word,
			&msg_len, ES_MSG_WRITE);
	if (rc) {
		printf(" Failed to prepare write message 0x%x\n", reg);
		goto out;
	}

	for (i = 0; i < msg_len / 4; i++) {
		rc = _escore_cmd(escore, api_word[i], &resp);
		if (rc < 0) {
			printf(" escore_cmd() 0x%x", api_word[i]);
			return rc;
		}
	}
	
out:
	return rc;
}

/* This function should be called under api_mutex protection */
int escore_reconfig_intr(struct escore_priv *escore)
{
	int rc = 0;
	u32 cmd, resp;

	cmd = (u32)ES_SYNC_CMD << 16;


	rc = escore->bus.ops.cmd(escore, cmd, &resp);
	if (rc < 0) {
		printf(
				" - failed sync cmd resume rc = %d\n", rc);
		goto out;
	}
		cmd = ((u32)ES_SET_EVENT_RESP << 16) | ES_RISING_EDGE;
		rc = escore->bus.ops.cmd(escore, cmd, &resp);
		if (rc < 0) {
			printf(
				"Error %d in setting event response\n",rc);
			goto out;
		}
	
out:
	return rc;
}

int escore_datablock_open(struct escore_priv *escore)
{
	int rc = 0;
	if (escore->bus.ops.high_bw_open)
		rc = escore->bus.ops.high_bw_open(escore);
	return rc;
}

int escore_datablock_close(struct escore_priv *escore)
{
	int rc = 0;
	if (escore->bus.ops.high_bw_close)
		rc = escore->bus.ops.high_bw_close(escore);
	return rc;
}

int escore_datablock_wait(struct escore_priv *escore)
{
	int rc = 0;
	if (escore->bus.ops.high_bw_wait)
		rc = escore->bus.ops.high_bw_wait(escore);
	return rc;
}



int escore_datablock_read(struct escore_priv *escore, u8 *buf,
		u16 len, int id)
{
	int rc;
	int size;
	u32 cmd;
	int rdcnt = 0;
	u32 resp;
	u8 flush_extra_blk = 0;
	u32 flush_buf;

	/* Reset read data block size */
	escore->datablock_dev.rdb_read_count = 0;


	if (escore->bus.ops.rdb) {
		rc = escore->bus.ops.rdb(escore, buf, len, id);
		goto out;
	}

	cmd = ((u32)ES_READ_DATA_BLOCK << 16) | (id & 0xFFFF);

	rc = escore->bus.ops.high_bw_cmd(escore, cmd, &resp);
	if (rc < 0) {
		printf(" escore_cmd() failed rc = %d\n",  rc);
		goto out;
	}
	if ((resp >> 16) != ES_READ_DATA_BLOCK) {
		printf(" Invalid response received: 0x%08x\n",
				 resp);
		rc = -EINVAL;
		goto out;
	}

	size = resp & 0xFFFF;
	printf(" RDB size = %d\n",  size);
	if (size == 0 || size % 4 != 0) {
		printf(" Read Data Block with invalid size:%d\n",
				 size);
		rc = -EINVAL;
		goto out;
	}

	if (len != size) {
		printf(" Requested:%d Received:%d\n", 
				len, size);
		if (len < size)
			flush_extra_blk = (size - len) % 4;
		else
			len = size;
	}

	for (rdcnt = 0; rdcnt < len;) {
		rc = escore->bus.ops.high_bw_read(escore, buf, 4);
		if (rc < 0) {
			printf(" Read Data Block error %d\n",
					 rc);
			goto out;
		}
		rdcnt += 4;
		buf += 4;
	}

	/* Store read data block size */
	escore->datablock_dev.rdb_read_count = size;

	/* No need to read in case of no extra bytes */
	if (flush_extra_blk) {
		/* Discard the extra bytes */
		rc = escore->bus.ops.high_bw_read(escore, &flush_buf,
							flush_extra_blk);
		if (rc < 0) {
			printf(" Read Data Block error in flushing %d\n",
					 rc);
			goto out;
		}
	}
	return len;
out:
	return rc;
}

/* This function should be called under api_mutex protection */
int escore_datablock_write(struct escore_priv *escore, u8 *name)
{
	int rc;
	int count;
	u32 resp;
	u32 cmd = (u32)ES_WRITE_DATA_BLOCK << 16;
	u16 size = 0;
	u16 remaining;





	 FILINFO finfo;
	 FIL fd;
	 UINT read_size;

	 

	 
	 finfo = OutPutFile(name);

	 remaining = finfo.fsize;
	

	 rc = f_open(&fd, finfo.fname, FA_OPEN_EXISTING | FA_READ); 	//以读的方式打开文件
	 if(rc < 0)
	 	printf("open file error %s" , name);



	while (remaining) {

		/* If multiple WDB blocks are written, some delay is required
		 * before starting next WDB. This delay is not documented but
		 * if this delay is not added, extra zeros are obsevred in
		 * escore_uart_read() causing WDB failure.
		 */

		rc = f_read(&fd, buffer, ES_WDB_MAX_SIZE, &read_size);

		
		
		if (read_size > 0)
			usleep_range(2000, 2000);

		size = remaining > ES_WDB_MAX_SIZE ? \
		       ES_WDB_MAX_SIZE : remaining;

		cmd = (u32)ES_WRITE_DATA_BLOCK << 16;
		cmd = cmd | (size & 0xFFFF);
		rc = escore->bus.ops.high_bw_cmd(escore, cmd, &resp);
		if (rc < 0) {
			printf(" escore_cmd() failed rc = %d\n",
					 rc);
			goto out;
		}
		if ((resp >> 16) != ES_WRITE_DATA_BLOCK) {
			printf(" Invalid response received: 0x%08x\n",
					 resp);
			rc = -EIO;
			goto out;
		}


		rc = escore->bus.ops.high_bw_write(escore, buffer, size);
		if (rc < 0) {
			printf(" WDB error:%d\n",  rc);
			goto out;
		}

		/* After completing wdb response should be 0x802f0000,
		   retry until we receive response*/
	

		count = ES_MAX_RETRIES + 5;

		while (count-- > 0) {
			resp = 0;
			usleep_range(10000, 10000);
			rc = escore->bus.ops.high_bw_read(escore, &resp,
					sizeof(resp));
			if (rc < 0) {
				printf(" WDB last ACK read error:%d\n",
					 rc);
				goto out;
			}

			resp = escore->bus.ops.bus_to_cpu(escore, resp);

			if (resp != ((u32)ES_WRITE_DATA_BLOCK << 16)) {
				printf(" response not ready 0x%0x\n",
						 resp);
				rc = -EIO;
			} else {
				break;
			}
			usleep_range(1000, 1000);

		}
		if (rc == -EIO) {
			printf(" write data block error 0x%0x\n",
					 resp);
			goto out;
		}
		

		
		remaining -= size;
	}

	f_close(&fd);
	return 0;

out:
	f_close(&fd);
	return rc;
}

int escore_write( unsigned int reg,
		       unsigned int value)
{
	int ret;
	ret = escore_pm_get_sync();
	if (ret > -1) {
		ret = _escore_write(reg, value);
		escore_pm_put_autosuspend();
	}
	return ret;

}




/*
 * Generic ISR for Audience chips. It is divided mainly into two parts to
 * process interrupts for:
 * 1) chips containing codec
 * 2) chips only having digital component
 */


int escore_start_int_osc(struct escore_priv *escore)
{
	int rc = 0;
	int retry = MAX_RETRY_TO_SWITCH_TO_LOW_POWER_MODE;
	u32 cmd, rsp;



	/* Start internal Osc. */
	cmd = (u32)ES_INT_OSC_MEASURE_START << 16;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf(" escore_cmd fail %d\n", rc);
		goto escore_int_osc_exit;
	}

	/* Poll internal Osc. status */
	do {
		/*
		 * Wait 20ms each time before reading
		 * up to 100ms
		 */
		msleep(20);
		cmd = (u32)ES_INT_OSC_MEASURE_STATUS << 16;
		rc = escore->bus.ops.cmd(escore, cmd, &rsp);
		if (rc) {
			printf( " escore_cmd fail %d\n", rc);
			goto escore_int_osc_exit;
		}
		rsp &= 0xFFFF;
		printf(
			" OSC Measure Status = 0x%04x\n",  rsp);
	} while (rsp && --retry);

	if (rsp > 0) {
		printf(
			" Unexpected OSC Measure Status = 0x%04x\n",
			 rc);
		printf(
			" Can't switch to Low Power Mode\n");
	}

escore_int_osc_exit:
	return rc;
}

int escore_wakeup(struct escore_priv *escore)
{
	u32 cmd = (u32)ES_SYNC_CMD << 16;
	u32 rsp;
	int rc = 0;

	/* Enable the clocks */
	if (escore_priv.pdata->esxxx_clk_cb) {
		escore_priv.pdata->esxxx_clk_cb(1);
		/* Setup for clock stablization delay */
		msleep(ES_PM_CLOCK_STABILIZATION);
	}

	/* Toggle the wakeup pin H->L the L->H */
	if (escore->wakeup_intf == ES_UART_INTF &&
				escore->escore_uart_wakeup) {
		rc = escore->escore_uart_wakeup(escore);
		if (rc) {
			printf(
			"Wakeup failed rc = %d\n",  rc);
			goto escore_wakeup_exit;
		}
	}
	else {
		printf(
			"Wakeup interface not defined\n");
		goto escore_wakeup_exit;
	}
	/* Give the device time to "wakeup" */
	msleep(20);


	rc = escore_priv.bus.ops.cmd(escore, cmd, &rsp);

	if (rc < 0) {
		printf("- failed sync cmd resume\n");
		goto escore_wakeup_exit;
	}
	if (cmd != rsp) {
		printf( "failed sync rsp resume\n");
		goto escore_wakeup_exit;
	}

escore_wakeup_exit:
	return rc;
}


void escore_gpio_reset(struct escore_priv *escore)
{

}

int escore_probe(struct escore_priv *escore,int curr_intf)
{
	int rc = 0;



	/* Update intf_probed only when a valid interface is probed */



	 if (curr_intf != ES_INVAL_INTF)		
			escore->intf_probed |= curr_intf;

	if (escore->intf_probed != (escore->pri_intf | escore->high_bw_intf)) {
		printf(" Both interfaces are not probed\n");
		
		return 0;
	}
	

	if (escore->wakeup_intf == ES_UART_INTF && !escore->uart_ready) {
		printf(" Wakeup mechanism not initialized\n");
		return 0;
	}

	escore->bus.setup_prim_intf(escore);

	rc = escore->bus.setup_high_bw_intf(escore);
	if (rc) {
		printf("Error while setting up high bw interface %d\n", rc);
		goto out;
	}

out:
	return rc;
}



