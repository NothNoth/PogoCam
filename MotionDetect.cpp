

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include <math.h>
#include "MotionDetect.h"
using namespace std;
//#define DEBUG
MotionDetect::MotionDetect() :
  _w(0),
  _h(0),
  _previousBuffer(NULL),
  _previousSize(0),
  _previousT(0),
  _frameDelay(500),
  _inMotion(false),
  _lastMotionT(0),
  _stillTimeout(10000),
  _imprecisionFilter(0x12),
  _movingPixelMaxRate(5.0),
  _workBuffer(NULL)
{
  _previousT = times(NULL)*10;
}

MotionDetect::~MotionDetect()
{

}

#define LUMFIX_MAGIC_PIXEL 0x07
void MotionDetect::ProcessFrame(int w, int h, char * buffer, int size)
{
  unsigned int luminosity = 0;
  bool skipDetection = false;

  if  (times(NULL)*10 - _previousT < _frameDelay)
    return;

  
  //first call or frame size changed
  if (!_workBuffer || !_previousBuffer || (_w != w) || (_h != h))
  {
    _w = w;
    _h = h;
    if (_workBuffer)
      delete(_workBuffer);
    else
      skipDetection = true;
    if (_previousBuffer)
      delete(_previousBuffer); 
    _previousBuffer = new char[_w * _h];
    memset(_previousBuffer, 0x00, _w*_h);
    _workBuffer = new char[_w * _h];
    memset(_workBuffer, 0x00, _w*_h);
  }
  
  //compute mean luminosity
  for (int i = 0; i < _w * _h * 2; i+=2)
    luminosity += (unsigned char) buffer[i];
  char lumCorrection = (char) floor(luminosity/((float) _w*_h)) - 0x80;
  // cout << "MeanLum : " << (int)(floor(luminosity/((float) _w*_h))) << "LumFIX : " << ((int)lumCorrection) << endl;
  
  //write current buffer to work buffer
  int idx = 0;
  for (int i = 0; i < _w * _h * 2; i+=2,idx++)
  {
    //FIXME: lumCorrection fails !
    int tmp = buffer[i] + 0; //lumCorrection; //apply lum fix
    //write to buffer and set ignored  pixels
    _workBuffer[idx] = ((tmp < 0)?LUMFIX_MAGIC_PIXEL:(tmp>0xFF?LUMFIX_MAGIC_PIXEL:tmp))&0xFF;
  }  
  if (skipDetection)
  {
    _previousT = times(NULL) * 10;
    return;
  }
  
  //diff _workBuffer and _previousBuffer
  int total = 0;
  int nbIgnored = 0;
  for (int i = 0; i < _w * _h; i++)
  {
    unsigned char d = abs(_workBuffer[i] - _previousBuffer[i])&0xFF;

    if (d < _imprecisionFilter)  //Sensor flickering filter
      d = 0x00;
    else  if ((_workBuffer[i] == LUMFIX_MAGIC_PIXEL) ||  //Ignored pixel (lum correction)
              (_previousBuffer[i] == LUMFIX_MAGIC_PIXEL)) //Ignored pixel (lum correction)
    {
      nbIgnored ++;
      d = 0x00;
    }
    _workBuffer[i] = d;
  }
#ifdef DEBUG
  printf("Ignored : %.2f\n", nbIgnored*100.0/((float)_w * _h));
#endif  
  //Done : save current buffer to previous
  idx = 0;
  for (int i = 0; i < _w * _h * 2; i+=2,idx++)
    _previousBuffer[idx] = buffer[i];

  //Filter work buffer (single pixels)
  total = HPFilter();
#ifdef DEBUG
  printf("Total pixels : %d/%d -- ratio : %.1f\n", total, _w*_h, ((float) total * 100.0 / (float)(_w*_h))); 
#endif

  /*
  2 x 2 pixels V4L2_PIX_FMT_YUYV
  Y00 Cb00 Y01 Cr00 Y02 Cb02 Y03 Cr02
  Y10 Cb10 Y11 Cr10 Y12 Cb12 Y13 Cr12
  Y20 Cb20 Y21 Cr20 Y22 Cb22 Y23 Cr22
  Y30 Cb30 Y31 Cr30 Y32 Cb32 Y33 Cr32
  
  */
printf("moving poxels : %f >? %f\n", ((float) total * 100.0 / (float)(_w*_h)), _movingPixelMaxRate);
  //Decision Trigger
  if (((float) total * 100.0 / (float)(_w*_h)) > _movingPixelMaxRate)
  {
    if (!_inMotion) _inMotionSince = times(NULL)*10; //Set inMotion start time
    _inMotion = true;
    _lastMotionT = times(NULL)*10;
  }
  else if (_inMotion) //in motion but actually still
  {
    if (times(NULL)*10 - _lastMotionT > _stillTimeout)
      _inMotion = false;
  }


#ifdef DEBUG
  for (int i = 0; i < _w * _h; i++)
  {
    buffer[i*2] = _workBuffer[i];
  }
#endif
  _previousT = times(NULL) * 10;
}


int MotionDetect::HPFilter()
{
  int movingCount = 0;
  for (int i = 1; i < _w - 1; i++)
  {
    for (int j = 1; j < _h - 1; j++)
    {
      if (_workBuffer[j * _w + i]) //Pixel in motion
      {
        unsigned a,b,c,d, e, f, g, h;
        
        a = _workBuffer[j * _w + i - 1]; //left
        b =  _workBuffer[j * _w + i + 1]; //right
        c =  _workBuffer[(j-1) * _w + i]; //top
        d =  _workBuffer[(j+1) * _w + i]; //bottom
        
        e =  _workBuffer[(j-1) * _w + i - 1]; //top left
        f =  _workBuffer[(j-1) * _w + i] + 1; //top right
        g =  _workBuffer[(j+1) * _w + i - 1]; //bottom left
        h =  _workBuffer[(j+1) * _w + i] + 1; //bottom right
        
                
        if (a + b + c + d + e + f + g + h < 6) //pixel is alone in the dark, ignore it
          _workBuffer[j * _w + i] = 0x00;
        else
          movingCount ++;
      }
    }
  }  
  return movingCount;
}

    
