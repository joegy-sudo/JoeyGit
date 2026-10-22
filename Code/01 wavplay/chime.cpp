#include "chime.h"
#include <algorithm>

Chime::Chime(const char *plug):m_plug(plug)
{
	qDebug()<<"@audio-chime constructor plug :"<<m_plug;
	m_loopflag = false;
	m_mode = 0;
	m_ringDurationMs = 0;
	m_looptime = 20;//no use
	m_stopRequested = false;
	pPdc = make_shared<WavPlay>(m_plug);
	pPdc->setSampleFadeEnabled(true);
}

void Chime::setSampleFadeEnabled(bool enable)
{
	if (pPdc)
		pPdc->setSampleFadeEnabled(enable);
}

void Chime::setSampleFadeMs(int ms)
{
	if (pPdc)
		pPdc->setSampleFadeMs(ms);
}

int Chime::getSampleFadeMs() const
{
	return pPdc ? pPdc->getSampleFadeMs() : 0;
}

void Chime::waitThreadEnd()
{
	ensureStopped(false);
}

bool Chime::isPdcRunning() const
{
	return pPdc && pPdc->isRunning();
}

void Chime::waitPdcExit(int softWaitMs, int hardWaitMs)
{
	if (!pPdc || !pPdc->isRunning())
		return;
	if (softWaitMs > 0 && pPdc->wait(static_cast<unsigned long>(softWaitMs)))
		return;
	/* hardWaitMs==0：只等淡出/自然退，不硬停 */
	if (hardWaitMs <= 0)
		return;
	pPdc->requestStop(false);
	if (!pPdc->wait(static_cast<unsigned long>(hardWaitMs)))
		VLOGE("@audio-chime waitPdcExit: WavPlay still running after hard stop");
}

void Chime::joinPdcWhilePlaying()
{
	if (!pPdc)
		return;
	waitUntilPdcStarted();
	while (pPdc->isRunning()) {
		plock.lock();
		const bool stop = m_stopRequested;
		plock.unlock();
		if (stop) {
			/* 常规停只等淡出+drain，不在此硬停 */
			waitPdcExit(getSampleFadeMs() + 150, 0);
			return;
		}
		pPdc->wait(50);
	}
}

void Chime::ensureStopped(bool fadeOut)
{
	if (fadeOut) {
		/* 常规停/类型切换：只淡出并等待干净退出，不硬 drop */
		const int fadeMs = pPdc ? pPdc->getSampleFadeMs() : 100;
		const int waitMs = fadeMs + 150;
		stopRinging(true);
		if (this->isRunning())
			this->wait(static_cast<unsigned long>(waitMs));
		if (pPdc && pPdc->isRunning())
			pPdc->wait(static_cast<unsigned long>(waitMs));
		if (this->isRunning() || isPdcRunning())
			VLOGD("@audio-chime ensureStopped fade: still alive chime=%d pdc=%d",
				this->isRunning() ? 1 : 0, isPdcRunning() ? 1 : 0);
		return;
	}

	for (int i = 0; i < 5; ++i) {
		stopRinging(false);
		if (this->isRunning())
			this->wait(200);
		if (pPdc && pPdc->isRunning()) {
			pPdc->requestStop(false);
			pPdc->wait(200);
		}
		if (!this->isRunning() && !isPdcRunning())
			return;
	}
	VLOGE("@audio-chime ensureStopped hard: still alive chime=%d pdc=%d",
		this->isRunning() ? 1 : 0, isPdcRunning() ? 1 : 0);
}

Chime::~Chime()
{
	ensureStopped(false);
}

void Chime::setRingDurationMs(int durationMs)
{
	plock.lock();
	m_ringDurationMs = durationMs > 0 ? durationMs : 0;
	plock.unlock();
}

void Chime::stopRinging(bool fadeOut)
{
	plock.lock();
	m_loopflag = false;
	m_stopRequested = true;
	plock.unlock();
	if (pPdc) {
		pPdc->setCycle(0);
		pPdc->requestStop(fadeOut);
	}
}

bool Chime::isRingTimedOut(const QElapsedTimer &ringTimer, int ringMs) const
{
	if (ringMs <= 0 || !ringTimer.isValid())
		return false;
	return ringTimer.elapsed() >= static_cast<qint64>(ringMs);
}

void Chime::waitUntilPdcStarted()
{
	if (!pPdc)
		return;

	for (int i = 0; i < 100 && !pPdc->isRunning() && !pPdc->isFinished(); ++i)
		QThread::msleep(2);
}

void Chime::waitPdcDone(const QElapsedTimer *ringTimer, int ringMs)
{
	if (!pPdc)
		return;
	waitUntilPdcStarted();
	while (pPdc->isRunning()) {
		if (ringTimer && isRingTimedOut(*ringTimer, ringMs)) {
			pPdc->requestStop(true);
			break;
		}
		plock.lock();
		const bool forceStop = m_stopRequested;
		plock.unlock();
		if (forceStop) {
			/* stopRinging 已按需 requestStop(fadeOut)，此处勿再打断淡出 */
			break;
		}
		QThread::msleep(20);
	}
	/* 淡出+drain 后结束；不在此硬停（硬停仅进倒车孤儿场景） */
	waitPdcExit(getSampleFadeMs() + 150, 0);
}

void Chime::sleepWithCheck(int ms, const QElapsedTimer &ringTimer, int ringMs)
{
	int left = ms;
	while (left > 0) {
		if (isRingTimedOut(ringTimer, ringMs))
			return;
		plock.lock();
		if (m_stopRequested) {
			plock.unlock();
			return;
		}
		plock.unlock();
		const int chunk = std::min(left, 50);
		QThread::msleep(chunk);
		left -= chunk;
	}
}

void Chime::runUnlimited()
{
	if (m_mode == 0) {
		while (1) {
			pPdc->start();
			joinPdcWhilePlaying();

			plock.lock();
			const bool keepLoop = m_loopflag;
			const bool stop = m_stopRequested;
			plock.unlock();
			if (!keepLoop || stop)
				break;

			QThread::msleep(m_looptime);
		}
	} else if (m_mode == 1) {
		pPdc->start();
		joinPdcWhilePlaying();
	}
}

void Chime::runWithDuration(const QElapsedTimer &ringTimer, int ringMs)
{
	if (m_mode == 0) {

		while (!isRingTimedOut(ringTimer, ringMs)) {
			pPdc->start();
			waitPdcDone(&ringTimer, ringMs);
			if (isRingTimedOut(ringTimer, ringMs))
				break;

			plock.lock();
			if (!m_loopflag) {
				plock.unlock();
				break;
			}
			plock.unlock();

			sleepWithCheck(m_looptime, ringTimer, ringMs);
		}
	} else if (m_mode == 1) {
		if (!isRingTimedOut(ringTimer, ringMs)) {
			pPdc->start();
			waitPdcDone(&ringTimer, ringMs);
		}
	}
}

void Chime::run()
{
	plock.lock();
	const int ringMs = m_ringDurationMs;
	const int mode = m_mode;
	m_stopRequested = false;
	plock.unlock();

	if (ringMs <= 0) {
		runUnlimited();
	} else {
		QElapsedTimer ringTimer;
		ringTimer.start();
		runWithDuration(ringTimer, ringMs);
	}

	if (pPdc)
		pPdc->setCycle(0);
	plock.lock();
	const bool userStop = m_stopRequested;
	m_loopflag = false;
	m_ringDurationMs = 0;
	m_stopRequested = false;
	plock.unlock();
	if (!userStop && (ringMs > 0 || mode == 0))
		emit naturalPlaybackFinished();
}

void Chime::setOutLoopFlag(bool flag)
{
	//qDebug()<<"@audio-chime\t"<<__func__<<"\t m_loopflag = "<<flag;
	plock.lock();
	m_loopflag = flag;
	plock.unlock();
}

void Chime::setloopTime(int looptime)
{
	plock.lock();
	m_looptime = looptime;
	plock.unlock();
}

void Chime::setWavPath(const char *path)
{
	if (pPdc)
		pPdc->setWavPath(path);
}

void Chime::setPDCdataloop(int flag)
{
	pPdc->setCycle(flag);
}

void Chime::setMode(int mode)
{
	plock.lock();
	m_mode = mode;
	plock.unlock();
}
