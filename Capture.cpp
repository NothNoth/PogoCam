/*
 * Capture.cpp
 *
 *  Created on: 27 févr. 2011
 *      Author: benoitgirard
 */

#include "Capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))



Capture::Capture(string device) :
  _ready(false),
  _device(device),
  _deviceHandler(-1),
  _buffers(NULL),
  _nbBuffers(0),
  _record(false)
{

  struct stat st;

  //Open Device
  if ((stat (_device.c_str(), &st) == -1) || !S_ISCHR (st.st_mode))
  {
    cout << "Invalid device :" << _device << endl;
    return;
  }

  _deviceHandler = open (_device.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

  if (_deviceHandler == -1)
  {
    cout << "Can't open device :" << _device << endl;
    return;
  }

  //Init camera
  struct v4l2_capability capability;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format frameFormat;

  if (xioctl (_deviceHandler, VIDIOC_QUERYCAP, &capability) == -1)
  {
    if (EINVAL == errno)
    {
      cout << "Device is not v4l compatible" << endl;
      return;
    }
    else
    {
      cout << "VIDIOC_QUERYCAP error" << endl;
      return;
    }
  }

  //Check capture
  if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
      cout << "Device does not support video capture" << endl;
      return;
  }
  //Check Streaming
  if (!(capability.capabilities & V4L2_CAP_STREAMING))
  {
      cout << "Device does not support video streaming" << endl;
      return;
  }

  cout << "Using camera " << capability.card << " (Driver : " << capability.driver << ") - " << capability.bus_info << endl;

  /* Select video input, video standard and tune here. */
  CLEAR (cropcap);
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (!xioctl (_deviceHandler, VIDIOC_CROPCAP, &cropcap))
  {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (xioctl (_deviceHandler, VIDIOC_S_CROP, &crop) == -1)
    {
      switch (errno)
      {
        case EINVAL:
          /* Cropping not supported. */
        break;
        default:
          /* Errors ignored. */
        break;
      }
    }
  }
  else
  {
    /* Errors ignored. */
  }


  CLEAR (frameFormat);
  frameFormat.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  frameFormat.fmt.pix.width       = 640;
  frameFormat.fmt.pix.height      = 480;
  frameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  frameFormat.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  frameFormat.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB ;
  if (xioctl (_deviceHandler, VIDIOC_S_FMT, &frameFormat) == -1)
  {
      cout << "Can't get frame format" << endl;
      return; //FIXME : marche qd meme ...
  }
  	printf("pixfmt: %c%c%c%c %dx%d\n",
            (frameFormat.fmt.pix.pixelformat & 0xff),
            (frameFormat.fmt.pix.pixelformat >> 8) & 0xff,
            (frameFormat.fmt.pix.pixelformat >> 16) & 0xff,
            (frameFormat.fmt.pix.pixelformat >> 24) & 0xff,
            frameFormat.fmt.pix.width, 
            frameFormat.fmt.pix.height);
		
  _w = frameFormat.fmt.pix.width;
  _h = frameFormat.fmt.pix.height;

  if (!InitMmap ())
    return;


  _ready = true;
}

Capture::~Capture()
{
  if (_ready)
  {
    close(_deviceHandler);
    UnInitMmap();
  }
}


int Capture::xioctl (int fd, int request, void * arg)
{
  int r;
  do r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);
  return r;
}



bool Capture::InitMmap(void)
{
  struct v4l2_requestbuffers req;

  CLEAR (req);

  req.count               = 4;
  req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory              = V4L2_MEMORY_MMAP;

  if (xioctl (_deviceHandler, VIDIOC_REQBUFS, &req) == -1)
  {
    if (EINVAL == errno)
    {
      cout << "mmap not supported" << endl;
      return false;
    }
    else
    {
      cout << "VIDIOC_REQBUFS failed" << endl;
      return false;
    }
  }

  if (req.count < 2)
  {
    cout << "Not enough memory" << endl;
    return false;
  }

  _buffers = (tBuffer*) calloc (req.count, sizeof (tBuffer));
  if (!_buffers)
  {
    cout << "Not enough memory" << endl;
    return false;
  }

  for (_nbBuffers = 0; _nbBuffers < req.count; ++_nbBuffers)
  {
    struct v4l2_buffer buf;

    CLEAR (buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = _nbBuffers;

    if (-1 == xioctl (_deviceHandler, VIDIOC_QUERYBUF, &buf))
    {
      cout << "VIDIOC_QUERYBUF failed" << endl;
      return false;
    }

    _buffers[_nbBuffers].length = buf.length;
    _buffers[_nbBuffers].start =
    mmap (NULL /* start anywhere */,
          buf.length,
          PROT_READ | PROT_WRITE /* required */,
          MAP_SHARED /* recommended */,
          _deviceHandler, buf.m.offset);

    if (MAP_FAILED == _buffers[_nbBuffers].start)
    {
      cout << "Mmap failed." << endl;
      return false;
    }
  }

  return true;
}

bool Capture::UnInitMmap(void)
{
  unsigned int i;

  for (i = 0; i < _nbBuffers; ++i)
  {
    if (munmap (_buffers[i].start, _buffers[i].length) == -1)
    {
      cout << "Un mmap failed" << endl;
      return false;
    }
  }
  free (_buffers);

  return true;
}

void * threadCallback(void * obj)
{
  Capture * cap = (Capture *) obj;
  cap->CaptureLoop();
  return NULL;
}


void Capture::CaptureLoop()
{
  bool fatalError = false;
  _captureKill = false;

  unsigned int i;
  enum v4l2_buf_type type;

  for (i = 0; i < _nbBuffers; ++i)
  {
    struct v4l2_buffer buf;
    CLEAR (buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = i;

    if (-1 == xioctl (_deviceHandler, VIDIOC_QBUF, &buf))
    {
      cout << "VIDIOC_QBUF error" << endl;
      return;
    }
  }
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl (_deviceHandler, VIDIOC_STREAMON, &type))
  {
    cout << "VIDIOC_STREAMON error" << endl;
    return;
  }

  while (!_captureKill && !fatalError)
  {
    for (;;)
    {
      fd_set fds;
      struct timeval tv;
      int r;
      FD_ZERO (&fds);
      FD_SET (_deviceHandler, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select (_deviceHandler + 1, &fds, NULL, NULL, &tv);

      if (r == -1)
      {
        if (EINTR == errno)
          continue;
        printf("select failed !!\n");
        fatalError = true;
        break;
      }
      else if (r == 0)
      {
        printf("select timeout\n");
        fatalError = true;
        break;
      }
      else
      {
        int ret = ReadFrame();
        if (ret == -1)
        {
          fatalError = true;
          break;
        }
        else if (ret)
          break;
      }
      if (_captureKill)
        break;
    }
  }

  if (fatalError)
    cout << "Fatal Error happened, killing capture thread." << endl;
  _captureKill = true;

  if (xioctl (_deviceHandler, VIDIOC_STREAMOFF, &type) == -1)
    cout << " Error VIDIOC_STREAMOFF" << endl;
  cout << "Capture thread halted" << endl;
}

int Capture::ReadFrame()
{
  struct v4l2_buffer buf;
  CLEAR (buf);

  buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (xioctl (_deviceHandler, VIDIOC_DQBUF, &buf) == -1)
  {
   cout << "Can't get frame" << endl;

   switch (errno)
   {
     case EAGAIN:
       return 0;
     case EIO:
     default:
     {
       cout << "VIDIOC_QBUF error" << endl;
       return -1;
     }
   }
  }
  else if (buf.index >= _nbBuffers)
  {
   cout << "Too many buffers" << endl;
   return -1;
  }
  else
  {
    for (unsigned int i = 0; i < _processors.size(); i++)
    {
      if ((_processors[i].frameInterval == -1) || 
          ((times(NULL) - _processors[i].lastCall)*10> _processors[i].frameInterval))
      {
        _processors[i].processor->ProcessFrame( _w, _h, (char *) 
                                      _buffers[buf.index].start, 
                                      _buffers[buf.index].length);
       _processors[i].lastCall = times(NULL);
      }
    }
    if (xioctl (_deviceHandler, VIDIOC_QBUF, &buf) == -1)
    {
      cout << "Can't release buffer !"<< endl;
      return -1;
    }
  }

  return 0;
}

bool Capture::StartCapturing()
{
  pthread_create( &_captureThread, NULL, threadCallback, (void*) this);
  return true;
}

bool Capture::StopCapturing()
{
  _captureKill = true;
  pthread_join(_captureThread, NULL);
  return true;
}

void Capture::RegisterFrameProcessor(FrameProcessor * fp, int interval)
{
  tFrameProcessorEntry e;
  e.processor = fp;
  e.frameInterval = interval;
  e.lastCall = -1;
  _processors.push_back(e);
}
