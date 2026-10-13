#ifndef AUDIOCONTROL_H
#define AUDIOCONTROL_H

#include "alsa/asoundlib.h"
#include "AudioService.h"

#define DEFAULT     "default"

#define EQ_PRESET_ID	"name='ARK1668E GEQ Preset'"
#define EQ_BAND0_ID		"name='ARK1668E GEQ Band0'"	//80hz
#define EQ_BAND1_ID		"name='ARK1668E GEQ Band1'"	//250hz
#define EQ_BAND2_ID		"name='ARK1668E GEQ Band2'"	//500hz
#define EQ_BAND3_ID		"name='ARK1668E GEQ Band3'"	//1khz
#define EQ_BAND4_ID		"name='ARK1668E GEQ Band4'"	//2.5khz
#define EQ_BAND5_ID		"name='ARK1668E GEQ Band5'"	//6khz
#define EQ_BAND6_ID		"name='ARK1668E GEQ Band6'"	//14khz
#define DSP_MEDIA_VOLID "name='ARK1668E Dsp Media Volume'"
#define DSP_MEDIA_MUTEID "name='ARK1668E Dsp Media mute'"
#define DSP_RADIO_VOLID "name='ARK1668E Dsp Radio Volume'"
#define DSP_RADIO_MUTEID "name='ARK1668E Dsp Radio mute'"
#define SOFTMASTER0_ID 	"name='softmaster'"
#define SOFTMASTER1_ID	"name='softmaster1'"
#define SOFTMASTER2_ID	"name='softmaster2'"
#define SOFTMASTER3_ID	"name='softmaster3'"
#define SOFTMASTER4_ID	"name='softmaster4'"
#define SOFTMASTER5_ID	"name='softmaster5'"
#define SOFTMASTER6_ID	"name='softmaster6'"
#define SOFTMASTER12_ID	"name='softmaster12'"
#define SOFTMASTER8_ID	"name='softmaster8'"
#define SOFTMASTER11_ID	"name='softmaster11'"
#define SOFTMASTER13_ID	"name='softmaster13'"
#define SOFTMASTER14_ID	"name='softmaster14'"

#define ADC_MUTE_ID 	"name='ADC Mute'"
typedef enum
{
    EQ_PRESET = 0x01,
    EQ_BAND0,
    EQ_BAND1,
    EQ_BAND2,
    EQ_BAND3,
    EQ_BAND4,
    EQ_BAND5,
    EQ_BAND6,
    MEDIA_VOL,
    MEDIA_MUTE,
    RADIO_VOL,
    RADIO_MUTE,
    SOFTMST0_VOL,//stream0(LocalMusic) -> softmaster
    SOFTMST2_VOL,//stream1(BtMusic) -> softmaster2
    SOFTMST1_VOL,//stream2(Navi) -> softmaster1
    SOFTMST3_VOL,//stream3(Calling) -> softmaster3
    SOFTMST4_VOL,//stream4(tts) -> softmaster4
    SOFTMST5_VOL,//stream5(alarm) -> softmaster5
    SOFTMST6_VOL,//stream6(beep) -> softmaster6
    SOFTMST8_VOL,//stream8(ring) -> softmaster8
    SOFTMST13_VOL,//stream13(hicar alarm) -> softmaster13
    SOFTMST14_VOL,//stream14(hicar notice) -> softmaster14
    SOFTMST11_VOL,//stream11(slide) -> softmaster11
    SOFTMST12_VOL,//stream12(carLock) -> softmaster12
    ADC_MUTE,
}TYPE_AUDIO;

typedef struct Packet
{
    int count;
    int type;
} Packet_t;

typedef struct Data
{
    int max;
    int min;
    int cur;
    int step;
}Data_t;


class AudioControl
{
#ifdef g_AudioCtrl
#undef g_AudioCtrl
#endif
#define g_AudioCtrl (AudioControl::getInstance())
public:
	inline static AudioControl* getInstance(){
		static AudioControl* audioctrl(new AudioControl());
		return audioctrl;
	}

    //get control value
    int Get(int Audio_Type ,int *nValue);

    //set control value
    int Set(int Audio_Type, int nValue);

    //get control range
    int GetRange(int Audio_Type, int *pMin ,int *pMax);


    int ALSAMixerOpen(int stream_name, int ctrlname);
    void ALSAMixerClose(void);
    int ArkALSADuckMixerGetVolume(void);
    void ArkALSADuckMixerSetVolume(int inVolume);
	
    void Set_Plug_Softvol_Volume(int streamtype,int volume);

	//void Set_Plug_Mixer_Control(int ctrlNo, int value);

protected:
	explicit AudioControl();
	~AudioControl();
    //get string from enum's
    char* getstring(int type);

    //get id from audio's string
    int getpid(int audio_type, snd_ctl_elem_id_t **ppid);

    //get info from id
    int getpinfo(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t **ppinfo);

    //get elem from id
    int getpelem(snd_ctl_elem_id_t *pid, snd_hctl_elem_t **ppelem);

    //get control from id and info
    int getpcontrol(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo ,snd_ctl_elem_value_t **ppcontrol);

    //get type
    int get_type(snd_ctl_elem_info_t *pinfo, int *ptype);

    //get size
    int get_size(snd_ctl_elem_info_t *pinfo, int *psize);

    //get range from elem and info
    //int get_range(int type, snd_hctl_elem_t *pelem, snd_ctl_elem_info_t *pinfo, Data *pdata);
    int get_range(int type, snd_ctl_elem_info_t *pinfo, Data *pdata);
    //get value
    int get_value(int type, int count, snd_ctl_elem_value_t *pcontrol, int *pvalue);

    //set value
    int set_value(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo, snd_ctl_elem_value_t *pcontrol, int value);
};

#endif // AUDIOCONTROL_H
