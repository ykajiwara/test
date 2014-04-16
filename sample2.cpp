#include <opencv2/core/core.hpp>
#include <opencv2/photo/photo.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>

cv::Mat inpaint_mask;
cv::Mat img0, img, inpainted;

cv::Point prev_pt;

void on_mouse(int event, int x, int y, int flags, void *){
  if(img.empty()){
    return;
  }

  if(event == CV_EVENT_LBUTTONUP || !(flags & CV_EVENT_FLAG_LBUTTON)){
    prev_pt = cv::Point(-1, -1); //init the start point
  }
  else if(event == CV_EVENT_LBUTTONDOWN){
    prev_pt = cv::Point(x, y); //set the start point
  }
  else if(event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON)){
      cv::Point pt(x, y);
      if(prev_pt.x < 0){
	prev_pt = pt;
      }
      
      //draw a line from the start point to the current point
      cv::line(inpaint_mask, prev_pt, pt, cv::Scalar(255), 5, 8, 0);
      cv::line(img, prev_pt, pt, cv::Scalar::all(255), 5, 8, 0);

      //set the current point to the new start point
      prev_pt = pt;

      //cv::Mat img_hdr = img
      cv::imshow("image", img);
    }
}

int main(int argc, char *argv[]){
  char *filename = (argc >= 2) ? argv[1] : (char*)"fruits.jpg";

  img0 = cv::imread(filename);
  if(img0.empty()){
    return 0;
  }

  //print hot keys

  printf("Hot keys: \n"
	 "\tESC - puit the program\n"
	 "\t i or ENTER - run inpainting algorithm\n"
	 "\t\t(before running it, paint something on the image)\n");

  cv::namedWindow("image", 1);

  img = img0.clone();
  inpainted = img0.clone();
  inpaint_mask.create(img0.size(), CV_8UC1);
 
  inpaint_mask = cv::Scalar(0);
  inpainted = cv::Scalar(0);
  cv::imshow("image", img);

  //set callback function for mouse operations
  cv::setMouseCallback("image", on_mouse, 0);

  bool loop_flag = true;
  while(loop_flag){
    fprintf(stderr, "1\n");
    int c = cv::waitKey(0);
    fprintf(stderr, "2\n");
    fprintf(stderr, "%c\n", c);
   
    switch(c){
      //fprintf(stderr, "4\n");
    case 27: 
    case 'q':
      // fprintf(stderr, "3");
      loop_flag = false;
      break;
    case 'r':
      inpaint_mask = cv::Scalar(0);
      img0.copyTo(img);
      cv::imshow("image", img);
      break;
    case 'i':
    case 10:	
      cv::namedWindow("inpainted image", 1);
      cv::inpaint(img, inpaint_mask, inpainted, 3.0, cv::INPAINT_TELEA);
      cv::imshow("inpainted image", inpainted);
      break;
    }
  }
  return 0;
}
