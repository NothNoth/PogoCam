#ifndef JEXPORT_H_
#define JEXPORT_H_
#include <iostream>
#include <pthread.h>
#include "jpeglib.h"
#include "FrameProcessor.h"
#include "MotionDetect.h"


using namespace std;
class JExport : FrameProcessor
{
  public:
    JExport(MotionDetect & md);  
    ~JExport();
    void ProcessFrame(int w, int h, char * buffer, int size);
    bool StartExporting();
    bool StopExporting();
    
  private:
    MotionDetect & _md;
    int _w;
    int _h;
    int _fps;
    unsigned char * _convBuffer;
    long _lastExportedFrame;
};

#endif
