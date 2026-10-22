#ifndef AUDIOPERSISTENT_H
#define AUDIOPERSISTENT_H

#include <QtGlobal>

enum SpeakerSoundItem {
    SSI_Undefine = -1,
    SSI_PowerOn,
    SSI_PowerOff,
};
#define SpeakerSoundItem int

enum MuteItem {
    MI_Undefine = -1,
    MI_Unmute,
    MI_Mute,
};
#define MuteItem int

enum FocusType {
	Focus_SYS = 0,
	Focus_AA,
	Focus_CP,
	Focus_Hicar,
};
#define FocusType int

enum TouchSound {
	TS_Undefine = -1,
	TS_OFF,
	TS_ON,
	TS_CHANXIN,
	TS_YINGRUN,
};
#define TouchSound int


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

enum AVASSwitch{
	AVAS_Undefine = -1,
	AVAS_OFF,
	AVAS_ON,
};
#define AVASSwitch int

enum AVASVOL{
	AV_Undefine = -1,
	AVAS_LOW,
	AVAS_MID,
	AVAS_HIGH,
};
#define AVASVOL int

enum SpdCmpLevel {
	SC_Undefine = -1,
	SC_OFF,
	SC_LOW,
	SC_MID,
	SC_HIGH,
};
#define SpdCmpLevel int

enum PdcSound {
	PDC_Undefine = -1,
	PDC_OFF,
	PDC_ON,
};
#define PdcSound int


enum EqualizerItem {
    EI_Undefine = -1,
    EI_Standard,
    EI_Popular,
    EI_Classic,
    EI_Jazz,
    EI_Custom,
};
#define EqualizerItem int

enum SoundItem {
    SI_Undefine = -1,
    SI_Master,
    SI_Slave,
    SI_RearLeft,
    SI_RearRight,
    SI_Custom,
};
#define SoundItem int

enum AudioSource {
    AS_Undefine = -1,
    AS_Idle = 0,
    AS_Radio = 1,
    AS_Music = 2,
    AS_Video = 3,
    AS_CarplayMusic = 4,
    AS_CarplayPhone = 5,
    AS_AutoMusic = 6,
    AS_AutoPhone = 7,
    AS_CarlifeMusic = 8,
    AS_CarlifePhone = 9,
    AS_BluetoothMusic = 10,
    AS_BluetoothPhone = 11,
    AS_Aux = 12,
    AS_ECLinkMusic = 13,
    AS_ECLinkBluetoothMusic = 14,
    AS_HiCarMusic = 15,
    AS_HiCarBluetoothMusic = 16,
    AS_Alarm = 17,
    AS_Beep = 18,
    AS_Navi = 19,
    AS_TTS = 20,
    AS_HiCarPhone = 21,
    AS_CarLinkPhone = 22,
    AS_CarLinkMusic = 23,
	AS_RING= 24,
	AS_Hicar_ALARM = 25,
};
#define AudioSource int

enum AUDIO_STREAM_NAME{
    AUDIO_LOCAL_MUSIC = 0,
    AUDIO_PHONE_MUSIC = 1,
    AUDIO_PHONE_NAVI = 2,
    AUDIO_PHONE_TELL = 3,
    AUDIO_PHONE_TTS = 4,
    AUDIO_LOCAL_ALARM = 5,
    AUDIO_LOCAL_BEEP = 6,
    AUDIO_PHONE_RING = 7,
    AUDIO_HICAR_ALARM = 8,
    AUDIO_HICAR_NOTICE = 9,  
    AUDIO_LOCAL_RADIO = 10,
    AUDIO_LOCAL_CARLOCK = 11,
    AUDIO_LOCAL_SLIDE = 12,
    AUDIO_LOCAL_ALLNUM = 13,
};
#define AUDIO_STREAM_NAME int

enum STREAM_CTRL_NAME{
    CTRL_LOCAL_MUSIC = 0,
    CTRL_PHONE_MUSIC = 1,
    CTRL_PHONE_NAVI = 2,
    CTRL_PHONE_TELL = 3,
    CTRL_PHONE_TTS = 4,
    CTRL_LOCAL_ALARM = 5,
    CTRL_LOCAL_BEEP = 6,
    CTRL_LOCAL_RADIO = 7,
    CTRL_LOCAL_RING = 8,
    
};
#define STREAM_CTRL_NAME int

class AudioService;
class AudioPersistent
{
    Q_DISABLE_COPY(AudioPersistent)
public:
    static void reset();
    static int  getVolume(const bool reload = false);
	static int  getMuteStatus(const bool reload = false);
	static void setMuteStatus(const int val);
    static int  getEqualizerItem(const bool reload = false);
    static int  getBass0Equalizer(const bool reload = false);
    static int  getMiddle0Equalizer(const bool reload = false);
    static int  getTreble0Equalizer(const bool reload = false);
	static int  getBass1Equalizer(const bool reload = false);
	static int  getMiddle1Equalizer(const bool reload = false);
 	static int  getTreble1Equalizer(const bool reload = false);
	static int  getMiddle2Equalizer(const bool reload = false);
	static int  getMICMuteStatus(const bool reload = false);
	
    static void setEqualizerItem(const int item);
    static void setBass0Equalizer(const int value);
    static void setMiddle0Equalizer(const int value);
    static void setTreble0Equalizer(const int value);
    static void setBass1Equalizer(const int value);
    static void setMiddle1Equalizer(const int value);
	static void setMiddle2Equalizer(const int value);
    static void setTreble1Equalizer(const int value);
	static void  setMICMuteStatus(const int value);
	
    static int  getSoundItem(const bool reload = false);
    static int  getLeftSound(const bool reload = false);
    static int  getRightSound(const bool reload = false);
    static void setSoundItem(const int item);
    static void setLeftSound(const int value);
    static void setRightSound(const int value);
    static void saveStreamVolume(const int streamtype, const int volume);
    static int  getStreamVolume(const int streamtype,const bool reload = false);

	static void setTouchSoundStatus(const int value);
	static int  getTouchSoundStatus(const bool reload = false);

	static void setSpdCmpStatus(const int value);
	static int  getSpdCmpStatus(const bool reload = false);

	static void setSpdCmpEnableStatus(const int value);
	static int  getSpdCmpEnableStatus(const bool reload = false);

	static void setAVASVolLevel(const int value);
	static int  getAVASVolLevel(const bool reload = false);

	static void setAVASSoundType(const int value);
	static int  getAVASSoundType(const bool reload = false);
	
	static int resetStreamVolume(const int streamtype);

	static void setNavMixMediaStatus(const int value);
	static int  getNavMixMediaStatus(const bool reload = false);
	
	static void setCarSoundLockEQ(const int value);
	static int  getCarSoundLockEQ(const bool reload = false);
private:
    friend class AudioService;
    friend class AudioServicePrivate;
    static void setVolume(int value);
};

#endif // AUDIOPERSISTENT_H
