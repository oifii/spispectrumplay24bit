/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "bass.h"

#include "resource.h"

#include <string>
using namespace std;

/*
//#define SPECWIDTH 368	// display width
//#define SPECHEIGHT 127	// height (changing requires palette adjustments too)
#define SPECWIDTH 1000	// display width
#define SPECHEIGHT 64	// height (changing requires palette adjustments too)
*/
int SPECWIDTH=500;	// display width
int SPECHEIGHT=250;	// display height 

BYTE global_alpha=200;

char global_buffer[1024];
string global_filename="testwav.wav";
float global_fSecondsPlay; //negative for playing only once
DWORD global_timer=0;
int global_x=200;
int global_y=200;

HWND win=NULL;
DWORD timer=0;

DWORD global_chan;

HDC specdc=0;
HBITMAP specbmp=0;
BYTE *specbuf;

int specmode=0,specpos=0; // spectrum mode (and marker pos for 2nd mode)

//new parameters
string global_classname="BASS-Spectrum";
string global_title="spispectrumplay (click to toggle mode)";
string global_begin="";
string global_end="";
int global_idcolorpalette=0;
int global_bands=20;

RGBQUAD global_c1RGBQUAD; //audio channel1 color
RGBQUAD global_c2RGBQUAD; //audio channel2 color
RGBQUAD global_bgRGBQUAD; //solid background color








void CALLBACK StopPlayingFile(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	PostMessage(win, WM_DESTROY, 0, 0);
}

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

BOOL PlayFile(const char* filename)
{
	if (!(global_chan=BASS_StreamCreateFile(FALSE,filename,0,0,BASS_SAMPLE_LOOP))
		&& !(global_chan=BASS_MusicLoad(FALSE,filename,0,0,BASS_MUSIC_RAMP|BASS_SAMPLE_LOOP,1))) 
	{
		Error("Can't play file");
		return FALSE; // Can't load the file
	}
	if(global_fSecondsPlay<=0)
	{
		QWORD length_byte=BASS_ChannelGetLength(global_chan,BASS_POS_BYTE);
		global_fSecondsPlay=BASS_ChannelBytes2Seconds(global_chan,length_byte);
	}
	global_timer=timeSetEvent(global_fSecondsPlay*1000,25,(LPTIMECALLBACK)&StopPlayingFile,0,TIME_ONESHOT);

	BASS_ChannelPlay(global_chan,FALSE);
	return TRUE;
}

// select a file to play, and play it
BOOL PlayFile()
{
	char file[MAX_PATH]="";
	OPENFILENAME ofn={0};
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=win;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrFile=file;
	ofn.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER;
	ofn.lpstrTitle="Select a file to play";
	ofn.lpstrFilter="playable files\0*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
	if (!GetOpenFileName(&ofn)) return FALSE;
	
	return PlayFile(file);
}

// update the spectrum display - the interesting bit :)
void CALLBACK UpdateSpectrum(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	HDC dc;
	int x,y,y1;

	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(global_chan,&ci); // get number of channels
	int NUM_CHANNELS = ci.chans;

	if (specmode==3 || specmode==4 || specmode==5 || specmode==6 ||
		specmode==7 || specmode==8 || specmode==9 || specmode==10 ||
		specmode==11 || specmode==12 || specmode==13 || specmode==14 || 
		specmode==15 || specmode==16 || specmode==17 || specmode==18) 
	{ // waveform and filled waveform
		int c;
		float *buf;

		//solid background
		if(specmode==3 || specmode==4 || specmode==5 || specmode==6)
		{
			/*
			//black background
			memset(specbuf,0,SPECWIDTH*SPECHEIGHT*3);
			*/
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=global_bgRGBQUAD.rgbBlue;
					specbuf[j*SPECWIDTH*3+i*3+1]=global_bgRGBQUAD.rgbGreen;
					specbuf[j*SPECWIDTH*3+i*3+2]=global_bgRGBQUAD.rgbRed;
				}
			}
		}			
		//noisy background
		else if(specmode==7 || specmode==8 || specmode==9 || specmode==10)
		{	
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					
					int random_integer;
					//int lowest=1, highest=127;
					int lowest=64, highest=96; //good
					int range=(highest-lowest)+1;
					random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
					specbuf[j*SPECWIDTH*3+i*3]=random_integer; //B
					random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
					specbuf[j*SPECWIDTH*3+i*3+1]=random_integer; //G
					random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
					specbuf[j*SPECWIDTH*3+i*3+2]=random_integer; //R
				}
			}
		}
		//solid slightly shifting background
		else if(specmode==11 || specmode==12 || specmode==13 || specmode==14)
		{
			int random_integer;
			//int lowest=1, highest=127;
			//int lowest=1, highest=255;
			int lowest=64, highest=96; //good
			int range=(highest-lowest)+1;
			random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
			/* //in grays
			int R = random_integer;
			int G = random_integer;
			int B = random_integer;
			*/
			/* //pink-ish
			int R = 225;
			int G = random_integer;
			int B = 127;
			*/
			/* //purple-ish
			int R = 225;
			int G = random_integer;
			int B = 255;
			*/
			//purple-isher
			int R = 127;
			int G = random_integer;
			int B = 127;
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=B;
					specbuf[j*SPECWIDTH*3+i*3+1]=G;
					specbuf[j*SPECWIDTH*3+i*3+2]=R;
				}
			}
		}
		//solid radically shifting background
		else //specmode==15 || specmode==16 || specmode==17 || specmode==18
		{
			int random_integer;
			//int lowest=1, highest=127;
			int lowest=1, highest=255;
			//int lowest=64, highest=96; //good
			int range=(highest-lowest)+1;
			random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
			/* in all colors
			random_integer = lowest+(int)(range*rand()/(RAND_MAX + 1.0));
			int R = random_integer;
			random_integer = lowest+(int)(range*rand()/(RAND_MAX + 1.0));
			int G = random_integer;
			random_integer = lowest+(int)(range*rand()/(RAND_MAX + 1.0));
			int B = random_integer;
			*/
			/* in gray tones
			random_integer = lowest+(int)(range*rand()/(RAND_MAX + 1.0));
			int R = random_integer;
			int G = random_integer;
			int B = random_integer;
			*/
			random_integer = lowest+(int)(range*rand()/(RAND_MAX + 1.0));
			int R = 0;
			int G = random_integer;
			int B = 0;
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=B;
					specbuf[j*SPECWIDTH*3+i*3+1]=G;
					specbuf[j*SPECWIDTH*3+i*3+2]=R;
				}
			}
		}

		buf=(float*)alloca(NUM_CHANNELS*SPECWIDTH*sizeof(float)); // allocate buffer for data
		BASS_ChannelGetData(global_chan,buf,(ci.chans*SPECWIDTH*sizeof(float))|BASS_DATA_FLOAT); // get the sample data (floating-point to avoid 8 & 16 bit processing)

		/*
		//global_err = Pa_ReadStream( global_stream, buf, FRAMES_PER_BUFFER );
		global_err = Pa_ReadStream( global_stream, buf, NUM_CHANNELS*SPECWIDTH );
		if( global_err != paNoError ) 
		{
			//char errorbuf[2048];
			//sprintf(errorbuf, "Error reading stream: %s\n", Pa_GetErrorText(global_err));
			//MessageBox(0,errorbuf,0,MB_ICONERROR);
			return;
		}
		*/

		//under waveform filled down to bottom
		if (specmode==6 || specmode==10 || specmode==14 || specmode==18)
		{
			for (c=0;c<NUM_CHANNELS;c++)
			{
				for (x=0;x<SPECWIDTH;x++) 
				{
					int v=(1-buf[x*NUM_CHANNELS+c])*SPECHEIGHT/2; // invert and scale to fit display
					if (v<0) v=0;
					else if (v>=SPECHEIGHT) v=SPECHEIGHT-1;
					//under waveform filled down to bottom
					y=v;
					while(--y>=0) 
					{
						specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
						specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
						specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
					}
				}
			}
		}
		//waveform filled towards center
		else if (specmode==5 || specmode==9 || specmode==13 || specmode==17)
		{
			for (c=0;c<NUM_CHANNELS;c++) 
			{
				for (x=0;x<SPECWIDTH;x++) 
				{
					int v=(1-buf[x*NUM_CHANNELS+c])*SPECHEIGHT/2; // invert and scale to fit display
					if (v<0) v=0;
					else if (v>=SPECHEIGHT) v=SPECHEIGHT-1;
					//waveform filled towards center
					y=v;
					if(y>(SPECHEIGHT/2))
						while(--y>=(SPECHEIGHT/2)) 
						{
							specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
							specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
							specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
						}
					else if(y<(SPECHEIGHT/2))
						while(++y<=(SPECHEIGHT/2))  
						{
							specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
							specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
							specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
						}
					else
					{
						specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
						specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
						specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
					}

				}
			}
		}
		//waveform filled towards opposite
		else if (specmode==4 || specmode==8 || specmode==12 || specmode==16)
		{
			for (c=0;c<NUM_CHANNELS;c++) 
			{
				for (x=0;x<SPECWIDTH;x++) 
				{
					int v=(1-buf[x*NUM_CHANNELS+c])*SPECHEIGHT/2; // invert and scale to fit display
					if (v<0) v=0;
					else if (v>=SPECHEIGHT) v=SPECHEIGHT-1;
					//waveform filled towards opposite
					y=v;
					if(y>(SPECHEIGHT/2))
						while(--y>=(SPECHEIGHT/2-(v-(SPECHEIGHT/2))))
						{
							specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
							specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
							specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
						}

					else if(y<(SPECHEIGHT/2))
						while(++y<=(SPECHEIGHT/2+((SPECHEIGHT/2)-v)))
						{
							specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
							specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
							specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
						}
					else
					{
						specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
						specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
						specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
					}
				}
			}
		}
		//waveform (original)
		else if(specmode==3 || specmode==7 || specmode==11 || specmode==15)
		{
			for (c=0;c<NUM_CHANNELS;c++) 
			{
				for (x=0;x<SPECWIDTH;x++) 
				{
					int v=(1-buf[x*NUM_CHANNELS+c])*SPECHEIGHT/2; // invert and scale to fit display
					if (v<0) v=0;
					else if (v>=SPECHEIGHT) v=SPECHEIGHT-1;
					if (!x) y=v;
					do 
					{ // draw line from previous sample...
						if (y<v) y++;
						else if (y>v) y--;
						specbuf[y*SPECWIDTH*3+x*3]=c&1?global_c2RGBQUAD.rgbBlue:global_c1RGBQUAD.rgbBlue; //B
						specbuf[y*SPECWIDTH*3+x*3+1]=c&1?global_c2RGBQUAD.rgbGreen:global_c1RGBQUAD.rgbGreen; //G
						specbuf[y*SPECWIDTH*3+x*3+2]=c&1?global_c2RGBQUAD.rgbRed:global_c1RGBQUAD.rgbRed; //R
					} while (y!=v);
				}
			}
		}
	} 
	else 
	{
		float fftbuf[2048];
		if(SPECWIDTH<2048)
		{
			BASS_ChannelGetData(global_chan,fftbuf,BASS_DATA_FFT2048); // get the FFT data, BASS_DATA_FFT2048 returns 1024 values
		}
		else if(SPECWIDTH<4096)
		{
			BASS_ChannelGetData(global_chan,fftbuf,BASS_DATA_FFT4096); // get the FFT data, BASS_DATA_FFT4096 returns 2048 values
		}
		/*
		float *buf2;
		int numberofsamples=SPECWIDTH;
		if(numberofsamples<1024) numberofsamples=1024;
		buf2=(float*)alloca(NUM_CHANNELS*numberofsamples*sizeof(float)); // allocate buffer for data
		global_err = Pa_ReadStream( global_stream, buf2, NUM_CHANNELS*numberofsamples );
		if( global_err != paNoError ) 
		{
			//char errorbuf[2048];
			//sprintf(errorbuf, "Error reading stream (2): %s\n", Pa_GetErrorText(global_err));
			//MessageBox(0,errorbuf,0,MB_ICONERROR);
			return;
		}
		fft(buf2, fftbuf, 1024);
		for(int i=0; i<1024; i++)
		{
			fftbuf[i]=abs(fftbuf[i]);
		}
		*/

		if (!specmode) 
		{ // "normal" FFT
			int R = 0;
			int G = 0;
			int B = 0;
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=B;
					specbuf[j*SPECWIDTH*3+i*3+1]=G;
					specbuf[j*SPECWIDTH*3+i*3+2]=R;
				}
			}
			/* //todo
			for (x=0;x<SPECWIDTH/2;x++) 
			{
#if 1
				y=sqrt(fftbuf[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
#else
				y=fftbuf[x+1]*10*SPECHEIGHT; // scale it (linearly)
#endif
				if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
				if (x && (y1=(y+y1)/2)) // interpolate from previous to make the display smoother
					//while (--y1>=0) specbuf[y1*SPECWIDTH+x*2-1]=y1+1;
					while (--y1>=0) specbuf[y1*SPECWIDTH+x*2-1]=(127*y1/SPECHEIGHT)+1;
				y1=y;
				//while (--y>=0) specbuf[y*SPECWIDTH+x*2]=y+1; // draw level
				while (--y>=0) specbuf[y*SPECWIDTH+x*2]=(127*y/SPECHEIGHT)+1; // draw level
			}
			*/
		} 
		else if (specmode==1) 
		{ // logarithmic, acumulate & average bins
			int b0=0;
			int R = 0;
			int G = 0;
			int B = 0;
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=B;
					specbuf[j*SPECWIDTH*3+i*3+1]=G;
					specbuf[j*SPECWIDTH*3+i*3+2]=R;
				}
			}
			/* //todo
//#define BANDS 28
//#define BANDS 80
//#define BANDS 12
			for (x=0;x<global_bands;x++) 
			{
				float peak=0;
				int b1=pow(2,x*10.0/(global_bands-1));
				if (b1>1023) b1=1023;
				if (b1<=b0) b1=b0+1; // make sure it uses at least 1 FFT bin
				for (;b0<b1;b0++)
					if (peak<fftbuf[1+b0]) peak=fftbuf[1+b0];
				y=sqrt(peak)*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
				if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
				while (--y>=0)
					//memset(specbuf+y*SPECWIDTH+x*(SPECWIDTH/global_bands),y+1,SPECWIDTH/global_bands-2); // draw bar
					memset(specbuf+y*SPECWIDTH+x*(SPECWIDTH/global_bands),(127*y/SPECHEIGHT)+1,SPECWIDTH/global_bands-2); // draw bar
			}
			*/
		} 
		else 
		{ // "3D"
			int R = 0;
			int G = 0;
			int B = 0;
			for(int i=0; i<SPECWIDTH; i++)
			{
				for (int j=0; j<SPECHEIGHT; j++)
				{
					//specbuf[j*SPECWIDTH+i]=random_integer;
					specbuf[j*SPECWIDTH*3+i*3]=B;
					specbuf[j*SPECWIDTH*3+i*3+1]=G;
					specbuf[j*SPECWIDTH*3+i*3+2]=R;
				}
			}
			/* //todo
			for (x=0;x<SPECHEIGHT;x++) 
			{
				y=sqrt(fftbuf[x+1])*3*127; // scale it (sqrt to make low values more visible)
				if (y>127) y=127; // cap it
				specbuf[x*SPECWIDTH+specpos]=128+y; // plot it
			}
			// move marker onto next position
			specpos=(specpos+1)%SPECWIDTH;
			for (x=0;x<SPECHEIGHT;x++) specbuf[x*SPECWIDTH+specpos]=255;
			*/
		}
	}

	// update the display
	dc=GetDC(win);
	BitBlt(dc,0,0,SPECWIDTH,SPECHEIGHT,specdc,0,0,SRCCOPY);
	ReleaseDC(win,dc);
}

// window procedure
long FAR PASCAL SpectrumWindowProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_PAINT:
			if (GetUpdateRect(h,0,0)) {
				PAINTSTRUCT p;
				HDC dc;
				if (!(dc=BeginPaint(h,&p))) return 0;
				BitBlt(dc,0,0,SPECWIDTH,SPECHEIGHT,specdc,0,0,SRCCOPY);
				EndPaint(h,&p);
			}
			return 0;

		case WM_LBUTTONUP:
			//specmode=(specmode+1)%4; // swap spectrum mode
			specmode=(specmode+1)%19; // swap spectrum mode
			memset(specbuf,0,SPECWIDTH*SPECHEIGHT);	// clear display
			return 0;

		case WM_RBUTTONUP:
			specmode=(specmode-1); // swap spectrum mode
			if(specmode<0) specmode = 19-1;
			memset(specbuf,0,SPECWIDTH*SPECHEIGHT);	// clear display
			return 0;

		case WM_CREATE:
			win=h;
			//spi, avril 2015, begin
			SetWindowLong(h, GWL_EXSTYLE, GetWindowLong(h, GWL_EXSTYLE) | WS_EX_LAYERED);
			SetLayeredWindowAttributes(h, 0, global_alpha, LWA_ALPHA);
			//SetLayeredWindowAttributes(h, 0, 200, LWA_ALPHA);
			//spi, avril 2015, end
			// initialize BASS
			if (!BASS_Init(-1,44100,0,win,NULL)) {
				Error("Can't initialize device");
				return -1;
			}
			//if (!PlayFile()) { // start a file playing
			if (!PlayFile(global_filename.c_str())) { // start a file playing
				BASS_Free();
				return -1;
			}
			{ 
				// create bitmap to draw spectrum in 24bit
				BYTE data[2000]={0};
				BITMAPINFOHEADER *bh=(BITMAPINFOHEADER*)data;
				RGBQUAD *pal=(RGBQUAD*)(data+sizeof(*bh));
				int a;
				bh->biSize=sizeof(*bh);
				bh->biWidth=SPECWIDTH;
				bh->biHeight=SPECHEIGHT; // upside down (line 0=bottom)
				bh->biSizeImage=SPECWIDTH*SPECHEIGHT*3;
				bh->biPlanes=1;
				bh->biCompression = BI_RGB;
				bh->biBitCount=24;
				bh->biClrUsed=bh->biClrImportant=0;
				
				// create the bitmap
				specbmp=CreateDIBSection(0,(BITMAPINFO*)bh,DIB_RGB_COLORS,(void**)&specbuf,NULL,0);
				specdc=CreateCompatibleDC(0);
				SelectObject(specdc,specbmp);
			}
			// setup update timer (40hz)
			timer=timeSetEvent(25,25,(LPTIMECALLBACK)&UpdateSpectrum,0,TIME_PERIODIC);
			//timer=timeSetEvent(100,100,(LPTIMECALLBACK)&UpdateSpectrum,0,TIME_PERIODIC);
			break;

		case WM_DESTROY:
			{
				if (timer) timeKillEvent(timer);
				if (global_timer) timeKillEvent(global_timer);
				BASS_Free();
				if (specdc) DeleteDC(specdc);
				if (specbmp) DeleteObject(specbmp);

				int nShowCmd = false;
				//ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
				//ShellExecuteA(NULL, "open", "end.ahk", "", NULL, nShowCmd);
				ShellExecuteA(NULL, "open", global_end.c_str(), "", NULL, nShowCmd);
				
				PostQuitMessage(0);
			}
			break;
	}
	return DefWindowProc(h, m, w, l);
}

PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	int nShowCmd = false;

	//LPWSTR *szArgList;
	LPSTR *szArgList;
	int argCount;
	//szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);
	szArgList = CommandLineToArgvA(GetCommandLine(), &argCount);
	if (szArgList == NULL)
	{
		MessageBox(NULL, "Unable to parse command line", "Error", MB_OK);
		return 10;
	}
	//global_filename="testwav.wav";
	if(argCount>1)
	{
		global_filename = szArgList[1]; 
	
		/*
		int ret = wcstombs ( global_buffer, szArgList[1], sizeof(global_buffer) );
		if (ret==sizeof(global_buffer)) global_buffer[sizeof(global_buffer)-1]='\0';
		global_filename = global_buffer; 
		*/
	}
	
	global_fSecondsPlay = -1.0; //negative for playing only once
	//global_fSecondsPlay = 30.0f; //negative for playing only once
	if(argCount>2)
	{
		global_fSecondsPlay = atof((LPCSTR)(szArgList[2]));
		//global_fSecondsPlay = _wtof(szArgList[2]);
	}
	if(argCount>3)
	{
		global_x = atoi((LPCSTR)(szArgList[3]));
	}
	if(argCount>4)
	{
		global_y = atoi((LPCSTR)(szArgList[4]));
	}
	if(argCount>5)
	{
		specmode = atoi((LPCSTR)(szArgList[5]));
	}
	if(argCount>6)
	{
		global_classname = szArgList[6]; 
	}
	if(argCount>7)
	{
		global_title = szArgList[7]; 
	}
	if(argCount>8)
	{
		global_begin = szArgList[8]; 
	}
	if(argCount>9)
	{
		global_end = szArgList[9]; 
	}
	/*
	if(argCount>10)
	{
		global_idcolorpalette = atoi((LPCSTR)(szArgList[10]));
	}
	*/
	if(argCount>10)
	{
		global_bands = atoi((LPCSTR)(szArgList[10]));
	}
	if(argCount>11)
	{
		SPECWIDTH = atoi((LPCSTR)(szArgList[11]));
	}
	if(argCount>12)
	{
		SPECHEIGHT = atoi((LPCSTR)(szArgList[12]));
	}
	if(argCount>13)
	{
		global_alpha = atoi(szArgList[13]);
	}
    //solid background color
    global_bgRGBQUAD.rgbRed=127;
    global_bgRGBQUAD.rgbGreen=0;
    global_bgRGBQUAD.rgbBlue=127;
    //audio channel 1 color
    global_c1RGBQUAD.rgbRed=255;
    global_c1RGBQUAD.rgbGreen=0;
    global_c1RGBQUAD.rgbBlue=0;
    //audio channel 2 color
    global_c2RGBQUAD.rgbRed=0;
    global_c2RGBQUAD.rgbGreen=0;
    global_c2RGBQUAD.rgbBlue=255;
    if(argCount>14)
    {
        global_bgRGBQUAD.rgbRed  = atoi((LPCSTR)(szArgList[14])); //solid background color - red component
    }
    if(argCount>15)
    {
        global_bgRGBQUAD.rgbGreen  = atoi((LPCSTR)(szArgList[15]));
    }
    if(argCount>16)
    {
        global_bgRGBQUAD.rgbBlue  = atoi((LPCSTR)(szArgList[16]));
    }
    if(argCount>17)
    {
        global_c1RGBQUAD.rgbRed  = atoi((LPCSTR)(szArgList[17])); //audio channel1 color - red component
    }
    if(argCount>18)
    {
        global_c1RGBQUAD.rgbGreen  = atoi((LPCSTR)(szArgList[18]));
    }
    if(argCount>19)
    {
        global_c1RGBQUAD.rgbBlue  = atoi((LPCSTR)(szArgList[19]));
    }
    if(argCount>20)
    {
        global_c2RGBQUAD.rgbRed  = atoi((LPCSTR)(szArgList[20])); //audio channel2 color - red component
    }
    if(argCount>21)
    {
        global_c2RGBQUAD.rgbGreen  = atoi((LPCSTR)(szArgList[21]));
    }
    if(argCount>22)
    {
        global_c2RGBQUAD.rgbBlue  = atoi((LPCSTR)(szArgList[22]));
    }


	LocalFree(szArgList);

	//ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);
	//ShellExecuteA(NULL, "open", "begin.ahk", "", NULL, nShowCmd);
	ShellExecuteA(NULL, "open", global_begin.c_str(), "", NULL, nShowCmd);

	WNDCLASS wc={0};
    MSG msg;

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		MessageBox(0,"An incorrect version of BASS.DLL was loaded",0,MB_ICONERROR);
		return 0;
	}

	// register window class and create the window
	wc.lpfnWndProc = SpectrumWindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); //spi, added
	wc.lpszClassName = global_classname.c_str();
	if (!RegisterClass(&wc) || !CreateWindow(global_classname.c_str(),
			//"BASS spectrum example (click to toggle mode)",
			//"spispectrumplay (click to toggle mode)",
			global_title.c_str(),
			//WS_POPUPWINDOW|WS_VISIBLE, global_x, global_y,
			WS_POPUP|WS_VISIBLE, global_x, global_y,
			
			//WS_POPUPWINDOW|WS_CAPTION|WS_VISIBLE, global_x, global_y,
			//WS_POPUPWINDOW|WS_VISIBLE, 200, 200,
			SPECWIDTH,
			//SPECWIDTH+2*GetSystemMetrics(SM_CXDLGFRAME),
			SPECHEIGHT,
			//SPECHEIGHT+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYDLGFRAME),
			NULL, NULL, hInstance, NULL)) {
		Error("Can't create window");
		return 0;
	}
	ShowWindow(win, SW_SHOWNORMAL);

	while (GetMessage(&msg,NULL,0,0)>0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
