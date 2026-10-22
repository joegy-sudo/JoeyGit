#ifndef WAVPLAY_H
#define WAVPLAY_H
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "alsa/asoundlib.h"
#include "bdlog.h"
typedef unsigned char uint8_t;

struct RIFF_t
{
    char ChunkID[4];
    uint32_t size;
    char Format[4];
};

struct FMT_t
{
    char Subchunk1ID[4];
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct DATA_t
{
    char Subchunk2ID[4];
    uint32_t Subchunk2Size;
};


class WavPlay : public QThread
{
public:
    explicit WavPlay(const char *device);
    ~WavPlay();

    void run() override;
    void setWavPath(const char *filepath);
	
    void printWavFmtInfo();

    int open_wavfile();
    int snd_pcm_init();

	ssize_t SNDWAV_WritePcm(unsigned char *buf, size_t wcount);
	ssize_t SNDWAV_P_SaveRead(int fd, void *buf, size_t count);

	void setCycle(int flag);
	void requestStop(bool fadeOut = true);
	void setSampleFadeEnabled(bool enable);
	void setSampleFadeMs(int ms);
	int getSampleFadeMs() const;
private:
	void applySampleFade(uint8_t *buf, size_t bytes);
	void resetSampleFadeState();
	bool seekToPcmData();

    const char *m_device;

    int file_fd;
    std::string m_wavpathStr;
    snd_pcm_t *m_pcm;
    FMT_t m_wavFmt;
	DATA_t wav_data;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buf_bytes;
    int m_periods;

	int mCycle;
	bool m_stopRequested;
	bool m_sampleFadeEnabled;
	bool m_fadeOutActive;
	bool m_fadeStopCompleted;
	float m_fadeGain;
	int m_sampleFadeMs;
	mutable QMutex aLock;
};

#endif // WAVPLAY_H
