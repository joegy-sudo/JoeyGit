/* Copyright Statement:
 *
 * This software/firmware and related documentation ("Bicv Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to Bicv Co.Ltd and/or its licensors. Without
 * the prior written permission of Bicv Co.Ltd and/or its licensors, any
 * reproduction, modification, use or disclosure of Bicv Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Bicv Co.Ltd (C) 2024. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("BICV SOFTWARE")
 * RECEIVED FROM BICV AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. BICV EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES BICV PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE BICV SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN BICV
 * SOFTWARE. BICV SHALL ALSO NOT BE RESPONSIBLE FOR ANY BICV SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND BICV'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE BICV SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT BICV'S OPTION, TO REVISE OR REPLACE THE
 * BICV SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO BICV FOR SUCH BICV SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("BICV
 * Software") have been modified by BICV Co.Ltd All revisions are subject to
 * any receiver's applicable license agreements with BICV Co.Ltd.
 */
 
 #ifndef _E21H_ALSA_INTERFACE_H_
 #define _E21H_ALSA_INTERFACE_H_
  enum AudioBspInterface {
  //syncronize with gCtrlName in audiocontrol.cpp
	 DSP_PRESET_EQ = 1,
	 DSP_EQ_BASS0,
	 DSP_EQ_BASS1,
	 DSP_EQ_MIDDLE0,
	 DSP_EQ_MIDDLE1,
	 DSP_EQ_TREBLE0,
	 DSP_EQ_TREBLE1,
	 DSP_MEDIA_VOL,
	 DSP_MEDIA_MUTE,
	 DSP_RADIO_VOL,
	 DSP_RADIO_MUTE,
	 INTERFACE_ALLNUM,
 };
 #define AudioBspInterface int
 
 enum EQPreset {
	 EQ_FLAT = 0,
	 EQ_POP,
	 EQ_ROCK,
	 EQ_JAZZ,
	 EQ_CLASSIC,
	 EQ_VOICE,
	 EQ_CUSTOM,
	 EQPresetNum
 };
 #define EQPreset int
 
 enum EQBand{
	 EQ_BASS0_80HZ,
	 EQ_BASS1_250HZ,
	 EQ_MIDDLE0_500HZ,
	 EQ_MIDDLE1_1KHZ,
	 EQ_MIDDLE2_2500HZ,
	 EQ_TREBLE0_6KHZ,
	 EQ_TREBLE1_14KHZ,
	 EQBandNum
 };
 #define EQBand int
 
 
 
#endif