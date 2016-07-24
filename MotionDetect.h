




#ifndef MOTIONDETECT_H_
#define MOTIONDETECT_H_
#include "FrameProcessor.h"
#include <sys/times.h>


class MotionDetect : FrameProcessor
{
  public:
    MotionDetect();
    ~MotionDetect();
    void ProcessFrame(int w, int h, char * buffer, int size);
    bool InMotion() {return _inMotion;}
    long InMotionForMs() {return times(NULL)*10 - _inMotionSince;}
  private:
    int _w;
    int _h;
    char *  _previousBuffer; //previous frame for comparison
    int     _previousSize; //previous frame size
    long    _previousT; //Time since last frame
    long    _frameDelay; //frame analyse delay
    bool    _inMotion; //in motion flag
    long    _inMotionSince; //in motion timer
    long    _lastMotionT; //times of last moving frame
    long    _stillTimeout; //motion timeout
    unsigned char _imprecisionFilter;
    float    _movingPixelMaxRate;
    
    char *  _workBuffer;
    
    
    int HPFilter();
};

#endif

