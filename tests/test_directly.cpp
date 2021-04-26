#include "simple_logger.h"

int
main()
{
	E_loggerInst;
	E_loggerInst.ConfigStd(E_DEBUG);
	E_loggerInst.ConfigFile(E_DEBUG, "./Logs", size_t{1024} * 1024, 1);

	E_Debug(nullptr, "aaaaaaaaaaaaaaaaaaa");
	const auto _trace = "thread_id_or_duty_uid";
//	const auto _trace = nullptr;
	E_Info(_trace, "bbbbbbbbbbbbbbbb");
	E_Warn(_trace, "cccccccccccccccccccc");
	E_Error(_trace, "dddddddddddddddddddd");
	E_DiyDebug("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	E_Debug(_trace, "aaaaaaaaaaaaaaaaaaa");
	E_Info(_trace, "bbbbbbbbbbbbbbbb");
	E_Warn(_trace, "cccccccccccccccccccc");
	E_Error(_trace, "dddddddddddddddddddd");
	E_DiyInfo("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	E_Debug(_trace, "aaaaaaaaaaaaaaaaaaa");
	E_Info(_trace, "bbbbbbbbbbbbbbbb");
	E_Warn(_trace, "cccccccccccccccccccc");
	E_Error(_trace, "dddddddddddddddddddd");
	E_DiyWarn("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	E_Debug(_trace, "aaaaaaaaaaaaaaaaaaa");
	E_Info(_trace, "bbbbbbbbbbbbbbbb");
	E_Warn(_trace, "cccccccccccccccccccc");
	E_Error(_trace, "dddddddddddddddddddd");
	E_DiyError("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
}
