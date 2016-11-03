/*
 * escore-vs.c  --  Audience Voice Sense component ALSA Audio driver
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "escore.h"
#include "escore-vs.h"

struct escore_voice_sense voice_sense_data;
int escore_is_sleep_aborted(struct escore_priv *escore)
{
	if (escore->sleep_abort)
		return -EABORT;
	else
		return 0;
}

static int escore_vs_sleep(struct escore_priv *escore)
{
	struct escore_voice_sense *voice_sense =
			(struct escore_voice_sense *) escore_priv.voice_sense;
	u32 cmd, rsp;
	int rc;




		/* Set smooth rate */
	cmd = ((u32)ES_SET_SMOOTH << 16) | ES_SET_SMOOTH_RATE;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd set smooth rate fail %d\n",
					 rc);
		goto vs_sleep_err;
	}
	

	/* change power state to OVERLAY */
	cmd = ((u32)ES_SET_POWER_STATE << 16) | ES_SET_POWER_STATE_VS_OVERLAY;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail %d\n",
					 rc);
		goto vs_sleep_err;
	}

	msleep(20);

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	/* download VS firmware */
	rc = escore_vs_load(escore);
	if (rc) {
		printf( "%s() VS FW load failed rc = %d\n",
					 rc);
		goto vs_sleep_err;
	}

	escore->escore_power_state = ES_SET_POWER_STATE_VS_OVERLAY;

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	cmd = (u32)ES_SET_PRESET << 16 | escore->es_vs_route_preset;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail %d\n",
					 rc);
		goto vs_sleep_err;
	}

	voice_sense->cvs_preset = escore->es_cvs_preset;
	cmd = (u32)ES_SET_CVS_PRESET << 16 | escore->es_cvs_preset;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail second%d\n",
					 rc);
		goto vs_sleep_err;
	}

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	/* write background model and keywords files */
	rc = escore_vs_write_bkg_and_keywords(escore);
	if (rc) {
		printf(
			" datablock write fail rc = %d\n",
			 rc);
		goto vs_sleep_err;
	}

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	cmd = (u32)ES_SET_ALGO_PARAM_ID << 16 | ES_VS_PROCESSING_MOE;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail %d\n",
				 rc);
		goto vs_sleep_err;
	}

	cmd = (u32)ES_SET_ALGO_PARAM << 16 | ES_VS_DETECT_KEYWORD;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail second %d\n",
					 rc);
		goto vs_sleep_err;
	}

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;
#if 1
	rc  = escore_start_int_osc(escore);
	if (rc) {
		printf( " int osc fail %d\n",  rc);
		goto vs_sleep_err;
	}

	cmd = ((u32)ES_SET_POWER_STATE << 16) | ES_SET_POWER_STATE_VS_LOWPWR;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf( " escore_cmd fail third %d\n",
				 rc);
		goto vs_sleep_err;
	}
	escore->escore_power_state = ES_SET_POWER_STATE_VS_LOWPWR;
#endif
escore_sleep_aborted:
vs_sleep_err:

	return rc;
}

int escore_vs_wakeup(struct escore_priv *escore)
{
	u32 cmd, rsp;
	int rc;



	escore->sleep_abort = 1;


	rc = escore_wakeup(escore);
	if (rc) {
		printf( "%s() wakeup failed rc = %d\n",
					 rc);
		goto vs_wakeup_err;
	}

	escore->escore_power_state = ES_SET_POWER_STATE_VS_OVERLAY;

	/* change power state to Normal*/
	cmd = ((u32)ES_SET_POWER_STATE << 16) | ES_SET_POWER_STATE_NORMAL;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc < 0) {
		printf( " - failed sync cmd resume\n");
		escore->pm_state = ES_PM_HOSED;
		goto vs_wakeup_err;
	} else {
		/* Time required for switching from VS to NS mode.
		 * This delay should be replaced with GPIO A event */
		msleep(50);

		escore->mode = STANDARD;
		escore->pm_state = ES_PM_NORMAL;
		escore->escore_power_state = ES_SET_POWER_STATE_NORMAL;
	}
vs_wakeup_err:
	escore->sleep_abort = 0;
	return rc;
}

static int escore_cvq_sleep_thread(void *ptr)
{
	struct escore_priv *escore = (struct escore_priv *)ptr;
	int rc;
	u32 cmd, rsp;


	rc = escore_vs_sleep(&escore_priv);
	if (rc != -EABORT)
	{
				printf(
			"escore_cvq_sleep_thread_exit\n");
		goto escore_cvq_sleep_thread_exit;


	}

				printf(
			"escore_cvq_sleep_thread Normal\n");
	/* change power state to Normal*/
	cmd = ((u32)ES_SET_POWER_STATE << 16) | ES_SET_POWER_STATE_NORMAL;
	rc = escore->bus.ops.cmd(escore, cmd, &rsp);
	if (rc) {
		printf(
			"Power State Normal failed rc = %d\n",
			 rc);
	}
	escore_priv.escore_power_state = ES_SET_POWER_STATE_NORMAL;

escore_cvq_sleep_thread_exit:

	return rc;
}

static int escore_voicesense_sleep(struct escore_priv *escore)
{
	return escore_cvq_sleep_thread((void *)escore);
}














int escore_vs_request_firmware(struct escore_priv *escore,
				const char *vs_filename)
{
  return 0;
}

void escore_vs_release_firmware(struct escore_priv *escore)
{
  return;
}

int escore_vs_request_bkg(struct escore_priv *escore, const char *bkg_filename)
{
  return 0;
}

void escore_vs_release_bkg(struct escore_priv *escore)
{
  return;
}

int escore_vs_request_keywords(struct escore_priv *escore)
{

	int rc = 0;

	return  rc;
}

int escore_vs_write_bkg_and_keywords(struct escore_priv *escore)
{
	int rc = 0;
	
	
	printf("start download DBKG.BIN");
	rc = escore_datablock_write(escore,"DBKG.BIN");
	if (rc) {
		printf( "firmware write BKG error %d\n", rc);
		rc = -EIO;
		goto write_bkg_error;
	}
	printf("start download OEM.BIN");
	rc = escore_datablock_write(escore,"OEM.BIN");
	if (rc) {
		printf( "firmware write KW1 error %d\n", rc);
		rc = -EIO;
		goto write_bkg_error;
	}
	#if 1
	printf("start download TKW.BIN");
	rc = escore_datablock_write(escore,"TKW1.BIN");
	if (rc) {
		printf( "firmware write TKW1 error %d\n", rc);
		rc = -EIO;
		goto write_bkg_error;
	}
	#endif
	
write_bkg_error:


	return rc;


}

static int escore_vs_isr(struct notifier_block *self, unsigned long action,
		void *dev)
{
	struct escore_priv *escore = (struct escore_priv *)dev;
	struct escore_voice_sense *voice_sense =
		(struct escore_voice_sense *) escore->voice_sense;
	u32 smooth_mute = (u32)ES_SET_SMOOTH_MUTE << 16 | ES_SMOOTH_MUTE_ZERO;
	u32 es_set_power_level = (u32)ES_SET_POWER_LEVEL << 16 | ES_POWER_LEVEL_6;
	u32 resp;
	int rc = 0;

	printf( " Event: 0x%04x\n",  action);

	if ((action & 0xFF) != ES_VS_INTR_EVENT) {
		printf( " Invalid event callback 0x%04x\n",
				 (u32) action);
		return NOTIFY_DONE;
	}
	printf( " VS event detected 0x%04x\n",
				 (u32) action);

	if (voice_sense->cvs_preset != 0xFFFF && voice_sense->cvs_preset != 0) {
		escore->escore_power_state = ES_SET_POWER_STATE_NORMAL;
		escore->mode = STANDARD;
		rc = escore_reconfig_intr(escore);
		if (rc) {
			printf(
				"%s() reconfig interrupt failed rc = %d\n",
				 rc);
			return NOTIFY_DONE;
		}
	}

	voice_sense->vs_get_event = action;

	/* If CVS preset is set (other than 0xFFFF), earSmart chip is
	 * in CVS mode. To make it switch from internal to external
	 * oscillator, send power level command with highest power
	 * level
	 */
	if (voice_sense->cvs_preset != 0xFFFF &&
			voice_sense->cvs_preset != 0) {

		rc = escore_cmd(escore, smooth_mute, &resp);
		if (rc < 0) {
			printf(" Error setting smooth mute\n");
			goto voiceq_isr_exit;
		}
		usleep_range(2000, 2005);
		rc = escore_cmd(escore, es_set_power_level, &resp);
		if (rc < 0) {
			printf(" Error setting power level\n");
			goto voiceq_isr_exit;
		}
		usleep_range(2000, 2005);

		/* Each time earSmart chip comes in BOSKO mode after
		 * VS detect, CVS mode will be disabled */
		voice_sense->cvs_preset = 0;
	}


	return NOTIFY_OK;

voiceq_isr_exit:
	return NOTIFY_DONE;
}



void escore_vs_init_intr(struct escore_priv *escore)
{

}


int escore_vs_sleep_enable(struct escore_priv *escore)
{
	struct escore_voice_sense *voice_sense =
			(struct escore_voice_sense *) escore->voice_sense;
	return voice_sense->vs_active_keywords;
}

int escore_vs_load(struct escore_priv *escore)
{
	struct escore_voice_sense *voice_sense =
			(struct escore_voice_sense *) escore->voice_sense;

	int rc = 0;

	
	escore->mode = VOICESENSE_PENDING;

	if (!escore->boot_ops.setup || !escore->boot_ops.finish) {
		printf(
			" boot setup or finish function undefined\n");
		rc = -EIO;
		goto escore_vs_uart_open_failed;
	}

	if (escore->bus.ops.high_bw_open) {
		rc = escore->bus.ops.high_bw_open(escore);
		if (rc) {
			printf( " high_bw_open failed %d\n",
				 rc);
			goto escore_vs_uart_open_failed;
		}
	}

	rc = escore->boot_ops.setup(escore);
	if (rc) {
		printf( " fw download start error\n");
		goto escore_vs_fw_download_failed;
	}




	rc = escore->bus.ops.high_bw_write_file(escore,
		"VS.BIN");
	if (rc) {
		printf( " vs firmware image write error\n");
		rc = -EIO;
		goto escore_vs_fw_download_failed;
	}

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	escore->mode = VOICESENSE;

	if (((struct escore_voice_sense *)escore->voice_sense)->vs_irq != TRUE)
		escore_vs_init_intr(escore);

	rc = escore->boot_ops.finish(escore);
	if (rc) {
		printf( "vs fw download finish error\n");
		goto escore_vs_fw_download_failed;
	}

	rc = escore_is_sleep_aborted(escore);
	if (rc == -EABORT)
		goto escore_sleep_aborted;

	rc = escore_reconfig_intr(escore);
	if (rc) {
		printf( "%s() config resume failed after VS load rc = %d\n",
			 rc);
		goto escore_vs_fw_download_failed;
	}



escore_sleep_aborted:
escore_vs_fw_download_failed:
	if (escore->bus.ops.high_bw_close) {
		rc = escore->bus.ops.high_bw_close(escore);
		if (rc) {
			printf( " high_bw_close failed %d\n",
				 rc);
		}
	}
escore_vs_uart_open_failed:

	return rc;
}



void escore_vs_exit(struct escore_priv *escore)
{


}

int escore_vs_init(struct escore_priv *escore)
{
	int rc = 0;
	int i;

	struct escore_voice_sense *voice_sense;
	voice_sense = &voice_sense_data;


	escore->voice_sense = (void *)voice_sense;

	/* Initialize variables */
	voice_sense->cvs_preset = 0;
	voice_sense->vs_active_keywords = 0;

	voice_sense->vs = NULL; 


	voice_sense->bkg = NULL;

	for (i = 0; i < MAX_NO_OF_VS_KW; i++) {
		voice_sense->kw[i] = NULL;

	}



	escore->vs_ops.escore_is_voicesense_sleep_enable =
					escore_vs_sleep_enable;

	
	escore->vs_ops.escore_voicesense_sleep = escore_voicesense_sleep;

	printf("escore->vs_ops.escore_voicesense_sleep1  %x\n",escore->vs_ops.escore_voicesense_sleep );
	escore->vs_ops.escore_voicesense_wakeup = escore_vs_wakeup;

	return rc;

kw_alloc_err:

bkg_alloc_err:
sysfs_init_err:
vs_alloc_err:
voice_sense_alloc_err:
	return rc;
}
