/*
 * es705-access.h  --  ES705 Soc Audio access values
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES705_ACCESS_H
#define _ES705_ACCESS_H
#include "es705-access.h"

#define ES705_API_WORD(upper, lower) ((upper << 16) | lower)

static struct escore_api_access es705_api_access[ES705_API_ADDR_MAX] = {
	//ES705_FW_FIRST_CHAR
	{
		{ ES705_API_WORD(0x8020, 0x0000) },
		4,
		{ ES705_API_WORD(0x8020, 0x0000) },
		4,
		0,
		255,
	},
	//ES705_FW_NEXT_CHAR
	{
		{ ES705_API_WORD(0x8021, 0x0000) },
		4,
		{ ES705_API_WORD(0x8021, 0x0000) },
		4,
		0,
		255,
	},
	//ES705_VS_INT_OSC_MEASURE_START
	{
		{ ES705_API_WORD(0x8070, 0x0000) },
		4,
		{ ES705_API_WORD(0x9070, 0x0000) },
		4,
		0,
		1,
	},
	//ES705_VS_INT_OSC_MEASURE_STATUS
	{
		{ ES705_API_WORD(0x8071, 0x0000) },
		4,
		{ ES705_API_WORD(0x8071, 0x0000) },
		4,
		0,
		1,
	},
	//SYNC COMMAND
	{
		{ ES705_API_WORD(0x8000, 0x0000) },
		4,
		{ ES705_API_WORD(0x8000, 0x0000) },
		4,
		0,
		255,
	},
	//ES705_POWER_STATE
	{
		{ ES705_API_WORD(0x800f, 0x0000) },
		4,
		{ ES705_API_WORD(0x9010, 0x0000) },
		4,
		0,
		6,
	},
	//ES705_VOICE_SENSE_TRAINING_MODE
	{
		{ ES705_API_WORD(ES705_GET_ALGO_PARAM, 0x5003) },
		4,
		{ ES705_API_WORD(ES705_SET_ALGO_PARAM_ID, 0x5003),
				   ES705_API_WORD(ES705_SET_ALGO_PARAM, 0x0000) },
		8,
		0,
		2,
	},
	//ES705_VOICE_SENSE_TRAINING_RECORD
	{
		{ ES705_API_WORD(ES705_GET_ALGO_PARAM, 0x5006) },
		4,
		{ ES705_API_WORD(ES705_SET_ALGO_PARAM_ID, 0x5006),
			       ES705_API_WORD(ES705_SET_ALGO_PARAM, 0x0000) },
		8,
		0,
		5,
	},
	//ES705_VOICE_SENSE_TRAINING_STATUS
	{
		{ ES705_API_WORD(ES705_GET_ALGO_PARAM, 0x5007) },
		4,
		{ ES705_API_WORD(ES705_SET_ALGO_PARAM_ID, 0x5007),
			       ES705_API_WORD(ES705_SET_ALGO_PARAM, 0x0000) },
		8,
		0,
		4,
	},
};

#endif /* _ES705_ACCESS_H */
