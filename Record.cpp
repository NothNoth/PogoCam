#include "Record.h"
#include <math.h>
#include <string.h>

Record::Record(MotionDetect & md,  string savePath) :
  _md(md),
  _savePath(savePath),
  _w(640),
  _h(480),
  _fps(5),
  _encoderHandle(-1),
  _convBuffer(NULL),
  _grayscale(false)
{

}


Record::~Record()
{
  if (_encoderHandle != -1)
    StopRecording();
  if (_convBuffer)
    delete(_convBuffer);
}





void Record::ProcessFrame(int w, int h, char * buffer, int size)
{
   
  if ((_w != w) || (_h != h) || !_convBuffer)
  {
    if (_convBuffer) 
    {
      delete(_convBuffer);
      _convBuffer = NULL;
    }
    _w = w;
    _h = h;
    _convBuffer = new unsigned char[_w*_h*4];  
  }

  if (!_md.InMotion())
  {
    if (_encoderHandle != -1)
      StopRecording();
    return;
  }
  
  //printf("Record frame\n");
  if (_md.InMotion() && _encoderHandle == -1)
  {
    StartRecording();
  }
  
  Revel_VideoFrame frame;
  frame.width = _w;
  frame.height = _h;
  frame.bytesPerPixel = 4;
  frame.pixelFormat = REVEL_PF_RGBA;
  
    /*
  2 x 2 pixels V4L2_PIX_FMT_YUYV
  Y00 Cb00 Y01 Cr00 Y02 Cb02 Y03 Cr02
  Y10 Cb10 Y11 Cr10 Y12 Cb12 Y13 Cr12
  Y20 Cb20 Y21 Cr20 Y22 Cb22 Y23 Cr22
  Y30 Cb30 Y31 Cr30 Y32 Cb32 Y33 Cr32
  
  */
  
  
  int idx = 0;
  for (int i = 0; i < _w*_h*2; i+=4)
  {
    if (_grayscale) //Grayscale
    {
      _convBuffer[idx] = buffer[i];
      _convBuffer[idx+1] = buffer[i];
      _convBuffer[idx+2] = buffer[i];
      _convBuffer[idx+3] = 0;

      _convBuffer[idx+4] = buffer[i+2];
      _convBuffer[idx+5] = buffer[i+2];
      _convBuffer[idx+6] = buffer[i+2];
      _convBuffer[idx+7] = 0;
    }
    else
    {
      int Y = buffer[i]&0xFF;    //Y
      int U = buffer[i+1]&0xFF;  //U
      int Y2 = buffer[i+2]&0xFF; //Y
      int V = buffer[i+3]&0xFF;  //V
      int res = yuv2rgb(Y,U,V);
      
      memcpy(_convBuffer+idx, &res, 4);

      res = yuv2rgb(Y2,U,V);
      memcpy(_convBuffer+idx+4, &res, 4);

    }
    idx +=8;
  } 
  frame.pixels = _convBuffer;
  Revel_Error revError = Revel_EncodeFrame(_encoderHandle, &frame, &size);
  if (revError != REVEL_ERR_NONE)
  {
    printf("Revel Error while writing frame: %d\n", revError);
    return;
  }
}

bool Record::StartRecording()
{
  Revel_Params revParams;
  Revel_InitializeParams(&revParams);
  revParams.width = _w;
  revParams.height = _h;
  revParams.frameRate = (float) _fps;
  revParams.quality = 1.0f;
  revParams.codec = REVEL_CD_XVID;
  revParams.hasAudio = 0;
  Revel_Error revError = Revel_CreateEncoder(&_encoderHandle);

  time_t t;
  char timestr[32];
  time(&t);
  strftime(timestr, 32, "record_%Y-%m-%d_%H_%M_%S.avi", localtime(&t));
  
  revError = Revel_EncodeStart(_encoderHandle, (_savePath+string(timestr)).c_str(), &revParams);
  if (revError != REVEL_ERR_NONE)
  {
    printf("Revel Error while starting encoding: %d\n", revError);
    return false;
  }

  printf("Video record start\n");
  return true;
}

bool Record::StopRecording()
{
  int totalSize;
  Revel_Error revError = Revel_EncodeEnd(_encoderHandle, &totalSize);
  if (revError != REVEL_ERR_NONE)
  {
    printf("Revel Error while ending encoding: %d\n", revError);
  }
  Revel_DestroyEncoder(_encoderHandle);
  _encoderHandle = -1;
  printf("Video record stop\n");
  return true;
}

