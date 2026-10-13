#ifndef HANDLER_H
#define HANDLER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>
#include <QDebug>

#define HANDLER_WHAT_ADJUST_VOL 		1001

#define HANDLER_STORE_STS_MUTE			2001
#define HANDLER_STORE_VAL_EQ			2002
#define HANDLER_STORE_VAL_BASS0			2003
#define HANDLER_STORE_VAL_MID0			2004
#define HANDLER_STORE_VAL_TRE0			2005
#define HANDLER_STORE_VAL_BASS1			2006
#define HANDLER_STORE_VAL_MID1			2007
#define HANDLER_STORE_VAL_TRE1			2008
#define HANDLER_STORE_STS_MIC			2009
#define HANDLER_STORE_STREAM_VOL		2010
#define HANDLER_STORE_STS_TOUCH			2011
#define HANDLER_STORE_VAL_SPD			2012
#define HANDLER_STORE_STS_SPD			2013
#define HANDLER_RESET_STREAM			2014
#define HANDLER_RESET_CLEAR				2015	
#define HANDLER_STORE_VAL_AVAS			2016
#define HANDLER_STORE_STS_NAVIMEDIA 	2017
#define HANDLER_STORE_VAL_CARLOCKSOUND 	2018
#define HANDLER_STORE_VAL_AVASTYPE		2019
#define HANDLER_STORE_VAL_MID2			2020

/**
 * This class allows to specify multiple time intervals to receive
 * notifications. A single thread is used internally.
 */
class Handler {
private:
    using Nanos = std::chrono::nanoseconds;
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock, Nanos>;
public:
    std::chrono::nanoseconds millisToNanoseconds(int64_t millis) const {
        if (millis < 0) {
            millis = 0;
        }
        return std::chrono::nanoseconds(1000000 * millis);
    }
    using Action = std::function<void(const int32_t &cookies, const int32_t arg0, const int32_t arg1)>;

    Handler(const Action &action) : mAction(action) {
        mTimerThread = std::thread(&Handler::loop, this, action);
    }

    virtual ~Handler() {
        stop();
    }


    void sendMessage(int32_t cookie, int32_t arg0, int32_t arg1) {
        sendMessageDelayed(cookie, 0, arg0, arg1);
    }

    void sendMessageDelayed(int32_t cookie, int64_t delayMillis,int32_t arg0, int32_t arg1) {
        {
            //            qDebug("sendMessageDelayed cookie = %d", cookie);
            Nanos nanos = millisToNanoseconds(delayMillis);
            TimePoint now = Clock::now();
            TimePoint absoluteTime = now + nanos;

            std::lock_guard<std::mutex> g(mLock);
            mCookieToEventsMap[cookie] = {nanos, cookie, absoluteTime, arg0, arg1};
        }
        mCond.notify_one();
    }

    void removeMessages(int32_t cookie) {
        {
            std::lock_guard<std::mutex> g(mLock);
            mCookieToEventsMap.erase(cookie);
        }
        mCond.notify_one();
    }


private:

    struct RecurrentEvent {
        Nanos delay;
        int32_t cookie;
        TimePoint absoluteTime;  // Absolute time of the next event.
		int32_t arg0;
		int32_t arg1;

        void updateNextEventTime(TimePoint now) {
            absoluteTime = now + delay;
        }
    };

    void loop(const Action &action) {
        static constexpr auto kInvalidTime = TimePoint(Nanos::max());

       // std::vector<int32_t> cookies;
		std::vector<RecurrentEvent> works;
        while (!mStopRequested) {
            works.clear();
            {
                std::unique_lock<std::mutex> g(mLock);
                auto now = Clock::now();
                
                // 只寻找超时任务
                std::unordered_map<int32_t, RecurrentEvent>::iterator it = mCookieToEventsMap.begin();
                while (it != mCookieToEventsMap.end()) {
                    RecurrentEvent& event = it->second;
                    if (event.absoluteTime <= now) {
                        //cookies.push_back(event.cookie);
						works.push_back(it->second);
                        it = mCookieToEventsMap.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            if (works.size() != 0 && action != nullptr) {
                for (auto work : works) {
                    action(work.cookie, work.arg0, work.arg1); // Actions may increase events
                }
            }

            std::unique_lock<std::mutex> g(mLock);
            if (mCookieToEventsMap.empty()) {
                // qDebug("empty wait ...");
                mCond.wait(g);
            } else {
                auto now = Clock::now();
                auto nextEventTime = kInvalidTime;
                bool time_out = false;

                // 防止在Action中增加新的任务，下个次循环时间点在这里统计
                std::unordered_map<int32_t, RecurrentEvent>::iterator it = mCookieToEventsMap.begin();
                while (it != mCookieToEventsMap.end()) {
                    RecurrentEvent& event = it->second;

                    if (event.absoluteTime <= now) {
                        time_out = true;    // 有超时任务，直接跳出循环
                        break;
                    } else {
                        if (nextEventTime > event.absoluteTime) {
                            nextEventTime = event.absoluteTime; // 下个任务时间点
                        }
                        ++it;
                    }
                }

                // 没有超时任务，等待下个任务
                if (!time_out) {
                    // qDebug("wait_until ...");
                    mCond.wait_until(g, nextEventTime);  // nextEventTime can be nanoseconds::max()
                }
            }
        }
    }

    void stop() {
        mStopRequested = true;
        {
            std::lock_guard<std::mutex> g(mLock);
            mCookieToEventsMap.clear();
        }
        mCond.notify_one();
        if (mTimerThread.joinable()) {
            mTimerThread.join();
        }
    }

private:
    mutable std::mutex mLock;
    std::thread mTimerThread;
    std::condition_variable mCond;
    std::atomic_bool mStopRequested{false};
    Action mAction;
    std::unordered_map<int32_t, RecurrentEvent> mCookieToEventsMap;
};



#endif // HANDLER_H
