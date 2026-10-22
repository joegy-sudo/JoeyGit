/******************************************************************************
 *                      Copyright Statement
 *                    CONFIDENTIAL DAYIN-TEC
 *
 *  This is an unpublished work of authorship, which contains trade secrets,
 *  created in 2022. Dayin-tec owns all rights to this work and
 *  intends to maintain it in confidence to preserve its trade secret status.
 *  Dayin-tec reserves the right, under the copyright laws of the
 *  China or those of any other countries that may have jurisdiction,
 *  to protect this work as an unpublished work, in the event of an inadvertent
 *  or deliberate unauthorized publication. Dayin-tec also reserves
 *  its rights under all copyright laws to protect this work as a published
 *  work, when appropriate. Those having access to this work may not copy it,
 *  use it, modify it or disclose the information contained in it without the
 *  written authorization of Dayin-tec.
 *
 *******************************************************************************
 *
 *  Module: yin_lib_interface.h
 *  Description: This is source file of interface for SDK driver package
 *  Project Scope: DAYIN for E21HA1
 *  Author: YuZhiWei
 *  Date: 2024/7/22
 *******************************************************************************/

#ifndef YIN_LIB_INTERFACE_H
#define YIN_LIB_INTERFACE_H

#include "alsa/asoundlib.h"
extern "C" {

typedef struct
{
	char *car_oem; 		/* OEM */
	char *car_tier1; 	/* Tier */
	char *module;  		/* function mode name, such as "avas", "ese", "chime", "icc", "ktv", "ktv_dry", "qav" "occ" */
	int car_type; 		/* vehicle type */
	int conditions_1; 	/* condition, Used to judge the sound effect corresponding to the vehicle type */
	int conditions_2;	/* condition, Used to judge the sound effect corresponding to the vehicle type */
} yin_cs_car_config_t;

typedef struct
{
	snd_pcm_t *handle;
	char *module;
} yin_cs_handle_t;

// error type
typedef enum yin_lib_error_statue
{
	YIN_LIB_ERROR_NONE,
	YIN_LIB_ERROR_CAR_CFG,
	YIN_LIB_ERROR_CFG_FILE_NO_FOUND,
	YIN_LIB_ERROR_MODULE_FUNCTION,
	YIN_LIB_ERROR_HANDLE_INVALID,
	YIN_LIB_ERROR_PARAM_ID,
	YIN_LIB_ERROR_MODE_VALUE,
	YIN_LIB_ERROR_MODE_NO_FOUND,
	YIN_LIB_ERROR_FAIL,
	YIN_LIB_ERROR_MAX
} yin_lib_error_statue_t;

typedef enum LIB_MSG_ID_LIST
{
	LIB_MSG_ID_Sound_Switch = 0,
	LIB_MSG_ID_Sound_Type,
	LIB_MSG_ID_Volume_Type,
	LIB_MSG_ID_Speed,
	LIB_MSG_ID_Gear,
	LIB_MSG_ID_Engine_Sts,
	LIB_MSG_ID_Pedal,
	LIB_MSG_ID_Brake,
	LIB_MSG_ID_Usg_Mode_Sts,
	LIB_MSG_ID_CarMode,
	LIB_MSG_ID_STR,
	LIB_SOURCE_TYPE,
	LIB_MSG_ID_MAX
} LIB_MSG_ID_LIST;

yin_lib_error_statue_t YIN_LIB_Init(yin_cs_handle_t **handle, yin_cs_car_config_t car_cfg, const char *cfg_file);
yin_lib_error_statue_t YIN_LIB_SendParam(yin_cs_handle_t *handle, LIB_MSG_ID_LIST param_id, int value, int timeout);
yin_lib_error_statue_t YIN_LIB_Deinit(yin_cs_handle_t *handle);

}
#endif /* YIN_CS_INTERFACE_H */