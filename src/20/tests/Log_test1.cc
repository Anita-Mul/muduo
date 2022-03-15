#include <muduo/base/Logging.h>
#include <errno.h>

using namespace muduo;

int main()
{
	LOG_TRACE<<"trace ...";
	LOG_DEBUG<<"debug ...";
	LOG_INFO<<"info ...";  		// 默认日志级别是 INFO
	LOG_WARN<<"warn ...";
	LOG_ERROR<<"error ...";		// 应用级别上的错误
	// LOG_FATAL<<"fatal ...";
	errno = 13;
	LOG_SYSERR<<"syserr ...";
	LOG_SYSFATAL<<"sysfatal ...";
	return 0;
}

/*
20220314 09:17:10.656961Z 20630 TRACE main trace ... - Log_test1.cc:8
20220314 09:17:10.657139Z 20630 DEBUG main debug ... - Log_test1.cc:9
20220314 09:17:10.657147Z 20630 INFO  info ... - Log_test1.cc:10
20220314 09:17:10.657153Z 20630 WARN  warn ... - Log_test1.cc:11
20220314 09:17:10.657156Z 20630 ERROR error ... - Log_test1.cc:12
20220314 09:17:10.657161Z 20630 ERROR Permission denied (errno=13) syserr ... - Log_test1.cc:15
20220314 09:17:10.657181Z 20630 FATAL Permission denied (errno=13) sysfatal ... - Log_test1.cc:16
已放弃(吐核)
*/