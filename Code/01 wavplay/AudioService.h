#ifndef AUDIOSERVICE_H
#define AUDIOSERVICE_H

#define AudioApplication             QString("-audio")
#define ArkMicroAudioService         QString("com.arkmicro.audio")
#define ArkMicroAudioPath            QString("/com/arkmicro/audio")
#define ArkMicroAudioInterface       QString("Local.DbusServer.Audio")
#define ArkMicroAudioRequest         QString("requestAudioSource")
#define ArkMicroAudioRelease         QString("releaseAudioSource")
//#define ArkMicroopen_amixer_mode     QString("open_amixer_mode")

#include "AudioPersistent.h"
#include <QObject>
#include <memory>
#include <iostream>
#include <QMutex>
#include "Handler.h"
#include "wavplay.h"
#include "chime.h"
#include "libvehicle_global.h"
#include "vehicleserviceproxy.h"
#include "yin_lib_interface.h"
#include "bdlog.h"

#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <QBuffer>

using namespace com::bicv::vehicle::bddbus;
using namespace com::bicv::vehicle;
using namespace std;
class AudioPriority{
public:
    AudioPriority();
    AudioPriority(int priority, int streamtype, int source):m_priority(priority),
        m_streamType(streamtype),m_audiosource(source){

    }

    ~AudioPriority(){
        qDebug()<<"AudioPriority unconstractor "<<m_audiosource;
    }
    int m_priority;
    int m_streamType;
    int m_audiosource;
};
class VolumeData {
public:
    int m_streamtype;
    bool m_muteState;
    int m_showVol;
    int m_realVol;

    VolumeData(){
        this->m_streamtype = 0;
        this->m_muteState = true;
        this->m_showVol = 0;
        this->m_realVol = 0;
    }

    ~VolumeData(){
        cout<<"@zhz VolumeData unconstracotr m_streamtype :"<<m_streamtype<<endl;
    }
};

class AudioServicePrivate;
class AudioService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioService)
    Q_CLASSINFO("D-Bus Interface", "Local.DbusServer.Bicvaudio")
public:
    explicit AudioService(QObject* parent = NULL);
    ~AudioService();
private:
	void setSpdCmpGain(int speed);
	void initRadar();
	void updateReverseStatus(uint8_t value);
	void updateRadarType(uint8_t value);
	void updatePlayRadarWarn(uint8_t value);
	void updateRadarDisplay(uint8_t value);
	uint8_t getReverseStatus();
	void initNetLink();
	void carLockSoundPlay();
public slots:
	//request or release audio source
    bool requestAudioSource(const int source);
	int getCurAudioSource();
    void releaseAudioSource(const int source);

	void requestReset();

    void requestMute(const int item);
	void requestMuteToggole();
	int getMuteStatus();
	void requestIncreaseVolume();
    void requestDecreaseVolume();

	//set or get stream volume
    void requestSetVolume(const int volume);
    void requestSetVolume(const int stream, const int volume);
	int  getVolume();
	int  getVolume(const int stream);
	//set or get effect type params
    //void setEQMode(const int mode);
	void setEqualizerItem(const int item, const int bass, const int middle, const int treble);
	void setBandLevel(const int band,const int level);
	
	int getEQMode();
	int getBandLevel(const int band);

	
    //amixer alsa phone volume controls 20190828
    int open_amixer_mode(int stream_name, int ctrlname);
    void close_amixer_mode();
    int get_amixersoftmaster_volume();
    void set_amixersoftmaster_volume(int volume);
    //bicv interface
    void pdcSoundPlay(const int type);
    void softkeySoundPlay();

	void setSpdCmpEnable(const int value);
	int  getSpdCmpEnable();
	
	void setSpdCmpLevel(const int value);
	int  getSpdCmpLevel();
	void setTouchSoundStatus(const int status);	
	int  getTouchSoundStatus();	

	void showVolumeInfo();
	void showFocusInfo();

	void setMICMute(int state);
	int  getMICMute();
	void entFadeIn();
	void entFadeOut();

	void setAVASSwitch(const int value);
	int getAVASSwitch();

	void setAVASVolumeLevel(const int value);
	int getAVASVolumeLevel();
	
	int  getCurMultimediaSource();

	void setNavMixMedia(const int value);
	int getNavMixMedia();

	bool getAMPEnableStatus();
	void setCarLockSound(const int value, bool isPlay, bool isApply);
	int getCarLockSound();
	void setFocusPolicy(int policy);
	int  getFocusPolicy();

	void volumeSliderEQPlay(int slideType, int volval);
signals:
    void onMuteChange(const int mute);
    void onVolumeChange(const int stream, const int volume);
    void onEqualizerItemChange(const int item, const int bass, const int middle, const int treble);
	void onAudioSourceChange(const int source);
	void onTouchSoundStatusChange(const int status);
	void onCurVolumeValueChange(const int curvol);
	void onVolSidePlayEndChange(const int slideType);
private slots:
	void onPropertyChangeEvent(const VehiclePropValue& prop);
    void onRegisterationStatusChange(bool isStarted);
	void onActivated();
	void onVolumeSlideNaturalEnd();
	void onRadarPlayNaturalEnd();
	void onCarLockSoundPlayEnd();
private:
    Q_DECLARE_PRIVATE(AudioService)
    AudioServicePrivate* const d_ptr;
	yin_cs_handle_t *avas_handle;
	uint8_t mReverseState;
	uint8_t mRadarType;
	uint8_t mRadarWarn;
	uint8_t mRadarRVC;
	QSocketNotifier *socket_notifier;
	int netlink_socket;
	int curSpeed;
	int mAVASEnable;
	int mAVASSoundType;
	int mAVASLevel;
	bool mCarLockNeedCloseAmp;
};

#endif // AUDIOSERVICE_H
