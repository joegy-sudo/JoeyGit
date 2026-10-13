#ifndef BDLOG_H
#define BDLOG_H

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <QDebug>
#include <stdint.h>
#include <syslog.h>
#include <sys/time.h>

#if 1
#define SYS_LOG_OPEN(TAG)       openlog (TAG, LOG_PID, LOG_USER)
#define SYS_LOG_CLOSE() 	    closelog()

#define SYS_LOG_BASE(__pri, fmt, arg...) syslog(__pri ,"[%s,%d] " fmt, __FUNCTION__, __LINE__, ##arg)

#define SLOG_EMERG(fmt, arg...) 	SYS_LOG_BASE(LOG_EMERG,fmt,##arg)	/* system is unusable */
#define SLOG_ALERT(fmt, arg...) 	SYS_LOG_BASE(LOG_ALERT,fmt,##arg)	/* action must be taken immediately */
#define SLOG_CRIT(fmt, arg...) 		SYS_LOG_BASE(LOG_CRIT,fmt,##arg)		/* critical conditions */
#define VLOGE(fmt, arg...) 		    SYS_LOG_BASE(LOG_ERR,fmt,##arg)		/* error conditions */
#define VLOGW(fmt, arg...) 	        SYS_LOG_BASE(LOG_WARNING,fmt,##arg)	/* warning conditions */
#define SLOG_NOTICE(fmt, arg...) 	SYS_LOG_BASE(LOG_NOTICE,fmt,##arg)	/* normal but significant condition */
#define VLOGI(fmt, arg...) 		    SYS_LOG_BASE(LOG_INFO,fmt,##arg)		/* informational */
#define VLOGD(fmt, arg...) 	        SYS_LOG_BASE(LOG_DEBUG,fmt,##arg)	/* debug-level messages */
#else 
#define SYS_LOG_OPEN(TAG)       ""
#define SYS_LOG_CLOSE() 	    ""

static std::string GetLocalTimeWithMs()
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    // int milli = curTime.tv_usec / 1000;

    char buffer[80] = {0};
    struct tm nowTime;
    localtime_r(&curTime.tv_sec, &nowTime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &nowTime);

    char currentTime[84] = {0};
    snprintf(currentTime, sizeof(currentTime), "%s:%06ld", buffer, curTime.tv_usec);
    return currentTime;
}

#define VLOGE(format, arg...) qDebug("[%s %s %s:%d] " format, GetLocalTimeWithMs().c_str(), __FILE__, __FUNCTION__, __LINE__, ##arg)
#define VLOGI(format, arg...) qDebug("[%s %s %s:%d] " format, GetLocalTimeWithMs().c_str(), __FILE__, __FUNCTION__, __LINE__, ##arg)
#define VLOGD(format, arg...) qDebug("[%s %s %s:%d] " format, GetLocalTimeWithMs().c_str(), __FILE__, __FUNCTION__, __LINE__, ##arg)
#define VLOGW(format, arg...) qDebug("[%s %s %s:%d] " format, GetLocalTimeWithMs().c_str(), __FILE__, __FUNCTION__, __LINE__, ##arg)
#endif

// old
#define ALOGE VLOGE
#define ALOGI VLOGI
#define ALOGD VLOGD
#define ALOGW VLOGW

#define ZeroMemory(destination, length) memset((destination), 0x0, (length))


namespace com {
namespace bicv {
namespace vehicle {

static void printhex(const std::vector<uint8_t> &msg, const char *tag) {
    uint8_t print_datas[1024] = {0};
    uint8_t* datas_p = print_datas;
    for(int i=0; i<(int)msg.size() && i<(int)(sizeof(print_datas)/sizeof(print_datas[0])/3); i++) {
        datas_p += sprintf((char*)datas_p, "%02x ", msg[i]);
    }
    VLOGD("%s: %s", tag, print_datas);
}

/* 打印16进制码流 */
static void printhex(const uint8_t *src, int len, const char *tag) {
    uint8_t print_datas[1024] = {0};
    uint8_t* datas_p = print_datas;
    for(int i=0; i<(int)len && i<(int)(sizeof(print_datas)/sizeof(print_datas[0])/3); i++) {
        datas_p += sprintf((char*)datas_p, "%02x ", src[i]);
    }
    VLOGD("%s: %s", tag, print_datas);
}

}
}
} // namespace android
#endif // BDLOG_H
