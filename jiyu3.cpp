#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include<opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cv.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fcntl.h> 
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <omp.h>
    	
int exit_sign = 0;
int s_flag = 0;
int sound(void);
 
int main (int argc, char **argv)
{ 
  cv::VideoCapture cap;
  cap.open(0);
  
  if (!cap.isOpened ())
  {
    std::cout << "Couldn't load the video." << std::endl;
    return -1;
  }
       
  const int win_size = 15;
  const int maxCorners = 20;
  const double qualityLevel = 0.05;
  const double minDistance = 5.0;
  const int cycle = 33;                     // For cv::waitKey
  int j = 3;
  int dist[20];

  // First frame.
  cv::Mat prev_frame;
  cap >> prev_frame;
  cv::Size frame_size = prev_frame.size ();
  cvWaitKey (cycle);
#pragma omp parallel sections
  {  
  // Process with video.
#pragma omp section
  {
  while (1)
  {
    // Get each frame.
    cv::Mat curr_frame;
    cap >> curr_frame;
 
    // Initialize destination image.
    cv::Mat dst_img = cv::Mat::zeros(frame_size, CV_8UC1);


    if(j == 3){
      j = 0;
    cv::Mat prev_frame_gray;
    cv::Mat curr_frame_gray;
    cv::cvtColor (curr_frame, curr_frame_gray, CV_BGR2GRAY);
    cv::cvtColor (prev_frame, prev_frame_gray, CV_BGR2GRAY);
 
  
    // Get corners from frames.
     std::vector<cv::Point2f> prev_frame_corners;
    std::vector<cv::Point2f> curr_frame_corners;
    curr_frame_corners.reserve (maxCorners);
    prev_frame_corners.reserve (maxCorners);
    cv::goodFeaturesToTrack (prev_frame_gray, prev_frame_corners,
        maxCorners, qualityLevel, minDistance, cv::Mat());
    cv::goodFeaturesToTrack (curr_frame_gray, curr_frame_corners,
        maxCorners, qualityLevel, minDistance, cv::Mat());
    cv::cornerSubPix (prev_frame_gray, prev_frame_corners, cv::Size (win_size, win_size),
        cv::Size (-1, -1), cv::TermCriteria (CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));
    cv::cornerSubPix (curr_frame_gray, curr_frame_corners, cv::Size (win_size, win_size),
        cv::Size (-1, -1), cv::TermCriteria (CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));
 
    // Calculate optical flow.
    std::vector<uchar> features_found;
    features_found.reserve (maxCorners);
    std::vector<float> feature_errors;
    feature_errors.reserve (maxCorners);
     
    cv::calcOpticalFlowPyrLK (prev_frame_gray, curr_frame_gray, prev_frame_corners,
        curr_frame_corners, features_found, feature_errors,
        cv::Size (win_size, win_size), 5,
        cvTermCriteria (CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3), 0);
     
    // Draw optical flow.
    for (unsigned int i = 0; i < features_found.size (); i++)
    {
    cv::Point p0 (ceil (prev_frame_corners[i].x), ceil (prev_frame_corners[i].y));
    cv::Point p1 (ceil (curr_frame_corners[i].x), ceil (curr_frame_corners[i].y));
    cv::line (dst_img, p0, p1, CV_RGB (20, 0, 200), 2);
    }

    //show optical flow
    for (unsigned int i = 0; i < features_found.size (); i++)
    {
      dist[i] =  (prev_frame_corners[i].x -  curr_frame_corners[i].x) *  (prev_frame_corners[i].x -  curr_frame_corners[i].x);
	+ (prev_frame_corners[i].y -  curr_frame_corners[i].y) *  (prev_frame_corners[i].y -  curr_frame_corners[i].y);

	if(dist[i] > 300){
	  fprintf(stderr, "motion captured%d\n", i);
	  if(s_flag == 0){
	    s_flag = 1;}
	  else if(s_flag == 1 || s_flag == 2){
	    exit_sign = 1;
	    s_flag = 2;}
	  break;}
	/*fprintf(stderr, "x_prev[%d] = %f, y_prev[%d] = %f\n", 
	      i, prev_frame_corners[i].x, i, prev_frame_corners[i].y);
      fprintf(stderr, "x_curr[%d] = %f, y_curr[%d] = %f\n", 
	      i,  curr_frame_corners[i].x, i,  curr_frame_corners[i].y);
	*/
	      }
    }
    
    
    cv::namedWindow ("Current", 0);
    cv::imshow ("Current", curr_frame);

   
    // Update previous frame.
    prev_frame = curr_frame.clone ();
     
    int key = cv::waitKey(cycle);
     if(key == 'q' || key == 'Q')
    break;

     j++;

  }
  }
#pragma omp section
  {
    while(1){
      if(s_flag == 1){
	fprintf(stderr, "ok1\n");
	sound();
      }
    }
  }
#pragma omp section
  {
    while(1){
      if(s_flag == 2){
	fprintf(stderr, "ok2\n");
	cvWaitKey (4000);
	sound();
      }
    }
  }

  }
  return 0;
}

	/* WAVE ファイルの情報を格納する構造体 */
   	typedef struct {
   		FILE* fp;        /* ファイル構造体 */
   		short is_pcm;    /* PCM フォーマットの場合は 1、それ以外は 0 */
   		short channel;   /* モノラルの場合は 1、ステレオの場合は 2 */
   		int   rate;      /* サンプリング周波数 */
   		short bits;      /* 量子化ビット数 */
   		long  offset;    /* ファイル先頭から PCM データまでのオフセット */
 		int   len;       /* PCM データ部の長さ */
   	}
  	WAVE;
   	
  	
  	/* 関数プロトタイプ */
   	static int  wave_read_file( char* fname, WAVE* wave );
   	static int  wave_setup_dsp( int* fd, WAVE* wave );
   	static void wave_progress( int* processed, int current, WAVE* wave );
   	
   	
  	#define BUFSIZE 64000
   	
int sound(void){
   		WAVE wave;
   		char buf[ BUFSIZE ];
   		int  dsp;
   		int  len;
  		int  processed = 0;
		char** argv = (char**)malloc(sizeof(char*));
		argv[1] = "speech_16k.wav";
   		/* WAVE ファイルヘッダ読み込み */
   		if ( wave_read_file( argv[1], &wave ) != 0 ) {
   			fprintf( stderr, "Failed to read specified WAVE file"
   				 ": %s\n", argv[1] );
   			return 1;
   		}
   	
   	
   		/* /dev/dsp の設定 */
   		if ( wave_setup_dsp( &dsp, &wave ) != 0 ) {
   			fprintf( stderr, "Setup /dev/dsp failed.\n" );
   			fclose( wave.fp );
   			return 1;
   		}
   	
   		printf( "Now playing specified wave file %s ...\n", argv[1] );
   		fflush( stdout );
		
   		fseek( wave.fp, wave.offset, SEEK_SET );
   		wave_progress( &processed, 0, &wave );
   	
   		while ( 1 ) {
   			len = fread( buf, 1, BUFSIZE, wave.fp );
   	
   			if ( len < BUFSIZE ) {
   				if ( feof( wave.fp ) ) {
   					if ( write( dsp, buf, len ) == -1 ) {
   						perror( "write()" );
   					}
   					else {
   						wave_progress( &processed, len, &wave );
   					}
   				}
   				else {
   					perror( "fread()" );
   				}
   	
   				break;
   			}
   	
   			if ( write( dsp, buf, len ) == -1 ) {
   				perror( "write()" );
   				break;
   			}
   	
   			wave_progress( &processed, len, &wave );
			if(exit_sign == 1){
			  fprintf(stderr, "exit_sign\n");
			  exit_sign = 0;
			  	fclose( wave.fp );
				close( dsp );
  	
				printf( "\ndone.\n" );
				return 0;}
   		}
  	
  		fclose( wave.fp );
  		close( dsp );
  	
  		printf( "\ndone.\n" );
		s_flag = 0;
		return 0;
  	}
  	
  	
  	/*
  	 * WAVE ファイル情報を読み込む
  	 */
    static int 
  	wave_read_file( char* fname, WAVE* wave )
  	{
  		char   buf[32];
  		int    len;
  	
  		if ( ( wave->fp = fopen( fname, "r" ) ) == NULL ) {
  			fprintf( stderr, "Failed to open %s\n", fname );
  			return -1;
  		}
  	
  		/* 
  		 * 先頭 4 バイトが "RIFF" であることを確認
  		 * 更に 4 バイトスキップしておく
  		 */
  		fread( buf, 8, 1, wave->fp );
  	
 		if ( strncmp( buf, "RIFF", 4 ) != 0 ) {
  			fprintf( stderr, "Specified file is not RIFF file.\n" );
  			fclose( wave->fp );
  			return -1;
  		}
  	
  	
  		/* 次の 4 バイトが "WAVE" であることを確認 */
  		fread( buf, 4, 1, wave->fp );
  	
  		if ( strncmp( buf, "WAVE", 4 ) != 0 ) {
  			fprintf( stderr, "Specified file is not WAVE file.\n" );
  			fclose( wave->fp );
  			return -1;
  		}
  	
  		/* fmt チャンクを探す */
  		while ( 1 ) {
  			fread( buf, 8, 1, wave->fp );
  			len = *( int* )( &buf[4] );
  	
  			if ( strncmp( buf, "fmt ", 4 ) != 0 ) {
  				if ( fseek( wave->fp, len, SEEK_CUR ) == -1 ) {
  					fprintf( stderr, "Failed to find fmt chunk.\n" );
  					fclose( wave->fp );
  					return -1;
  				}
  			}
  			else {
  				break;
  			}
  		}
  	
  	
  		/* WAVE フォーマットを読み込む */
  		fread( buf, len, 1, wave->fp );
  	
  		wave->is_pcm  = *( ( short* )( &buf[0]  ) );
 		wave->channel = *( ( short* )( &buf[2]  ) );
  		wave->rate    = *( ( int*   )( &buf[4]  ) );
  		wave->bits    = *( ( short* )( &buf[14] ) );
  	
  		if ( wave->is_pcm != 1 ) {
			wave->is_pcm = 0;
  		}
  	
  		/* data チャンクを探す */
  		while ( 1 ) {
  			fread( buf, 8, 1, wave->fp );
  			len = *( int* )( &buf[4] );
  	
  			if ( strncmp( buf, "data", 4 ) != 0 ) {
  				if ( fseek( wave->fp, len, SEEK_CUR ) == -1 ) {
  					fprintf( stderr, "Failed to find data chunk.\n" );
  					fclose( wave->fp );
  					return -1;
  				}
  			}
  			else {
 				break;
  			}
  		}
 	
  		wave->len = len;
  	
  		if ( ( wave->offset = ftell( wave->fp ) ) == -1 ) {
  			fprintf( stderr, "Failed to find offset of PCM data.\n" );
  			fclose( wave->fp );
  			return -1;
 		}
	       
  		return 0;
	}
  	
  	
  	/*
  	 * /dev/dsp を設定する
  	 */
  	static int 
  	wave_setup_dsp( int* dsp, WAVE* wave )
  	{
  		int format;
  		int rate;
  		int channel;
  	
  		/*
 		 * PCM フォーマットであるかどうか確認する
  		 */
  		if ( ! wave->is_pcm ) {
  			fprintf( stderr, "Specified file's sound data is not PCM format.\n" );
  			fclose( wave->fp );
  			return -1;
  		}
  	
  		/*
  		 * 8 bit の場合は AFMT_U8、16 bit の場合は AFMT_S16_LE をフォーマット
  		 * として選択し、それ以外の数の場合はエラーとする。
  		 */
  		if ( wave->bits == 8 ) {
  			format = AFMT_U8;
 		}
 	else if ( wave->bits == 16 ) {
  			format = AFMT_S16_LE;
  		}
  		else {
  			fprintf( stderr, "Specified file's sound data is %d bits,"
  				 "not 8 nor 16 bits.\n", wave->bits );
  			fclose( wave->fp );
  
			return -1;
  		}
  	
  		rate    = ( int )wave->rate;
  		channel = ( int )wave->channel;
  	
  	
  		if ( ( *dsp = open( "/dev/dsp", O_WRONLY ) ) == -1 ) {
  			perror( "open()" );
  			return -1;
  		}
  
	
  		if ( ioctl( *dsp, SNDCTL_DSP_SETFMT, &format ) == -1 ) {
  			perror( "ioctl( SOUND_PCM_SETFMT )" );
  			close( *dsp );
  			return -1;
  		}
  	
  		if ( ioctl( *dsp, SOUND_PCM_WRITE_RATE, &rate ) == -1 ) {
  			perror( "ioctl( SOUND_PCM_WRITE_RATE )" );
  			close( *dsp );
  			return -1;
  		}
  	
  		if ( ioctl( *dsp, SOUND_PCM_WRITE_CHANNELS, &channel ) == -1 ) {
  			perror( "ioctl( SOUND_PCM_WRITE_CHANNELS )" );
  			close( *dsp );
  			return -1;
  		}
  	
  		return 0;
  	}
  	
  	
  	/*
  	 * 進捗率の表示
  	 */
  	static void 
 	wave_progress( int* processed, int current, WAVE* wave )
  	{
 		int progress;
  	
  		*processed += current;
  		progress = (int)( ( ( double )*processed / ( double )wave->len ) * 100 );
 		printf( "\r%3d%% played.", progress );
  		fflush( stdout );
  	}
  	
  	
  	/* End of wave.c */
