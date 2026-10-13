#ifndef CHIME_H
#define CHIME_H
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QElapsedTimer>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <iostream>
#include "wavplay.h"

using namespace std;
class WavPlay;
class Chime : public QThread
{
	Q_OBJECT
public:
    explicit Chime(const char *plug);
    ~Chime();

	void run() override;

	void setOutLoopFlag(bool flag);
	void setloopTime(int looptime);
	void setWavPath(const char *path);
	void setPDCdataloop(int flag);
	void setMode(int mode);
	void waitThreadEnd();
	bool isPdcRunning() const;
	void ensureStopped(bool fadeOut = true);
	void setRingDurationMs(int durationMs);
	void stopRinging(bool fadeOut = true);
	void setSampleFadeEnabled(bool enable);

	void setSampleFadeMs(int ms);
	int getSampleFadeMs() const;
signals:
	void naturalPlaybackFinished();
private:
	void runUnlimited();
	void runWithDuration(const QElapsedTimer &ringTimer, int ringMs);
	void waitPdcDone(const QElapsedTimer *ringTimer, int ringMs);
	void waitUntilPdcStarted();
	void waitPdcExit(int softWaitMs, int hardWaitMs);
	void joinPdcWhilePlaying();
	bool isRingTimedOut(const QElapsedTimer &ringTimer, int ringMs) const;
	void sleepWithCheck(int ms, const QElapsedTimer &ringTimer, int ringMs);

	const char *m_plug;
	bool m_loopflag;
	int m_looptime;
	int m_mode;
	int m_ringDurationMs;
	bool m_stopRequested;
	shared_ptr<WavPlay> pPdc;
    QMutex plock;
};

#endif // CHIME_H
