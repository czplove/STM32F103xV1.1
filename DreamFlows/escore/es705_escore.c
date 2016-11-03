/*
 * es705.c  --  Audience eS705 ALSA SoC Audio driver
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 * Code Updates:
 *       Genisim Tsilker <gtsilker@audience.com>
 *            - Code refactoring
 *            - FW download functions update
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "es705_escore.h"
#include "escore.h"
#include "escore-uart.h"
#include "escore-uart-common.h"
#include "escore-vs.h"
#include "es705-access.h"

#if defined(CONFIG_MQ100_SENSOR_HUB)
#include "es705-api.h"
#endif

u8 ModelBuff[4096];
/* local function proto type */
/* static int es705_dev_rdb(struct escore_priv *es705, void *buf, int len);
static int es705_dev_wdb(struct escore_priv *es705, const void *buf, int len);
*/

#define ES705_CMD_ACCESS_WR_MAX 2
#define ES705_CMD_ACCESS_RD_MAX 2




/* Route state for Internal state management */
enum es705_power_state {
ES705_POWER_BOOT,
ES705_POWER_SLEEP,
ES705_POWER_SLEEP_PENDING,
ES705_POWER_AWAKE
};


enum es705_vs_training_record{
PREVIOUS_KEYWORD,
KEYWORD1_KEYWORD,
KEYWORD2_KEYWORD,
KEYWORD3_KEYWORD,
KEYWORD4_KEYWORD,
KEYWORD5_KEYWORD
};


enum es705_vs_training_status{
BUSY,
SUCCESSFUL,
UTTERANCE_LONG,
UTTERANCE_SHORT,
VERIFICATION_FAILED,
FAILED_BAD_LENGTH
};

#define TRAIN_USER_DEFINE_KEYWORD 2


static const char *power_state_cmd[] = {
	"not defined",
	"sleep",
	"mp_sleep",
	"mp_cmd",
	"normal",
	"vs_overlay",
	"vs_lowpwr",
};

/* codec private data TODO: move to runtime init */
struct escore_priv escore_priv;
struct esxxx_platform_data pdata;

const char *esxxx_mode[] = {
	"SBL",
	"STANDARD",
	"VOICESENSE",
};
int convert_model_to_download_format(char *name, int num);


static int es705_fw_download(struct escore_priv *es705, int fw_type)
{
	int rc = 0;
	struct escore_voice_sense *voice_sense =
		(struct escore_voice_sense *) es705->voice_sense;

	if (fw_type != VOICESENSE && fw_type != STANDARD) {
		printf( "Unknown firmware type\n");
		goto es705_fw_download_failed;
	}
	if (!es705->boot_ops.setup || !es705->boot_ops.finish) {
		printf( "boot setup or finish func undef\n");
		goto es705_fw_download_failed;
	}
	if (es705->bus.ops.high_bw_open) {
		rc = es705->bus.ops.high_bw_open(es705);
		if (rc) {
			printf( "high_bw_open failed %d\n", rc);
			goto es705_high_bw_open_failed;
		}
	}
	rc = es705->boot_ops.setup(es705);
	if (rc) {
		printf( "firmware download start error %d\n", rc);
		goto es705_fw_download_failed;
	}
	if (fw_type == VOICESENSE)
	{
		rc = es705->bus.ops.high_bw_write_file(es705,"VS.BIN");

	}
	else
	{	
		rc = es705->bus.ops.high_bw_write_file(es705,"NS.BIN");

	}
	if (rc) {
		printf( "firmware write error %d\n", rc);
		rc = -EIO;
		goto es705_fw_download_failed;
	}
	es705->mode = fw_type;


	rc = es705->boot_ops.finish(es705);
	if (rc) {
		printf( "firmware download finish error %d\n", rc);
			goto es705_fw_download_failed;
	}

es705_fw_download_failed:
	if (es705->bus.ops.high_bw_close)
		es705->bus.ops.high_bw_close(es705);

es705_high_bw_open_failed:
	return rc;
}



static int es705_init(struct escore_priv *es705)
{
	es705->pm_state = ES705_POWER_AWAKE;
	es705->intf_probed = 0;


	
	es705->pm_state = ES705_POWER_AWAKE;
	es705->es_vs_route_preset = ES705_DMIC0_VS_ROUTE_PREST;
	es705->es_cvs_preset = ES705_DMIC0_CVS_PREST;


 	es705->api_addr_max = ES_API_ADDR_MAX;




	es705->escore_power_state = ES_SET_POWER_STATE_NORMAL;



	es705->sleep_delay = 3000;
	es705->wake_count = 0;


	es705->pri_intf = ES_I2C_INTF;
	es705->high_bw_intf = ES_UART_INTF;
	es705->wakeup_intf = ES_UART_INTF;
	es705->cmd_compl_mode = ES_CMD_COMP_POLL;
	es705->pdata = &pdata;
	es705->pdata->ext_clk_rate = 0x08;
	es705->api_access = es705_api_access;

	

	return 0;

}

#define SIZE_OF_VERBUF 256
static int  es705_fw_version_show(void)
{
	int idx = 0;
	unsigned int value;
	char versionbuffer[SIZE_OF_VERBUF];
	char *verbuf = versionbuffer;

	memset(verbuf, 0, SIZE_OF_VERBUF);


	value = escore_read( ES705_FW_FIRST_CHAR);
	*verbuf++ = (value & 0x00ff);
	for (idx = 0; idx < (SIZE_OF_VERBUF-2); idx++) {
		value = escore_read(ES705_FW_NEXT_CHAR);
		*verbuf++ = (value & 0x00ff);
		if (!value)
			break;
	}
	/* Null terminate the string*/
	printf("firmware version: %s", versionbuffer);

}



static int  es705_sync(void)
{
	int idx = 0;
	u32 value;



	value = escore_read( ES705_SYNC_COMMAND);



}

int es705_bootup(void)
{
	int rc;

	struct escore_priv *es705 = &escore_priv;
	
	es705_init(es705);
	escore_vs_init(es705);
	
	escore_i2c_init();
	escore_uart_probe();
	es705->pm_state = ES705_POWER_BOOT;
	


	rc = es705_fw_download(es705, STANDARD);

	if (rc) {
		printf( "STANDARD fw download error %d\n", rc);
	} else {
		
		es705->pm_state = ES705_POWER_AWAKE;

	}

	
	es705_sync();
	es705_fw_version_show();

	


	


	return rc;
}


int  set_es705_vq_mode(void)
{
	struct escore_priv *es705 = &escore_priv;

	return es705->vs_ops.escore_voicesense_sleep(es705);

}


static int es705_wakeup(struct escore_priv *es705)
{
	int rc = 0;

	rc = escore_wakeup(es705);

	return rc;
}

int process_es70x_interrupt(struct escore_priv *es705)
{
	
	int rc,resp;
	u32 event_type, cmd = 0;
	struct escore_voice_sense *voice_sense =(struct escore_voice_sense *) es705->voice_sense;
	u32 smooth_mute = ES_SET_SMOOTH_MUTE << 16 | ES_SMOOTH_MUTE_ZERO;
	u32 es_set_power_level = ES_SET_POWER_LEVEL << 16 | ES_POWER_LEVEL_6;

	/* Delay required for firmware to be ready in case of CVQ mode */
	msleep(200);

	if (!es705) {
		printf(" Invalid IRQ data\n");
		goto irq_exit;
	}

	/* Enable the clocks if not enabled */
	if (es705->pdata->esxxx_clk_cb) {
		es705->pdata->esxxx_clk_cb(1);
		/* Setup for clock stabilization delay */
		msleep(ES_PM_CLOCK_STABILIZATION);
	}

	if(es705_wakeup(es705))
		goto irq_exit;

	if (es705->cvs_preset != 0xFFFF &&
			es705->cvs_preset != 0) {
		es705->escore_power_state = ES_SET_POWER_STATE_NORMAL;
		es705->mode = STANDARD;
		msleep(30);
	} else if (es705->escore_power_state ==
				ES_SET_POWER_STATE_VS_LOWPWR) {
		es705->escore_power_state = ES_SET_POWER_STATE_VS_OVERLAY;
	}
	cmd = ES_GET_EVENT << 16;

	rc = escore_cmd(es705, cmd, &event_type);
	if (rc < 0) {
		printf("Error reading IRQ event\n");
		goto irq_exit;
	}
	event_type &= ES_MASK_INTR_EVENT;
	es705->escore_event_type = event_type;

	if (event_type != ES_NO_EVENT) {


	printf("Event: 0x%04x\n", (u32)event_type);

	if ((event_type & 0xFF) != ES_VS_INTR_EVENT) {
		printf( "Invalid event callback 0x%04x\n",
				 (u32) event_type);
		return NOTIFY_DONE;
	}
	printf( "VS event detected 0x%04x\n",
				 (u32) event_type);

	if (voice_sense->cvs_preset != 0xFFFF && voice_sense->cvs_preset != 0) {
		es705->escore_power_state = ES_SET_POWER_STATE_NORMAL;
		es705->mode = STANDARD;
		rc = escore_reconfig_intr(es705);
		if (rc) {
			printf(
				"reconfig interrupt failed rc = %d\n",
				 rc);
			return NOTIFY_DONE;
		}
	}


	voice_sense->vs_get_event = event_type;


	/* If CVS preset is set (other than 0xFFFF), earSmart chip is
	 * in CVS mode. To make it switch from internal to external
	 * oscillator, send power level command with highest power
	 * level
	 */
	if (voice_sense->cvs_preset != 0xFFFF &&
			voice_sense->cvs_preset != 0) {

		rc = escore_cmd(es705, smooth_mute, &resp);
		if (rc < 0) {
			printf("Error setting smooth mute\n");
			goto irq_exit;
		}
		usleep_range(2000, 2005);
		rc = escore_cmd(es705, es_set_power_level, &resp);
		if (rc < 0) {
			printf("Error setting power level\n");
			goto irq_exit;
		}
		usleep_range(2000, 2005);

		/* Each time earSmart chip comes in BOSKO mode after
		 * VS detect, CVS mode will be disabled */
		voice_sense->cvs_preset = 0;
	}
	}

irq_exit:
	return rc;

}

int es705_interrupt_service(void)
{

	struct escore_priv *es705 = &escore_priv;
	return process_es70x_interrupt(es705);

}
static int es705_start_int_osc(void)
{
	int rc = 0;
	int retry = MAX_RETRY_TO_SWITCH_TO_LOW_POWER_MODE;


	/* Start internal Osc. */
	rc = escore_write(ES705_VS_INT_OSC_MEASURE_START, 0);
	if (rc) {
		printf(
			"OSC Measure Start fail\n");
		return rc;
	}

	/* Poll internal Osc. status */
	do {
		/*
		 * Wait 20ms each time before reading
		 * up to 100ms
		 */
		msleep(20);
		rc = escore_read(ES705_VS_INT_OSC_MEASURE_STATUS);

		if (rc < 0) {
			printf(
				"OSC Measure Read Status fail\n");
			break;
		}
		printf(
			"OSC Measure Status = 0x%04x\n", rc);
	} while (rc && --retry);

	if (rc > 0) {
		printf(
			"Unexpected OSC Measure Status = 0x%04x\n", rc);
		printf(
			"Can't switch to Low Power Mode\n");
	}

	return rc;
}




static int es705_power_transition(int next_power_state,
				unsigned int set_power_state_cmd)
{
	struct escore_priv *escore = &escore_priv;
	int rc = 0;
	int reconfig_intr = 0;

	while (next_power_state != escore->escore_power_state) {
		switch (escore->escore_power_state) {
		case ES_SET_POWER_STATE_SLEEP:
			/* Wakeup Chip */
			rc = es705_wakeup(escore);
			if (rc) {
				printf(" Wakeup failed: %d\n",
						 rc);
				goto es705_power_transition_exit;
			}
			escore->escore_power_state = ES_SET_POWER_STATE_NORMAL;
			break;
		case ES_SET_POWER_STATE_NORMAL:

			/* Either switch to Sleep or VS Overlay mode */
			if (next_power_state == ES_SET_POWER_STATE_SLEEP)
				escore->escore_power_state =
					ES_SET_POWER_STATE_SLEEP;
			else
				escore->escore_power_state =
					ES_SET_POWER_STATE_VS_OVERLAY;

			rc = escore_write(set_power_state_cmd,
					escore->escore_power_state);
			if (rc) {
				printf(" Failed to set power state :%d\n",
					 rc);
				escore->escore_power_state =
					ES_SET_POWER_STATE_NORMAL;
				goto es705_power_transition_exit;
			}

			/* VS fw download */
			if (escore->escore_power_state ==
					ES_SET_POWER_STATE_VS_OVERLAY) {
				/* wait es705 SBL mode */
				msleep(50);
				rc = escore_vs_load(&escore_priv);
				if (rc) {
					printf(" vs fw downlaod failed\n");
					goto es705_power_transition_exit;
				}
			}
			break;
		case ES_SET_POWER_STATE_VS_OVERLAY:
			/* Either switch to VS low power or Normal mode */
			if (next_power_state == ES_SET_POWER_STATE_VS_LOWPWR) {
				/* Start internal oscillator */
				rc = es705_start_int_osc();
				if (rc)
					goto es705_power_transition_exit;

				escore->escore_power_state =
					ES_SET_POWER_STATE_VS_LOWPWR;

			} else {
				escore->escore_power_state =
					ES_SET_POWER_STATE_NORMAL;
				escore->mode = STANDARD;
				reconfig_intr = 1;
			}

			rc = escore_write(set_power_state_cmd,
					escore->escore_power_state);
			if (rc) {
				printf(" Power state cmd write failed\n");
				escore->escore_power_state =
					ES_SET_POWER_STATE_VS_OVERLAY;
				goto es705_power_transition_exit;
			}

			if (escore->escore_power_state ==
					ES_SET_POWER_STATE_VS_LOWPWR)
				/* Disable the clocks */
				if (escore->pdata->esxxx_clk_cb)
					escore->pdata->esxxx_clk_cb(0);

			escore->pm_state = ES_PM_ASLEEP;

			if (reconfig_intr) {
				msleep(20);
				rc = escore_reconfig_intr(&escore_priv);
				if (rc < 0) {
					printf(
					" Interrupt config failed :%d\n",
							 rc);
					goto es705_power_transition_exit;
				}
			}
			break;
		case ES_SET_POWER_STATE_VS_LOWPWR:
			/* Wakeup Chip */
			rc = es705_wakeup(&escore_priv);
			if (rc) {
				printf("es705 wakeup failed\n");
				goto es705_power_transition_exit;
			}
			escore_priv.escore_power_state =
				ES_SET_POWER_STATE_VS_OVERLAY;
			break;
		default:
			printf("Unsupported state in es705\n");
			rc = -EINVAL;
			goto es705_power_transition_exit;
		}
	}
	printf( "Power state change successful\n");
es705_power_transition_exit:
	return rc;
}

int es705_download_training_bkg(struct escore_priv *es705)
{
	int rc = 0;
	
	rc = escore_datablock_write(es705,"TBKG.BIN");
	if (rc) {
		printf( "firmware write BKG error %d\n", rc);
		rc = -EIO;
		goto write_bkg_error;
	}
	
write_bkg_error:
	return rc;
}

int setVoiceSenseTrainingRoutes(struct escore_priv *es705)
{
	u32 cmd, rsp;
	int rc;

	cmd = (u32)ES_SET_PRESET << 16 | 0x566;
	rc = es705->bus.ops.cmd(es705, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail %d\n",
					 rc);
		goto route_err;
	}
	
route_err:
	return rc;

}


int copy_training_model_file_from_es705(struct escore_priv *es705, char *name)
{

	int rc;
	int size;
	int check;
	u32 cmd;
	int rdcnt = 0;
	u32 resp;
	u8 flush_extra_blk = 0;
	u32 flush_buf;


	
	 FILINFO finfo;
	 FIL fd;
	


	
	/* Reset read data block size */
	es705->datablock_dev.rdb_read_count = 0;




	cmd = ((u32)ES_READ_DATA_BLOCK << 16) | 0x07;

	rc = es705->bus.ops.high_bw_cmd(es705, cmd, &resp);
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
	if (size == 0 || size % 4 != 0) {
		printf(" Read Data Block with invalid size:%d\n",
				 size);
		rc = -EINVAL;
		goto out;
	}



	rc = es705->bus.ops.high_bw_read(es705, ModelBuff, size);
	if (rc < 0) {
		printf(" Read Data Block error %d\n",
				 rc);
		goto out;
	}
	
	 rc = f_open(&fd, name, FA_WRITE | FA_CREATE_ALWAYS); 	//以读的方式打开文件
	 if(rc < 0)
	 	printf("open file error");


	rc = f_write(&fd, ModelBuff,size,&check);

	 if(rc < 0)
	 	printf("f_write fail %d" , rc);		

	 f_close(&fd);
	
	
	
out:

	return rc;

}


static int convert_model_to_download_format(char *name, int num)
{
	 FILINFO finfo;
	 FIL fd;
	 FIL final_fd;
	 UINT readsize1, readsize2;
	 int  rc;
	 int  loop_cntr = 0;
	 int  blk_seq = 0;
	 int f_in_len;
	 u8 buf[4];
	 int no_of_kw = 0, count = 0, kw_sel_flag = 0;
	 int size;
	 int j;
	 u8 data;
	 
 	 u8 version = 0x00;
	 u8 byte_0f = 0x0F, byte_ff = 0xFF, byte_f0 = 0xF0;
	 
	 finfo = OutPutFile(name);
	 
	 rc = f_open(&fd, name, FA_OPEN_EXISTING | FA_READ); 	//以读的方式打开文件
	 if(rc < 0)
	 	printf("open file error %s rc = %d" , name,rc);


	 rc = f_open(&final_fd, "TKW1.BIN",FA_WRITE | FA_CREATE_ALWAYS); 	//以读的方式打开文件
	 if(rc < 0)
	 	printf("open file error TKW1.BIN rc =%d", rc);
	 f_in_len = finfo.fsize;
	 
		
		while (loop_cntr < f_in_len) {
			size = 512;

			if (0 != loop_cntr)
				size = 508;

			if (loop_cntr + size > f_in_len)
				size = (int) (f_in_len - loop_cntr);

			// read chunk, last chunk can be of lesser size
			
			rc = f_read(&fd, ModelBuff, size, &readsize1);
		 	if(rc < 0)
		 		printf("f_read fail %d" , rc);	
	

			count = readsize1;
			printf("#### (byte counter %d, req sz %d, read sz %d\n" ,loop_cntr,size, count);
			if (0 == loop_cntr) {
				// Insert KW ID and block sequence #
				version = (ModelBuff[2] & byte_0f);
				ModelBuff[2] = ((( num+ 1) << 4 & byte_f0) | version);

				buf[0] = ModelBuff[0];
				buf[1] = ModelBuff[1];
				buf[2] = ModelBuff[2];
				buf[3] = ModelBuff[3];
			}

			// Insert block sequence
			if (loop_cntr + count < f_in_len && size == count)
				buf[3] =  (blk_seq++);
			else
				buf[3] = byte_ff;

			// write header for second chunk onwards
			// [reserved][kw type][kw ID | version][chunk #]
			if (0 != loop_cntr) {
				for ( j = 0; j < 4; j++)
					f_write(&final_fd,&buf[j], 1, &readsize2);
					printf("readsize2 =%d" ,readsize2);
			}

			// write chunk to destination file

				f_write(&final_fd,ModelBuff, count, &readsize2);

			
			loop_cntr += count;

		}
		f_close(&fd);
		f_close(&final_fd);
	


}
	 


int set_es705_to_training_mode(void)
{
	 int  status = 0;
	 int count = 0;
	struct escore_priv *escore = &escore_priv;

	es705_power_transition(ES_SET_POWER_STATE_NORMAL,ES705_POWER_STATE);
	msleep(100);
	es705_power_transition(ES_SET_POWER_STATE_VS_OVERLAY,ES705_POWER_STATE);
	es705_fw_version_show();
	setVoiceSenseTrainingRoutes(escore);
	es705_download_training_bkg(escore);
	escore_write(ES705_VOICE_SENSE_TRAINING_MODE, TRAIN_USER_DEFINE_KEYWORD);
	escore_write(ES705_VOICE_SENSE_TRAINING_RECORD, KEYWORD1_KEYWORD);

	for(;;)
	{
	

		status = escore_read(ES705_VOICE_SENSE_TRAINING_STATUS);
		printf("status = %d", status);

		switch(status)
		{
			case BUSY:
				break;
			case SUCCESSFUL:
				count ++;
				printf("count = %d", count);
				escore_write(ES705_VOICE_SENSE_TRAINING_RECORD, PREVIOUS_KEYWORD);
				break;
			case UTTERANCE_LONG:
			case UTTERANCE_SHORT:
			case VERIFICATION_FAILED:
			case FAILED_BAD_LENGTH:
			default:
				escore_write(ES705_VOICE_SENSE_TRAINING_RECORD, PREVIOUS_KEYWORD);
				break;
				
		}

		if( count  == 4)
			break;
		msleep(1000);
		

	}

	copy_training_model_file_from_es705(escore,"TRAIN1.BIN");
	convert_model_to_download_format("TRAIN1.BIN", 2);
	es705_power_transition(ES_SET_POWER_STATE_NORMAL,ES705_POWER_STATE);
	
	return 0;

}
