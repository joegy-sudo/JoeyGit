#include "AudioPersistent.h"
#include "ark_api.h"
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QCoreApplication>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef gcc
static const QString persistentPath("/tmp/Audio.ini");
#else
static const QString persistentPath("/data/Audio.ini");
#endif
static const QString Exists("Exists");
static const QString Volume("Volume");
static const QString MuteFlag("MuteFlag");
static const QString TSFlag("TSFlag");
static const QString SpdFlag("SpdFlag");
static const QString SpdSwitch("SpdSwitch");
static const QString Equalizer("Equalizer");
static const QString Bass0("Bass0");
static const QString Bass1("Bass1");
static const QString Middle0("Middle0");
static const QString Middle1("Middle1");
static const QString Middle2("Middle2");
static const QString Treble0("Treble0");
static const QString Treble1("Treble1");
static const QString Sound("Sound");
static const QString Left("Left");
static const QString Right("Right");
static const QString LocalMusicVol("LocalMusicVol");
static const QString EntVol("EntVol");
static const QString MicMute("MicMute");
static const QString NavMediaFlag("NavMediaFlag");
static const QString CarLockSound("CarLockSound");

static const QString BtMusicVol("BtMusicVol");
static const QString CallVol("CallVol");
static const QString NaviVol("NaviVol");
static const QString TTSVol("TTSVol");
static const QString AlarmVol("AlarmVOl");
static const QString BeepVol("BeepVol");
static const QString RadioVol("RadioVol");
static const QString CarLockVol("CarLockVol");
static const QString RingVol("RingVol");
static const QString HicarAlarmVol("HicarAlarmVol");
static const QString NoticeVol("NoticeVol");

static const QString SlideVol("SlideVol");
static const QString AVASLevel("AVASLevel");
static const QString AVASSound("AVASSound");

class AudioSettings : public QSettings
{
    Q_DISABLE_COPY(AudioSettings)
#define g_Settings (AudioSettings::instance())
public:
    inline static AudioSettings* instance() {
        static AudioSettings* settings(new AudioSettings(qApp));
        return settings;
    }
    void clear();
    void sync();
    void setValue(const QString &key, const QVariant &value);
private:
    explicit AudioSettings(QObject* parent = NULL);
    ~AudioSettings();
};

void AudioPersistent::reset()
{
    g_Settings->clear();
    g_Settings->sync();
}

int AudioPersistent::getVolume(const bool reload)
{
    int defaultValue(20);

    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Volume, defaultValue).toInt(&flag);
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getMuteStatus(const bool reload)
{
	int defaultValue(0);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(MuteFlag, defaultValue).toInt(&flag);
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getTouchSoundStatus(const bool reload)
{
	int defaultValue(TS_ON);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(TSFlag, defaultValue).toInt(&flag);
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getSpdCmpStatus(const bool reload)
{
	int defaultValue(SC_MID);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(SpdFlag, defaultValue).toInt(&flag);
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setSpdCmpStatus(const int value)
{
	g_Settings->setValue(SpdFlag, value);
    g_Settings->sync();
}

int AudioPersistent::getSpdCmpEnableStatus(const bool reload)
{
	int defaultValue(TS_ON);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(SpdSwitch, defaultValue).toInt(&flag);
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setSpdCmpEnableStatus(const int value)
{
	g_Settings->setValue(SpdSwitch, value);
    g_Settings->sync();
}

void AudioPersistent::setAVASVolLevel(const int value)
{
	g_Settings->setValue(AVASLevel, value);
    g_Settings->sync();
}

int	AudioPersistent::getAVASVolLevel(const bool reload)
{
	int defaultValue(AVAS_MID);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
	
    int ret = g_Settings->value(AVASLevel, defaultValue).toInt(&flag);
	
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setAVASSoundType(const int value)
{
	g_Settings->setValue(AVASSound, value);
    g_Settings->sync();
}

int	AudioPersistent::getAVASSoundType(const bool reload)
{
	int defaultValue(1);

	bool flag(false);
	if (reload) {
        g_Settings->sync();
    }
	
    int ret = g_Settings->value(AVASSound, defaultValue).toInt(&flag);
	
    if (!flag) {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::resetStreamVolume(const int streamtype)
{
	int defaultValue = 10;

	if(streamtype == AUDIO_LOCAL_MUSIC || streamtype == AUDIO_PHONE_MUSIC || streamtype == AUDIO_LOCAL_RADIO) {
		g_Settings->setValue(EntVol, defaultValue);
	} else if (streamtype == AUDIO_PHONE_NAVI) {
        g_Settings->setValue(NaviVol, defaultValue);
    } else if (streamtype == AUDIO_PHONE_TELL) {
        g_Settings->setValue(CallVol, defaultValue);
    } else if (streamtype == AUDIO_PHONE_TTS) {
        g_Settings->setValue(TTSVol, defaultValue);
    } else if (streamtype == AUDIO_LOCAL_BEEP) {
		defaultValue = 16;
        g_Settings->setValue(BeepVol, defaultValue);
    } else if (streamtype == AUDIO_PHONE_RING) {
        g_Settings->setValue(RingVol, defaultValue);
	} else if (streamtype == AUDIO_LOCAL_SLIDE) {
		g_Settings->setValue(SlideVol, defaultValue);
	} else if (streamtype == AUDIO_LOCAL_CARLOCK) {
		defaultValue = 37;
		g_Settings->setValue(CarLockVol, defaultValue);
	} else if(streamtype == AUDIO_HICAR_ALARM) {
		g_Settings->setValue(HicarAlarmVol, defaultValue);
	} else if(streamtype == AUDIO_HICAR_NOTICE) {
		defaultValue = 16;
		g_Settings->setValue(NoticeVol, defaultValue);
	} else if(streamtype == AUDIO_LOCAL_ALARM) {
		defaultValue = 23;
		g_Settings->setValue(AlarmVol, defaultValue);
	}

	return defaultValue;
}


void AudioPersistent::setMuteStatus(const int val)
{
    g_Settings->setValue(MuteFlag, val);
    g_Settings->sync();
}


int AudioPersistent::getEqualizerItem(const bool reload)
{
    int defaultValue(EQ_FLAT);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Equalizer, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getBass0Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Bass0, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getBass1Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Bass1, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getMiddle0Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Middle0, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getMiddle1Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Middle1, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getMiddle2Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Middle2, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getTreble0Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Treble0, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getTreble1Equalizer(const bool reload)
{
    int defaultValue(10);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Treble1, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getMICMuteStatus(const bool reload)
{
    int defaultValue(0);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(MicMute, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setEqualizerItem(const int item)
{
    g_Settings->setValue(Equalizer, item);
    g_Settings->sync();
}

void AudioPersistent::setBass0Equalizer(const int value)
{
    g_Settings->setValue(Bass0, value);
    g_Settings->sync();
}

void AudioPersistent::setBass1Equalizer(const int value)
{
    g_Settings->setValue(Bass1, value);
    g_Settings->sync();
}

void AudioPersistent::setMiddle0Equalizer(const int value)
{
    g_Settings->setValue(Middle0, value);
    g_Settings->sync();
}

void AudioPersistent::setMiddle1Equalizer(const int value)
{
    g_Settings->setValue(Middle1, value);
    g_Settings->sync();
}

void AudioPersistent::setMiddle2Equalizer(const int value)
{
    g_Settings->setValue(Middle2, value);
    g_Settings->sync();
}

void AudioPersistent::setTreble0Equalizer(const int value)
{
    g_Settings->setValue(Treble0, value);
    g_Settings->sync();
}
void AudioPersistent::setTreble1Equalizer(const int value)
{
    g_Settings->setValue(Treble1, value);
    g_Settings->sync();
}

void AudioPersistent::setTouchSoundStatus(const int value)
{
	g_Settings->setValue(TSFlag, value);
    g_Settings->sync();
}


void AudioPersistent::setMICMuteStatus(const int value)
{
	g_Settings->setValue(MicMute, value);
    g_Settings->sync();
}

void AudioPersistent::setNavMixMediaStatus(const int value)
{
	g_Settings->setValue(NavMediaFlag, value);
    g_Settings->sync();
}

int	AudioPersistent::getNavMixMediaStatus(const bool reload)
{
	int defaultValue(1);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(NavMediaFlag, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;

}

void AudioPersistent::setCarSoundLockEQ(const int value)
{
	g_Settings->setValue(CarLockSound, value);
    g_Settings->sync();
}
int	AudioPersistent::getCarSoundLockEQ(const bool reload)
{
	int defaultValue(1);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(CarLockSound, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;

}

int AudioPersistent::getSoundItem(const bool reload)
{
    int defaultValue(SI_Custom);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Sound, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getLeftSound(const bool reload)
{
    int defaultValue(0);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Left, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

int AudioPersistent::getRightSound(const bool reload)
{
    int defaultValue(0);
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }
    int ret = g_Settings->value(Right, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setSoundItem(const int item)
{
    g_Settings->setValue(Sound, item);
    g_Settings->sync();
}

void AudioPersistent::setLeftSound(const int value)
{
    g_Settings->setValue(Left, value);
    g_Settings->sync();
}

void AudioPersistent::setRightSound(const int value)
{
    g_Settings->setValue(Right, value);
    g_Settings->sync();
}

void AudioPersistent::saveStreamVolume(const int streamtype, const int volume)
{
    if(streamtype == AUDIO_LOCAL_MUSIC)
        g_Settings->setValue(EntVol, volume);
    else if(streamtype == AUDIO_PHONE_MUSIC)
        g_Settings->setValue(EntVol, volume);
    else if(streamtype == AUDIO_PHONE_NAVI)
        g_Settings->setValue(NaviVol, volume);
    else if(streamtype == AUDIO_PHONE_TELL)
        g_Settings->setValue(CallVol, volume);
    else if(streamtype == AUDIO_PHONE_TTS)
        g_Settings->setValue(TTSVol, volume);
    else if(streamtype == AUDIO_LOCAL_ALARM)
        g_Settings->setValue(AlarmVol, volume);
    else if(streamtype == AUDIO_LOCAL_BEEP)
        g_Settings->setValue(BeepVol, volume);
	else if(streamtype == AUDIO_LOCAL_RADIO)
		g_Settings->setValue(EntVol, volume);
	else if(streamtype == AUDIO_PHONE_RING)
		g_Settings->setValue(RingVol, volume);
	else if(streamtype == AUDIO_HICAR_ALARM)
		g_Settings->setValue(HicarAlarmVol, volume);
	
    g_Settings->sync();
}

int AudioPersistent::getStreamVolume(const int streamtype, const bool reload)
{
    int defaultValue(10);
    QString tmp;
    bool flag(false);
    if (reload) {
        g_Settings->sync();
    }

    if(streamtype == AUDIO_LOCAL_MUSIC){
        tmp = EntVol;
        defaultValue = 10;
    }
    else if(streamtype == AUDIO_PHONE_MUSIC){
        tmp = EntVol;
        defaultValue = 10;
    }
    else if(streamtype == AUDIO_PHONE_NAVI)
        tmp = NaviVol;
    else if(streamtype == AUDIO_PHONE_TELL)
        tmp = CallVol;
	 else if(streamtype == AUDIO_PHONE_RING)
        tmp = RingVol;
    else if(streamtype == AUDIO_PHONE_TTS)
        tmp = TTSVol;
    else if(streamtype == AUDIO_LOCAL_ALARM){
        tmp = AlarmVol;
		defaultValue = 23;
    }else if(streamtype == AUDIO_LOCAL_BEEP){
        tmp = BeepVol;
		defaultValue = 16;
	}else if(streamtype == AUDIO_LOCAL_RADIO){
		tmp = EntVol;
		defaultValue = 10;
	}else if(streamtype == AUDIO_LOCAL_CARLOCK){
		tmp = CarLockVol;
		defaultValue = 37;
	}else if(streamtype == AUDIO_LOCAL_SLIDE){
		tmp = SlideVol;
		defaultValue = 10;
	}else if(streamtype == AUDIO_HICAR_ALARM){
		tmp = HicarAlarmVol;
	}else if(streamtype == AUDIO_HICAR_NOTICE){
		tmp = NoticeVol;
		defaultValue = 16;
	}

    int ret = g_Settings->value(tmp, defaultValue).toInt(&flag);
    if (!flag)
    {
        ret = defaultValue;
    }
    return ret;
}

void AudioPersistent::setVolume(int value)
{
    g_Settings->setValue(Volume, value);
    g_Settings->sync();
}

void AudioSettings::clear()
{
    QSettings::clear();
}

void AudioSettings::sync()
{
    QSettings::sync();
}

void AudioSettings::setValue(const QString &key, const QVariant &value)
{
    if (!contains(Exists)) {
        QSettings::setValue(Exists, QVariant(bool(true)));
    }
    QSettings::setValue(key, value);
}

AudioSettings::AudioSettings(QObject *parent)
    : QSettings(persistentPath, QSettings::IniFormat, parent)
{
    std::string filename = "/data/Audio.ini.lock";
    if (std::remove(filename.c_str()) == 0) {
        qDebug()<<__FUNCTION__<<__LINE__<<"@audio File deleted successfully: " << filename.c_str();
    }
}

AudioSettings::~AudioSettings()
{
}
