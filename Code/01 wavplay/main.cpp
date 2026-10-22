#include <QCoreApplication>
#include "libvehicle_global.h"
#include "AudioService.h"
#include "breakpad/handler/QBreakpadHandler.h"

using namespace com::bicv::vehicle;

int main(int argc, char *argv[])
{
	SYS_LOG_OPEN("AUDIO_SERVICE");
    QCoreApplication a(argc, argv);

	VLOGD("@audio start audio service pid: %lld", QCoreApplication::applicationPid());

	std::shared_ptr<QBreakpadHandler> audioBreakpad = std::make_shared<QBreakpadHandler>();
    if(audioBreakpad){
        audioBreakpad->setDumpPath("/data/log");
        qDebug()<<"[AudioService] Breakpad initialized version: "<<audioBreakpad->version();
    }
	
	nice(-5);
	
	VerifyVersionProtobufLibrary();
		
    AudioService audioService;
    (void)audioService;

    int ret = a.exec();

	ShutdownProtobufLibrary();
	SYS_LOG_CLOSE();
	return ret;
}
