/*
 * Capture.h
 *
 *  Created on: 27 févr. 2011
 *      Author: benoitgirard
 */

#ifndef CAPTURE_H_
#define CAPTURE_H_
#include <iostream>
#include <pthread.h>
#include <vector>
#include "revel.h"
#include "FrameProcessor.h"

using namespace std;


typedef struct
{
  void *                  start;
  size_t                  length;
} tBuffer;

typedef struct
{
  FrameProcessor * processor;
  int frameInterval;
  long lastCall;
} tFrameProcessorEntry;

class Capture
{
  public:
    Capture(string device);
    virtual ~Capture();
    bool IsReady() { return _ready; }
    void RegisterFrameProcessor(FrameProcessor * fp, int interval = -1);
    bool StartCapturing();
    bool StopCapturing();
    void CaptureLoop();
    bool StartRecording() {_record = true; return true;}
    bool StopRecording() {_record = false; return true;}
    
  private:
    bool                _ready;
    string              _device;
    int                 _deviceHandler;
    tBuffer *           _buffers;
    unsigned int        _nbBuffers;
    unsigned int        _w;
    unsigned int        _h;
    pthread_t           _captureThread;
    bool                _captureKill;
    bool                _record;
    vector<tFrameProcessorEntry> _processors;
    
    int xioctl (int fd, int request, void * arg);
    bool InitMmap(void);
    bool UnInitMmap(void);
    int ReadFrame();
};

#endif /* CAPTURE_H_ */
