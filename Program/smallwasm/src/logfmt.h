#pragma once

// =======================
//  Build switch
// =======================
#ifdef DEBUG

extern "C" {

// -------- core --------
void js_log0(const char* fmt);
void js_log1(const char* fmt, int a);
void js_log2(const char* fmt, int a, int b);
void js_log3(const char* fmt, int a, int b, int c);
void js_log4(const char* fmt, int a, int b, int c, int d);

// -------- string --------
void js_logs1(const char* fmt, const char* s);
void js_logs2(const char* fmt, const char* s1, const char* s2);
void js_logis(const char* fmt, int a, const char* s);

// -------- levels --------
void js_debug(const char* fmt, const char* file, int line);
void js_warn (const char* fmt, const char* file, int line);
void js_error(const char* fmt, const char* file, int line);

}

// =======================
//  helpers
// =======================
#define LOG_SELECT(_0,_1,_2,_3,_4,_5,NAME,...) NAME
#define LOG_DISPATCH(...) \
  LOG_SELECT(,##__VA_ARGS__,LOG4,LOG3,LOG2,LOG1,LOG0)(__VA_ARGS__)

// =======================
//  basic log
// =======================
#define LOG0(fmt)              js_log0(fmt)
#define LOG1(fmt,a)            js_log1(fmt,a)
#define LOG2(fmt,a,b)          js_log2(fmt,a,b)
#define LOG3(fmt,a,b,c)        js_log3(fmt,a,b,c)
#define LOG4(fmt,a,b,c,d)      js_log4(fmt,a,b,c,d)

#define LOG(...)               LOG_DISPATCH(__VA_ARGS__)

// =======================
//  string helpers
// =======================
#define LOGS(fmt,s)            js_logs1(fmt,s)
#define LOGSS(fmt,s1,s2)       js_logs2(fmt,s1,s2)
#define LOGIS(fmt,a,s)         js_logis(fmt,a,s)

// =======================
//  levels (file:line)
// =======================
#define LOGD(fmt)  js_debug(fmt, __FILE__, __LINE__)
#define LOGW(fmt)  js_warn (fmt, __FILE__, __LINE__)
#define LOGE(fmt)  js_error(fmt, __FILE__, __LINE__)

#else

// =======================
//  RELEASE = stripped
// =======================
#define LOG(...)
#define LOGS(...)
#define LOGSS(...)
#define LOGIS(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)

#endif
