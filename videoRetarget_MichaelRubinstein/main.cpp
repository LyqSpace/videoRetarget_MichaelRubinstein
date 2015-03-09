/*
*	Copyright (C)   Lyq root#lyq.me
*	File Name     : cartoonResizing
*	Creation Time : 2015-3-5 14:47:00 UTC+8
*	Environment   : Windows8.1-64bit VS2013 OpenCV2.4.9
*	Homepage      : http://www.lyq.me
*/

#ifndef OPENCV
#define OPENCV
#include <opencv2\opencv.hpp>
#include <opencv2\video\video.hpp>
using namespace cv;
#endif // !OPENCV

#ifndef STD
#define STD
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <queue>
#include <ctime>
using namespace std;
#define INF 2000000000
#define sqr(_x) ( (_x) * (_x) )
struct typeEdge {
	int x, y, w, next;
	bool flag;
};
#endif // !STD

void help( void ); 
bool readVideo( VideoCapture &cap, const char *videoName, vector<Mat> &frames, double sizeScalar );
void buildGraph( vector<Mat> &frames );
void maxFlow();
void surfaceCarving( vector<Mat> &frames, int surfaceNum );
bool writeVideo( const char *videoName );

inline int txy2num( int t, int x, int y, int frameCount, Size frameSize );
inline bool num2txy( int num, int &t, int &x, int &y, int N, int frameCount, Size frameSize );
inline void buildEdge( vector<int> &edgeHead, vector<typeEdge> &edge, int x, int y, int w, bool flag = false );
int bfsDinic();
int dfsDinic( int nowP, int minFlow );

vector<int> edgeHead;
vector<typeEdge> edge;
vector<int> tag;
vector<bool>viewedP;
int N;
int temp;

int main( void ) {

	help();
	
	int funcType = 1;

	if ( funcType == 0 ) {

		char *videoName = "test1.avi";
		VideoCapture cap;
		vector<Mat> frames;
		bool state = readVideo( cap, videoName, frames, 1 );
		if ( !state ) return -2;

		int surfaceDeleted = 0;
		while ( surfaceDeleted < 10 ) {

			int startTime = clock();
			surfaceDeleted++;
			cout << "\n>>> Start curving surface : " << surfaceDeleted << endl;
			buildGraph( frames );
			maxFlow();
			surfaceCarving( frames, surfaceDeleted );
			int endTime = clock();
			cout << "<<< Time used : " << endTime - startTime << " ms" << endl;
		}
	} else {

		char *videoName = "test1_out.avi";
		bool state = writeVideo( videoName );
		if ( !state ) return -2;
	}
	

	return 0;
}

void help( void ) {

	printf( "===	Copyright (C)   Lyq root#lyq.me\n"
			"===	File Name : cartoonResizing\n"
			"===	Creation Time : 2015-3-5 14:47:00 UTC+8\n"
			"===	Environment : Windows8.1-64bit VS2013 OpenCV2.4.9\n"
			"===	Homepage : http ://www.lyq.me\n"
			"===	\n"
			"===	This program demostrated a simple method of cartoon resizing.\n"
			"===	It learns the pixels' Energy and calculates the min surface Curve.\n"
			"===	Reference: Improved Seam Carving for Video Retargeting\n"
			"		\n" );
}

bool readVideo( VideoCapture &cap, const char *videoName, vector<Mat> &frames, double sizeScalar ) {

	if ( !cap.open( videoName ) )
	{
		cout << "Could not open the output video for reading!" << endl;
		return false;
	}

	Mat inputFrame, resizeFrame, grayFrame, blurFrame;
	int count = 0;
	char bmpName[100];
	
	//int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC));
	//cout << ex;

	while ( cap.read( inputFrame ) ) {

		resize( inputFrame, resizeFrame, Size(), sizeScalar, sizeScalar );
		cvtColor( resizeFrame, grayFrame, CV_RGB2GRAY );
		medianBlur( grayFrame, blurFrame, 3 );
		frames.push_back( blurFrame.clone() );

		//imshow( "originVideo", blurFrame );
		//waitKey( 50 );

		sprintf( bmpName, "inResource//%d.bmp", count );
		imwrite( bmpName, blurFrame );

		count++;
		if ( count == 100 ) break;
	}
	return true;
}

void buildGraph( vector<Mat> &frames ) {

	int frameCount = frames.size();
	Size frameSize = Size( frames[0].cols, frames[0].rows );
	cout << "frameSize.height : " << frameSize.height << " frameSize.width : " << frameSize.width << endl;
	cout << "frameCount : " << frameCount << endl;

	N = frameCount * frameSize.width * frameSize.height + 2;
	int S = 0, T = N - 1;

	edgeHead.clear();
	edge.clear();
	
	edgeHead = vector<int>( N, -1 );
	cout << "Node size : " << N << endl;

	int p0, p1, p2, w;
	for ( int y = 0; y < frameSize.height; y++ ) {
		for ( int t = 0; t < frameCount; t++ ) {
			
			p0 = txy2num( t, 0, y, frameCount, frameSize );
			buildEdge( edgeHead, edge, S, p0, INF );
			buildEdge( edgeHead, edge, p0, S, 0 );
			p0 = txy2num( t, frameSize.width - 1, y, frameCount, frameSize );
			buildEdge( edgeHead, edge, p0, T, INF );
			buildEdge( edgeHead, edge, T, p0, 0 );
		}
	}
	//cout << "After S & T edge.size() = " << edge.size() << endl;
	
	// XY-Plane
	for ( int t = 0; t < frameCount; t++ ) {

		// LR
		for ( int y = 0; y < frameSize.height; y++ ) {

			uchar *rowData = frames[t].ptr<uchar>( y );
			for ( int x = 0; x < frameSize.width - 1; x++ ) {

				p0 = txy2num( t, x, y, frameCount, frameSize );
				p1 = txy2num( t, x + 1, y, frameCount, frameSize );
				p2 = txy2num( t, x - 1, y, frameCount, frameSize );
				if ( p1 != -1 && p2 != -1) {
					w = abs( rowData[x + 1] - rowData[x - 1] );
				} else {
					if ( p1 != -1 ) {
						w = abs( rowData[x + 1] - rowData[x] );
					} else {
						w = abs( rowData[x] - rowData[x - 1] );
					}
				}
			
				buildEdge( edgeHead, edge, p0, p1, w, true );
				buildEdge( edgeHead, edge, p1, p0, INF );
			}
		}

		for ( int y = 1; y < frameSize.height; y++ ) {

			uchar *rowData0 = frames[t].ptr<uchar>( y - 1 );
			uchar *rowData1 = frames[t].ptr<uchar>( y );
			for ( int x = 0; x < frameSize.width; x++ ) {
				
				// LU
				p0 = txy2num( t, x, y, frameCount, frameSize );
				p1 = txy2num( t, x, y - 1, frameCount, frameSize );
				p2 = txy2num( t, x - 1, y, frameCount, frameSize );
				if ( p1 != -1 && p2 != -1 ) {
					//w = abs( rowData0[x] - rowData1[x - 1] );
					w = abs( rowData0[ x - 1 ] - rowData1[ x ] - rowData0[ x ] + rowData1[ x - 1 ] );
					buildEdge( edgeHead, edge, p0, p1, w );
					buildEdge( edgeHead, edge, p1, p0, 0 );
					buildEdge( edgeHead, edge, p1, p2, INF );
					buildEdge( edgeHead, edge, p2, p1, 0 );
				}
				
				// LD
				p0 = p1;
				p1 = txy2num( t, x - 1, y - 1, frameCount, frameSize );
				p2 = txy2num( t, x, y, frameCount, frameSize );
				if ( p1 != -1 && p2 != -1 ) {
					//w = abs( rowData0[x - 1] - rowData1[x] );
					w = abs( rowData0[ x ] - rowData1[ x - 1 ] - rowData0[ x - 1 ] + rowData1[ x ] );
					buildEdge( edgeHead, edge, p0, p2, w );
					buildEdge( edgeHead, edge, p2, p0, 0 );			
					buildEdge( edgeHead, edge, p2, p1, INF );
					buildEdge( edgeHead, edge, p1, p2, 0 );
				}
			}
		}
	}

	//cout << "After XY-Plane edge.size() = " << edge.size() << endl;

	// XT-Plane
	for ( int y = 0; y < frameSize.height; y++ ) {

		for ( int t = 1; t < frameCount; t++ ) {
			
			uchar *rowData0 = frames[t - 1].ptr<uchar>( y );
			uchar *rowData1 = frames[t].ptr<uchar>( y );
			for ( int x = 0; x < frameSize.width; x++ ) {
				
				// LU
				p0 = txy2num( t, x, y, frameCount, frameSize );
				p1 = txy2num( t, x - 1, y, frameCount, frameSize );
				p2 = txy2num( t - 1, x, y, frameCount, frameSize );
				if ( p1 != -1 && p2 != -1 ) {
					//w = abs( rowData0[x] - rowData1[x - 1] );
					w = abs( rowData0[ x - 1 ] - rowData1[ x ] - rowData0[ x ] + rowData1[ x - 1 ] );
					buildEdge( edgeHead, edge, p0, p2, w );
					buildEdge( edgeHead, edge, p2, p0, 0 );
					buildEdge( edgeHead, edge, p2, p1, INF );
					buildEdge( edgeHead, edge, p1, p2, 0 );
				}

				// LD
				p0 = p2;
				p1 = txy2num( t, x, y, frameCount, frameSize );
				p2 = txy2num( t - 1, x - 1, y, frameCount, frameSize );
				if ( p1 != -1 && p2 != -1 ) {
					//w = abs( rowData0[x - 1] - rowData1[x] );
					w = abs( rowData0[ x ] - rowData1[ x - 1 ] - rowData0[ x - 1 ] + rowData1[ x ] );
					buildEdge( edgeHead, edge, p0, p1, w );
					buildEdge( edgeHead, edge, p1, p0, 0 );
					buildEdge( edgeHead, edge, p1, p2, INF );
					buildEdge( edgeHead, edge, p2, p1, 0 );
				}
			}
		}
	}
	//cout << "After XT-Plane edge.size() = " << edge.size() << endl;
	cout << "Edge size : " << edge.size() << endl;
}

inline int txy2num( int t, int x, int y, int frameCount, Size frameSize ) {

	if ( t < 0 || t >= frameCount ) return -1;
	if ( x < 0 || x >= frameSize.width ) return -1;
	if ( y < 0 || y >= frameSize.height ) return -1;
	return t * frameSize.width * frameSize.height + y * frameSize.width + x + 1;

}

inline bool num2txy( int num, int &t, int &x, int &y, int N, int frameCount, Size frameSize ) {

	if ( num < 0 || num >= N ) return false;

	num--;
	int temp = frameSize.width * frameSize.height;
	t = num / temp;
	num = num - t * temp;
	y = num / frameSize.width;
	num = num - y * frameSize.width;
	x = num;

	return true;
}

inline void buildEdge( vector<int> &edgeHead, vector<typeEdge> &edge, int x, int y, int w, bool flag ) {

	typeEdge oneEdge;
	oneEdge.x = x;
	oneEdge.y = y;
	oneEdge.w = w;
	oneEdge.flag = flag;
	oneEdge.next = edgeHead[x];
	edgeHead[x] = edge.size();
	edge.push_back( oneEdge );
}

void maxFlow() {

	long long ans = 0;

	while ( bfsDinic() ) {

		int tans = 1;
		while ( tans > 0 ) {

			temp = N;
			viewedP = vector<bool>( N, false );
			tans = dfsDinic( 0, 0x7fffffff );
			ans += tans;
		}
	}
	cout << "maxFlow : " << ans << endl;
}

int bfsDinic() {

	queue<int> que;
	tag = vector<int>( N, -1 );
	tag[0] = 0;
	que.push( 0 );
	while ( !que.empty() ) {

		int nowP = que.front();
		que.pop();
		for ( int p = edgeHead[nowP]; p != -1; p = edge[p].next ) {

			int nextP = edge[p].y;
			if ( tag[nextP] == -1 && edge[p].w > 0 ) {
				tag[nextP] = tag[nowP] + 1;
				que.push( nextP );
			}
		}
	}
	if ( tag[N - 1] > 0 ) {
		return 1;
	} else {
		return 0;
	}
}

int dfsDinic( int nowP, int minFlow ) {

	//cout << nowP << " " << tag[nowP] << " " << minFlow << endl;

	viewedP[nowP] = true; temp--;
	//cout << temp << " ";
	if ( minFlow == 0 ) return 0;
	if ( nowP == N - 1 ) return minFlow;

	for ( int p = edgeHead[nowP]; p != -1; p = edge[p].next ) {

		int nextP = edge[p].y;
		if ( tag[nextP] != tag[nowP] + 1 ) continue;
		if ( edge[p].w <= 0 ) continue;
		if ( viewedP[nextP] ) continue;

		int flow = dfsDinic( nextP, min( minFlow, edge[p].w ) );
		if ( flow > 0 ) {
			edge[p].w -= flow;
			edge[p + 1].w += flow;
			return flow;
		}
	}
	return 0;
}

void surfaceCarving( vector<Mat> &frames, int surfaceNum ) {

	bfsDinic();

	int N = edgeHead.size();
	bool isRemoved;
	int frameCount = frames.size();
	Size frameSize = Size( frames[0].cols, frames[0].rows );
	int frameType = frames[0].type();
	vector<Mat> frames2;
	vector<Mat> frames3;

	for ( int i = 0; i < frameCount; i++ ) {
		frames2.push_back( Mat( frameSize.height, frameSize.width-1, frameType ) );
		frames3.push_back( Mat( frameSize.height, frameSize.width, frameType ) );
		//imshow( "test", frames[i] );
		//waitKey( 50 );
	}
	
	for ( int i = 1; i < N; i++ ) {
		
		if ( tag[i] == -1 ) continue;
		isRemoved = false;
		for ( int p = edgeHead[i]; p != -1; p = edge[p].next ) {

			//cout << edge[p].flag << " " << tag[edge[p].y] << endl;
			if ( edge[p].flag && tag[edge[p].y] == -1 ) {
				isRemoved = true;
				break;
			}
		}
		
		if ( !isRemoved ) continue;

		int t0, x0, y0;
		if ( !num2txy( i, t0, x0, y0, N, frameCount, frameSize ) ) continue;

		uchar *rowData0, *rowData1;
		rowData0 = frames[t0].ptr<uchar>( y0 );
		rowData1 = frames2[t0].ptr<uchar>( y0 );
		for ( int x = 0; x < x0; x++ ) rowData1[x] = rowData0[x];
		for ( int x = x0 + 1; x < frameSize.width; x++ ) rowData1[x - 1] = rowData0[x];

		frames3[t0].row( y0 ) = frames[t0].row( y0 );
		frames3[t0].ptr<uchar>( y0 )[x0] = 0;
	}

	char bmpName[100];
	for ( int t = 0; t < frameCount; t++ ) {

		sprintf( bmpName, "outSurface//%d_%d.bmp", surfaceNum, t );
		imwrite( bmpName, frames3[t] );

		sprintf( bmpName, "outResult//%d_%d.bmp", surfaceNum, t );
		imwrite( bmpName, frames2[t] );
	}

	frames = frames2;
} 

bool writeVideo( const char *videoName ) {

	VideoWriter outputVideo;
	outputVideo.open(videoName, CV_FOURCC('I', 'Y', 'U', 'V'), 25, Size(505, 250));

	if ( !outputVideo.isOpened() ) {

		cout << "Could not open the output video for writing!" << endl;
		return false;
	}

	char inFrameName[100], outFrameName[100], combineFrameName[100];
	Mat inFrame, outFrame, outFrame1, combineFrame;
	uchar *rowData0, *rowData1;
	namedWindow( "combineVideo" );

	for ( int i = 0; i < 20; i++ ) {
		
		sprintf( inFrameName, "inResource//%d.bmp", i);
		inFrame = imread( inFrameName );
		cvtColor( inFrame, inFrame, CV_RGB2GRAY );
		resize( inFrame, inFrame, Size(), 10, 10 );
		sprintf( outFrameName, "outResult//10_%d.bmp", i );
		outFrame = imread( outFrameName );
		cvtColor( outFrame, outFrame, CV_RGB2GRAY );
		resize( outFrame, outFrame, Size(), 10, 10 );

		sprintf( outFrameName, "outResult1//10_%d.bmp", i );
		outFrame1 = imread( outFrameName );
		cvtColor( outFrame1, outFrame1, CV_RGB2GRAY );
		resize( outFrame1, outFrame1, Size(), 10, 10 );

		combineFrame = Mat::zeros( inFrame.rows, inFrame.cols * 3 + 10, inFrame.type() );
		
		for ( int y = 0; y < inFrame.rows; y++ ) {
			 
			rowData0 = combineFrame.ptr<uchar>( y );
			rowData1 = inFrame.ptr<uchar>( y );
			for ( int x = 0; x < inFrame.cols; x++ ) rowData0[x] = rowData1[x];
			
			rowData1 = outFrame.ptr<uchar>( y );
			for ( int x = 0; x < outFrame.cols; x++) rowData0[x + inFrame.cols + 5] = rowData1[x];

			rowData1 = outFrame1.ptr<uchar>( y );
			for ( int x = 0; x < outFrame.cols; x++ ) rowData0[x + inFrame.cols*2 + 10] = rowData1[x];
		
		}

		sprintf(combineFrameName, "combineFrame//%d.bmp", i);
		imwrite(combineFrameName, combineFrame);
		outputVideo << combineFrame;
		imshow( "combineVideo", combineFrame );
		waitKey( 100 );
	}

	waitKey( 0 );

	return true;
}