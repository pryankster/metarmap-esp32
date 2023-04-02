#ifndef _H_MSGBOX_
#define _H_MSGBOX_

void showMessagef(int timeout_ms, const char *fmt, ... );
void vshowMessagef(int timeout_ms, const char *fmt, va_list arg );
void showMessage(int timeout_ms, const char *msg);
void dismissMsg();

#endif // _H_MSGBOX_