/*
 * escore.h  --  Audience earSmart Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ESCORE_H
#define _ESCORE_H

#include <string.h>              
#include <math.h>
#include "escore-uart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
//-#include "stm32f10x.h"   //-������ϰ汾��ͷ�ļ�,���µ�������Ҫȥ��
#include "stm32f10x_dma.h"
#include "stm32f10x_i2c.h"
//-#include "platform_config.h"
#include "stdint.h"
#include "ff.h"
#define DEBUG

#ifdef DEBUG 

#define log(format,arg)	USART_OUT(USART1,format,arg);

#else

#define log(format,arg)

#endif
#define BIT(nr)  (1UL << (nr))

#define min(x,y) ((x) < (y) ? x:y)

#define TX_BUFF_SIZE 512

//FATFS fs2;  
#define RX_BUFF_SIZE  4096

#define 	NOTIFY_DONE      	0x0000
#define 	NOTIFY_OK			0x0001

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 	 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC	 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK	15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR	20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY	26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */
#define 	ETIMEDOUT	60



#define ES_EXT_CLK_6000KHZ  0x00 /* 6.0 MHz    */
#define ES_EXT_CLK_6144KHZ  0x01 /* 6.144 MHz  */
#define ES_EXT_CLK_9600KHZ  0x02 /* 9.6 MHz    */
#define ES_EXT_CLK_12000KHZ 0x03 /* 12.0 MHz   */
#define ES_EXT_CLK_12288KHZ 0x04 /* 12.288 MHz */
#define ES_EXT_CLK_13000KHZ 0x05 /* 13.0 MHz   */
#define ES_EXT_CLK_15360KHZ 0x06 /* 15.36 MHz  */
#define ES_EXT_CLK_18432KHZ 0x07 /* 18.432 MHz */
#define ES_EXT_CLK_19200KHZ 0x08 /* 19.2 MHz   */
#define ES_EXT_CLK_24000KHZ 0x09 /* 24 MHz     */
#define ES_EXT_CLK_24576KHZ 0x0a /* 24.576 MHz */
#define ES_EXT_CLK_26000KHZ 0x0b /* 26 MHz     */

/*
 * IRQ type
 */

enum {
	ES_DISABLED,
	ES_ACTIVE_LOW,
	ES_ACTIVE_HIGH,
	ES_FALLING_EDGE,
	ES_RISING_EDGE,
};

enum {
	ES_CMD_COMP_POLL,
	ES_CMD_COMP_INTR,
};


#define ES_READ_VE_OFFSET		0x0804
#define ES_READ_VE_WIDTH		4
#define ES_WRITE_VE_OFFSET		0x0800
#define ES_WRITE_VE_WIDTH		4

#define ES_CMD_ACCESS_WR_MAX 9
#define ES_CMD_ACCESS_RD_MAX 9


#define ES_I2S_PORTA		7
#define ES_I2S_PORTB		8
#define ES_I2S_PORTC		9
#define EABORT			0x5555

/* TODO: condition of kernel version or commit code to specific kernels */



/* Standard commands used by all chips */

#define ES_SR_BIT			28
#define ES_SYNC_CMD			0x8000
#define ES_SYNC_POLLING			0x0000
#define ES_SYNC_ACK			0x80000000
#define ES_SUPRESS_RESPONSE		0x1000
#define ES_SET_ALGO_PARAM_ID		0x9017
#define ES_SET_ALGO_PARAM		0x9018

#define ES_SET_EVENT_RESP		0x901A


#define ES_SET_SMOOTH			0x904E
#define ES_SET_SMOOTH_RATE		0x0000

#define ES_GET_POWER_STATE		0x800f
#define ES_SET_POWER_STATE_CMD		0x8010
#define ES_SET_POWER_STATE		0x9010
#define ES_SET_POWER_STATE_SLEEP	0x0001
#define ES_SET_POWER_STATE_MP_SLEEP	0x0002
#define ES_SET_POWER_STATE_MP_CMD	0x0003
#define ES_SET_POWER_STATE_NORMAL	0x0004
#define ES_SET_POWER_STATE_VS_OVERLAY	0x0005
#define ES_SET_POWER_STATE_VS_LOWPWR	0x0006

#define ES_SYNC_MAX_RETRY 5
#define ES_SBL_RESP_TOUT	500 /* 500ms */
#define MAX_RETRY_TO_SWITCH_TO_LOW_POWER_MODE 5

#define ES_NOT_READY		0x00000000
#define ES_ILLEGAL_CMD		0xFFFF0000

#define ES_EVENT_RESPONSE_CMD	0x801a
#define ES_INT_OSC_MEASURE_START	0x9070
#define ES_INT_OSC_MEASURE_STATUS	0x8071

/*
 * Codec Interrupt event type
 */
#define ES_NO_EVENT			0x0000
#define ES_CODEC_INTR_EVENT		0x0001
#define ES_VS_INTR_EVENT		0x0002
#define ES_MASK_INTR_EVENT		0x0000FFFF
/*
 * Interrupt status bits
 */

/* Specific to A212 Codec */
#define ES_IS_CODEC_READY(x)		(x & 0x80)
#define ES_IS_THERMAL_SHUTDOWN(x)	(x & 0x100)
#define ES_IS_LO_SHORT_CKT(x)		(x & 0x200)
#define ES_IS_HFL_SHORT_CKT(x)		(x & 0x400)
#define ES_IS_HFR_SHORT_CKT(x)		(x & 0x800)
#define ES_IS_HP_SHORT_CKT(x)		(x & 0x1000)
#define ES_IS_EP_SHORT_CKT(x)		(x & 0x2000)
#define ES_IS_PLUG_EVENT(x)		(x & 0x40)
#define ES_IS_UNPLUG_EVENT(x)		(x & 0x20)

/*
 * Accessory status bits
 */
#define ES_IS_ACCDET_EVENT(x)			(x & 0x10)
#define ES_IS_SHORT_BTN_PARALLEL_PRESS(x)	(x & 0x01)
#define ES_IS_LONG_BTN_PARALLEL_PRESS(x)	(x & 0x02)
#define ES_IS_SHORT_BTN_SERIAL_PRESS(x)		(x & 0x04)
#define ES_IS_LONG_BTN_SERIAL_PRESS(x)		(x & 0x08)
#define ES_IS_BTN_PRESS_EVENT(x)		(x & 0x0f)

#define ES_IS_LRGM_HEADSET(x)			(x == 1)
#define ES_IS_LRMG_HEADSET(x)			(x == 3)
#define ES_IS_LRG_HEADPHONE(x)			(x == 2)
#define ES_IS_HEADSET(x) (ES_IS_LRGM_HEADSET(x) || ES_IS_LRMG_HEADSET(x))

#define ES_ACCDET_ENABLE	1
#define ES_ACCDET_DISABLE	0

#define ES_BTNDET_ENABLE	1
#define ES_BTNDET_DISABLE	0

#define ES_SYNC_POLLING                0x0000
#define ES_SYNC_INTR_ACITVE_LOW        0x0001
#define ES_SYNC_INTR_ACITVE_HIGH       0x0002
#define ES_SYNC_INTR_FALLING_EDGE      0x0003
#define ES_SYNC_INTR_RISING_EDGE       0x0004

#define ES_WDB_CMD			0x802f
#define ES_RDB_CMD			0x802e
#define ES_WDB_MAX_SIZE			512


#define ES_SET_PRESET			0x9031
#define ES_SET_CVS_PRESET		0x906F
#define ES_VS_PROCESSING_MOE		0x5003
#define ES_VS_DETECT_KEYWORD		0x0000


#define ES_READ_DATA_BLOCK				0x802E
#define ES_WRITE_DATA_BLOCK				0x802F
/* SPI sends data in Big endian format.*/
#define ES_READ_DATA_BLOCK_SPI				0x2E80
#define ES_WRITE_DATA_BLOCK_SPI				0x2F80

#define ES_SET_SMOOTH_MUTE				0x804E
#define ES_SMOOTH_MUTE_ZERO				0x0000

#define ES_SET_POWER_LEVEL				0x8011
#define ES_POWER_LEVEL_6				0x0006

#define ES_WAKEUP_TIME				30
#define ES_PM_CLOCK_STABILIZATION		1 /* 1ms */
#define ES_RESP_TOUT_MSEC			20 /* 20ms */
#define ES_RESP_TOUT				20000 /* 20ms */
#define ES_RESP_POLL_TOUT			4000  /*  4ms */
#define ES_FIN_TOUT				10000 /* 10ms */
#define ES_FIN_POLL_TOUT			2000  /* 2ms */
#define ES_MAX_RETRIES		\
			(ES_RESP_TOUT / ES_RESP_POLL_TOUT)
#define ES_MAX_FIN_RETRIES		\
			(ES_FIN_TOUT / ES_FIN_POLL_TOUT)

#define ES_SPI_RETRY_DELAY 1000  /*  1ms */
#define ES_SPI_MAX_RETRIES 500 /* Number of retries */
#define ES_SPI_CONT_RETRY 25 /* Retry for read without delay */
#define ES_SPI_1MS_DELAY 1000  /*1 ms*/

#define ES_UART_WAKE_CMD	0xa

enum {
	SBL,
	STANDARD,
	VOICESENSE_PENDING,
	VOICESENSE,
};

struct escore_reg_cache {
	int value;
	int is_volatile;
};

struct escore_api_access {
	u32 read_msg[ES_CMD_ACCESS_RD_MAX];
	unsigned int read_msg_len;
	u32 write_msg[ES_CMD_ACCESS_WR_MAX];
	unsigned int write_msg_len;
	unsigned int val_shift;
	unsigned int val_max;
};

#define ES_INVAL_INTF		(0x1 << 0)
#define ES_SLIM_INTF		(0x1 << 1)
#define ES_I2C_INTF		(0x1 << 2)
#define ES_SPI_INTF		(0x1 << 3)
#define ES_UART_INTF		(0x1 << 4)


enum {
	ES_MSG_READ,
	ES_MSG_WRITE,
};


enum {
	ES_PM_NORMAL,
	ES_PM_RUNTIME_SLEEP,
	ES_PM_ASLEEP,
	ES_PM_HOSED,
};
/* Notifier chain priority */
enum {
	ES_LOW,
	ES_NORMAL,
	ES_HIGH,
};



/* Maximum size of keyword parameter block in bytes. */
#define ESCORE_VS_KEYWORD_PARAM_MAX 512

struct es_stream_device {
	int (*open)(struct escore_priv *escore);
	int (*read)(struct escore_priv *escore, void *buf, int len);
	int (*close)(struct escore_priv *escore);
	int intf;
	int no_more_bit;
};
struct es_datablock_device {
	int (*open)(struct escore_priv *escore);
	int (*read)(struct escore_priv *escore, void *buf, int len);
	int (*close)(struct escore_priv *escore);
	int (*wait)(struct escore_priv *escore);
	char *rdb_read_buffer;
	int rdb_read_count;
};

struct escore_intr_regs {
	u32 get_intr_status;
	u32 clear_intr_status;
	u32 set_intr_mask;
	u32 accdet_config;
	u32 accdet_status;
	u32 enable_btndet;

	u32 btn_serial_cfg;
	u32 btn_parallel_cfg;
	u32 btn_detection_rate;
	u32 btn_press_settling_time;
	u32 btn_bounce_time;
	u32 btn_long_press_time;
};

struct escore_flags {
	u32 is_codec:1;
	u32 is_fw_ready:1;
	u32 local_slim_ch_cfg:1;
	u32 rx1_route_enable:1;
	u32 tx1_route_enable:1;
	u32 rx2_route_enable:1;
	u32 reset_done:1;
/* 705 */
	u32 vs_enable:1;
	u32 sleep_enable:1;
	u32 sleep_abort:1;
	u32 ns:1;
	u32 zoom:2;	/* Value can be in range 0-3 */
};

/*Generic boot ops*/
struct escore_boot_ops {
	int (*setup)(struct escore_priv *escore);
	int (*finish)(struct escore_priv *escore);
	int (*bootup)(struct escore_priv *escore);
	int (*escore_abort_config)(struct escore_priv *escore);
};

/*Generic Bus ops*/
struct escore_bus_ops {
	int (*read)(struct escore_priv *escore, void *buf, int len);
	int (*write)(struct escore_priv *escore,
			const void *buf, int len);
	int (*high_bw_write)(struct escore_priv *escore,
			const void *buf, int len);
	int (*high_bw_read)(struct escore_priv *escore,
			void *buf, int len);
	int (*cmd)(struct escore_priv *escore, u32 cmd, u32 *resp);
	int (*high_bw_cmd)(struct escore_priv *escore, u32 cmd, u32 *resp);
	int (*high_bw_wait)(struct escore_priv *escore);
	int (*high_bw_open)(struct escore_priv *escore);
	int (*high_bw_close)(struct escore_priv *escore);
	u32 (*cpu_to_bus)(struct escore_priv *escore, u32 resp);
	u32 (*bus_to_cpu)(struct escore_priv *escore, u32 resp);
	int (*rdb)(struct escore_priv *escore, void *buf, u16 len, int id);
	int (*high_bw_write_file)(struct escore_priv *escore,
			const char *name);
};

/*Generic bus function*/
struct escore_bus {
	struct escore_bus_ops ops;
	void (*setup_prim_intf)(struct escore_priv *escore);
	int (*setup_high_bw_intf)(struct escore_priv *escore);
	u32 last_response;
};

/* escore device pm_ops */

/* escore voicesense ops */
struct escore_voicesense_ops {
	int (*escore_is_voicesense_sleep_enable)(struct escore_priv *escore);
	int (*escore_voicesense_sleep)(struct escore_priv *escore);
	int (*escore_voicesense_wakeup)(struct escore_priv *escore);
};

struct esxxx_platform_data {
	int	(*esxxx_clk_cb) (int);
	u8	ext_clk_rate;
	
};
struct escore_macro {
	u32 cmd;
};

#define	ES_MAX_ROUTE_MACRO_CMD		100
extern struct escore_macro cmd_hist[ES_MAX_ROUTE_MACRO_CMD];
extern int cmd_hist_index;


extern  uint32_t TimingDelay;

struct escore_priv {

	int mode;
	FIL* ns;
	unsigned int intf_probed;
	struct escore_flags flag;
	unsigned int pri_intf;
	unsigned int high_bw_intf;
	unsigned int wakeup_intf;

	void *voice_sense;
	struct escore_voicesense_ops vs_ops;

	struct esxxx_platform_data *pdata;
	struct es_datablock_device datablock_dev;

	struct escore_boot_ops boot_ops;
	struct escore_bus bus;
	

	int (*sleep)(struct escore_priv *escore);
	int (*wakeup)(struct escore_priv *escore);
	int (*escore_uart_wakeup)(struct escore_priv *escore);

	u16 es_vs_route_preset;
	u16 es_cvs_preset;
	int sleep_abort;
	int sleep_delay;
	int vs_abort_kw;
	int wake_count;
	int fw_requested;
	u16 preset;
	u16 cvs_preset;

	int pm_state;
	long internal_route_num;
	long internal_rate;
	u32 api_addr_max;
	struct escore_intr_regs *regs;
	struct escore_api_access *api_access;
	struct escore_reg_cache *reg_cache;
	u8 cmd_compl_mode;
	u8 uart_ready;
	int escore_power_state;
	u32 escore_event_type;
};

#define escore_resp(obj) ((obj)->bus.last_response)
extern struct escore_priv escore_priv;
extern int escore_read_and_clear_intr(struct escore_priv *escore);
extern int escore_accdet_config(struct escore_priv *escore, int enable);
extern int escore_btndet_config(struct escore_priv *escore, int enable);
extern int escore_process_accdet(struct escore_priv *escore);

extern void escore_process_analog_intr(struct escore_priv *escore);
extern void escore_process_digital_intr(struct escore_priv *escore);

extern void escore_gpio_reset(struct escore_priv *escore);
extern int escore_probe(struct escore_priv *escore,  int curr_intf);

extern int escore_cmd(struct escore_priv *escore, u32 cmd, u32 *resp);
extern int escore_write_block(struct escore_priv *escore,
		const u32 *cmd_block);
extern unsigned int escore_read(
			       unsigned int reg);
extern int escore_write(unsigned int reg,
		       unsigned int value);
extern int escore_prepare_msg(struct escore_priv *escore, unsigned int reg,
		unsigned int value, char *msg, int *len, int msg_type);



extern int escore_datablock_read(struct escore_priv *escore, u8 *buf,
					u16 len, int id);
extern int escore_datablock_write(struct escore_priv *escore, u8 *name);
extern int escore_datablock_open(struct escore_priv *escore);
extern int escore_datablock_close(struct escore_priv *escore);
extern int escore_datablock_wait(struct escore_priv *escore);
extern void TimingDelay_Decrement(void);
extern FILINFO  OutPutFile(const char *name);

u32 cpu_to_be32(u32 data);
u32 be32_to_cpu(u32 data);
int msleep(int msec);
int usleep_range(int usec1,int usec2);
int escore_start_int_osc(struct escore_priv *escore);

int escore_wakeup(struct escore_priv *escore);
void escore_pm_put_autosuspend(void);
int escore_pm_get_sync(void);

int escore_retrigger_probe(void);
extern int escore_reconfig_intr(struct escore_priv *escore);
extern void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data,...);
extern const struct dev_pm_ops escore_pm_ops;


#define ESCORE_STREAM_DISABLE	0
#define ESCORE_STREAM_ENABLE	1
#define ESCORE_DATALOGGING_CMD_ENABLE    0x803f0001
#define ESCORE_DATALOGGING_CMD_DISABLE   0x803f0000
#endif /* _ESCORE_H */
