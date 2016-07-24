#include "JExport.h"
#include <math.h>
#include <string.h>
#include <malloc.h>
JExport::JExport(MotionDetect & md) :
  _md(md),
  _w(640),
  _h(480),
  _convBuffer(NULL),
  _lastExportedFrame(-1)
{

}


JExport::~JExport()
{
  StopExporting();
  if (_convBuffer)
    delete(_convBuffer);
}


#define  EXPORT_DELAY 5000 //start after 5s of motion
#define FRAME_INTERVAL 5000 //New frame every 5s

void JExport::ProcessFrame(int w, int h, char * buffer, int size)
{
  jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  
  if (!_md.InMotion() || //Not moving 
      (_md.InMotion() && (_md.InMotionForMs() < EXPORT_DELAY)) || //Too Early for export
      (_md.InMotion() && (times(NULL)*10-_lastExportedFrame < FRAME_INTERVAL))) //new frame ?
  {
    return;
  }

// Convert frame to RGB
  char * convbuffer = (char*) malloc(w*h*3);
  int idx = 0;
  for (int i = 0; i < _w*_h*2; i+=4)
  {
    int Y = buffer[i]&0xFF;    //Y
    int U = buffer[i+1]&0xFF;  //U
    int Y2 = buffer[i+2]&0xFF; //Y
    int V = buffer[i+3]&0xFF;  //V
    int res = yuv2rgb(Y,U,V);
    
    memcpy(convbuffer+idx, &res, 3);

    res = yuv2rgb(Y2,U,V);
    memcpy(convbuffer+idx+3, &res, 3);

    idx +=6;
  } 
  
  time_t t;
  char timestr[32];
  time(&t);
  strftime(timestr, 32, "snap_%Y-%m-%d_%H_%M_%S.jpg", localtime(&t));
  FILE * f = fopen(timestr, "wb");
  if (!f)
    return;
  
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, f);
  cinfo.image_width = w; 	/* image width and height, in pixels */
  cinfo.image_height = h;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, 100, TRUE /* limit to baseline-JPEG values */);
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = w * 3;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height)
  {
    row_pointer[0] = (JSAMPLE *) &convbuffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  jpeg_finish_compress(&cinfo);
  fclose(f);
  jpeg_destroy_compress(&cinfo);
  printf("Jpeg exported.\n");
  _lastExportedFrame = times(NULL) * 10;
}

bool JExport::StartExporting()
{
  printf("JExport start\n");
  return true;
}

bool JExport::StopExporting()
{

  printf("JExport stop\n");
  return true;
}

