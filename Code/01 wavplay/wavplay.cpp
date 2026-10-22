#include "wavplay.h"
#include <fcntl.h>
#include <algorithm>
#include <cmath>

static const float kSampleFadeInStart = 0.0f;
static const int kSampleFadeMsDefault = 100;

WavPlay::WavPlay(const char *device):m_device(device)
{
    qDebug()<<"@audio-wav "<<__func__<<" m_device : "<<m_device;
	file_fd = -1;
	m_pcm = nullptr;
    m_periods = 4;
	mCycle = 0;
	m_stopRequested = false;
	m_sampleFadeEnabled = false; 
	m_fadeOutActive = false;
	m_fadeStopCompleted = false;
	m_fadeGain = 1.0f;
	m_sampleFadeMs = kSampleFadeMsDefault;
    memset(&m_wavFmt, 0x0, sizeof(FMT_t));
}

void WavPlay::setSampleFadeEnabled(bool enable)
{
	QMutexLocker locker(&aLock);
	m_sampleFadeEnabled = enable;
}

void WavPlay::setSampleFadeMs(int ms)
{
	QMutexLocker locker(&aLock);
	m_sampleFadeMs = (ms > 0) ? ms : kSampleFadeMsDefault;
}

int WavPlay::getSampleFadeMs() const
{
	QMutexLocker locker(&aLock);
	return m_sampleFadeMs;
}

void WavPlay::resetSampleFadeState()
{/* 每次 run() 起播淡入；文件内 mCycle 回绕不重进 run()，不会重复淡入 */
	m_fadeOutActive = false;
	m_fadeStopCompleted = false;
	if (m_sampleFadeEnabled)
		m_fadeGain = kSampleFadeInStart;
	else
		m_fadeGain = 1.0f;
}

bool WavPlay::seekToPcmData()
{
	if (file_fd < 0)
		return false;
	if (lseek(file_fd, sizeof(RIFF_t) + 8 + m_wavFmt.Subchunk1Size + sizeof(DATA_t), SEEK_SET) < 0) {
		VLOGE("@audio-wav lseek pcm data error");
		return false;
	}
	return true;
}

void WavPlay::requestStop(bool fadeOut)
{
	QMutexLocker locker(&aLock);
	if (fadeOut && m_sampleFadeEnabled && !m_stopRequested) {
		
		if (m_fadeGain <= 0.0f)
			m_fadeGain = 1.0f;
		m_fadeOutActive = true;
		mCycle = 0;
		return;
	}
	m_stopRequested = true;
	m_fadeOutActive = false;
	mCycle = 0;
	if (m_pcm)
		snd_pcm_drop(m_pcm);
}

void WavPlay::applySampleFade(uint8_t *buf, size_t bytes)
{
	if (!buf || bytes == 0 || m_wavFmt.BitsPerSample != 16 || m_wavFmt.NumChannels == 0)
		return;

	float gain = 1.0f;
	float gainStep = 0.0f;
	bool fadeOut = false;
	bool fadeEnabled = false;
	int fadeMs = kSampleFadeMsDefault;
	{
		QMutexLocker locker(&aLock);
		fadeEnabled = m_sampleFadeEnabled;
		fadeOut = m_fadeOutActive;
		gain = m_fadeGain;
		fadeMs = m_sampleFadeMs > 0 ? m_sampleFadeMs : kSampleFadeMsDefault;
	}
	if (!fadeEnabled && !fadeOut)
		return;

	const size_t frameBytes = m_wavFmt.BlockAlign ? m_wavFmt.BlockAlign : (2 * m_wavFmt.NumChannels);
	const size_t frames = bytes / frameBytes;
	if (frames == 0)
		return;

	const uint32_t rate = m_wavFmt.SampleRate ? m_wavFmt.SampleRate : 48000;
	const float fadeFrames = std::max(1.0f, static_cast<float>(rate) * fadeMs / 1000.0f);
	if (fadeOut)
		gainStep = -1.0f / fadeFrames; /* 1→0，100ms 线性 */
	else if (gain < 1.0f)
		gainStep = (1.0f - kSampleFadeInStart) / fadeFrames; /* 0.1→1，100ms 线性 */
	else
		return;

	int16_t *samples = reinterpret_cast<int16_t *>(buf);
	const size_t ch = m_wavFmt.NumChannels;

	for (size_t f = 0; f < frames; ++f) {
		for (size_t c = 0; c < ch; ++c) {
			const size_t idx = f * ch + c;
			int v = static_cast<int>(std::lround(static_cast<float>(samples[idx]) * gain));
			if (v > 32767)
				v = 32767;
			else if (v < -32768)
				v = -32768;
			samples[idx] = static_cast<int16_t>(v);
		}
		gain += gainStep;
		if (fadeOut) {
			if (gain <= 0.0f) {
				gain = 0.0f;
				/* 余下样点静音 */
				for (size_t nf = f + 1; nf < frames; ++nf) {
					for (size_t c = 0; c < ch; ++c)
						samples[nf * ch + c] = 0;
				}
				QMutexLocker locker(&aLock);
				m_fadeGain = 0.0f;
				m_stopRequested = true;
				m_fadeOutActive = false;
				m_fadeStopCompleted = true; /* 淡出到静音：结尾应 drain，勿 drop */
				mCycle = 0;
				return;
			}
		} else if (gain >= 1.0f) {
			gain = 1.0f;
			QMutexLocker locker(&aLock);
			m_fadeGain = 1.0f;
			/* 后续帧保持满幅，不再乘因子 */
			return;
		}
	}

	QMutexLocker locker(&aLock);
	m_fadeGain = gain;
}

WavPlay::~WavPlay()
{
	if (this->isRunning()) {
		requestStop(false);
		if (!this->wait(1000))
			VLOGE("@audio-wav destructor: WavPlay still running after hard stop");
	}
	qDebug() << __func__ << " bicv unconstractor " << m_device;
}

void WavPlay::setCycle(int flag)
{
	//qDebug()<<"@audio-wav\t"<<__func__<<"\t"<<flag;
	aLock.lock();
	mCycle = flag;
	aLock.unlock();
}

ssize_t  WavPlay::SNDWAV_P_SaveRead(int fd, void *buf, size_t count)  
{  
    ssize_t result = 0, res;  

    while (count > 0) {  
        if ((res = read(fd, buf, count)) == 0)  
            break;  
        if (res < 0)  
            return result > 0 ? result : res;  
        count -= res;  
        result += res;  
        buf = (char *)buf + res;
    }  
    return result;  
}  


ssize_t WavPlay::SNDWAV_WritePcm(uint8_t *buf, size_t wcount)  
{  
    ssize_t r;  
    ssize_t result = 0;  
    uint8_t *data = buf;
  	int bits_per_frame = 0;
	bits_per_frame = m_wavFmt.BitsPerSample * m_wavFmt.NumChannels;
    if (wcount < period_size) {  
        snd_pcm_format_set_silence(SND_PCM_FORMAT_S16_LE,   
            data + wcount * bits_per_frame / 8,   
            (period_size - wcount) * m_wavFmt.NumChannels);  
        wcount = period_size;  
    }  
    while (wcount > 0) {  
        r = snd_pcm_writei(m_pcm, data, wcount);  
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {  
            snd_pcm_wait(m_pcm, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(m_pcm);  
            fprintf(stderr, "@audio-wav <<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>/n");  
        } else if (r == -ESTRPIPE) {              
            fprintf(stderr, "@audio-wav <<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>/n");          
        } else if (r < 0) {  
            fprintf(stderr, "@audio-wav Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
        if (r > 0) {  
            result += r;  
            wcount -= r;  
            data += r * bits_per_frame / 8;  
        }  
    }  
    return result;  
}  
  

void WavPlay::run()
{
	int ret;
	
    if(open_wavfile() < 0){
        VLOGE("@audio-wav error: open_wavfile failed");
        return ;
    }

    if(snd_pcm_init() < 0){
        VLOGE("@audio-wav error: snd_pcm_init failed");
		close(file_fd);
		file_fd = -1;
        return ;
    }

    uint8_t *buf = (uint8_t *)malloc(buf_bytes);
    if(!buf){
        VLOGE("@audio-wav malloc error ");
        close(file_fd);
		file_fd = -1;
		{
			QMutexLocker locker(&aLock);
			if (m_pcm) {
				snd_pcm_close(m_pcm);
				m_pcm = nullptr;
			}
		}
        return;
    }

	unsigned long wsize = period_size;
	bool stoppedByRequest = false;
	{
		QMutexLocker locker(&aLock);
		m_stopRequested = false;
		resetSampleFadeState();
	}
	//qDebug()<<"@audio-wav run count : "<<wav_data.Subchunk2Size;
	while(1){
		{
			QMutexLocker locker(&aLock);
			if (m_stopRequested) {
				stoppedByRequest = true;
				break;
			}
		}
		memset(buf, 0x0, buf_bytes);
		ret = read(file_fd, buf, buf_bytes);
		if(ret <= 0){
			/* 连续循环：文件读完后回到 PCM 起点，不重启线程、不重新淡入 */
			int cycle = 0;
			{
				QMutexLocker locker(&aLock);
				cycle = mCycle;
			}
			if (ret == 0 && cycle == 1) {
				if (!seekToPcmData())
					break;
				wsize = period_size;
				continue;
			}
            if(ret != 0)
                VLOGE("@audio-wav read data error");
            break;
        }else if(ret < (int)buf_bytes){
			int cycle = 0;
			{
				QMutexLocker locker(&aLock);
				cycle = mCycle;
			}
			if(cycle == 1){
				/* 先写出文件尾部剩余样点(保持当前增益)，再回绕，避免每圈淡入淡出 */
				wsize = ret * 8 / (m_wavFmt.BitsPerSample * m_wavFmt.NumChannels);
				applySampleFade(buf, static_cast<size_t>(ret));
				bool stopAfterWrite = false;
				{
					QMutexLocker locker(&aLock);
					stopAfterWrite = m_stopRequested;
				}
				int wr = snd_pcm_writei(m_pcm, buf, wsize);
				if (stopAfterWrite) {
					stoppedByRequest = true;
					break;
				}
				if (wr < 0) {
					QMutexLocker locker(&aLock);
					if (m_stopRequested) {
						stoppedByRequest = true;
						break;
					}
					if (wr == -EPIPE)
						snd_pcm_prepare(m_pcm);
					else
						break;
				}
				if (!seekToPcmData())
					break;
				wsize = period_size;
				continue;
			}else{//结尾字节不足 插入 静音数据
				wsize = ret * 8 / (m_wavFmt.BitsPerSample * m_wavFmt.NumChannels);
				snd_pcm_format_set_silence(SND_PCM_FORMAT_S16_LE, buf+ret, (period_size - wsize)*m_wavFmt.NumChannels);
				wsize = period_size;
				ret = static_cast<int>(buf_bytes);
			}
        } else {// == buf_bytes
			wsize = period_size;
		}

		applySampleFade(buf, static_cast<size_t>(ret));
		bool stopAfterWrite = false;
		{
			QMutexLocker locker(&aLock);
			stopAfterWrite = m_stopRequested;
		}
		ret = snd_pcm_writei(m_pcm, buf, wsize);
		if (stopAfterWrite) {
			stoppedByRequest = true;
			break;
		}
		if(ret < 0){
			{
				QMutexLocker locker(&aLock);
				if (m_stopRequested) {
					stoppedByRequest = true;
					break;
				}
			}
			if (ret == -EPIPE) {
				snd_pcm_prepare(m_pcm);
				fprintf(stderr, "@audio-wav snd_pcm_writei error:%s %d\n",strerror(ret), ret),
				VLOGD("@audio-wav error(EPIPE) end");
				continue;
			}else {
				VLOGD("@audio-wav error end");
				break;
			}
		}
		
	}
	
	close(file_fd);
	file_fd = -1;
	snd_pcm_t *pcm = nullptr;
	bool doDrop = false;
	{
		QMutexLocker locker(&aLock);
		pcm = m_pcm;
		doDrop = stoppedByRequest && !m_fadeStopCompleted;
	}
	if (pcm) {
		if (doDrop)
			snd_pcm_drop(pcm);
		else
			snd_pcm_drain(pcm);
		{
			QMutexLocker locker(&aLock);
			if (m_pcm == pcm) {
			snd_pcm_close(m_pcm);
			m_pcm = nullptr;
			}
		}
	}
	free(buf);
}

void WavPlay::setWavPath(const char *filepath)
{
	if (this->isRunning()) {
		for (int i = 0; i < 5 && this->isRunning(); ++i) {
			requestStop(false);
			if (this->wait(200))
				break;
		}
		if (this->isRunning()) {
			VLOGE("@audio-wav setWavPath: WavPlay still running, skip path change");
			return;
		}
	}

	if (file_fd >= 0) {
		close(file_fd);
		file_fd = -1;
	}

	if (filepath)
		m_wavpathStr = filepath;
	else
		m_wavpathStr.clear();

	VLOGD("@audio-wav  m_wavpath : %s", m_wavpathStr.c_str());
}

void WavPlay::printWavFmtInfo()
{
    qDebug()<<"@audio-wav <<audio *.wav info display>>";
    qDebug()<<"@audio-wav   file name:     "<< m_wavpathStr.c_str();
    qDebug()<<"@audio-wav   Subchunk1Size: "<< m_wavFmt.Subchunk1Size;
    qDebug()<<"@audio-wav   AudioFormat:   "<< m_wavFmt.AudioFormat;
    qDebug()<<"@audio-wav   NumChannels:   "<< m_wavFmt.NumChannels;
    qDebug()<<"@audio-wav   SampleRate:    "<< m_wavFmt.SampleRate;
    qDebug()<<"@audio-wav   ByteRate:      "<< m_wavFmt.ByteRate;
    qDebug()<<"@audio-wav   BlockAlign:    "<< m_wavFmt.BlockAlign;
    qDebug()<<"@audio-wav   BitsPerSample: "<< m_wavFmt.BitsPerSample;
}

int WavPlay::open_wavfile()
{
    RIFF_t wav_riff;
    
    int ret = 0;

    if(m_wavpathStr.empty()){
        VLOGE("@audio-wav open_wavfile path empty");
        return -1;
    }
	
    file_fd = open(m_wavpathStr.c_str(), O_RDONLY);
    if(file_fd < 0){
        fprintf(stderr, "@audio-wav open error: %s: %s\n", m_wavpathStr.c_str(), strerror(errno));
		VLOGE("@audio-wav file_fd < 0");
        return -1;
    }

    ret = read(file_fd, &wav_riff, sizeof(RIFF_t));
    if(ret != sizeof(RIFF_t)){
		VLOGE("@audio-wav ret != sizeof(RIFF_t)");
        if(ret < 0)
            perror("@audio-wav read RIFF_t error");
        else
            fprintf(stderr, "@audio-wav check error : %s\n", m_wavpathStr.c_str());
        close(file_fd);
		file_fd = -1;
        return -1;
    }

    if(strncmp("RIFF", wav_riff.ChunkID, 4) ||
            strncmp("WAVE", wav_riff.Format, 4)){
            VLOGE("@audio-wav lseek error .4");
        fprintf(stderr, "@audio-wav check error : %s\n", m_wavpathStr.c_str());
    }

    ret = read(file_fd, &m_wavFmt, sizeof(FMT_t));
    if(ret != sizeof(FMT_t)){
		VLOGE("@audio-wav ret != sizeof(FMT_t)");
        if(ret < 0)
            perror("@audio-wav read FMT_t error");
        else
            fprintf(stderr, "@audio-wav check error : %s\n", m_wavpathStr.c_str());
        close(file_fd);
		file_fd = -1;
        return -1;
    }

    if(strncmp("fmt ", m_wavFmt.Subchunk1ID, 4)){
		VLOGE("@audio-wav strncmp  m_wavFmt.Subchunk1ID ");
        fprintf(stderr, "@audio-wav check error : %s\n", m_wavpathStr.c_str());
        close(file_fd);
		file_fd = -1;
        return -1;
    }

    //printWavFmtInfo();

    if(lseek(file_fd, sizeof(RIFF_t) + 8 + m_wavFmt.Subchunk1Size, SEEK_SET) < 0){
        VLOGE("@audio-wav LSEEK ");
        close(file_fd);
		file_fd = -1;
        return -1;
    }

    while(sizeof(DATA_t) == read(file_fd, &wav_data, sizeof(DATA_t))){
        if(!strncmp("data", wav_data.Subchunk2ID, 4)){
			//VLOGE("@audio-wav check data Trunk ok ");
            return 0;
        }
        if(lseek(file_fd, wav_data.Subchunk2Size, SEEK_CUR) < 0){
			VLOGE("@audio-wav lseek error ");
            close(file_fd);
			file_fd = -1;
            return -1;
        }
    }

    fprintf(stderr, "@audio-wav check error: %s\n", m_wavpathStr.c_str());
	VLOGE("@audio-wav lseek END ");
	close(file_fd);
	file_fd = -1;
    return -1;
}

int WavPlay::snd_pcm_init()
{
    int ret = 0;
    snd_pcm_hw_params_t *hwparams = NULL;
	
    ret = snd_pcm_open(&m_pcm, m_device, SND_PCM_STREAM_PLAYBACK, 0);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_open error: %s: %s\n", m_device, snd_strerror(ret));
		m_pcm = nullptr;
        return -1;
    }

    snd_pcm_hw_params_malloc(&hwparams);

    ret = snd_pcm_hw_params_any(m_pcm, hwparams);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_any error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_access(m_pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_set_access error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_format(m_pcm, hwparams, SND_PCM_FORMAT_S16_LE);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_set_format error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_rate_near(m_pcm, hwparams, &m_wavFmt.SampleRate, 0);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_set_rate_near error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }

    ret = snd_pcm_hw_params_set_channels(m_pcm, hwparams, m_wavFmt.NumChannels);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_set_channels error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }

    snd_pcm_uframes_t frames_per_buffer = 0;
    snd_pcm_hw_params_get_buffer_size_max(hwparams, &frames_per_buffer);

    ret = snd_pcm_hw_params_set_buffer_size_near(m_pcm, hwparams, &frames_per_buffer);
    if(ret < 0)
        qDebug()<<"@audio-wav snd_pcm_hw_params_set_buffer_size_near error";

    period_size = frames_per_buffer / m_periods;

    ret = snd_pcm_hw_params_set_period_size_near(m_pcm, hwparams, &period_size, 0);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params_set_period_size_near error: %s\n", snd_strerror(ret));
        snd_pcm_hw_params_free(hwparams);
        snd_pcm_close(m_pcm);
		m_pcm = nullptr;
        return -1;
    }
	
    ret = snd_pcm_hw_params(m_pcm, hwparams);
    if( ret < 0 ){
        fprintf(stderr, "@audio-wav snd_pcm_hw_params error: %s\n", snd_strerror(ret));
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

