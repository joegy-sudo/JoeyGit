#ifndef WAVPLAYINTERRUPTIBLE_H
#define WAVPLAYINTERRUPTIBLE_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "alsa/asoundlib.h"
#include "bdlog.h"
#include "wavplay.h"

class WavPlayInterruptible : public QThread
{
    Q_OBJECT
public:
    explicit WavPlayInterruptible(const char *device);
    ~WavPlayInterruptible() override;

    void setWavPath(const char *filepath);
    void play();
    void play(const char *filepath);
    void stop();

signals:
    void naturalPlaybackFinished();

private:
    void run() override;

    int openWavFile(const std::string &path);
    void closeWavFile();
    int sndPcmInit();
    void sndPcmClose();

    const char *m_device;

    QMutex m_ctrlMutex;
    QWaitCondition m_ctrlCond;
    bool m_quit = false;
    bool m_newPlayPending = false;
    bool m_stopCurrent = false;
    std::string m_userPath;
    std::string m_pendingPath;

    int file_fd = -1;
    snd_pcm_t *m_pcm = nullptr;
    FMT_t m_wavFmt;
    DATA_t wav_data;
    snd_pcm_uframes_t period_size = 0;
    snd_pcm_uframes_t buf_bytes = 0;
    int m_periods = 4;
};

#endif
