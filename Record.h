#ifndef RECORD_H_
#define RECORD_H_
#include <iostream>
#include <pthread.h>
#include "revel.h"
#include "FrameProcessor.h"
#include "MotionDetect.h"


using namespace std;
class Record : FrameProcessor
{
  public:
    Record(MotionDetect & md, string savePath);  
    ~Record();
    void ProcessFrame(int w, int h, char * buffer, int size);
    bool StartRecording();
    bool StopRecording();
    
  private:
    MotionDetect & _md;
    string _savePath;
    int _w;
    int _h;
    int _fps;
    int _encoderHandle;
    unsigned char * _convBuffer;
    bool _grayscale;
};

#endif
