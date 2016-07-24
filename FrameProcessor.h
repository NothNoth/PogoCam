#ifndef FRAMEPROCESSOR_H_
#define FRAMEPROCESSOR_H_

class FrameProcessor
{
  public:
    FrameProcessor() {}
    ~FrameProcessor() {}
    virtual void ProcessFrame(int w, int h, char * buffer, int size) = 0;  
    int yuv2rgb(int y, int u, int v)
    {
       unsigned int pixel32;
       unsigned char *pixel = (unsigned char *)&pixel32;
       int r, g, b;
       r = (1.164 * (y - 16)) + (2.018 * (v - 128));
       g = (1.164 * (y - 16)) - (0.813 * (u - 128)) - (0.391 * (v - 128));
       b = (1.164 * (y - 16)) + (1.596 * (u - 128));

       // Even with proper conversion, some values still need clipping.
       if (r > 255) r = 255;
       if (g > 255) g = 255;
       if (b > 255) b = 255;
       if (r < 0) r = 0;
       if (g < 0) g = 0;
       if (b < 0) b = 0;

       // Values only go from 0-220..  Why?
       pixel[0] = r * 220 / 256;
       pixel[1] = g * 220 / 256;
       pixel[2] = b * 220 / 256;
       pixel[3] = 0;

       return pixel32;
    }


};

#endif

