/*
 * pogocam.cpp
 *
 *  Created on: 27 févr. 2011
 *      Author: benoitgirard
 */
#include <stdlib.h>
#include <iostream>
#include <signal.h>

#include "Capture.h"
#include "MotionDetect.h"
#include "Record.h"
#include "JExport.h"

using namespace std;


bool killed = false;
void  sigCb(int sig)
{
  killed = true;
  printf("Killing..\n");
  return;
}


int main (int argc, char *argv[])
{
  MotionDetect * md;
  Record * rc;
  JExport * je;

  if (argc < 3)
  {
    cout << "Usage : " << argv[0] << " <device> <save path>" << endl;
    return -1;
  }
  signal(SIGINT, sigCb);

  Capture cap(argv[1]);

  if (!cap.IsReady())
  {
    exit(-1);
  }
  md = new MotionDetect();
  rc = new Record(*md, string(argv[2]));
  je = new JExport(*md);
  cap.RegisterFrameProcessor((FrameProcessor *)md); //Motion call at each frame
  cap.RegisterFrameProcessor((FrameProcessor *)rc, 200); //Record 5fps
  cap.RegisterFrameProcessor((FrameProcessor *)je, 1000); //Export at 1 fps

  cap.StartCapturing();
  while (!killed) 
    sleep(1);
  printf("Stopping all subsystems...\n");
  cap.StopCapturing();
  delete(rc);
  delete(md);
  printf("Halted.\n");
  return 0;
}
