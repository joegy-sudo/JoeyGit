#include "AudioService.h"
#include "audiocontrol.h"
#include "wavplayinterruptible.h"
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <limits>
#include <cmath>
#include <QString>


#define VOL_MAX_LEVEL 39
#define VOL_MIN_LEVEL 0
#define VOL_SUPPRESSION 6

#define KEY_WAV_FILE "/media/sound/key_classic.wav"
#define KEY_WAV_EQ_1 "/media/sound/key_chanxin.wav"
#define KEY_WAV_EQ_2 "/media/sound/key_yingrun.wav"

#define CAR_SOUND_EQ1 "/media/sound/dajidali.wav"
#define CAR_SOUND_EQ2 "/media/sound/haoyunbugu.wav"
#define CAR_SOUND_EQ3 "/media/sound/nengliangshouheng.wav"
#define CAR_SOUND_EQ4 "/media/sound/xingguangshanyao.wav"
#define CAR_SOUND_EQ5 "/media/sound/yuedongyinghe.wav"

#define CAR_SOUND_EQ1_PRE "/media/sound/pre_dajidali.wav"
#define CAR_SOUND_EQ2_PRE "/media/sound/pre_haoyunbugu.wav"
#define CAR_SOUND_EQ3_PRE "/media/sound/pre_nengliangshouheng.wav"
#define CAR_SOUND_EQ4_PRE "/media/sound/pre_xingguangshanyao.wav"
#define CAR_SOUND_EQ5_PRE "/media/sound/pre_yuedongyinghe.wav"

#define VOLUME_SLIDE_MEDIA	"/media/sound/volume_media.wav"
#define VOLUME_SLIDE_CALL	"/media/sound/volume_calling.wav"
#define VOLUME_SLIDE_NAVI 	"/media/sound/volume_navi.wav"
#define VOLUME_SLIDE_RING	"/media/sound/volume_ring.wav" 
#define VOLUME_SLIDE_VOICE	"/media/sound/volume_voice.wav"

#define AVAS_EQ_CLASSIC		"/media/sound/avas_classic.wav"
#define AVAS_EQ_SKY			"/media/sound/avas_sky.wav"
#define AVAS_EQ_YINHE		"/media/sound/avas_yinhe.wav"

#define RADAR_PRE_SHOW		"/media/sound/radar_preShow.wav"
#define CLIMATE_SOUND		"/media/sound/climate.wav"

#define RADAR_LOW_HZ		"/media/sound/FRadar_Slow.wav"
#define RADAR_HIGH_HZ		"/media/sound/FRadar_Continue.wav"
#define RADAR_MID_HZ		"/media/sound/FRadar_Normal.wav"
#define RADAR_SAMPLE_FADE_MS	100

#define RVC_NONREVERSE          0x00     // 非倒档
#define RVC_REVERSE             0x01     // 倒档

#define RADARWARN_TYPE_NONE     0x00     // 无警告
#define RADARWARN_TYPE_ON       0x01     // 长鸣警告
#define RADARWARN_TYPE_4HZ      0x02     // 4hz警告
#define RADARWARN_TYPE_2HZ      0x03     // 2hz警告
#define RADARWARN_TYPE_ONCE 	0x04	 // 雷达只响一声
#define RADARWARN_TYPE_TIME 	0x05	 // 长鸣1s

#define RVC_SHOW 0x01	//倒车画面显示
#define RVC_EXIT 0X00	//倒车画面推出

#define RADAR_FAIL_NORMAL 0x00			//雷达故障正常

#define USE_ON	1

#define IOCTL_GET_SHOW_STATUS					_IOR(0x9A, 30, int)	

/**
* volume gain
* index:0-39 value:0-127
* x = (51-|db|)*(128/52)
* alsa:-51db~0db resolution:128
**/
static int volume_Gain[]=
{
    0,  1,  2, //0-2
	5,  15, 25,//3-5
	32, 39, 44,//6-8
	49, 54, 57,//9-11
	59, 62, 64,//12-14
	66, 69, 71,//15-17
	74, 76, 79,//18-20
	81, 84, 86,//21-23
	89, 91, 94,//24-26
	96, 98, 101,//27-29
    103, 106, 108,//30-32
    111, 113, 116,//33-35
    118, 121, 123,//36-38
    127 //39
};

static int bt_Gain[]=
{
    0,  1,  2, //0-2
	5,  15, 25,//3-5
	32, 39, 44,//6-8
	49, 54, 57,//9-11
	59, 62, 64,//12-14
	66, 69, 71,//15-17
	74, 76, 79,//18-20
	81, 84, 86,//21-23
	89, 91, 94,//24-26
	96, 98, 101,//27-29
    103, 106, 108,//30-32
    111, 113, 116,//33-35
    118, 121, 123,//36-38
    127 //39
};

static int spdVolUp[4][29] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4},
	{0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,6,7,7,7,7,8,8,8},
	{0,1,2,2,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15},
};
static int spdVolDown[4][29] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4},
	{0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8},
	{0,1,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15},
};

class AudioServicePrivate
{
public:
    explicit AudioServicePrivate(AudioService* parent);
    ~AudioServicePrivate();
    void initVolumeMap();
	
	void resetVolumeMap();// 恢复音量大小
	
    void initializeEQ();

	/* ark1668e interface */
	void controlMute(int value);
    void controlGain(int value);
    void controlBass(int value);
    void controlMiddle(int value);
    void controlTreble(int value);
    void SetInputAudio(AudioSource source);
    void SetOutputAudio(int volume);
    void SetOutputVolume(int volume);
    int pos2volume(int pos/* 0 ~ VOL_MAX_LEVEL*/);

    /* bicv interface defined */
    bool requestAudioSource(const int source);

	int getCurAudioSource();
	
    void releaseAudioSource(const int source);
	
	int getVolume(const int streamType);
	
    void handlerMessage(const int32_t &msg, const int32_t arg0, const int32_t arg1);

	void handlerStoreMessage(const int32_t &msg, const int32_t arg0, const int32_t arg1);
	
    void setStreamVolume(const int streamType, const int volume);
	
    shared_ptr<VolumeData> getVolumeData(const int &streamType);

	int getVDObjMemberShowVol(const int &streamType);

	int getVDObjMemberRealVol(const int &streamType);

	bool getVDObjMuteState(const int &streamType);

	void setVDObjMemberShowVol(const int &streamType, int value);
	
	void setVDObjMemberRealVol(const int &streamType, int value);
	
	void setVDObjMuteState(const int &streamType, bool value);

    bool adjustVolumeData(shared_ptr<VolumeData> volumedata);
	bool adjustVolumeDataInHicar(shared_ptr<VolumeData> volumedata);
    void adjustVolume();

	void storeAudioIniInfo(int code, int arg0, int arg1);
	
    void setALSAVolume(int streamType, int volume);

	/** 
    * @brief 步进音量调节,用于 fadeout
    * @param	[in] volume	当前音量
    * 			[in] aimVol 目标音量
	* @retval close more to aimvol
    */
    int getGradualVolume(int volume, int aimVol);

    int getCurStreamType();

    void pdcSoundPlay(const int type);

    void pdcSoundPlayProxy(const int start, const int type, bool withFade = true);
	void onRadarPlayNaturalEnd();
	
    void softkeySoundPlay();

	void carLockSoundPlay();
	/** 
    * @brief 音量自动调节 兜底喇叭烧蚀
    * @param [in] db : 降db数
    */
	void adjustDspGainAuto(const int db);

	void showVolumeInfo();

	void showFocusInfo();
	
	void requestMute(const int item);

	void requestMuteToggole();

	int getMuteStatus();

	void requestIncreaseVolume();

    void requestDecreaseVolume();

	//void setEQMode(const int mode);
	void setEqualizerItem(const int item, const int bass, const int middle, const int treble);
	
	int  getEQMode();
	
	void setBandLevel(const int band,const int level);

	int  getBandLevel(const int band);

	void setTouchSoundStatus(const int status);

	int  getTouchSoundStatus();	

	void setSpdCompensation(const int value);

	int  getSpdCompensation();	

	void setSpdCmpEnable(const int value);

	int  getSpdCmpEnable();

	void setSpdCmpLevel(const int value);
	
	int  getSpdCmpLevel();
	
	void setSpdCmpGain(int speed);

	void setMICMute(int state);

	int getMICMute();

	bool isEntStreamType(const int streamType);

	void entFadeIn();

	void entFadeOut();

	int getNavMixMedia();

	void setFocusPolicy(int policy);
	int getFocusPolicy();
	void setNavMixMedia(int value);

	void setCarLockSound(int value, bool play, bool isApply);

	int getCarLockSound();

	void entVolumeAdjust();
	void applyHicarNavEntRealVol(shared_ptr<VolumeData> vd);

	void volumeSliderEQPlay(int slideType, int volval);

	bool isEntAudioSouce(const int source);
private:
    Q_DECLARE_PUBLIC(AudioService)
    AudioService* const q_ptr;

	int mCurEqmode;
	
	int m_CustomBass0;
	int m_CustomBass1;
	int m_CustomMiddle0;
	int m_CustomMiddle1;
	int m_CustomMiddle2;
    int m_CustomTreble0;
	int m_CustomTreble1;

	int m_CurStreamType;
	int m_CurrentVolume;

    AudioSource m_AudioSource;
    MuteItem m_MuteItem;		//! 静音状态
    TouchSound m_TouchSound;
	SpdCmpLevel m_Spdcmp;
	int m_SpdcmpLastInx;
	bool m_SpdChg;

	FocusType mFocusType;
    Handler mHandler;
	Handler mStoreHandler;
    map<int, shared_ptr<VolumeData>> mVolumeMap;
    vector<shared_ptr<AudioPriority>> m_ASFocus;

	shared_ptr<Chime> pChime;
	int mLastPdcType;
    shared_ptr<WavPlay> pSoftkey;
	shared_ptr<WavPlay> pCarLockSound;
	shared_ptr<WavPlayInterruptible> pSlide;
	int m_volSlidePlaySlideType;
    bool isAdjustVolInterrupted;
    bool isReversing;
    bool isCalling;
    bool isVR;
    bool isNav;
	bool isRing;
	bool isHicarAlarm;

	int mActiveENT;//streamtype
	int mEntVol;//music,bt,radio

	int mMICMute;
	QMutex mSpdLock;

	int m_isNavSupressMedia;
	int mSpdCmpEnable;
	int mCmpVal;
	int mCarLockSound;
};

AudioService::AudioService(QObject *parent)
    : QObject(parent)
    , d_ptr(new AudioServicePrivate(this))
{
	VLOGD("@audio-version E211 2026/10/10 13:00 %lld",QCoreApplication::applicationPid());
	
	//init member
	mReverseState = 0;
	mRadarType = 0;
	mRadarRVC = 0;
	mRadarWarn = 0;
	netlink_socket = 0;
	socket_notifier = nullptr;
	avas_handle = nullptr;
	curSpeed = 0;
	//QDBUS Register
	bool ret = QDBusConnection::sessionBus().registerService("com.bicv.audio"); 
    ret = ret && QDBusConnection::sessionBus().registerObject("/com/bicv/audio", this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
    if (!ret) {
        VLOGD("@audio-error AudioService Register QDbus failed!");
        exit(EXIT_FAILURE);
    }
	
	//VehicleService Register
    QObject::connect(g_VehicleProxy, SIGNAL(propertyChangeEvent(const VehiclePropValue&)),
                     this, SLOT(onPropertyChangeEvent(const VehiclePropValue&)));
    QObject::connect(g_VehicleProxy, SIGNAL(registrationStatusChange(bool)),
                     this, SLOT(onRegisterationStatusChange(bool)));
	QObject::connect(d_ptr->pSlide.get(), SIGNAL(naturalPlaybackFinished()),
					 this, SLOT(onVolumeSlideNaturalEnd()));
	QObject::connect(d_ptr->pChime.get(), SIGNAL(naturalPlaybackFinished()),
					 this, SLOT(onRadarPlayNaturalEnd()));
	QObject::connect(d_ptr->pCarLockSound.get(), SIGNAL(finished()),
					 this, SLOT(onCarLockSoundPlayEnd()));

	mCarLockNeedCloseAmp = false;

    g_VehicleProxy->registerProperty();

	g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_BCM_ExternalLockingReimnd);

	VehiclePropValue propSoundspk;
	propSoundspk  = g_VehicleProxy->subscribe(VehicleProperty::ID_MCU_AUDIO_MUTE);
	int valLockSpk = propSoundspk.bytes_value().data()[0];
	if(valLockSpk == 1){
		VLOGD("@audio-amp enable, speak lock sound directly");
		g_VehicleProxy->setIntProperty(VehicleProperty::ID_MCU_AUDIO_MUTE, 0x06);
		QThread::msleep(60);
		carLockSoundPlay();
	}
	
	
	VehiclePropValue propSpeed;
	propSpeed = g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_ESC_SPEED);
	int valSpeed = propSpeed.bytes_value().data()[0];

	VehiclePropValue propEng;
	propEng = g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_EPTREADY);
	int valEng = propEng.bytes_value().data()[0];
	VehiclePropValue propGear;
	propGear = g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_MODE_GEAR_STATUS);
	int valGear = propGear.bytes_value().data()[0];


	mAVASEnable = AVAS_ON;
	mAVASSoundType = AudioPersistent::getAVASSoundType();
	//mAVASLevel = AudioPersistent::getAVASVolLevel();
	//init AVAS 
	VLOGD("@audio init avas enable:%d spd:%d Gear:%d Eng:%d",mAVASEnable, valSpeed,valGear,valEng);
	avas_handle = nullptr;
	char car[6] = "Geely";
	char tier1[5] = "Bicv";
	char module[5] = "avas";
	yin_cs_car_config_t car_cfg ;
	memset(&car_cfg,0x0, sizeof(yin_cs_car_config_t));
	car_cfg.car_oem = car;
	car_cfg.car_tier1 = tier1;
	car_cfg.module = module;
	car_cfg.car_type = 0xA1;
	car_cfg.conditions_1 = 0;
	car_cfg.conditions_2 = 0;
	
	YIN_LIB_Init(&avas_handle, car_cfg, "/media/sound/avas/Config.txt");

	YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Type, mAVASSoundType, 0);
	
	YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Switch, mAVASEnable, 0);

	YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Gear, valGear, 0);

	YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Engine_Sts, valEng, 0);

	YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Speed, 0, 0);

	//YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Volume_Type, mAVASLevel, 0);
	
	g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVASDisabledSet, mAVASEnable+1);//匹配CAN信号
	g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVAS_AVASDisabledSts, !mAVASEnable);
	initNetLink();
	initRadar();
}



void AudioService::initRadar()
{
	
	g_VehicleProxy->subscribe(VehicleProperty::ID_MCU_VEHICLE_RVC);
	g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_RADARWARN_WARN_TYPE);
	g_VehicleProxy->subscribe(VehicleProperty::INFO_ID_RADARWARN_FAILURE_MODE);

	mReverseState = g_VehicleProxy->getUInt8Property(VehicleProperty::ID_MCU_VEHICLE_RVC);
	if(mReverseState == RVC_REVERSE){
		g_VehicleProxy->setIntProperty(VehicleProperty::ID_MCU_AUDIO_MUTE, 0x06);
	}
	mRadarType = g_VehicleProxy->getUInt8Property(VehicleProperty::INFO_ID_RADARWARN_WARN_TYPE);
	mRadarWarn = g_VehicleProxy->getUInt8Property(VehicleProperty::INFO_ID_RADARWARN_FAILURE_MODE);
	mRadarRVC = getReverseStatus();
	
	g_VehicleProxy->setIntProperty(VehicleProperty::ID_SOC_RVC_SHOW, mRadarRVC);

	//calculate
	if(mRadarType != 0xFF){
		mRadarType = (mRadarType & 0x30) >> 4;
	}
	
	VLOGD("@audio mReverseStatus:%d,mRadarType:%d,mRadarWarn:%d,mRadarRVC:%d",mReverseState,mRadarType,mRadarWarn,mRadarRVC);
	if(mReverseState == RVC_REVERSE){
		if((mRadarWarn != RADAR_FAIL_NORMAL)&&(mRadarWarn != 0xFF)){
			VLOGD("@audio radar failure when power on,failure code %d", mRadarWarn);
			pdcSoundPlay(RADARWARN_TYPE_TIME);
		}else if((mRadarRVC == RVC_SHOW)&&(mRadarType != RADARWARN_TYPE_NONE)&&(mRadarType != 0xFF)){
			VLOGD("@audio radar sound when power on,failure code %d", mRadarWarn);
			pdcSoundPlay(mRadarType);
		} 
	}
}


AudioService::~AudioService()
{
	qDebug()<<"@audio unconstractor audio service";
	
	if(socket_notifier != nullptr){
		socket_notifier->setEnabled(false);
		delete socket_notifier;
	}
	
    if (NULL != d_ptr) {
        delete d_ptr;
    }

	if(avas_handle != nullptr){
		YIN_LIB_Deinit(avas_handle);
		avas_handle = nullptr;
	}
}

bool AudioService::getAMPEnableStatus()
{
	uint8_t ret = g_VehicleProxy->getUInt8Property(VehicleProperty::ID_MCU_AUDIO_MUTE);
	VLOGD("@audio-amp %s , AMP mute status : %d (true=muted)", __func__, ret);
	return (((ret>>1)&0x01) == 0x01);
}
void AudioService::onPropertyChangeEvent(const VehiclePropValue &prop)
{
   if(prop.prop() == VehicleProperty::INFO_ID_MODE_GEAR_STATUS){
		//1. Gear Status
        int value = 0;
		value = prop.bytes_value().data()[0];
		//qDebug()<<"@audio-avas Gear :"<<value;
		VLOGD("@audio-avas Gear : %d", value);
		YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Gear, value, 0);
    }else if(prop.prop() == VehicleProperty::INFO_ID_ESC_SPEED){
		//2. Speed Info
		int value = 0;
   		int nspeed = 0;
		int valid = 0;
		valid = prop.int32_valid();

		if(valid == 0){
			for(uint8_t i =0;i< prop.bytes_value().size(); ++i){
				value = (value << 8 )| prop.bytes_value().data()[i];
			}
			float fspeed = (float)value * 0.0563;
			nspeed = (int)(fspeed * 100);

			YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Speed, nspeed, 0);
			//记录用于音随等级切换时使用
			curSpeed = nspeed;
			setSpdCmpGain(nspeed/100);
		}
	} else if(prop.prop() == VehicleProperty::INFO_ID_EPTREADY){
		//3. PTReady Status
		int value = 0;
		value = prop.bytes_value().data()[0];
		VLOGD("@audio-avas Eng : %d",value);
		//qDebug()<<"@audio-avas Eng :"<<value;
		YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Engine_Sts, value, 0);
		
	}
	else if(prop.prop() == VehicleProperty::ID_MCU_VEHICLE_RVC) {
		//0B 08
        if (prop.bytes_value().size () > 0) {
            uint8_t value = prop.bytes_value()[0];
            updateReverseStatus(value);
        }
	}
	else if(prop.prop() == VehicleProperty::INFO_ID_RADARWARN_WARN_TYPE) {
		//0E 04
        if (prop.bytes_value().size () > 0) {
            uint8_t value = prop.bytes_value()[0];
			if(value == 0xFF)
				return;
			
			value = (value & 0x30) >> 4 ;//IPC protocal 0E 04
            updateRadarType(value);
        }
	}
	else if(prop.prop() == VehicleProperty::INFO_ID_RADARWARN_FAILURE_MODE) {
		//0E 02
		if (prop.bytes_value().size () > 0) {
			uint8_t value = prop.bytes_value()[0];
			if(value == 0xFF)
				return;
			
			updatePlayRadarWarn(value);
		}
	}
	else if(prop.prop() == VehicleProperty::INFO_ID_BCM_ExternalLockingReimnd){
		//锁车提示音音效播放
		if (prop.bytes_value().size () > 0) {
			uint8_t value = prop.bytes_value()[0];
			VLOGD("@audio receive LockingReimnd %d mCarLockSound:%d", value, getCarLockSound());//0x0: No request  0x1: Speaking Request 
			if(getCarLockSound() != 0 && value == 0x01){
				/* true=功放静音：临时 0x06 解除，播完 0x05 再静音；未静音则不动 */
				mCarLockNeedCloseAmp = false;
				if(getAMPEnableStatus() == true){
					VLOGD("@audio receive LockingReimnd amp muted, unmute AMP(0x06)");
					g_VehicleProxy->setIntProperty(VehicleProperty::ID_MCU_AUDIO_MUTE, 0x06);
					QThread::msleep(60);
					mCarLockNeedCloseAmp = true;
				} else {
					VLOGD("@audio receive LockingReimnd amp already unmuted, skip AMP ctrl");
				}

				carLockSoundPlay();
				if (mCarLockNeedCloseAmp && d_ptr->pCarLockSound && !d_ptr->pCarLockSound->isRunning()) {
					onCarLockSoundPlayEnd();
				}
			}
		}
	}
}

uint8_t AudioService::getReverseStatus()
{
	int fd = ::open("/dev/carback", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        VLOGD("@audio-error /dev/carback open fd failed!");
        return false;
    }

    int data = 0;
    int ret = ioctl(fd, IOCTL_GET_SHOW_STATUS, &data);
    if (ret != 0) {
        VLOGD("@audio-error getReverseStatus failed, %s.", strerror(errno));
    }

    if (fd != -1) {
        ::close(fd);
    }
    
    VLOGD("@audio-as getReverseStatus data=%d", data);
    return (uint8_t)data;
}

void AudioService::initNetLink()
{
	qDebug()<<"@audio-as initNetLink *3.1*";

	int retval = 0;
    struct sockaddr_nl snl;
	const int buffersize = 2*1024*1024;
    memset(&snl, 0x00, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_groups = 1;

	netlink_socket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if(netlink_socket == -1){
		VLOGD("@audio-error get socket error");
		return;
	}
	setsockopt(netlink_socket, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
	
	int bind = ::bind(netlink_socket, (struct sockaddr*)&snl, sizeof(struct sockaddr_nl));
    if (bind < 0)
    {
        VLOGD("@audio-error bind netlink socket fail!");
		close(netlink_socket);
		netlink_socket = -1;
		
        return;
    }else if(bind == 0){
		struct sockaddr_nl _snl;
        socklen_t _addrlen;
		_addrlen = sizeof(struct sockaddr_nl);
        retval = getsockname(netlink_socket, (struct sockaddr *)&_snl, &_addrlen);
        if (retval == 0)
            snl.nl_pid = _snl.nl_pid;
	}

    socket_notifier = new QSocketNotifier(netlink_socket, QSocketNotifier::Read, this);
    connect(socket_notifier, SIGNAL(activated(int)),SLOT(onActivated()));

	socket_notifier->setEnabled(true);

}

void AudioService::carLockSoundPlay()
{
	Q_D(AudioService);
	d->carLockSoundPlay();
}

void AudioService::onCarLockSoundPlayEnd()
{
	if (!mCarLockNeedCloseAmp)
		return;

	VLOGD("@audio LockingReimnd play end, mute AMP(0x05)");
	g_VehicleProxy->setIntProperty(VehicleProperty::ID_MCU_AUDIO_MUTE, 0x05);
	mCarLockNeedCloseAmp = false;
}


void AudioService::onActivated()
{
    QByteArray data;
    data.resize(1024);
	data.fill(0);
    size_t len = read(socket_notifier->socket(), data.data(), 1024);
    data.resize(len);
    data = data.replace(0, '\n').trimmed();
    QString filter(data);
    
    if(filter.contains(QString("DEVNAME=carback")))
    {
    	//VLOGD()<<"@audio-as m_SocketNotifier read:"<<filter;
        if(filter.contains(QString("video_detect=1"))) // 进入倒车
        {
			VLOGD("@audio --------------RVC_SHOW------------");
			g_VehicleProxy->setIntProperty(VehicleProperty::ID_SOC_RVC_SHOW, 0x01);
			updateRadarDisplay(RVC_SHOW); 	
        }else if(filter.contains(QString("video_detect=0"))){ // 退出倒车
			VLOGD("@audio --------------RVC_EXIT------------");
			g_VehicleProxy->setIntProperty(VehicleProperty::ID_SOC_RVC_SHOW, 0x00);
           	updateRadarDisplay(RVC_EXIT);
        }
    }
}

void AudioService::onVolumeSlideNaturalEnd()
{
	Q_D(AudioService);
	//VLOGD("@zhz onVolumeSlideNaturalEnd");
	emit onVolSidePlayEndChange(d->m_volSlidePlaySlideType);
}

void AudioService::onRadarPlayNaturalEnd()
{
	Q_D(AudioService);
	d->onRadarPlayNaturalEnd();
}

void AudioService::updateReverseStatus(uint8_t value)
{
	VLOGD("%s mReverseState = %d value = %d.", __func__, mReverseState, value);

	if (mReverseState != value) {
		mReverseState = value;
		if (value != RVC_REVERSE)
			pdcSoundPlay(RADARWARN_TYPE_NONE);
	}
}

void AudioService::updateRadarType(uint8_t value)
{
	VLOGD("%s mRadarType : %d, value : %d", __func__, mRadarType, value);

	if((mRadarWarn != RADAR_FAIL_NORMAL)&&(mRadarWarn != 0xFF)){
		VLOGD("%s radar failure!",__func__);
		return;
	}
	
	uint8_t tmpRVC = getReverseStatus();
	if(mRadarType != value){
		mRadarType = value; 
		updateRadarDisplay(tmpRVC);
	}
}

void AudioService::updatePlayRadarWarn(uint8_t value)
{
	int tmpbakRadarWarn = mRadarWarn;
	mRadarWarn = value;
	VLOGD("%s WARN : %d", __func__, value);
	if((mRadarWarn!=RADAR_FAIL_NORMAL)&&(mReverseState == RVC_REVERSE)&&(mRadarWarn!=0xFF)){
		mRadarType = RADARWARN_TYPE_TIME;
		pdcSoundPlay(RADARWARN_TYPE_TIME);
	}
	else if((tmpbakRadarWarn!=mRadarWarn)&&(mRadarWarn==RADAR_FAIL_NORMAL)&&(mReverseState == RVC_REVERSE)&&(tmpbakRadarWarn!=0xFF)){
		
		uint8_t ntypeVal = g_VehicleProxy->getUInt8Property(VehicleProperty::INFO_ID_RADARWARN_WARN_TYPE);
		VLOGD("%s Radar failure :%d -> normal:%d , ntypeVal:%d ", __func__, tmpbakRadarWarn, mRadarWarn, ntypeVal);
		if(ntypeVal != 0xFF) {
			ntypeVal = (ntypeVal & 0x30) >> 4 ;
	        updateRadarType(ntypeVal);
		}
	}
}

void AudioService::updateRadarDisplay(uint8_t value)
{
	VLOGD("%s Show :%d RVC:%d mRadarType:%d WARN:%d", __func__, value, mReverseState, mRadarType, mRadarWarn);
	
	if(getAMPEnableStatus() == true){
		VLOGD("@audio Radar enble AMP");
		g_VehicleProxy->setIntProperty(VehicleProperty::ID_MCU_AUDIO_MUTE, 0x06);
	}
	
	mRadarRVC = value;
	if(mRadarRVC == RVC_SHOW) {
		if((mReverseState == RVC_REVERSE)&&(mRadarWarn == RADAR_FAIL_NORMAL)&&(mRadarType!=0xFF)){
			pdcSoundPlay(mRadarType);
		}else if((mReverseState != RVC_REVERSE)&&(mRadarWarn == RADAR_FAIL_NORMAL)){
			pdcSoundPlay(RADARWARN_TYPE_NONE);
		}else if((mReverseState == RVC_REVERSE)&&(mRadarWarn != RADAR_FAIL_NORMAL)&&(mRadarWarn != 0xFF)){
			pdcSoundPlay(RADARWARN_TYPE_TIME);
		}
	}else{
		pdcSoundPlay(RADARWARN_TYPE_NONE);
	}
}

void AudioService::setSpdCmpGain(int speed)
{
	Q_D(AudioService);
	d->setSpdCmpGain(speed);
}

void AudioService::onRegisterationStatusChange(bool isStarted)
{

}

bool AudioService::requestAudioSource(const AudioSource source)
{
    Q_D(AudioService);
    bool ret = true;
	if(source < AS_Idle || source > AS_Hicar_ALARM){
		VLOGD("@audio-error %s error source : %d", __func__, source);
		return false;
	}
	
    ret = d->requestAudioSource(source);
    if(!ret){
        VLOGD("%s requset Audio Source busy,try next", __func__);
    }
	return ret;
}

int AudioService::getCurAudioSource()
{
	Q_D(AudioService);
	return d->getCurAudioSource();
}


void AudioService::releaseAudioSource(const AudioSource source)
{
    Q_D(AudioService);

	if(source < AS_Idle || source > AS_Hicar_ALARM){
		VLOGD("@audio-error %s error source : %d", __func__, source);
		return ;
	}
	
    d->releaseAudioSource(source);
}


void AudioService::requestReset()
{
	Q_D(AudioService);
	VLOGD("%s", __func__);
	//断B+:记忆音随开关 and 等级 and AVAS Level
	d->m_Spdcmp = AudioPersistent::getSpdCmpStatus();
	d->mSpdCmpEnable = AudioPersistent::getSpdCmpEnableStatus();
	d->mCarLockSound = AudioPersistent::getCarSoundLockEQ();
	mAVASLevel = AudioPersistent::getAVASVolLevel();
	d->m_isNavSupressMedia = AudioPersistent::getNavMixMediaStatus();
	//清空Audio.ini 后延时10ms,后再存入,防止无序先存后删	
	d->storeAudioIniInfo( HANDLER_RESET_CLEAR, 0 , 0 );
	

	
	//重新拉起 触摸音状态不同步问题
	d->resetVolumeMap();
	emit onTouchSoundStatusChange(d->m_TouchSound);
	
	d->mCurEqmode = AudioPersistent::getEqualizerItem();
	
    d->m_CustomBass0 = AudioPersistent::getBass0Equalizer();
	d->m_CustomBass1 = AudioPersistent::getBass1Equalizer();

    d->m_CustomMiddle0 = AudioPersistent::getMiddle0Equalizer();
	d->m_CustomMiddle1 = AudioPersistent::getMiddle1Equalizer();
	d->m_CustomMiddle2 = AudioPersistent::getMiddle2Equalizer();

    d->m_CustomTreble0 = AudioPersistent::getTreble0Equalizer();
	d->m_CustomTreble1 = AudioPersistent::getTreble1Equalizer();
	
	d->initializeEQ();
	d->storeAudioIniInfo( HANDLER_STORE_VAL_SPD, d->m_Spdcmp, 0 );
	d->storeAudioIniInfo( HANDLER_STORE_STS_SPD, d->mSpdCmpEnable, 0 );
	d->storeAudioIniInfo( HANDLER_STORE_VAL_AVAS, mAVASLevel, 0);
	d->storeAudioIniInfo( HANDLER_STORE_STS_NAVIMEDIA, d->m_isNavSupressMedia, 0);
	d->storeAudioIniInfo( HANDLER_STORE_VAL_CARLOCKSOUND, d->mCarLockSound, 0);
	d->setCarLockSound(d->mCarLockSound, false, true);
}

void AudioService::requestMute(const int item)
{
	Q_D(AudioService);
	
	VLOGD("%s item : %d",__func__, item);
	if((item != MI_Mute) && (item != MI_Unmute)){
		VLOGD("@audio-error %s error item : %d", __func__, item);
		return;
	}
	
	d->requestMute(item);
}

void AudioService::requestMuteToggole()
{
	Q_D(AudioService);
	d->requestMuteToggole();
}

int AudioService::getMuteStatus()
{
	Q_D(AudioService);
	return d->getMuteStatus();
}

void AudioService::requestIncreaseVolume()
{
	Q_D(AudioService);
	d->requestIncreaseVolume();
}

void AudioService::requestDecreaseVolume()
{
	Q_D(AudioService);
	d->requestDecreaseVolume();
}

void AudioService::requestSetVolume(const int volume)
{
    Q_D(AudioService);
    VLOGD("%s volume :%d", __func__, volume);
    if (volume > VOL_MAX_LEVEL) {
        d->m_CurrentVolume = VOL_MAX_LEVEL;
    } else if (volume < 0) {
        d->m_CurrentVolume = 0;
    } else {
        d->m_CurrentVolume = volume;
    }

	emit onCurVolumeValueChange(d->m_CurrentVolume);
	
	requestSetVolume(d->getCurStreamType(), volume);

}

void AudioService::requestSetVolume(const int stream, const int volume)
{
    Q_D(AudioService);

	VLOGD("%s stream : %d volume : %d", __func__, stream, volume);
	
	if(stream < AUDIO_LOCAL_MUSIC || stream > AUDIO_LOCAL_RADIO){
		VLOGD("@audio-error %s error parameters stream:%d ", __func__, stream);
		return;
	}

	if(volume < VOL_MIN_LEVEL || volume > VOL_MAX_LEVEL){
		VLOGD("@audio-error %s error parameters volume:%d", __func__, volume);
		return;
	}
	
	if(volume != getVolume(stream))
    	emit onVolumeChange(stream ,volume);
    d->setStreamVolume(stream,volume);

	d->storeAudioIniInfo( HANDLER_STORE_STREAM_VOL, stream, volume );

}

void AudioServicePrivate::setStreamVolume(const int streamType, const int volume)
{//only set by setting UI
    Q_Q(AudioService);
	//设置类型 ，当前类型是 ENT 之一
	int tmpstream = 0;
	if(isEntStreamType(streamType)){
		if(isEntStreamType(m_CurStreamType))
			tmpstream = m_CurStreamType;	
		else
			tmpstream = mActiveENT;
	}  else {
		tmpstream = streamType;
	}
		
    shared_ptr<VolumeData> volumeData = getVolumeData(tmpstream);
	if(volumeData == nullptr){
		VLOGD("@audio-error %s, volumeData getVolumeData(nullptr)", __func__);
		return ;
	}
	
    int tempval = volume;

    if(volumeData->m_showVol == volume)
        return;

    if(volume >= VOL_MAX_LEVEL)
        tempval = VOL_MAX_LEVEL;
    else if(volume <= VOL_MIN_LEVEL)
        tempval = VOL_MIN_LEVEL;

    if(tmpstream == AUDIO_PHONE_TELL){
        if( volume == VOL_MIN_LEVEL)
            tempval = 1;
    }

    if(volumeData->m_showVol != 0 && tempval == 0 ){
        volumeData->m_muteState = true;//just navi and media volume slider
        if(isEntStreamType(streamType)){//静音仅针对ENT
			m_MuteItem = MI_Mute;
			emit q->onMuteChange(volumeData->m_muteState);
        }
    }else if(volumeData->m_showVol == 0 && tempval !=0) {
		volumeData->m_muteState = false;
		if(isEntStreamType(streamType)){
			m_MuteItem = MI_Unmute;
			emit q->onMuteChange(volumeData->m_muteState);
		}
	}

    volumeData->m_showVol = tempval;
	if(isEntStreamType(tmpstream)) {
		//bug:204764 负一屏 静音未取消
		if(m_MuteItem == MI_Mute && tempval !=0){
			volumeData->m_muteState = false;
			volumeData->m_realVol = volumeData->m_showVol - 1;
			m_MuteItem = MI_Unmute;
			emit q->onMuteChange(volumeData->m_muteState);
		}//- end 204764 -
		mEntVol = tempval;
	}

	if((m_CurStreamType != AUDIO_LOCAL_ALARM) && (m_CurStreamType != AUDIO_LOCAL_BEEP)&&(tmpstream == m_CurStreamType)) {
		m_CurrentVolume = tempval;
		emit q->onCurVolumeValueChange(m_CurrentVolume);
	}

	//AudioPersistent::setMuteStatus(m_MuteItem);
	
    adjustVolume();
}

int AudioService::getVolume()
{
	Q_D(AudioService);
	
	VLOGD("%s volume : %d", __func__, d->m_CurrentVolume);
	
	return d->m_CurrentVolume;
}

int  AudioService::getVolume(const int stream)
{
	Q_D(AudioService);
	return d->getVolume(stream);
}

void AudioService::setEqualizerItem(const int item, const int bass, const int middle, const int treble)
{
	Q_D(AudioService);
	d->setEqualizerItem(item, bass, middle, treble);

	emit onEqualizerItemChange(item, bass, middle, treble);
}

int AudioService::getEQMode()
{
	 Q_D(AudioService);
	 return d->mCurEqmode;
}

void AudioService::setBandLevel(const int band,const int level)
{
	Q_D(AudioService);
	d->setBandLevel(band, level);
}

int AudioService::getBandLevel(const int band)
{
	Q_D(AudioService);
	return d->getBandLevel(band);
}


void AudioService::showVolumeInfo()
{
	Q_D(AudioService);
    d->showVolumeInfo();
}

void AudioService::showFocusInfo(){
	Q_D(AudioService);
    d->showFocusInfo();
}

void AudioService::setMICMute(int state)
{
	Q_D(AudioService);

	if((state !=0)&&(state!=1)){
		VLOGD("@audio-error %s error state:%d", __func__, state);
		return;
	}
	
    d->setMICMute(state);
}

int AudioService::getMICMute()
{
	Q_D(AudioService);
	return	d->getMICMute();
}

void AudioService::entFadeIn()
{
	Q_D(AudioService);
	d->entFadeIn();
}

void AudioService::entFadeOut()
{
	Q_D(AudioService);
	d->entFadeOut();
}

void AudioService::setAVASSwitch(const int value)
{
	Q_D(AudioService);

	VLOGD("%s set %d ->%d",__func__, mAVASEnable, value);

	if(mAVASEnable == 0 && value !=0){
		if(avas_handle != nullptr){
			mAVASEnable = AVAS_ON;
			if(mAVASSoundType != value) {
				YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Type, value, 0);
				mAVASSoundType = value;
				d->storeAudioIniInfo( HANDLER_STORE_VAL_AVASTYPE, mAVASSoundType, 0);
			}
			YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Switch, AVAS_ON, 0);//off -> on
			g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVASDisabledSet, 0x2);//CAN: 0x2: Enable Request
			g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVAS_AVASDisabledSts, !mAVASEnable);
		}else{
			VLOGD("@audio-error %s avas handle is null, set error",__func__);
		}
	}else if(mAVASEnable != 0 && value == 0){
		if(avas_handle != nullptr){
			mAVASEnable = AVAS_OFF;
			YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Switch, 0, 0);//on->off
			g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVASDisabledSet, 0x1);//CAN: 0x1: Disable Request
			g_VehicleProxy->setIntProperty(VehicleProperty::INFO_ID_AVAS_AVASDisabledSts, !mAVASEnable);
		}else{
			VLOGD("@audio-error %s avas handle is null, set error",__func__);
		}
	}else{
		if(mAVASSoundType != value) {
			YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Sound_Type, value, 0);
			mAVASSoundType = value;
			d->storeAudioIniInfo( HANDLER_STORE_VAL_AVASTYPE, mAVASSoundType, 0);
		}
	}
	
}

int AudioService::getAVASSwitch()
{
	if(mAVASEnable == AVAS_OFF)
		return mAVASEnable;
	else
		return mAVASSoundType;
}

void AudioService::setAVASVolumeLevel(const int value)
{
	Q_D(AudioService);
	VLOGD("%s set %d -> %d E211 NO USE",__func__, mAVASLevel, value);

	if(mAVASLevel != value){
		if(avas_handle != nullptr){
			mAVASLevel = value;
			//YIN_LIB_SendParam(avas_handle, LIB_MSG_ID_Volume_Type, value, 0);
			d->storeAudioIniInfo(HANDLER_STORE_VAL_AVAS, mAVASLevel, 0);
		}
		else
			VLOGD("@audio-error %s avas handle is null, set error",__func__);
	}
	
}

int AudioService::getAVASVolumeLevel()
{
	return mAVASLevel;
}

void AudioService::setFocusPolicy(int policy)
{
	Q_D(AudioService);
	d->setFocusPolicy(policy);
}

int AudioService::getFocusPolicy()
{
	Q_D(AudioService);
	return d->getFocusPolicy();
}

void AudioService::setNavMixMedia(const int value)
{
	Q_D(AudioService);
	d->setNavMixMedia(value);
}
int AudioService::getNavMixMedia()
{
	Q_D(AudioService);
	return d->getNavMixMedia();
}

void AudioService::setCarLockSound(const int value, bool isPlay, bool isApply)
{
	Q_D(AudioService);
	d->setCarLockSound(value, isPlay, isApply);
}
int AudioService::getCarLockSound()
{
	Q_D(AudioService);
	return d->getCarLockSound();
}

void AudioService::volumeSliderEQPlay(int slideType, int volval)
{
	Q_D(AudioService);
	d->volumeSliderEQPlay(slideType, volval);
}

int AudioService::getCurMultimediaSource()
{
	Q_D(AudioService);
	return d->mActiveENT;
}

int AudioService::open_amixer_mode(int stream_name, int ctrlname)
{
     g_AudioCtrl->ALSAMixerOpen(stream_name,ctrlname);
     return 0;
}

void AudioService::close_amixer_mode()
{
    g_AudioCtrl->ALSAMixerClose();
}

int AudioService::get_amixersoftmaster_volume()
{
    int volume = g_AudioCtrl->ArkALSADuckMixerGetVolume();
    return volume;
}

void AudioService::set_amixersoftmaster_volume(int volume)
{
    g_AudioCtrl->ArkALSADuckMixerSetVolume(volume);
}

void AudioService::pdcSoundPlay(const int type)
{
    Q_D(AudioService);
    d->pdcSoundPlay(type);
}

void AudioService::softkeySoundPlay()
{
    Q_D(AudioService);
    d->softkeySoundPlay();
}

void AudioService::setSpdCmpEnable(const int value)
{
	Q_D(AudioService);
	d->setSpdCmpEnable(value);
}

int  AudioService::getSpdCmpEnable()
{
	Q_D(AudioService);
	return d->getSpdCmpEnable();
}

void AudioService::setSpdCmpLevel(const int value)
{
	Q_D(AudioService);

	if( value < SC_OFF || value > SC_HIGH ){
		VLOGD("@audio-error %s error parameter : %d", __func__, value);
		return;
	}
	
	d->setSpdCmpLevel(value);
}

int  AudioService::getSpdCmpLevel()
{
	Q_D(AudioService);
	return d->getSpdCmpLevel();
}

void AudioService::setTouchSoundStatus(const int status)
{
	Q_D(AudioService);
	if(status < TS_OFF || status > TS_YINGRUN){
		VLOGD("@audio-error %s error parameter : %d", __func__, status);
		return;
	}
	
	d->setTouchSoundStatus(status);
}

int  AudioService::getTouchSoundStatus()
{
	Q_D(AudioService);
	return d->getTouchSoundStatus();
}

AudioServicePrivate::AudioServicePrivate(AudioService *parent)
    : q_ptr(parent),mHandler(std::bind(&AudioServicePrivate::handlerMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)),
    mStoreHandler(std::bind(&AudioServicePrivate::handlerStoreMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3))
{
	VLOGD("@audio AudioServicePrivate constructor --start--");
		
	m_volSlidePlaySlideType = 1;
	
	mVolumeMap.clear();
	
    m_CurrentVolume = 15;

	mCurEqmode = AudioPersistent::getEqualizerItem();
	
    m_CustomBass0 = AudioPersistent::getBass0Equalizer();
	m_CustomBass1 = AudioPersistent::getBass1Equalizer();

    m_CustomMiddle0 = AudioPersistent::getMiddle0Equalizer();
	m_CustomMiddle1 = AudioPersistent::getMiddle1Equalizer();
	m_CustomMiddle2 = AudioPersistent::getMiddle2Equalizer();

    m_CustomTreble0 = AudioPersistent::getTreble0Equalizer();
	m_CustomTreble1 = AudioPersistent::getTreble1Equalizer();
	m_isNavSupressMedia = AudioPersistent::getNavMixMediaStatus();

	m_SpdChg = 0;

	mMICMute = AudioPersistent::getMICMuteStatus();
	
    m_CurStreamType = 0;
    m_ASFocus.clear();

	pChime = make_shared<Chime>("plug:softvol5");
	pChime->setSampleFadeMs(RADAR_SAMPLE_FADE_MS);
	mLastPdcType = 0;
	
    pSoftkey = make_shared<WavPlay>("plug:softvol6");
    pSoftkey->setWavPath(KEY_WAV_FILE);

	pCarLockSound = make_shared<WavPlay>("plug:softvol12");//softvol12
	pCarLockSound->setWavPath(CAR_SOUND_EQ1);

	pSlide = make_shared<WavPlayInterruptible>("plug:softvol11");
	pSlide->setWavPath(VOLUME_SLIDE_MEDIA);//default
	
	m_AudioSource = AS_Music;
	mActiveENT = AUDIO_LOCAL_MUSIC;
	
	mFocusType = Focus_SYS;
	
    m_CurrentVolume = AudioPersistent::getStreamVolume(AUDIO_LOCAL_MUSIC);
	if(m_CurrentVolume == 0)
		m_MuteItem = true;
	else
		m_MuteItem = false;
	
	mEntVol = m_CurrentVolume;
	
	initVolumeMap();
	initializeEQ();

	isAdjustVolInterrupted = false;
	isNav = false;
	isCalling = false;
	isReversing = false;
	isVR = false;
	isRing = false;
	isHicarAlarm = false;
	requestAudioSource(m_AudioSource);
	VLOGD("@audio AudioServicePrivate constructor --end--");
}

AudioServicePrivate::~AudioServicePrivate()
{
    cout<<"AudioServicePrivate unconstractor"<<endl;
}

void AudioServicePrivate::initVolumeMap()
{
	VLOGD("@audio-init %s", __func__);

    for(int i = 0 ; i <= AUDIO_LOCAL_RADIO ; ++i){
        shared_ptr<VolumeData> ptmp = make_shared<VolumeData>();
        ptmp->m_streamtype = i;
        ptmp->m_showVol = AudioPersistent::getStreamVolume(i);

		if(ptmp->m_showVol != 0)
            ptmp->m_muteState = false;
		else
			ptmp->m_muteState = true;

		if(i== AUDIO_PHONE_MUSIC){
			ptmp->m_showVol = VOL_MIN_LEVEL;
			ptmp->m_muteState = true;
		}else if(i== AUDIO_LOCAL_RADIO){
			ptmp->m_showVol = VOL_MIN_LEVEL;
			ptmp->m_muteState = true;
        }
		
        ptmp->m_realVol = ptmp->m_showVol;
        mVolumeMap[i] = ptmp;

		setALSAVolume(ptmp->m_streamtype, ptmp->m_showVol);
    }
	setALSAVolume(AUDIO_LOCAL_CARLOCK, 37);
	setALSAVolume(AUDIO_LOCAL_SLIDE, 10);
	//初始化记忆参数
	m_TouchSound = AudioPersistent::getTouchSoundStatus();
	m_isNavSupressMedia = AudioPersistent::getNavMixMediaStatus();

	m_Spdcmp = AudioPersistent::getSpdCmpStatus();
	mSpdCmpEnable = AudioPersistent::getSpdCmpEnableStatus(); //音随使能
	mCarLockSound = AudioPersistent::getCarSoundLockEQ();
	setCarLockSound(mCarLockSound, false, true);
	m_SpdcmpLastInx = 0;
	
	mCmpVal = 0;//补偿值
	VLOGD("@zhz m_TouchSound init value = %d", m_TouchSound);
}

void AudioServicePrivate::resetVolumeMap()
{
	VLOGD("@audio-reset %s, size %d", __func__, mVolumeMap.size());
	if(mVolumeMap.size() == 0){
		VLOGD("@audio-reset %s map size : %d error", __func__, mVolumeMap.size());
		return;
	}

	if(mVolumeMap.size() == (AUDIO_LOCAL_RADIO+1)) {
		
		for(unsigned int i = 0 ; i < mVolumeMap.size() ; ++i){
			shared_ptr<VolumeData> tmp = getVolumeData(i);
			if(tmp == nullptr){
				VLOGD("@audio-error %s, tmp getVolumeData(nullptr)", __func__);
				return ;
			}
			
			tmp->m_showVol = AudioPersistent::resetStreamVolume(i);
			
			if(tmp->m_showVol != 0)
	            tmp->m_muteState = false;
			else
				tmp->m_muteState = true;

			if(i == AUDIO_PHONE_MUSIC || i== AUDIO_LOCAL_RADIO){
				tmp->m_showVol = VOL_MIN_LEVEL;
				tmp->m_muteState = true;
			}
			
	        tmp->m_realVol = tmp->m_showVol;

			//VLOGD("@audio %d, tmp->m_showVol = %d", tmp->m_streamtype, tmp->m_showVol);
			setALSAVolume(tmp->m_streamtype, tmp->m_showVol);
		}
	} else {
	
		VLOGD("@audio-reset %s, size %d error!", __func__, mVolumeMap.size());
	}
	setALSAVolume(AUDIO_LOCAL_CARLOCK, 37);
	setALSAVolume(AUDIO_LOCAL_SLIDE, 10);
	m_TouchSound = AudioPersistent::getTouchSoundStatus();
	m_isNavSupressMedia = AudioPersistent::getNavMixMediaStatus();

	m_Spdcmp = AudioPersistent::getSpdCmpStatus();
	mSpdCmpEnable = AudioPersistent::getSpdCmpEnableStatus(); //音随使能
	mCarLockSound = AudioPersistent::getCarSoundLockEQ();
	setCarLockSound(mCarLockSound, false, true);
	m_SpdcmpLastInx = 0;
	
	mCmpVal = 0;//补偿值
}

void AudioServicePrivate::initializeEQ()
{
	//init EQ
	if(mCurEqmode != EQ_CUSTOM){
		g_AudioCtrl->Set(EQ_PRESET, mCurEqmode);
	}else{
		g_AudioCtrl->Set(EQ_BAND0, m_CustomBass0);
		g_AudioCtrl->Set(EQ_BAND1, m_CustomBass1);	
		g_AudioCtrl->Set(EQ_BAND2, m_CustomMiddle0);	
		g_AudioCtrl->Set(EQ_BAND3, m_CustomMiddle1);
		g_AudioCtrl->Set(EQ_BAND4, m_CustomMiddle2);
		g_AudioCtrl->Set(EQ_BAND5, m_CustomTreble0);
		g_AudioCtrl->Set(EQ_BAND6, m_CustomTreble1);	
	}
}

void AudioServicePrivate::controlMute(int value)
{
	VLOGD("%s value : %d", __func__, value);
}

void AudioServicePrivate::controlGain(int value)
{
	VLOGD("%s value : %d", __func__, value);
}

void AudioServicePrivate::controlBass(int value)
{
	VLOGD("%s value : %d", __func__, value);
}

void AudioServicePrivate::controlMiddle(int value)
{
	VLOGD("%s value : %d", __func__, value);
}

void AudioServicePrivate::controlTreble(int value)
{
	VLOGD("%s value : %d", __func__, value);
}

void AudioServicePrivate::SetInputAudio(int source)
{

}

void AudioServicePrivate::SetOutputAudio(int volume)
{

}

void AudioServicePrivate::SetOutputVolume(int volume)
{
 
}

int AudioServicePrivate::pos2volume(int pos)
{
	return 0;
}

int AudioServicePrivate::getVolume(const int streamType)
{
	int val = 0;
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return val;
	}
	
	val = ptmp->m_showVol;
	return val;
}

int AudioServicePrivate::getVDObjMemberShowVol(const int &streamType)
{
	return getVolume(streamType);
}

int AudioServicePrivate::getVDObjMemberRealVol(const int &streamType)
{
	int val = 0;
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return val;
	}
	val = ptmp->m_realVol;
	return val;
}

bool AudioServicePrivate::getVDObjMuteState(const int &streamType)
{
	int val = 0;
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return false;
	}
	val = ptmp->m_muteState;
	return val;
}

void AudioServicePrivate::setVDObjMemberShowVol(const int &streamType, int value)
{
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return ;
	}
	ptmp->m_showVol = value;
}

void AudioServicePrivate::setVDObjMemberRealVol(const int &streamType, int value)
{
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return ;
	}
	
	ptmp->m_realVol = value;
}

void AudioServicePrivate::setVDObjMuteState(const int &streamType, bool value)
{
	shared_ptr<VolumeData> ptmp = getVolumeData(streamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, ptmp getVolumeData(nullptr)", __func__);
		return ;
	}
	
	ptmp->m_muteState = value;
}

void AudioServicePrivate::releaseAudioSource(const int source)
{
	Q_Q(AudioService);
	
	VLOGD("%s source : %d", __func__, source);
    shared_ptr<AudioPriority> ptmp = nullptr;
	if(source == AS_Alarm)
		 isReversing = false;
	
    for( vector<shared_ptr<AudioPriority>>::iterator itor = m_ASFocus.begin();itor!=m_ASFocus.end();++itor){

		if(source == (*itor)->m_audiosource){
			
            if((source == AS_CarlifePhone)||(source == AS_AutoPhone)||(source == AS_CarplayPhone)||\
                (source == AS_BluetoothPhone)||(source == AS_HiCarPhone)||(source == AS_CarLinkPhone)) {
                isCalling = false;
            }else if(source == AS_RING){
				isRing = false;
			}else if(source == AS_TTS) {
                isVR = false;
            }else if(source == AS_Navi) {
                isNav = false;
            }else if(source == AS_Hicar_ALARM){
				isHicarAlarm = false;
			}

			if(!isEntAudioSouce(source))
            	itor = m_ASFocus.erase(itor);
			else{
				requestAudioSource(AS_Music);
			}
			
            break;
        }
    }

    if(m_ASFocus.size() != 0){
        ptmp = m_ASFocus.back();
        m_CurStreamType = ptmp->m_streamType;
		m_AudioSource = ptmp->m_audiosource;
    }else{
        m_CurStreamType = AUDIO_LOCAL_MUSIC;//default  
        m_AudioSource = AS_Music;

		shared_ptr<AudioPriority> baseFocus = make_shared<AudioPriority>( 4, AUDIO_LOCAL_MUSIC, AS_Music);
		m_ASFocus.push_back(baseFocus);
    }

	if(source != AS_Alarm)
		emit q->onAudioSourceChange(m_AudioSource);
	
	m_CurrentVolume = getVolume(m_CurStreamType);
	emit q->onCurVolumeValueChange(m_CurrentVolume);
	
    adjustVolume();
}

int AudioServicePrivate::getCurAudioSource()
{
	VLOGD("%s source : %d", __func__, m_AudioSource);
	return m_AudioSource;
}

bool AudioServicePrivate::requestAudioSource(const int source)
{
	VLOGD("%s E211 source : %d", __func__, source);

	Q_Q(AudioService);

    int priority = 0;
    int streamtype = 0;
	int nTmpRealVol = 0;
	int nTmpShowVol = 0;
	bool ret = true;
	
    switch(source)  {
    case AS_Alarm:
    case AS_Aux: {
        priority = 0;
        streamtype = AUDIO_LOCAL_ALARM;
        break;
    }
	case AS_Hicar_ALARM:{
		priority = 0;
        streamtype = AUDIO_HICAR_ALARM;
		break;
	}
    case AS_CarlifePhone:
    case AS_AutoPhone:
    case AS_CarplayPhone:
    case AS_BluetoothPhone: 
	case AS_HiCarPhone:
	case AS_CarLinkPhone:{
        priority = 0;
        streamtype = AUDIO_PHONE_TELL;
        break;
    }
	case AS_RING: {
		priority = 1;
		streamtype = AUDIO_PHONE_RING;
		break;
	}
    case AS_TTS:{
        priority = 2;
        streamtype = AUDIO_PHONE_TTS;
        break;
    }
    case AS_Navi:{
        priority = 3;
        streamtype = AUDIO_PHONE_NAVI;
        break;
    }
    case AS_Radio: {
        priority = 4;
		streamtype = AUDIO_LOCAL_RADIO;
        break;
    }
	case AS_Video:
    case AS_Music:
    case AS_Idle:{
        priority = 4;
        streamtype = AUDIO_LOCAL_MUSIC;
        break;
    }
    case AS_CarplayMusic:
    case AS_ECLinkBluetoothMusic:
    case AS_HiCarBluetoothMusic:
    case AS_BluetoothMusic:
    case AS_AutoMusic:
    case AS_CarlifeMusic:
    case AS_ECLinkMusic:
    case AS_HiCarMusic: 
	case AS_CarLinkMusic:{
        priority = 4;
        streamtype = AUDIO_PHONE_MUSIC;
        break;
    }
    default: {
        priority = 4;
        streamtype = AUDIO_LOCAL_MUSIC;
        break;
    }
    }
    shared_ptr<AudioPriority> pAudioPriority = make_shared<AudioPriority>(priority, streamtype, source);
    if(m_ASFocus.empty()){
        m_ASFocus.push_back(pAudioPriority);
    } else {
        shared_ptr<AudioPriority> ptmp = m_ASFocus.back();
        if(ptmp->m_priority < pAudioPriority->m_priority){
			
			// 娱乐音源可以 替换
			if(isEntStreamType(pAudioPriority->m_streamType)){
				// Ent 优先级最低且只能存在一个, index must be 0
				int ntmpStream = m_ASFocus[0]->m_streamType;
				int ntmpSource = m_ASFocus[0]->m_audiosource;
				if(ntmpSource == pAudioPriority->m_audiosource){
					VLOGD("%s lower priority and same Ent source ,return directly!", __func__);
					emit q->onAudioSourceChange(pAudioPriority->m_audiosource);
            		return false;
				}

				VLOGD("%s lower priority,just replace Ent directly!", __func__);
				//replace Ent source
				m_ASFocus[0]->m_audiosource = pAudioPriority->m_audiosource;
				m_ASFocus[0]->m_streamType = pAudioPriority->m_streamType;
				//replacce Ent volume,old->new
				shared_ptr<VolumeData> vdswapA = getVolumeData(ntmpStream);
				if(vdswapA == nullptr){
					VLOGD("@audio-error %s, vdswapA getVolumeData(nullptr)", __func__);
					return false;
				}
				
				nTmpRealVol = vdswapA->m_realVol;
				nTmpShowVol = vdswapA->m_showVol;
				vdswapA->m_showVol = VOL_MIN_LEVEL;
				vdswapA->m_realVol = VOL_MIN_LEVEL;
				vdswapA->m_muteState = true;

				shared_ptr<VolumeData> vdswapB = getVolumeData(pAudioPriority->m_streamType);
				if(vdswapB == nullptr){
					VLOGD("@audio-error %s, vdswapB getVolumeData(nullptr)", __func__);
					return false;
				}
				
				vdswapB->m_showVol = nTmpShowVol;
				vdswapB->m_realVol = nTmpRealVol;
				mActiveENT = pAudioPriority->m_streamType;
				if (mFocusType == Focus_Hicar && isNav)
					applyHicarNavEntRealVol(vdswapB);
				//adjustVolume 会在释放优先级高的焦点时调用调整
				entVolumeAdjust();
				emit q->onAudioSourceChange(pAudioPriority->m_audiosource);
				return true;
			}else{
				VLOGD("%s lower priority,return directly!", __func__);
            	return false;
			}
			
        }else if(ptmp->m_priority == pAudioPriority->m_priority){
			
			if(ptmp->m_streamType != pAudioPriority->m_streamType){
				if(isEntStreamType(pAudioPriority->m_streamType)){
					//delete forward
					shared_ptr<VolumeData> vd = getVolumeData(ptmp->m_streamType);
					if(vd == nullptr){
						VLOGD("@audio-error %s, vd getVolumeData(nullptr)", __func__);
						return false;
					}
					
					nTmpRealVol = vd->m_realVol;
					nTmpShowVol = vd->m_showVol;
					vd->m_showVol = VOL_MIN_LEVEL;
					vd->m_realVol = VOL_MIN_LEVEL;
					vd->m_muteState = true;
					m_ASFocus.pop_back();
					//replace new
					shared_ptr<VolumeData> vd2 = getVolumeData(pAudioPriority->m_streamType);
					if(vd2 == nullptr){
						VLOGD("@audio-error %s, vd2 getVolumeData(nullptr)", __func__);
						return false;
					}
					
					vd2->m_showVol = nTmpShowVol;
					vd2->m_realVol = nTmpRealVol;
					
					mActiveENT = pAudioPriority->m_streamType;
				}
				
				m_ASFocus.push_back(pAudioPriority);
				VLOGD("%s have replace new!", __func__);
			} else {
				if(ptmp->m_audiosource == pAudioPriority->m_audiosource){
					VLOGD("%s have already existed! same AudioSource", __func__);
					return true;
				}

				/* 同流异音源（如 Music↔Video）：只更新源并通知，不调音量渐变 */
				VLOGD("%s have already existed! switch source %d -> %d", __func__, ptmp->m_audiosource, source);
				ptmp->m_audiosource = source;
				m_AudioSource = source;
				if(source != AS_Alarm)
					emit q->onAudioSourceChange(m_AudioSource);
				return true;
			}
		}else{
            m_ASFocus.push_back(pAudioPriority);
        }
    }

	m_AudioSource = source;

	if(source != AS_Alarm)
		emit q->onAudioSourceChange(m_AudioSource);
	
    if(source == AS_Alarm || source == AS_Aux) {
		m_ASFocus.pop_back();
        isReversing = true;
    }else if((source == AS_CarlifePhone)||(source == AS_AutoPhone)||(source == AS_CarplayPhone)||\
                (source == AS_BluetoothPhone)||(source == AS_HiCarPhone)||(source == AS_CarLinkPhone)) {
        isCalling = true;
    }else if(source == AS_RING){
		isRing = true;
	}else if(source == AS_TTS){
        isVR = true;
    }else if(source == AS_Navi){
        isNav = true;
    }else if(source == AS_Hicar_ALARM){
		isHicarAlarm = true;
	}

	//倒车场景下不更新当前流及音量值,会导致 Call(此情况可调节+/-) 的音量无法调节
	if(source != AS_Alarm) {
		shared_ptr<VolumeData> vd3 = getVolumeData(streamtype);
		if(vd3 == nullptr){
			VLOGD("@audio-error %s, vd3 getVolumeData(nullptr)", __func__);
			return false;
		}
		m_CurrentVolume = vd3->m_showVol;
   		m_CurStreamType = streamtype;
		VLOGD("@audio %s CurStreamType:%d CurVol:%d", __func__, m_CurStreamType, m_CurrentVolume);
		emit q->onCurVolumeValueChange(m_CurrentVolume);
	}
	
    adjustVolume();

    return ret;
}

void AudioServicePrivate::requestMute(const int item)
{
	Q_Q(AudioService);
	int finalVol = 0;
	
	shared_ptr<VolumeData> ptmp = getVolumeData(mActiveENT);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
		return;
	}
	if(item == MI_Mute){
		ptmp->m_muteState = true;
		ptmp->m_realVol = 0;
	
		finalVol = ptmp->m_realVol;
	}else if(item == MI_Unmute){
		/*
		* 1. 解除静音时,音量0,恢复默认
		* 2. RVC场景,vol>3抑制,无vol信号
		*/
		ptmp->m_muteState = false;
		
		if(ptmp->m_showVol == VOL_MIN_LEVEL){

			ptmp->m_showVol = AudioPersistent::resetStreamVolume(mActiveENT);
			storeAudioIniInfo(HANDLER_STORE_STREAM_VOL, mActiveENT, ptmp->m_showVol);
			//同步切源时产生的声音不同步
			mEntVol = ptmp->m_showVol;
		}

		//同步 +/- 按键音量
		m_CurrentVolume = ptmp->m_showVol;
		emit q->onCurVolumeValueChange(m_CurrentVolume);
		
		if(isReversing == true || isNav == true){
			//RCV NAVI will suppress ENT
			if(ptmp->m_showVol >= VOL_SUPPRESSION)
				ptmp->m_realVol = VOL_SUPPRESSION;
			else
				ptmp->m_realVol = ptmp->m_showVol;

			finalVol = ptmp->m_realVol;
		}else{
			ptmp->m_realVol = ptmp->m_showVol;
			emit q->onVolumeChange(mActiveENT, ptmp->m_showVol);

			//speed compensation
			if(mSpdCmpEnable == USE_ON){
				finalVol = ptmp->m_realVol + mCmpVal;
				if(finalVol >= VOL_MAX_LEVEL)
					finalVol = VOL_MAX_LEVEL;
			} else {
				finalVol = ptmp->m_realVol;
			}
		}	
	}

	setALSAVolume(mActiveENT, finalVol);
	m_MuteItem = item;
	//AudioPersistent::setMuteStatus(item);
	emit q->onMuteChange(item);
}

void AudioServicePrivate::requestMuteToggole()
{
	VLOGD("%s m_MuteItem : %d -> %d",__func__, m_MuteItem, !m_MuteItem);

	//Call VR Navi状态下调节静音,再+/-音量键会导致音量同步到媒体ENT音量上
	if(isCalling == true || isVR == true || isNav == true || isRing == true)
		return;
	
	if(m_MuteItem == MI_Unmute){
		requestMute(MI_Mute);
	}else if(m_MuteItem == MI_Mute){
		requestMute(MI_Unmute);
	}
}

int AudioServicePrivate::getMuteStatus()
{
	VLOGD("%s m_MuteItem : %d", __func__, m_MuteItem);
	return m_MuteItem;
}

void AudioServicePrivate::requestIncreaseVolume()
{
	Q_Q(AudioService);
	VLOGD("%s CurStreamType : %d CurrentVolume : %d", __func__, m_CurStreamType, m_CurrentVolume);
	if((isReversing == true) && (isCalling == false	))
		return;
	//补偿后总音量
	int finalVol = 0;
	
	//‘+’按键 音量不变，恢复出声. 条件 当前流类型为 ENT
	if((m_MuteItem == MI_Mute)&&(m_CurrentVolume !=0)&&(isEntStreamType(m_CurStreamType))){
		requestMuteToggole();
		return;	
	}
	
	++m_CurrentVolume;
	if(m_CurrentVolume >= VOL_MAX_LEVEL)
		m_CurrentVolume = VOL_MAX_LEVEL;
	
	emit q->onCurVolumeValueChange(m_CurrentVolume);

	shared_ptr<VolumeData> ptmp = getVolumeData(m_CurStreamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
		return;
	}
	
	if(m_CurrentVolume == 1){
		ptmp->m_muteState = false;
	}
	
	ptmp->m_realVol = m_CurrentVolume;
	ptmp->m_showVol = m_CurrentVolume;

	//Speed Compensation
	if((mSpdCmpEnable == USE_ON) && isEntStreamType(m_CurStreamType)){
		finalVol = m_CurrentVolume + mCmpVal;
		finalVol = (finalVol > VOL_MAX_LEVEL) ? VOL_MAX_LEVEL:finalVol;
	} else
		finalVol = m_CurrentVolume;
		
	setALSAVolume(m_CurStreamType, finalVol);

	storeAudioIniInfo(HANDLER_STORE_STREAM_VOL,m_CurStreamType,m_CurrentVolume);

	if(isEntStreamType(m_CurStreamType))
	{
		mEntVol = m_CurrentVolume;
		if(m_MuteItem == MI_Mute)
			emit q->onMuteChange(MI_Unmute);
		
		m_MuteItem = MI_Unmute;
		//AudioPersistent::setMuteStatus(m_MuteItem);
		setVDObjMuteState(mActiveENT, false);
	}
		
	emit q->onVolumeChange(m_CurStreamType, m_CurrentVolume);

	//同步解除静音状态
	
}

void AudioServicePrivate::requestDecreaseVolume()
{
	Q_Q(AudioService);
	VLOGD("%s CurStreamType : %d CurrentVolume : %d", __func__, m_CurStreamType, m_CurrentVolume);
	
	if((isReversing == true) && (isCalling == false	))
		return;

	int finalVol = 0;
	//‘-’按键 音量不变，恢复出声. 电话,导航,语音状态下 不解除 ENT静音
	if((m_MuteItem == MI_Mute)&&(m_CurrentVolume !=0)&&(isEntStreamType(m_CurStreamType))){
		requestMuteToggole();
		return;	
	}
	
	--m_CurrentVolume;
	if(m_CurrentVolume <= VOL_MIN_LEVEL) {
		//通话音量 >= 1
		if(m_CurStreamType == AUDIO_PHONE_TELL || m_CurStreamType == AUDIO_PHONE_RING )
			m_CurrentVolume = VOL_MIN_LEVEL + 1;
		else
			m_CurrentVolume = VOL_MIN_LEVEL;
	}
	emit q->onCurVolumeValueChange(m_CurrentVolume);
	
	shared_ptr<VolumeData> ptmp = getVolumeData(m_CurStreamType);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
		return;
	}
	
	if(m_CurrentVolume == VOL_MIN_LEVEL){
		ptmp->m_muteState = true;
	}
	ptmp->m_realVol = m_CurrentVolume;
	ptmp->m_showVol = m_CurrentVolume;

	//Speed Compensation
	if((mSpdCmpEnable == USE_ON) && isEntStreamType(m_CurStreamType)&&(m_CurrentVolume!=VOL_MIN_LEVEL)){
		finalVol = m_CurrentVolume + mCmpVal;
		finalVol = (finalVol > VOL_MAX_LEVEL) ? VOL_MAX_LEVEL:finalVol;
	} else
		finalVol = m_CurrentVolume;
	
	setALSAVolume(m_CurStreamType, finalVol);

	storeAudioIniInfo(HANDLER_STORE_STREAM_VOL,m_CurStreamType,m_CurrentVolume);

	if(isEntStreamType(m_CurStreamType))
	{
		mEntVol = m_CurrentVolume;
		//音量未衰减到0时,同步解除静音状态
		if(m_CurrentVolume != VOL_MIN_LEVEL){
			setVDObjMuteState(mActiveENT, false);
			if(m_MuteItem == MI_Mute) {
			//Mute -> 音量 -
				m_MuteItem = MI_Unmute;
				emit q->onMuteChange(m_MuteItem);
			}
		}else{
			m_MuteItem = MI_Mute;
			setVDObjMuteState(mActiveENT, true);
			emit q->onMuteChange(m_MuteItem);
		}
		//AudioPersistent::setMuteStatus(m_MuteItem);
	}	
	emit q->onVolumeChange(m_CurStreamType, m_CurrentVolume);
	
}


void AudioServicePrivate::adjustDspGainAuto(const int db)
{
	VLOGD("%s no use %d", __func__, db);
}

void AudioServicePrivate::showVolumeInfo()
{
	qDebug()<<"stream\trealvol\tshowvol\tmutestate\t\tLinuxVol";
	QString str;
	int value = -1;
	for(int i=0;i<=AUDIO_LOCAL_RADIO;++i){
		shared_ptr<VolumeData> vd = getVolumeData(i);
		if(vd == nullptr){
			VLOGD("@audio-error %s, vd getVolumeData(nullptr)", __func__);
			return;
		}
		switch(i){
			case  0: str = "Msic";	break;
			case  1: str = "Bt  ";	break;
			case  2: str = "Navi";	break;
			case  3: str = "Call";	break;
			case  4: str = "TTS ";	break;
			case  5: str = "Alar";	break;
			case  6: str = "Beep";	break;
			case  7: str = "Ring";	break;
			case  8: str = "Hial";	break;
			case  9: str = "HiNo";	break;
			case 10: str = "Radi";	break;
			default : str = " ";	break;
		}
		if(i == AUDIO_LOCAL_RADIO)
			g_AudioCtrl->Get(RADIO_VOL, &value);
		else
			g_AudioCtrl->Get(i + SOFTMST0_VOL, &value);
		qDebug()<<str<<"\t"<<vd->m_realVol<<"\t"<<vd->m_showVol<<"\t"<<vd->m_muteState<<"\t"<<value<<"%";
	}
	qDebug()<<"-------------------------------------------------";
	qDebug()<<"|isReversing:"<<isReversing<<"\t|isCalling:"<<isCalling<<"\t|";
	qDebug()<<"|isVR       :"<<isVR<<"\t|isNav\t  :"<<isNav<<"\t|";
	qDebug()<<"|isRing	    :"<<isRing;	
	qDebug()<<"|isMute       :"<<m_MuteItem<<"\t|isTouch :"<<m_TouchSound<<"\t     \t|";
	qDebug()<<"-------------------------------------------------";
}

void AudioServicePrivate::showFocusInfo()
{
	qDebug()<<"---------------------------------";
	qDebug()<<"|pri\t|"<<"stream|"<<"Source";
	qDebug()<<"---------------------------------";
	for(auto i = m_ASFocus.end()-1; i >= m_ASFocus.begin(); --i){
		qDebug()<<"|"<<(*i)->m_priority<<"\t|"<<(*i)->m_streamType<<"\t|"<<(*i)->m_audiosource;
	}
	qDebug()<<"---------------------------------";
}

void AudioServicePrivate::adjustVolume()
{
    isAdjustVolInterrupted = true;
    mHandler.sendMessage(HANDLER_WHAT_ADJUST_VOL,0,0);
}

void AudioServicePrivate::handlerMessage(const int32_t &msg, const int32_t arg0, const int32_t arg1)
{
    if(HANDLER_WHAT_ADJUST_VOL == msg)
    {
        isAdjustVolInterrupted = false;

        shared_ptr<VolumeData> vd = getVolumeData(m_CurStreamType);
		if(vd == nullptr){
			VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
			return;
		}
		
        if(mFocusType == Focus_Hicar){
			if(!adjustVolumeDataInHicar(vd)){
		        return;
		    }
		}else {
			if(!adjustVolumeData(vd)){
		        return;
		    }
		}
		
        for(auto it = mVolumeMap.begin(); it!=mVolumeMap.end();it++){
            shared_ptr<VolumeData> data = it->second;
            if(data->m_streamtype == m_CurStreamType)
                continue;
			
			if(mFocusType == Focus_Hicar){
				if(!adjustVolumeDataInHicar(data)){
		        	break;
		    	}
			}else {
				if(!adjustVolumeData(data)){
		        	break;
		    	}
			}
        }
    }

}


void AudioServicePrivate::storeAudioIniInfo(int code, int arg0, int arg1)
{
	mStoreHandler.sendMessage(code,arg0,arg1);
}

void AudioServicePrivate::handlerStoreMessage(const int32_t &msg, const int32_t arg0, const int32_t arg1)
{
	VLOGD("@audio %s msg %d, arg0 %d, arg1 %d",__func__, msg, arg0, arg1);
    if( HANDLER_STORE_STS_MUTE == msg ){
		AudioPersistent::setMuteStatus(arg0);
	}else if( HANDLER_STORE_VAL_EQ == msg ){
		AudioPersistent::setEqualizerItem(arg0);
	}else if( HANDLER_STORE_VAL_BASS0 == msg ){
		AudioPersistent::setBass0Equalizer(arg0);
	}else if( HANDLER_STORE_VAL_MID0 == msg ){
		AudioPersistent::setMiddle0Equalizer(arg0);
	}else if( HANDLER_STORE_VAL_TRE0 == msg ){
		AudioPersistent::setTreble0Equalizer(arg0);
	}else if( HANDLER_STORE_VAL_BASS1 == msg ){
		AudioPersistent::setBass1Equalizer(arg0);
	}else if( HANDLER_STORE_VAL_MID1 == msg ){
		AudioPersistent::setMiddle1Equalizer(arg0);
	}else if( HANDLER_STORE_VAL_TRE1 == msg ){
		AudioPersistent::setTreble1Equalizer(arg0);
	}else if( HANDLER_STORE_STS_MIC == msg ){
		AudioPersistent::setMICMuteStatus(arg0);
	}else if( HANDLER_STORE_STREAM_VOL == msg ){
		AudioPersistent::saveStreamVolume(arg0, arg1);
	}else if( HANDLER_STORE_STS_TOUCH == msg ){
		AudioPersistent::setTouchSoundStatus(arg0);
	}else if( HANDLER_STORE_VAL_SPD == msg ){
		AudioPersistent::setSpdCmpStatus(arg0);
	}else if( HANDLER_STORE_STS_SPD == msg ){
		AudioPersistent::setSpdCmpEnableStatus(arg0);
	}else if( HANDLER_RESET_STREAM == msg ){
		AudioPersistent::resetStreamVolume(arg0);
	}else if( HANDLER_RESET_CLEAR == msg ){
		AudioPersistent::reset();
	}else if( HANDLER_STORE_VAL_AVAS == msg){
		AudioPersistent::setAVASVolLevel(arg0);
	}else if( HANDLER_STORE_STS_NAVIMEDIA == msg){
		AudioPersistent::setNavMixMediaStatus(arg0);
	}else if( HANDLER_STORE_VAL_CARLOCKSOUND == msg){
		AudioPersistent::setCarSoundLockEQ(arg0);
	}else if( HANDLER_STORE_VAL_AVASTYPE == msg){
		AudioPersistent::setAVASSoundType(arg0);
	}
}


bool AudioServicePrivate::isEntStreamType(const int streamType)
{
	bool ret = false;
	
	if(streamType == AUDIO_LOCAL_MUSIC ||streamType == AUDIO_PHONE_MUSIC||streamType == AUDIO_LOCAL_RADIO)
		ret = true;
	
	return ret;
}

bool AudioServicePrivate::isEntAudioSouce(const int source)
{
	return (source == AS_Radio ||
			source == AS_Music ||
			source == AS_Idle  ||
			source == AS_Video || 
			source == AS_CarplayMusic ||
		    source == AS_ECLinkBluetoothMusic ||
		    source ==  AS_HiCarBluetoothMusic ||
		    source ==  AS_BluetoothMusic 	  ||
		    source ==  AS_AutoMusic 			||
		    source ==  AS_CarlifeMusic 			||
		    source ==  AS_ECLinkMusic 			||
		    source ==  AS_HiCarMusic 			||
			source ==  AS_CarLinkMusic );
}

void AudioServicePrivate::entFadeIn()
{
	//当前媒体流非ENT
	if(!isEntStreamType(m_CurStreamType))
		return;
	
	int step = 3;
	int finalVol = 0;
	
	shared_ptr<VolumeData> ptmp = getVolumeData(mActiveENT);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
		return;
	}
	
	if(isReversing == true){
		ptmp->m_realVol = VOL_SUPPRESSION;
		ptmp->m_muteState = false;
		//VLOGD("@audio setALSAVolume --55--");
		setALSAVolume(mActiveENT, ptmp->m_realVol);
		return;
	}
	while(ptmp->m_realVol != ptmp->m_showVol){
		QThread::msleep(40);
		if(ptmp->m_realVol == ptmp->m_showVol)
			break;
		
		ptmp->m_realVol += step;
		if(ptmp->m_realVol >= ptmp->m_showVol)
			ptmp->m_realVol = ptmp->m_showVol;

		if(mSpdCmpEnable == USE_ON){
			finalVol = ptmp->m_realVol + mCmpVal;
			if(finalVol > VOL_MAX_LEVEL)
				finalVol = VOL_MAX_LEVEL;
		}else
			finalVol = ptmp->m_realVol;
		setALSAVolume(mActiveENT, finalVol);	
	}
}

void AudioServicePrivate::entFadeOut()
{
	//当前媒体流非ENT
	if(!isEntStreamType(m_CurStreamType))
		return;
	
	int step = 3;
	shared_ptr<VolumeData> ptmp = getVolumeData(mActiveENT);
	if(ptmp == nullptr){
		VLOGD("@audio-error %s, getVolumeData(nullptr)", __func__);
		return;
	}
	
	while(ptmp->m_realVol != VOL_MIN_LEVEL){
		QThread::msleep(40);
		if(ptmp->m_realVol == VOL_MIN_LEVEL)
			break;
		
		ptmp->m_realVol -= step;
		if(ptmp->m_realVol <= VOL_MIN_LEVEL)
			ptmp->m_realVol = VOL_MIN_LEVEL;
		
		setALSAVolume(mActiveENT, ptmp->m_realVol);	
	}

}



shared_ptr<VolumeData> AudioServicePrivate::getVolumeData(const int &streamType)
{
	shared_ptr<VolumeData> ret = nullptr;
	//map<int, shared_ptr<VolumeData>>::iterator it;

	if(streamType > AUDIO_LOCAL_RADIO){
		VLOGD("@audio-error getVolumeData params nullptr!");
		return nullptr;
	}else{
		auto it = mVolumeMap.find(streamType);
		if(it != mVolumeMap.end()){
			ret = it->second;
		}
	}

    return ret;
}

bool AudioServicePrivate::adjustVolumeDataInHicar(shared_ptr<VolumeData> vd)
{
    if( vd == nullptr ){
        return false;
    }

    int sleepTime = 0;
    int tmpVolume = 0;
	int finalVol = 0;
    while(!isAdjustVolInterrupted){
        sleepTime = 40;
        tmpVolume = vd->m_realVol;
        if(isReversing && (vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype ==AUDIO_LOCAL_RADIO || \
           vd->m_streamtype == AUDIO_PHONE_NAVI || vd->m_streamtype == AUDIO_PHONE_TTS || vd->m_streamtype ==AUDIO_LOCAL_BEEP || vd->m_streamtype == AUDIO_HICAR_NOTICE )){
           
			if(isCalling || isRing ){
                setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                vd->m_realVol = VOL_MIN_LEVEL;
                vd->m_muteState = true;
                return true;
            }
			
			if(vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype ==AUDIO_LOCAL_RADIO || \
				vd->m_streamtype == AUDIO_PHONE_NAVI || vd->m_streamtype == AUDIO_PHONE_TTS|| vd->m_streamtype == AUDIO_HICAR_NOTICE){
				
				if(tmpVolume >= VOL_SUPPRESSION){
					setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (m_MuteItem == MI_Mute) && (isEntStreamType(vd->m_streamtype)) ){
					//Do nothing,keep status
					setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_MIN_LEVEL);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (vd->m_showVol >= VOL_SUPPRESSION)){
					setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (vd->m_showVol < VOL_SUPPRESSION)){
					setALSAVolume(vd->m_streamtype, vd->m_showVol);
					setVDObjMemberRealVol(vd->m_streamtype, vd->m_showVol);
				}else{
					setALSAVolume(vd->m_streamtype, tmpVolume);
				}
				
				if(tmpVolume == VOL_MIN_LEVEL)
					setVDObjMuteState(vd->m_streamtype, true);
				else
					setVDObjMuteState(vd->m_streamtype, false);
				
			}else if(vd->m_streamtype == AUDIO_LOCAL_BEEP || vd->m_streamtype == AUDIO_HICAR_ALARM){
				//恢复beep音量
				vd->m_realVol = vd->m_showVol;
				vd->m_muteState = false;
				setALSAVolume(vd->m_streamtype, vd->m_realVol);
				return true;
			} else {//no use
            	setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
           		vd->m_realVol = VOL_SUPPRESSION;
            	vd->m_muteState = false;
			}

            return true;
        }else if(isCalling && (vd->m_streamtype != AUDIO_LOCAL_ALARM)&&(vd->m_streamtype != AUDIO_PHONE_TELL)){
            //HFT mute other except Alarm
            setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
            vd->m_realVol = VOL_MIN_LEVEL;
            vd->m_muteState = true;
            return true;
		 }else if((isRing) && (vd->m_streamtype != AUDIO_LOCAL_ALARM)&&(vd->m_streamtype != AUDIO_PHONE_TELL)&&(vd->m_streamtype != AUDIO_PHONE_RING)){
            //Ring mute other except Alarm
            setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
            vd->m_realVol = VOL_MIN_LEVEL;
            vd->m_muteState = true;
            return true;
        }else if( isVR &&(vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype == AUDIO_LOCAL_BEEP || \
                          vd->m_streamtype == AUDIO_PHONE_NAVI ||vd->m_streamtype == AUDIO_LOCAL_RADIO || vd->m_streamtype == AUDIO_HICAR_NOTICE )){

            if(vd->m_streamtype == AUDIO_LOCAL_BEEP){
                if(vd->m_realVol != vd->m_showVol){
                    setALSAVolume(vd->m_streamtype, vd->m_showVol);
                    vd->m_realVol = vd->m_showVol;
					if(vd->m_showVol != VOL_MIN_LEVEL)
                    	vd->m_muteState = false;
                }
                return true;
            }else {
                //if(tmpVolume != VOL_MIN_LEVEL){
                    setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                    vd->m_realVol = VOL_MIN_LEVEL;
                    vd->m_muteState = true;
                //}
                return true;
            }
        }else if( isNav && (vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || \
        		vd->m_streamtype == AUDIO_LOCAL_BEEP|| vd->m_streamtype == AUDIO_LOCAL_RADIO)){

            if(vd->m_streamtype == AUDIO_LOCAL_BEEP || vd->m_streamtype == AUDIO_PHONE_MUSIC){
                setALSAVolume(vd->m_streamtype, vd->m_showVol);
                vd->m_realVol = vd->m_showVol;
                if(vd->m_showVol != VOL_MIN_LEVEL)
                	vd->m_muteState = false;
            }else{
            	/* isNavSupressMedia 导航自动降音开关 */
				if(m_isNavSupressMedia != 0 && m_isNavSupressMedia != 1){
					VLOGD("@audio-error m_isNavSupressMedia value is invalid %d!", m_isNavSupressMedia);
					m_isNavSupressMedia = 1;//default value means suppress
				}
				
				if(m_isNavSupressMedia == 1) {
					if(tmpVolume >= VOL_SUPPRESSION){
						setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
						setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
					}else{
						setALSAVolume(vd->m_streamtype, tmpVolume);
					}
				}else{
					vd->m_realVol = vd->m_showVol;
					tmpVolume = vd->m_showVol;
					setALSAVolume(vd->m_streamtype, tmpVolume);
				}
				
				if(tmpVolume == VOL_MIN_LEVEL)
					setVDObjMuteState(vd->m_streamtype, true);
				else
					setVDObjMuteState(vd->m_streamtype, false);
					
            }
            return true;
        } else {

            sleepTime = 100;
			//条件2说明: 申请焦点,ENT覆盖时,real = show 时并未真正执行,所以不能返回
			if((tmpVolume == vd->m_showVol)&&(!isEntStreamType(vd->m_streamtype)))
				return true;

			/* HiCar 通知音：直接开关，不走步进淡入淡出 */
			if(vd->m_streamtype == AUDIO_HICAR_NOTICE){
				setALSAVolume(vd->m_streamtype, vd->m_showVol);
				vd->m_realVol = vd->m_showVol;
				vd->m_muteState = (vd->m_showVol == VOL_MIN_LEVEL);
				return true;
			}

			//适用于 ENT 焦点申请 互相替换时, 及时MUTE 其它(BT/USB/RADIO 三选一)
            if((vd->m_muteState == true)&&(vd->m_showVol == VOL_MIN_LEVEL)){
                setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                vd->m_realVol = VOL_MIN_LEVEL;
                return true;
            } else {
				//场景: ENT 处于静音，保持静音
				if((m_MuteItem == MI_Mute)&&(isEntStreamType(vd->m_streamtype)))
					return true;//vol=0下解除降音仍然静音
				
                tmpVolume = getGradualVolume(tmpVolume, vd->m_showVol);

				if(isEntStreamType(vd->m_streamtype)&&(mSpdCmpEnable==USE_ON)){
					if(tmpVolume == VOL_MIN_LEVEL)
						finalVol = VOL_MIN_LEVEL;
					else{
						finalVol = tmpVolume + mCmpVal;
						finalVol = (finalVol > VOL_MAX_LEVEL)?VOL_MAX_LEVEL:finalVol;
					}
				}else{
					finalVol = tmpVolume;
				}

                setALSAVolume(vd->m_streamtype, finalVol);
                vd->m_realVol = tmpVolume;

				if(vd->m_realVol == VOL_MIN_LEVEL)
					vd->m_muteState = true;
				else
					vd->m_muteState = false;
				
                if(tmpVolume == vd->m_showVol){
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
    return true;
}


bool AudioServicePrivate::adjustVolumeData(shared_ptr<VolumeData> vd)
{
    if( vd == nullptr ){
        return false;
    }

    int sleepTime = 0;
    int tmpVolume = 0;
	int finalVol = 0;
    while(!isAdjustVolInterrupted){
        sleepTime = 40;
        tmpVolume = vd->m_realVol;
        if(isReversing && (vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype ==AUDIO_LOCAL_RADIO || \
           vd->m_streamtype == AUDIO_PHONE_NAVI || vd->m_streamtype == AUDIO_PHONE_TTS || vd->m_streamtype ==AUDIO_LOCAL_BEEP || vd->m_streamtype == AUDIO_HICAR_NOTICE )){
			/* 在倒车兼电话场景, ENT 需要静音 */
			if((isCalling || isRing)){
                //if(tmpVolume != VOL_MIN_LEVEL){
                    setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                    vd->m_realVol = VOL_MIN_LEVEL;
                    vd->m_muteState = true;
               // }
                return true;
            }
			/* 在倒车场景, ENT 只需当前激活的流 抑制 */
			if(vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype ==AUDIO_LOCAL_RADIO || \
				vd->m_streamtype == AUDIO_PHONE_NAVI || vd->m_streamtype == AUDIO_PHONE_TTS || vd->m_streamtype == AUDIO_HICAR_NOTICE){
				
				if(tmpVolume >= VOL_SUPPRESSION){
					setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (m_MuteItem == MI_Mute) && (isEntStreamType(vd->m_streamtype)) ){
					//Do nothing,keep status
					setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_MIN_LEVEL);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (vd->m_showVol >= VOL_SUPPRESSION)){
					setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
					setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
				}else if((tmpVolume == VOL_MIN_LEVEL) && (vd->m_showVol < VOL_SUPPRESSION)){
					setALSAVolume(vd->m_streamtype, vd->m_showVol);
					setVDObjMemberRealVol(vd->m_streamtype, vd->m_showVol);
				}else{
					setALSAVolume(vd->m_streamtype, tmpVolume);
				}
				//update mute status
				if(tmpVolume == VOL_MIN_LEVEL)
					setVDObjMuteState(vd->m_streamtype, true);
				else
					setVDObjMuteState(vd->m_streamtype, false);
				
			}else if(vd->m_streamtype == AUDIO_LOCAL_BEEP){
				//恢复beep音量
				vd->m_realVol = vd->m_showVol;
				vd->m_muteState = false;
				setALSAVolume(vd->m_streamtype, vd->m_realVol);
				return true;
			} else {//no use
            	setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
           		vd->m_realVol = VOL_SUPPRESSION;
            	vd->m_muteState = false;
			}

            return true;
        }else if(isCalling && (vd->m_streamtype != AUDIO_LOCAL_ALARM)&&(vd->m_streamtype != AUDIO_PHONE_TELL)){
            //HFT mute other except Alarm
            //if(tmpVolume != VOL_MIN_LEVEL){
                setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                vd->m_realVol = VOL_MIN_LEVEL;
                vd->m_muteState = true;
            //}
            return true;
		 }else if((isRing) && (vd->m_streamtype != AUDIO_LOCAL_ALARM)&&(vd->m_streamtype != AUDIO_PHONE_TELL)&&(vd->m_streamtype != AUDIO_PHONE_RING)){
            //Ring mute other except Alarm
            //if(tmpVolume != VOL_MIN_LEVEL){
                setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                vd->m_realVol = VOL_MIN_LEVEL;
                vd->m_muteState = true;
           // }
            return true;
        }else if( isVR &&(vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || vd->m_streamtype == AUDIO_LOCAL_BEEP || \
                          vd->m_streamtype == AUDIO_PHONE_NAVI ||vd->m_streamtype == AUDIO_LOCAL_RADIO || vd->m_streamtype == AUDIO_HICAR_NOTICE )){

            if(vd->m_streamtype == AUDIO_LOCAL_BEEP ){
                if(vd->m_realVol != vd->m_showVol){
                    setALSAVolume(vd->m_streamtype, vd->m_showVol);
                    vd->m_realVol = vd->m_showVol;
					if(vd->m_showVol != VOL_MIN_LEVEL)
                    	vd->m_muteState = false;
                }
                return true;
            }else {
                //if(tmpVolume != VOL_MIN_LEVEL){
                    setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                    vd->m_realVol = VOL_MIN_LEVEL;
                    vd->m_muteState = true;
                //}
                return true;
            }
        }else if( isNav && (vd->m_streamtype == AUDIO_LOCAL_MUSIC || vd->m_streamtype == AUDIO_PHONE_MUSIC || \
        		vd->m_streamtype == AUDIO_LOCAL_BEEP|| vd->m_streamtype == AUDIO_LOCAL_RADIO  || vd->m_streamtype == AUDIO_HICAR_NOTICE )){

            if(vd->m_streamtype == AUDIO_LOCAL_BEEP){
                if(vd->m_realVol != vd->m_showVol){
                    setALSAVolume(vd->m_streamtype, vd->m_showVol);
                    vd->m_realVol = vd->m_showVol;
                    if(vd->m_showVol != VOL_MIN_LEVEL)
                    	vd->m_muteState = false;
                }
            }else{
            	/* isNavSupressMedia 导航自动降音开关 */
				if(m_isNavSupressMedia != 0 && m_isNavSupressMedia != 1){
					VLOGD("@audio-error m_isNavSupressMedia value is invalid %d!", m_isNavSupressMedia);
					m_isNavSupressMedia = 1;//default value means suppress
				}
				
				if(m_isNavSupressMedia == 1) {
					if(tmpVolume >= VOL_SUPPRESSION){
						setALSAVolume(vd->m_streamtype, VOL_SUPPRESSION);
						setVDObjMemberRealVol(vd->m_streamtype, VOL_SUPPRESSION);
					}else{
						setALSAVolume(vd->m_streamtype, tmpVolume);
					}
				}else{
					vd->m_realVol = vd->m_showVol;
					tmpVolume = vd->m_showVol;
					setALSAVolume(vd->m_streamtype, tmpVolume);
				}
				
				if(tmpVolume == VOL_MIN_LEVEL)
					setVDObjMuteState(vd->m_streamtype, true);
				else
					setVDObjMuteState(vd->m_streamtype, false);
					
            }
            return true;
        } else {

            sleepTime = 100;
			//条件2说明: 申请焦点,ENT覆盖时,real = show 时并未真正执行,所以不能返回
			if((tmpVolume == vd->m_showVol)&&(!isEntStreamType(vd->m_streamtype)))
				return true;

			//适用于 ENT 焦点申请 互相替换时, 及时MUTE 其它(BT/USB/RADIO 三选一)
            if((vd->m_muteState == true)&&(vd->m_showVol == VOL_MIN_LEVEL)){
                setALSAVolume(vd->m_streamtype, VOL_MIN_LEVEL);
                vd->m_realVol = VOL_MIN_LEVEL;
                return true;
            } else {
				//场景: ENT 处于静音，保持静音
				if((m_MuteItem == MI_Mute)&&(isEntStreamType(vd->m_streamtype)))
					return true;//vol=0下解除降音仍然静音
				
                tmpVolume = getGradualVolume(tmpVolume, vd->m_showVol);

				if(isEntStreamType(vd->m_streamtype)&&(mSpdCmpEnable==USE_ON)){
					if(tmpVolume == VOL_MIN_LEVEL)
						finalVol = VOL_MIN_LEVEL;
					else{
						finalVol = tmpVolume + mCmpVal;
						finalVol = (finalVol > VOL_MAX_LEVEL)?VOL_MAX_LEVEL:finalVol;
					}
				}else{
					finalVol = tmpVolume;
				}

                setALSAVolume(vd->m_streamtype, finalVol);
                vd->m_realVol = tmpVolume;

				if(vd->m_realVol == VOL_MIN_LEVEL)
					vd->m_muteState = true;
				else
					vd->m_muteState = false;
				
                if(tmpVolume == vd->m_showVol){
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
    return true;
}


void AudioServicePrivate::setALSAVolume(int streamType, int volume)
{
	if(volume > VOL_MAX_LEVEL){
		VLOGD("@audio-error %s  STREAM : %d, volume : %d error", __func__, streamType, volume);
		return;
	}
		
	if(streamType == AUDIO_LOCAL_RADIO)
		g_AudioCtrl->Set(RADIO_VOL, volume);
	else if((streamType == AUDIO_PHONE_TELL)||(streamType==AUDIO_PHONE_RING))
		g_AudioCtrl->Set(streamType + SOFTMST0_VOL, bt_Gain[volume]);
	else if(streamType == AUDIO_LOCAL_CARLOCK)
		g_AudioCtrl->Set(SOFTMST12_VOL, volume_Gain[volume]);
	else if(streamType == AUDIO_LOCAL_SLIDE)
		g_AudioCtrl->Set(SOFTMST11_VOL, volume_Gain[volume]);
	else
    	g_AudioCtrl->Set(streamType + SOFTMST0_VOL, volume_Gain[volume]);//音量曲线
}

int AudioServicePrivate::getGradualVolume(int volume, int aimVol)
{
    if(volume == aimVol)
        return volume;

    if(volume < aimVol){

        volume += 3;
        if(volume > aimVol)
            volume = aimVol;
    }else if(volume > aimVol){

        volume -= 3;
        if(volume < aimVol)
            volume = aimVol;
    }

    return volume;
}

int AudioServicePrivate::getCurStreamType()
{
    return m_CurStreamType;
}

#define Buzzer_OFF 0
#define Buzzer_ON 1
#define Buzzer_ON_4HZ 2
#define Buzzer_ON_2HZ 3
#define Buzzer_ON_ONCE 4
#define Buzzer_ON_TIME 5

#define WORK_CYCLE_ON 0
#define WORK_CYCLE_OFF 1

void AudioServicePrivate::pdcSoundPlay(const int type)
{
	VLOGD("%s type : %d last:%d isReversing:%d chime:%d pdc:%d", __func__, type, mLastPdcType, isReversing,
		(pChime && pChime->isRunning()) ? 1 : 0,
		(pChime && pChime->isPdcRunning()) ? 1 : 0);

	const bool playbackAlive = pChime && (pChime->isRunning() || pChime->isPdcRunning());

	if (type == Buzzer_OFF) {
		if (mLastPdcType != Buzzer_OFF || playbackAlive) {
			pdcSoundPlayProxy(PDC_OFF, Buzzer_OFF, true);
			if (pChime->isRunning() || pChime->isPdcRunning())
				pdcSoundPlayProxy(PDC_OFF, Buzzer_OFF, false);
		}
		if (isReversing)
			releaseAudioSource(AS_Alarm);
	} else {
		if (!isReversing)
			requestAudioSource(AS_Alarm);

		if (mLastPdcType != Buzzer_OFF) {
			/* 类型切换：只淡出 */
			pdcSoundPlayProxy(PDC_OFF, Buzzer_OFF, true);
		} else if (playbackAlive) {
			/* 进倒车/起播时上一次 WavPlay 未退：硬停清孤儿 */
			pdcSoundPlayProxy(PDC_OFF, Buzzer_OFF, false);
		}

		pdcSoundPlayProxy(PDC_ON, type, true);
	}

	mLastPdcType = type;
}

void AudioServicePrivate::onRadarPlayNaturalEnd()
{
	VLOGD("%s last:%d isReversing:%d chimeRunning:%d", __func__, mLastPdcType, isReversing,
		(pChime && pChime->isRunning()) ? 1 : 0);

	if (pChime && pChime->isRunning())
		return;
	if (mLastPdcType != Buzzer_ON_TIME && mLastPdcType != Buzzer_ON_ONCE)
		return;
	mLastPdcType = Buzzer_OFF;
	if (isReversing)
		releaseAudioSource(AS_Alarm);
}
void AudioServicePrivate::pdcSoundPlayProxy(const int start, const int type, bool withFade)
{
	if(start == PDC_ON){
		const char *wavPath = nullptr;
		int ringDurationMs = 0;
		int dataLoop = 1;
		int mode = WORK_CYCLE_OFF;
		bool outLoop = false;

		switch (type) {
		case Buzzer_ON:
			wavPath = RADAR_HIGH_HZ;
			break;
		case Buzzer_ON_4HZ:
			wavPath = RADAR_MID_HZ;
			break;
		case Buzzer_ON_2HZ:
			wavPath = RADAR_LOW_HZ;
			break;
		case Buzzer_ON_ONCE:
			wavPath = RADAR_MID_HZ;
			dataLoop = 0;
			mode = WORK_CYCLE_ON;
			break;
		case Buzzer_ON_TIME:
			wavPath = RADAR_MID_HZ;
			ringDurationMs = 1000;
			break;
		default:
			return;
		}

		pChime->setSampleFadeEnabled(true);
		if (pChime->isRunning() || pChime->isPdcRunning()) {
			VLOGE("%s PDC_ON but playback still alive, hard stop", __func__);
			pChime->ensureStopped(false);
		}
		if (pChime->isRunning() || pChime->isPdcRunning()) {
			VLOGE("%s skip start: chime/pdc still running", __func__);
			return;
		}
		pChime->setWavPath(wavPath);
		pChime->setRingDurationMs(ringDurationMs);
		pChime->setMode(mode);
		pChime->setPDCdataloop(dataLoop);
		pChime->setOutLoopFlag(outLoop);
		pChime->start();
	}else{
		pChime->setSampleFadeEnabled(true);
		pChime->ensureStopped(withFade);
	}
}

void AudioServicePrivate::softkeySoundPlay()
{
	if(m_TouchSound >= TS_ON && m_TouchSound <= TS_YINGRUN)
		pSoftkey->start();
}

void AudioServicePrivate::setEqualizerItem(const int item, int bass, int middle, int treble)
{
	VLOGD("%s item : %d, bass : %d, middle : %d, treble : %d",__func__,item,bass,middle,treble);
	if((item == mCurEqmode)&&(mCurEqmode != EQ_CUSTOM))
		return;

	int bass0 = 0;
	int bass1 = 0;
	int middle0 = 0;
	int middle1 = 0;	
	int middle2 = 0;
	int treble0 = 0;
	int treble1 = 0;	
	
	mCurEqmode = item;
	if(item != EQ_CUSTOM){
		g_AudioCtrl->Set(EQ_PRESET, item);
	}else{
		bass0 = bass & 0xff;//80hz
		g_AudioCtrl->Set(EQ_BAND0, bass0);
		if(m_CustomBass0 != bass0){
			m_CustomBass0 = bass0;
			storeAudioIniInfo(HANDLER_STORE_VAL_BASS0, m_CustomBass0, 0);
		}
		
		bass1 = (bass >> 8) & 0xff;//250hz
		g_AudioCtrl->Set(EQ_BAND1, bass1);
		if(m_CustomBass1 != bass1){
			m_CustomBass1 = bass1;
			storeAudioIniInfo(HANDLER_STORE_VAL_BASS1, m_CustomBass1, 0);
		}

		middle0 = middle & 0xff;//500hz
		g_AudioCtrl->Set(EQ_BAND2, middle0);
		if(m_CustomMiddle0 != middle0){
			m_CustomMiddle0 = middle0;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID0, m_CustomMiddle0, 0);
		}
		
		middle1 = (middle >> 8) & 0xff;//1Khz
		g_AudioCtrl->Set(EQ_BAND3, middle1);
		if(m_CustomMiddle1 != middle1){
			m_CustomMiddle1 = middle1;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID1, m_CustomMiddle1, 0);
		}

		middle2 = (middle >> 16) & 0xff;//2.5khz
		g_AudioCtrl->Set(EQ_BAND4, middle2);
		if(m_CustomMiddle2 != middle2){
			m_CustomMiddle2 = middle2;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID2, m_CustomMiddle2, 0);
		}
		
		treble0 = treble & 0xff;//6khz
		g_AudioCtrl->Set(EQ_BAND5, treble0);
		if(m_CustomTreble0 != treble0){
			m_CustomTreble0 = treble0;
			storeAudioIniInfo(HANDLER_STORE_VAL_TRE0, m_CustomTreble0, 0);
		}
		
		treble1 = (treble >> 8) & 0xff;//14Khz
		g_AudioCtrl->Set(EQ_BAND6, treble1);
		if(m_CustomTreble1 != treble1){
			m_CustomTreble1 = treble1;
			storeAudioIniInfo(HANDLER_STORE_VAL_TRE1, m_CustomTreble1, 0);
		}
	}
	storeAudioIniInfo(HANDLER_STORE_VAL_EQ, item, 0);
}


int AudioServicePrivate::getEQMode(){
	VLOGD("%s mCurEqmode : %d", __func__, mCurEqmode);
	return mCurEqmode;
}

void AudioServicePrivate::setBandLevel(const int band, const int level)
{
	VLOGD("%s band : %d  level : %d", __func__, band, level);
	if(band == EQ_BASS0_80HZ){
		g_AudioCtrl->Set(EQ_BAND0, level);
		if(level != m_CustomBass0){
			m_CustomBass0 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_BASS0, m_CustomBass0, 0);
		}
	}else if(band == EQ_BASS1_250HZ){
		g_AudioCtrl->Set(EQ_BAND1, level);
		if(level != m_CustomBass1){
			m_CustomBass1 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_BASS1, m_CustomBass1, 0);
		}
	}else if(band == EQ_MIDDLE0_500HZ){
		g_AudioCtrl->Set(EQ_BAND2, level);
		if(level != m_CustomMiddle0){
			m_CustomMiddle0 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID0, m_CustomMiddle0, 0);
		}
	}else if(band == EQ_MIDDLE1_1KHZ){
		g_AudioCtrl->Set(EQ_BAND3, level);
		if(level != m_CustomMiddle1){
			m_CustomMiddle1 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID1, m_CustomMiddle1, 0);
		}
	}else if(band == EQ_MIDDLE2_2500HZ){
		g_AudioCtrl->Set(EQ_BAND4, level);
		if(level != m_CustomMiddle2){
			m_CustomMiddle2 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_MID2, m_CustomMiddle2, 0);
		}
	}else if(band == EQ_TREBLE0_6KHZ){
		g_AudioCtrl->Set(EQ_BAND5, level);
		if(level != m_CustomTreble0){
			m_CustomTreble0 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_TRE0, m_CustomTreble0, 0);
		}
	}else if(band == EQ_TREBLE1_14KHZ){
		g_AudioCtrl->Set(EQ_BAND6, level);
		if(level != m_CustomTreble1){
			m_CustomTreble1 = level;
			storeAudioIniInfo(HANDLER_STORE_VAL_TRE1, m_CustomTreble1, 0);
		}
	}
}

int AudioServicePrivate::getBandLevel(const int band)
{
	int ret = 0;

	if(band == EQ_BASS0_80HZ)
		ret = m_CustomBass0;
	else if(band == EQ_BASS1_250HZ)
		ret = m_CustomBass1;
	else if(band == EQ_MIDDLE0_500HZ)
		ret = m_CustomMiddle0;
	else if(band == EQ_MIDDLE1_1KHZ)
		ret = m_CustomMiddle1;
	else if(band == EQ_MIDDLE2_2500HZ)
		ret = m_CustomMiddle2;
	else if(band == EQ_TREBLE0_6KHZ)
		ret = m_CustomTreble0;
	else if(band == EQ_TREBLE1_14KHZ)
		ret = m_CustomTreble1;

	VLOGD("%s band : %d  level : %d", __func__, band, ret);
	return ret;
}

void AudioServicePrivate::setTouchSoundStatus(const int status)
{
	Q_Q(AudioService);
	VLOGD("%s status : %d -> %d", __func__, m_TouchSound, status);

	if(status == 1) {
		pSoftkey->setWavPath(KEY_WAV_FILE);
	} else if(status == 2) {
		pSoftkey->setWavPath(KEY_WAV_EQ_1);
	} else if(status == 3) {
		pSoftkey->setWavPath(KEY_WAV_EQ_2);
	}
		
	
	m_TouchSound = status;
	emit q->onTouchSoundStatusChange(m_TouchSound);
	storeAudioIniInfo(HANDLER_STORE_STS_TOUCH, status, 0);

}

int AudioServicePrivate::getTouchSoundStatus()
{
	VLOGD("@zhz %s m_TouchSound : %d", __func__, m_TouchSound);
	return m_TouchSound;
}

void AudioServicePrivate::setSpdCmpEnable(const int value)
{
	Q_Q(AudioService);

	VLOGD("@audio %s value : %d",__func__, value);
	if(value > 1 || value < 0)
		return;
	
	mSpdCmpEnable = value;

	mSpdLock.lock();
	m_SpdChg = true;
	mSpdLock.unlock();
	
	setSpdCmpGain(q->curSpeed/100);
	storeAudioIniInfo(HANDLER_STORE_STS_SPD, value, 0);
}

int  AudioServicePrivate::getSpdCmpEnable()
{
	return mSpdCmpEnable;
}

void AudioServicePrivate::setSpdCmpLevel(const int value)
{
	Q_Q(AudioService);

	VLOGD("@audio %s value : %d",__func__, value);

	if(value == m_Spdcmp)
		return;
	
	m_Spdcmp = value;
	mSpdLock.lock();
	m_SpdChg = true;
	mSpdLock.unlock();
	
	setSpdCmpGain(q->curSpeed/100);
	storeAudioIniInfo(HANDLER_STORE_VAL_SPD, value, 0);
}
	
int  AudioServicePrivate::getSpdCmpLevel()
{
	return m_Spdcmp;
}

void AudioServicePrivate::setSpdCmpGain(int speed)
{
	if((mSpdCmpEnable == 0)&&(m_SpdChg == false))
		return;

	if(isReversing == true)
		return;
	
	int inx = 0;

	if(speed > 9)
	 	inx = (speed%10)<5 ? ((speed /10)*2 - 1) : ((speed /10)*2);

	//速度超过150 不再补偿
	if((inx > 28)&&(m_SpdChg == false))
		return;
	//车速未变 不补偿
	if((m_SpdcmpLastInx == inx)&&(m_SpdChg == false))
		return;
	
	if(isEntStreamType(m_CurStreamType))
	{
		int val = 0;
		//当 Vol＝ 0/MAX 时不进行车速补偿
		if(m_CurrentVolume == VOL_MIN_LEVEL || m_CurrentVolume == VOL_MAX_LEVEL)
			return;
		//index max size is 0~28
		if(inx > 28)
			inx = 28;

		VLOGD("%s m_SpdcmpLastInx:%d  inx:%d", __func__, m_SpdcmpLastInx, inx);
		
		if(m_SpdcmpLastInx < inx)
			mCmpVal = spdVolUp[m_Spdcmp][inx];
		else if(m_SpdcmpLastInx > inx)
			mCmpVal = spdVolDown[m_Spdcmp][inx];
		else if(m_SpdcmpLastInx == inx)
			mCmpVal = spdVolUp[m_Spdcmp][inx];

		if(mSpdCmpEnable == 0)
			mCmpVal = 0;

		val = m_CurrentVolume + mCmpVal;
		//补偿后的音量不会超过 Vol MAX
		if(val >= VOL_MAX_LEVEL)
			val = VOL_MAX_LEVEL;

		m_SpdcmpLastInx = inx;
		if(m_SpdChg == true){
			mSpdLock.lock();
			m_SpdChg = false;
			mSpdLock.unlock();
		}

		//当 Vol＝ MUTE 时不进行车速补偿,mCmpVal须全局更新
		if(m_MuteItem == MI_Mute)
			return;
		
		setALSAVolume(mActiveENT, val);
		
	}
}


void AudioServicePrivate::setMICMute(int state)
{
	VLOGD("%s state : %d", __func__, state);

	if(state != mMICMute){
		g_AudioCtrl->Set(ADC_MUTE,state);
		mMICMute = state;
		storeAudioIniInfo(HANDLER_STORE_STS_MIC, mMICMute, 0);
	}		
}

int AudioServicePrivate::getMICMute()
{
	VLOGD("%s mMICMute : %d", __func__, mMICMute);
	return mMICMute;
}

int AudioServicePrivate::getNavMixMedia()
{
	VLOGD("%s value:%d",__func__, m_isNavSupressMedia);
	return m_isNavSupressMedia;
}

void AudioServicePrivate::setNavMixMedia(int value)
{
	VLOGD("%s value: %d -> %d",__func__, m_isNavSupressMedia, value);

	if(value != 0 && value != 1){
		value = 1;//default
	}

	if(value != m_isNavSupressMedia) {
		m_isNavSupressMedia = value;
		storeAudioIniInfo(HANDLER_STORE_STS_NAVIMEDIA, m_isNavSupressMedia, 0);
	}
}


void  AudioServicePrivate::setFocusPolicy(int policy)
{
	VLOGD("%s set audio policy: %d -> %d", __func__, mFocusType, policy);

	mFocusType = policy;
	adjustVolume();
}

int  AudioServicePrivate::getFocusPolicy()
{
	VLOGD("%s mFocusType : %d", __func__, mFocusType);
	return mFocusType;
}

void AudioServicePrivate::setCarLockSound(const int value, bool play, bool isApply)
{
	VLOGD("%s value: %d -> %d",__func__, mCarLockSound, value);

	if(isApply == true) {
		if(value == 1){
			pCarLockSound->setWavPath(CAR_SOUND_EQ1);
		}else if(value == 2){
			pCarLockSound->setWavPath(CAR_SOUND_EQ2);
		}else if(value == 3){
			pCarLockSound->setWavPath(CAR_SOUND_EQ3);
		}else if(value == 4){
			pCarLockSound->setWavPath(CAR_SOUND_EQ4);
		}else if(value == 5){
			pCarLockSound->setWavPath(CAR_SOUND_EQ5);
		}
	}
	
	if(value != mCarLockSound && (isApply == true)) {
		mCarLockSound = value;
		storeAudioIniInfo(HANDLER_STORE_VAL_CARLOCKSOUND, mCarLockSound, 0);
	}

	if(!play)
		return;
	
	if(value == 1){
		volumeSliderEQPlay(0x09 , -1);//CAR_SOUND_EQ1_PRE
	}else if(value == 2){
		volumeSliderEQPlay(0x0A , -1);//CAR_SOUND_EQ2_PRE
	}else if(value == 3){
		volumeSliderEQPlay(0x0B , -1);//CAR_SOUND_EQ3_PRE
	}else if(value == 4){
		volumeSliderEQPlay(0x0C , -1);//CAR_SOUND_EQ4_PRE
	}else if(value == 5){
		volumeSliderEQPlay(0x0D , -1);//CAR_SOUND_EQ5_PRE
	}
	
}
int AudioServicePrivate::getCarLockSound()
{
	return mCarLockSound;
}

void AudioServicePrivate::carLockSoundPlay()
{
    pCarLockSound->start();
}

void AudioServicePrivate::applyHicarNavEntRealVol(shared_ptr<VolumeData> vd)
{
	if (vd == nullptr)
		return;

	if (vd->m_streamtype == AUDIO_PHONE_MUSIC) {
		vd->m_realVol = vd->m_showVol;
		vd->m_muteState = (vd->m_showVol == VOL_MIN_LEVEL);
		VLOGD("%s PHONE_MUSIC keep showVol:%d", __func__, vd->m_realVol);
		return;
	}

	if (vd->m_streamtype != AUDIO_LOCAL_MUSIC && vd->m_streamtype != AUDIO_LOCAL_RADIO)
		return;

	if (m_isNavSupressMedia != 0 && m_isNavSupressMedia != 1) {
		VLOGD("@audio-error m_isNavSupressMedia value is invalid %d!", m_isNavSupressMedia);
		m_isNavSupressMedia = 1;
	}

	if (m_isNavSupressMedia == 1) {
		if (vd->m_showVol >= VOL_SUPPRESSION)
			vd->m_realVol = VOL_SUPPRESSION;
		else
			vd->m_realVol = vd->m_showVol;
	} else {
		vd->m_realVol = vd->m_showVol;
	}
	vd->m_muteState = (vd->m_realVol == VOL_MIN_LEVEL);
	VLOGD("%s stream:%d realVol:%d showVol:%d suppress:%d", __func__,
		vd->m_streamtype, vd->m_realVol, vd->m_showVol, m_isNavSupressMedia);
}

void AudioServicePrivate::entVolumeAdjust()
{
	shared_ptr<VolumeData> tmpMusic = getVolumeData(AUDIO_LOCAL_MUSIC);
	if(tmpMusic == nullptr){
		VLOGD("@audio-error %s, tmpMusic getVolumeData(nullptr)", __func__);
		return ;
	}
	setALSAVolume(AUDIO_LOCAL_MUSIC, tmpMusic->m_realVol);
	
	shared_ptr<VolumeData> tmpRadio = getVolumeData(AUDIO_LOCAL_RADIO);
	if(tmpRadio == nullptr){
		VLOGD("@audio-error %s, tmpRadio getVolumeData(nullptr)", __func__);
		return ;
	}
	setALSAVolume(AUDIO_LOCAL_RADIO, tmpRadio->m_realVol);

	shared_ptr<VolumeData> tmpBT = getVolumeData(AUDIO_PHONE_MUSIC);
	if(tmpBT == nullptr){
		VLOGD("@audio-error %s, tmpBT getVolumeData(nullptr)", __func__);
		return ;
	}
	setALSAVolume(AUDIO_PHONE_MUSIC, tmpBT->m_realVol);
}

void AudioServicePrivate::volumeSliderEQPlay(int slideType, int volval)
{
	VLOGD("%s slidetype = %d volval  = %d", __func__, slideType, volval);

	m_volSlidePlaySlideType = slideType;

	if (slideType == 16) {
		setALSAVolume(AUDIO_LOCAL_SLIDE, VOL_MIN_LEVEL);
		pSlide->stop();
		return;
	}

	int defaultVolume = 16;

	if(volval != -1) {
		defaultVolume = volval;	
	}
	
	setALSAVolume(AUDIO_LOCAL_SLIDE, defaultVolume);
	
	if(slideType == 1){
		pSlide->setWavPath(VOLUME_SLIDE_MEDIA);
	}else if(slideType == 2){
		pSlide->setWavPath(VOLUME_SLIDE_CALL);
	}else if(slideType == 3){
		pSlide->setWavPath(VOLUME_SLIDE_NAVI);
	}else if(slideType == 4){
		pSlide->setWavPath(VOLUME_SLIDE_RING);
	}else if(slideType == 5){
		pSlide->setWavPath(VOLUME_SLIDE_VOICE);
	}else if(slideType == 6){//AVAS
		pSlide->setWavPath(AVAS_EQ_CLASSIC);
	}else if(slideType == 7){
		pSlide->setWavPath(AVAS_EQ_SKY);
	}else if(slideType == 8){
		pSlide->setWavPath(AVAS_EQ_YINHE);
	}else if(slideType == 9){//CAR SOUND
		pSlide->setWavPath(CAR_SOUND_EQ1_PRE);
	}else if(slideType == 10){
		pSlide->setWavPath(CAR_SOUND_EQ2_PRE);
	}else if(slideType == 11){
		pSlide->setWavPath(CAR_SOUND_EQ3_PRE);
	}else if(slideType == 12){
		pSlide->setWavPath(CAR_SOUND_EQ4_PRE);
	}else if(slideType == 13){
		pSlide->setWavPath(CAR_SOUND_EQ5_PRE);
	}else if(slideType == 14){//RADAR
		pSlide->setWavPath(RADAR_PRE_SHOW);
	}else if(slideType == 15){//climate
		pSlide->setWavPath(CLIMATE_SOUND);
	}

	if((slideType == 14 && isReversing == true) || (isCalling == true)){
		//倒车状态下不播放预览音
	}else{
		pSlide->play();
	}
	
}

