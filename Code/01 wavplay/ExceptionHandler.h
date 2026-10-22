#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include <map>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <cxxabi.h>
#include <math.h>

#define FORMAT_TIME_MAX_LEN         64 // 格式化时间最长长度
#define APP_NAME_MAX_LEN            128 // APP名称最大长度
#define APP_MAX_LEN                 1024 // 定义APP处理最长长度
#define RATIO_1000                  1000 // 1000的进率
#define FOLDER_MAX_LEN              512 // 文件夹长度最大值

/*************************************************************
 * 功能：获取应用运行目录，不包含应用名称
 * 输入参数：
 *		pPath：存放应用运行目录的内存空间
 *		uiLen：存放应用运行目录的内存空间长度，字节
 * 输出参数：
 *		pPath：存放应用运行目录的内存空间
 * ***********************************************************/
void getAppRunPath(char* pPath, unsigned int uiLen)
{
    if (!pPath)
    {
        return;
    }
 
    memset(pPath, 0, uiLen);
    std::string strPath = "";
	
    char szAbsPath[APP_MAX_LEN] = { 0 };
    ssize_t iPathLen = readlink("/proc/self/exe", szAbsPath, APP_MAX_LEN); // 获取可执行程序的绝对路径
    if (0 < iPathLen && iPathLen < APP_MAX_LEN)
    {
        std::string strFullPath(szAbsPath);
        size_t iPos = strFullPath.find_last_of('/');
        if (iPos != std::string::npos)
        {
            strPath = strFullPath.substr(0, iPos); // return the directory without the file name
        }
    }
 
    memcpy(pPath, strPath.data(), (uiLen < strPath.length() ? uiLen : strPath.length()));
	
    return;
}

/*************************************************************
 * 功能：创建文件目录
 * 输入参数：
 *		pFolderPath：文件目录
 * 返回值：
 *		bool：创建结果，true -- 成功，false -- 失败
 * ***********************************************************/
bool createDirectory(const char* pFolderPath)
{
    if (!pFolderPath)
    {
        return false;
    }
 
    bool bRet = true;
 
    // 创建目录，函数方式
    if (0 != access(pFolderPath, 0))
    {
        // 返回0表示创建成功，-1表示失败
        if (0 > mkdir(pFolderPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO))
        {
            bRet = false;
        }
    }
 
    return bRet;
}

/*************************************************************
 * 功能：获取应用名称
 * 输入参数：
 *		pName：存放应用名称的内存空间
 *		uiLen：存放应用名称的内存空间长度，字节
 * 输出参数：
 *		pName：存放应用名称的内存空间
 * ***********************************************************/
void getAppName(char* pName, unsigned int uiLen)
{
    if (!pName)
    {
        return;
    }
 
    memset(pName, 0, uiLen);
    std::string strAppName = "unknown";
 
    char szAbsPath[APP_MAX_LEN] = { 0 };
    ssize_t iPathLen = readlink("/proc/self/exe", szAbsPath, APP_MAX_LEN); // 获取可执行程序的绝对路径
    if (0 < iPathLen && iPathLen < APP_MAX_LEN)
    {
        std::string strFullPath(szAbsPath);
        size_t iPos = strFullPath.find_last_of('/');
        if (iPos != std::string::npos)
        {
            strAppName = strFullPath.substr(iPos + 1);
        }
    }
 
    memcpy(pName, strAppName.data(), (uiLen < strAppName.length() ? uiLen : strAppName.length()));
 
    return;
}

/*************************************************************
 * 功能：获取格式化时间，格式为yyyyMMdd_HHmmss.zzz
 * 返回值：
 *		std::string：格式化时间
 * ***********************************************************/
static std::string getFormatTime()
{
    time_t stTimeNow;
    time(&stTimeNow);
    char szTmp[FORMAT_TIME_MAX_LEN] = { 0 };
 
    strftime(szTmp, sizeof(szTmp), "%Y%m%d_%H%M%S", localtime(&stTimeNow));
 
    int iMillsec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    iMillsec = tv.tv_usec / RATIO_1000;
 
    char szTime[FORMAT_TIME_MAX_LEN] = { 0 };
    sprintf(szTime, "%s_%03d", szTmp, iMillsec);
    std::string strTime(szTime);
	
    return strTime;
}

/*************************************************************
 * 功能：注册异常处理，linux系统
 * 输入参数：
 *		signum：信号值
 *      info：信息
 *      ctx：上下文
 * ***********************************************************/
static void signal_exception_handler(int signum)
{
    signal(signum, SIG_DFL); // 还原默认的信号处理
 
    // 写入异常自定义头部信息
    std::ostringstream oss;
    oss << "Stack Trace: " << std::endl;
    oss << "Signal (" << signum << "), " << strsignal(signum) << std::endl;
 
    // 读取堆栈信息
    const size_t sFrameSize = 32; // 堆栈大小
    void* pStackBuffer[sFrameSize] = { 0 };
    int iSize = backtrace(pStackBuffer, sFrameSize);
    //backtrace_symbols_fd(pStackBuffer, iSize, STDOUT_FILENO);
    char** symbols = backtrace_symbols(pStackBuffer, iSize);
    if (!symbols)
    {
        return;
    }
 
    // 解析函数符号信息
    for (int i = 0; i < iSize; ++i)
    {
        char* mangleName = 0;
        char* offsetBegin = 0;
        char* offsetEnd = 0;
 
        for (char* p = symbols[i]; *p; ++p)
        {
            if ('(' == *p)
            {
                mangleName = p;
            }
            else if ('+' == *p)
            {
                offsetBegin = p;
            }
            else if (')' == *p)
            {
                offsetEnd = p;
                break;
            }
        }
 
        if (mangleName && offsetBegin && offsetEnd && (mangleName < offsetBegin))
        {
            *mangleName++ = '\0';
            *offsetBegin++ = '\0';
            *offsetEnd++ = '\0';
 
            int status = -4;
			// 获取混淆解析后可读的函数符号信息
            char* retRealName = abi::__cxa_demangle(mangleName, nullptr, nullptr, &status); // 解析符号，得到真正的函数名
            if (0 == status && retRealName)
            {
                oss << symbols[i] << ": " << retRealName << " + ";
            }
            else
            {
                oss << symbols[i] << ": " << mangleName << " + ";
            }
 
            oss << offsetBegin << offsetEnd << std::endl;
 
            if (retRealName)
            {
                free(retRealName);
                retRealName = nullptr;
            }
        }
        else
        {
            oss << "[audio " << i << "] status(-4), " << symbols[i] << std::endl;
        }
    }
    free(symbols);
 
    oss << std::endl;
 
    // 异常写入文件
    char szAppName[APP_NAME_MAX_LEN] = { 0 };
    struct timeval curTime;
    gettimeofday(&curTime, NULL);

    // char buffer[80] = {0};
    struct tm nowTime;
    localtime_r(&curTime.tv_sec, &nowTime);
    strftime(szAppName, sizeof(szAppName), "/data/log/crash_audio_%Y%m%d%H%M%S.log", &nowTime);
    std::ofstream fout(szAppName, std::ios_base::app | std::ios_base::out);
    do
    {
        if (!(fout.is_open()))
        {
            break;
        }
 
        fout << oss.str().data();
        fout.close();
 
    } while (0);
 
    exit(1);
 
    return;
}

/*************************************************************
 * 功能：注册异常处理，linux系统
 * ***********************************************************/
static void registerExceptionHandler()
{
    struct sigaction sigact;
    sigact.sa_handler = signal_exception_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(SIGINT, &sigact, NULL);
    //sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);
    sigaction(SIGABRT, &sigact, NULL);

    return;
}