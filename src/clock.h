#ifndef _H_CLOCK_
#define _H_CLOCK_

void beginClock();
void loopClock();

// only on the 'main' screen
void setEnableClock(bool flag);
bool getEnableClock();

#endif /* _H_CLOCK_ */