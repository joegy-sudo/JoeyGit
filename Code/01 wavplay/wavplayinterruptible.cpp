#include "wavplayinterruptible.h"
#include <QDebug>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <errno.h>

WavPlayInterruptible::WavPlayInterruptible(const char *device)
    : m_device(device)
{
    qDebug() << "@audio-wav-i" << __func__ << "m_device:" << m_device;
    memset(&m_wavFmt, 0, sizeof(FMT_t));
    start();
}

WavPlayInterruptible::~WavPlayInterruptible()
{
    {
        QMutexLocker locker(&m_ctrlMutex);
        m_quit = true;
        m_stopCurrent = true;
        m_newPlayPending = true;
        m_ctrlCond.wakeAll();
    }
    wait();
    qDebug() << "@audio-wav-i" << __func__ << m_device;
}

void WavPlayInterruptible::setWavPath(const char *filepath)
{
    QMutexLocker locker(&m_ctrlMutex);
    if (filepath)
        m_userPath = filepath;
    else
        m_userPath.clear();
    VLOGD("@audio-wav-i m_userPath: %s", m_userPath.c_str());
}

void WavPlayInterruptible::play()
{
    QMutexLocker locker(&m_ctrlMutex);
    if (m_userPath.empty()) {
        VLOGE("@audio-wav-i play: path empty, call setWavPath first");
        return;
    }
    m_stopCurrent = true;
    m_pendingPath = m_userPath;
    m_newPlayPending = true;
    m_ctrlCond.wakeAll();
}

void WavPlayInterruptible::play(const char *filepath)
{
    if (!filepath || !filepath[0]) {
        VLOGE("@audio-wav-i play(filepath): invalid path");
        return;
    }
    QMutexLocker locker(&m_ctrlMutex);
    m_userPath = filepath;
    VLOGD("@audio-wav-i m_userPath: %s", m_userPath.c_str());
    m_stopCurrent = true;
    m_pendingPath = m_userPath;
    m_newPlayPending = true;
    m_ctrlCond.wakeAll();
}

void WavPlayInterruptible::stop()
{
    QMutexLocker locker(&m_ctrlMutex);
    m_stopCurrent = true;
    m_ctrlCond.wakeAll();
}

void WavPlayInterruptible::closeWavFile()
{
    if (file_fd >= 0) {
        close(file_fd);
        file_fd = -1;
    }
}

void WavPlayInterruptible::sndPcmClose()
{
    if (m_pcm) {
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
    }
}

void WavPlayInterruptible::run()
{
    while (true) {
        std::string path;
        {
            QMutexLocker locker(&m_ctrlMutex);
            while (!m_quit && !m_newPlayPending)
                m_ctrlCond.wait(&m_ctrlMutex, 100);
            if (m_quit)
                return;
            path = m_pendingPath;
            m_newPlayPending = false;
            m_stopCurrent = false;
        }

        if (path.empty())
            continue;

        if (openWavFile(path) < 0) {
            VLOGE("@audio-wav-i openWavFile failed");
            continue;
        }

        if (sndPcmInit() < 0) {
            VLOGE("@audio-wav-i sndPcmInit failed");
            closeWavFile();
            continue;
        }

        {
            QMutexLocker locker(&m_ctrlMutex);
            if (m_quit) {
                closeWavFile();
                sndPcmClose();
                return;
            }
            if (m_stopCurrent) {
                closeWavFile();
                sndPcmClose();
                continue;
            }
        }

        uint8_t *buf = static_cast<uint8_t *>(malloc(buf_bytes));
        if (!buf) {
            VLOGE("@audio-wav-i malloc failed");
            closeWavFile();
            sndPcmClose();
            continue;
        }

        unsigned long wsize = period_size;
        bool interrupted = false;
        bool userQuit = false;
        bool readError = false;

        while (true) {
            {
                QMutexLocker locker(&m_ctrlMutex);
                if (m_quit) {
                    userQuit = true;
                    interrupted = true;
                    break;
                }
                if (m_stopCurrent) {
                    interrupted = true;
                    break;
                }
            }

            memset(buf, 0, buf_bytes);
            int ret = read(file_fd, buf, buf_bytes);
            if (ret <= 0) {
                if (ret < 0) {
                    VLOGE("@audio-wav-i read error");
                    readError = true;
                }
                break;
            }
            if (ret < static_cast<int>(buf_bytes)) {
                wsize = static_cast<unsigned long>(ret) * 8u /
                        (static_cast<unsigned long>(m_wavFmt.BitsPerSample) *
                         static_cast<unsigned long>(m_wavFmt.NumChannels));
                snd_pcm_format_set_silence(SND_PCM_FORMAT_S16_LE, buf + ret,
                    (period_size - wsize) * m_wavFmt.NumChannels);
                wsize = period_size;
            } else {
                wsize = period_size;
            }

            {
                QMutexLocker locker(&m_ctrlMutex);
                if (m_quit) {
                    userQuit = true;
                    interrupted = true;
                    break;
                }
                if (m_stopCurrent) {
                    interrupted = true;
                    break;
                }
            }

            ret = snd_pcm_writei(m_pcm, buf, static_cast<snd_pcm_uframes_t>(wsize));
            if (ret < 0) {
                if (ret == -EPIPE) {
                    snd_pcm_prepare(m_pcm);
                    continue;
                }
                VLOGD("@audio-wav-i snd_pcm_writei error, end");
                interrupted = true;
                break;
            }
        }

        free(buf);
        closeWavFile();

        if (m_pcm) {
            if (interrupted)
                snd_pcm_drop(m_pcm);
            else
                snd_pcm_drain(m_pcm);
            snd_pcm_close(m_pcm);
            m_pcm = nullptr;
        }

        bool suppressEndSignal = false;
        {
            QMutexLocker locker(&m_ctrlMutex);
            if (m_stopCurrent || m_newPlayPending)
                suppressEndSignal = true;
        }
        if (!interrupted && !userQuit && !readError && !suppressEndSignal)
            emit naturalPlaybackFinished();

        if (userQuit)
            return;
    }
}

int WavPlayInterruptible::openWavFile(const std::string &path)
{
    RIFF_t wav_riff;
    int ret = 0;

    closeWavFile();

    file_fd = open(path.c_str(), O_RDONLY);
    if (file_fd < 0) {
        fprintf(stderr, "@audio-wav-i open error: %s: %s\n", path.c_str(), strerror(errno));
        return -1;
    }

    ret = static_cast<int>(read(file_fd, &wav_riff, sizeof(RIFF_t)));
    if (ret != static_cast<int>(sizeof(RIFF_t))) {
        if (ret < 0)
            perror("@audio-wav-i read RIFF_t");
        closeWavFile();
        return -1;
    }

    if (strncmp("RIFF", wav_riff.ChunkID, 4) || strncmp("WAVE", wav_riff.Format, 4))
        fprintf(stderr, "@audio-wav-i RIFF/WAVE check: %s\n", path.c_str());

    ret = static_cast<int>(read(file_fd, &m_wavFmt, sizeof(FMT_t)));
    if (ret != static_cast<int>(sizeof(FMT_t))) {
        closeWavFile();
        return -1;
    }

    if (strncmp("fmt ", m_wavFmt.Subchunk1ID, 4)) {
        closeWavFile();
        return -1;
    }

    if (lseek(file_fd, sizeof(RIFF_t) + 8 + m_wavFmt.Subchunk1Size, SEEK_SET) < 0) {
        closeWavFile();
        return -1;
    }

    while (static_cast<size_t>(sizeof(DATA_t)) ==
           static_cast<size_t>(read(file_fd, &wav_data, sizeof(DATA_t)))) {
        if (!strncmp("data", wav_data.Subchunk2ID, 4))
            return 0;
        if (lseek(file_fd, wav_data.Subchunk2Size, SEEK_CUR) < 0) {
            closeWavFile();
            return -1;
        }
    }

    closeWavFile();
    return -1;
}

int WavPlayInterruptible::sndPcmInit()
{
    int ret = 0;
    snd_pcm_hw_params_t *hwparams = nullptr;

    ret = snd_pcm_open(&m_pcm, m_device, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        fprintf(stderr, "@audio-wav-i snd_pcm_open error: %s: %s\n", m_device, snd_strerror(ret));
        return -1;
    }

    snd_pcm_hw_params_malloc(&hwparams);

    ret = snd_pcm_hw_params_any(m_pcm, hwparams);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_access(m_pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_format(m_pcm, hwparams, SND_PCM_FORMAT_S16_LE);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_rate_near(m_pcm, hwparams, &m_wavFmt.SampleRate, 0);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_channels(m_pcm, hwparams, m_wavFmt.NumChannels);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    snd_pcm_uframes_t frames_per_buffer = 0;
    snd_pcm_hw_params_get_buffer_size_max(hwparams, &frames_per_buffer);

    ret = snd_pcm_hw_params_set_buffer_size_near(m_pcm, hwparams, &frames_per_buffer);
    if (ret < 0)
        qDebug() << "@audio-wav-i snd_pcm_hw_params_set_buffer_size_near error";

    period_size = frames_per_buffer / m_periods;

    ret = snd_pcm_hw_params_set_period_size_near(m_pcm, hwparams, &period_size, 0);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params(m_pcm, hwparams);
    if (ret < 0) {
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
        m_pcm = nullptr;
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
    buf_bytes = period_size * m_wavFmt.BlockAlign;

    snd_pcm_hw_params_free(hwparams);
    return 0;
}
