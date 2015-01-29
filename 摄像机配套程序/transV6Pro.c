
/***************************************************************************************************************************/

#include <vcrt.h>
#include <vclib.h>
#include <macros.h>
#include <sysvar.h>
#include <flib.h>
/***************************************************************************************************************************/

#define  IniNrOfScreens     4  
#define  ReleaseAllocation  -1
#define  NextScreen          0

#define  PollTime          10

#define RECEIVE_BYTES     2  /* int value (4 Bytes) for x, y, dx, dy, incrx, incry */
#define SERVER_PORT     2002  /* waiting at port 2002 for any connection */
#define MEMORY        500000  /* memory to store one image */

/***************************************************************************************************************************/


static int *Screen,*ScrAdr;

void calculation   (image* area);
int  ScreenBuffer  (int mode,image* area);

void TCP_trans(void);
void timedely_ms(int delay);//delay单位为ms，应确保<1000
/***************************************************************************************************************************/

void main(void)
{
  int   TimeAllMs,result;
  char  key=0; 
  image area;
  vmode(vmStill);

  /* ini Screen buffer (mode>0) */
  result = ScreenBuffer(IniNrOfScreens,&area);
  if (result < 0) {print("Screen Allocation Error %d\n",result); return;}

 
  print("Press 'q' to quit\n"); 
  print("Press any key to start\n"); 
  getchar();
  

  TCP_trans();
  /* release screen allocation (mode<0) */
  ScreenBuffer(ReleaseAllocation,&area);
  
  }

/***************************************************************************************************************************/


/***************************************************************************************************************************/

int ScreenBuffer (int mode,image* area)
  {
  int i, exp=200, gain=0x6000;
  static int NrOfScreens=0, *TrackNr, disp, calc, capt;
  image Iniarea;

  /* only possible in vmStill mode */
  vmode(vmStill);

  /* screen allocation mode */
  if (mode>0)
    {
    if (NrOfScreens != 0) return(-5);
    if (mode<3)  return(-4);
    if (mode>60) return(-4);
    
    NrOfScreens=mode;

    ScrAdr=(int *)sysmalloc(NrOfScreens,2); 
    if((int)ScrAdr==0) 
      {
      return(-1);
      }

    Screen=(int *)sysmalloc(NrOfScreens,2); 
    if((int)Screen==0) 
      {
      sysfree((void *)ScrAdr);
      return(-1);
      }

    TrackNr=(int *)sysmalloc(NrOfScreens,2); 
    if((int)TrackNr==0) 
      {
      sysfree((void *)Screen);
      sysfree((void *)ScrAdr);
      return(-1);
      }

    Screen[0]=ScrGetCaptPage;
    for (i=1; i<NrOfScreens; i++)  
      {
      ScrAdr[i]=(int)DRAMDisplayMalloc(); 
      if (ScrAdr[i]==0)
        {
        for (i-=1; i>0; i--)  
          {
          DRAMPgFree(ScrAdr[i]);
          }
        sysfree((void *)TrackNr);
        sysfree((void *)Screen);
        sysfree((void *)ScrAdr);
        return(-1);
        }

      /* Image display addresses have to be in alignments of 1024 */ 
      Screen[i]=(ScrAdr[i]&0xFFFFFC00) + 1024;

     
      ImageAssign(&Iniarea, (long)Screen[i], DispGetColumns, DispGetRows, ScrGetPitch);  
      set(&Iniarea,0);
      }

    for (i=0; i<NrOfScreens-2; i++)  
      {
      TrackNr[i]=capture_request(exp,gain,(int *)Screen[i],0);
      if(TrackNr[i]==0) return(-2);
      }
    calc=0;
    disp=NrOfScreens-1;
    capt=NrOfScreens-2;
    }
    
  /* change screen for display, calculation and acquisition */
  if (mode==0)
    {
    if (NrOfScreens==0) return(-3);

    /* waiting for complete image */
    while(TrackNr[calc] > getvar(IMGREADY));

    TrackNr[capt]=capture_request(exp,gain,(int *)Screen[capt],0);
    if(TrackNr[capt]==0) return(-2);

    ScrSetDispPage(Screen[disp]);
    ScrSetLogPage (Screen[calc]);
    ImageAssign(area, (long)Screen[calc], DispGetColumns, DispGetRows, ScrGetPitch);
    capt=disp;
    disp=calc;
    calc++;
    if (calc==NrOfScreens) calc=0;
    }


  /* release screen allocation mode */
  if (mode<0)
    {
    if (NrOfScreens==0) return(-3);
 
    ScrSetPhysPage(Screen[0]);

    for (i=NrOfScreens-1; i>0; i--)  
      {
      DRAMPgFree(ScrAdr[i]);
      }

    NrOfScreens=0;

    sysfree((void *)TrackNr);
    sysfree((void *)Screen);
    sysfree((void *)ScrAdr);
    
    ScrSetLogPage(Screen[capt]);
    vmode(vmLive);
    }
    
    
  return(0);
  }

/***************************************************************************************************************************/
void TCP_trans(void)
{
	unsigned char *linebuffer, *plinebuf, *pixel;
	char  receivedata[RECEIVE_BYTES];
	int  i, j, x, y, dx, dy, incrx, incry, result, ms, count, ScreenIndex = 0;
	long sec;
	sockaddr_in    laddr, raddr;
	unsigned       sock, listensock;
	unsigned       error;
	uint_16        rlen;
	
	image area;
	char  key = 0;


	/* transfer the image from the physical page */
	// ScrSetLogPage((int)ScrGetPhysPage);


	/* temporarily work around */
	//  tpict();
	// setvar(DISP_ACTIVE,0);


	/* allocate memory for image raw data */
	linebuffer = (unsigned char *)sysmalloc(MEMORY, 3);
	if ((int)linebuffer == 0) print("not enough memory\n");


	/* server services port */
	laddr.sin_family = AF_INET;
	laddr.sin_port = SERVER_PORT;   /* server listen at port SERVER_PORT (2002) */
	laddr.sin_addr.s_addr = INADDR_ANY;    /* every IP addr can connect to the camera */


	/* use TCP/IP protocoll and search for a free port */
	sock = socket_stream();

	if (sock == VCRT_HANDLE_ERROR)
	{
		print("\nCreate stream socket failed");
		return;
	}

	/* connection between free port and selected SERVER_PORT. Now SERVER_PORT is free again for new connections. */
	error = bind(sock, &laddr, sizeof(laddr));

	if (error != VCRT_OK)
	{
		print("\nStream bind failed - 0x%x", error);
		return;
	}

	/* activate sock for TCP/IP connections */
	error = listen(sock, 0);

	if (error != VCRT_OK)
	{
		print("\nListen failed - 0x%x", error);
		return;
	}

	listensock = sock;

	print("\n\nFast Ethernet port %d\n", laddr.sin_port);

	for (;;) {

		/* Are there connections at any port? The number 0 means: Wait until connection */
		sock = VCRT_selectall(0);

		/* Is there one who wants to connect to port SERVER_PORT */
		if (sock == listensock)
		{
			/* Connection requested; accept it */
			rlen = sizeof(raddr);

			/* accept connection */
			print("\nwait for accept");
			sock = accept(listensock, &raddr, &rlen);
			if (sock == VCRT_HANDLE_ERROR)
			{
				print("\nAccept failed, error 0x%x", VCRT_geterror(listensock));
				continue;
			}
			else
			{
				print("\naccept ready");
			}

			print("\nConnection from %d.%d.%d.%d, port %d",
				(raddr.sin_addr.s_addr >> 24) & 0xFF,
				(raddr.sin_addr.s_addr >> 16) & 0xFF,
				(raddr.sin_addr.s_addr >> 8) & 0xFF,
				raddr.sin_addr.s_addr & 0xFF,
				raddr.sin_port);

			while(1)
			{
				print("R=%d ", ScreenBuffer(NextScreen, &area));
				
				recv(sock, (char *)receivedata, RECEIVE_BYTES, 0); 
				if (kbhit())
				{
					key = rs232rcv();
					if (key == 'q')break;
				}
				if(receivedata[0]=='T'&&receivedata[1]=='C')
				{
					print("TC");
					
//					do
//					{
						print("\n\nWaiting for image size");

						x = 0;
						y = 0;
						dx = 600;
						dy = 480;
						incrx = 1;
						incry = 1;

						count = recv(sock, (char *)receivedata, RECEIVE_BYTES, 0);
						ScreenIndex = 0;

						if (count == VCRT_ERROR)
							print("\nVCRT_ERROR 0x%x", VCRT_geterror(sock));
						else
							if (count < RECEIVE_BYTES)
								print("\nonly %d bytes read", count);
							else /* no errors*/
							{
								ScreenIndex = receivedata[0];
							}

						/* dx=0 : quit program */
						if (ScreenIndex != 'e')
						{
							// tpict();
							//ImageAssign(&trans_img, (long)Screen[ScreenIndex], DispGetColumns, DispGetRows, ScrGetPitch);
							print("\nTrains NO.%d image", (int)ScreenIndex);
						//	ScrSetLogPage(Screen[ScreenIndex]);
							ScrSetLogPage(Screen[1]);

							plinebuf = linebuffer; /* 4 Bytes for length and 24 Bytes for image data */

							count = 0;

							for (j = 0; j<dy; j += incry)
							{
								pixel = (unsigned char *)ScrByteAddr(x, y + j);

								for (i = 0; i<dx; i += incrx)
								{
									*plinebuf++ = *pixel;
									pixel += incrx;
									count++;
								}
							}

							/*      plinebuf=linebuffer;

							count+=RECEIVE_BYTES;
							pixel=(unsigned char *)&count;
							for(i=0;i<4;i++)
							{
							*plinebuf++ = *pixel++;
							}

							pixel=(unsigned char *)receivedata;
							for(i=0;i<RECEIVE_BYTES;i++)
							{
							*plinebuf++ = *pixel++;
							}
							*/
							sec = getvar(SEC);
							ms = getvar(MSEC);

							result = send(sock, (char *)linebuffer, count, 0);
							if (result == VCRT_ERROR)
							{
								print("\nSend VCRT_ERROR=%lx", VCRT_geterror(sock));
							}
							else
							{
								if (result != count)
									print("\nSend error wrong length=%d result=%d", count, result);
							}

							sec = getvar(SEC) - sec;
							ms = getvar(MSEC) - ms;

							ms = 1000 * (int)sec + ms;
							print(" \n trans t=%dms", ms);
						}
//					} while (ScreenIndex != 'e');
					continue;
				}
				
				
			}
			

			shutdown(sock, FLAG_CLOSE_TX);
		};

		break;

	} /* Endfor */
	shutdown(listensock, FLAG_CLOSE_TX);

	sysfree(linebuffer);


	/* activate image display */
	//  setvar(DISP_ACTIVE,1);


}

void timedely_ms(int delay)//delay单位为ms，应确保<1000
{
	int TimeAllMs,Timeinit;
	Timeinit = getvar(MSEC);
	do
	{
		TimeAllMs = getvar(MSEC)-Timeinit; 
    	if (TimeAllMs<0) TimeAllMs+=1000;
	}while(delay>=TimeAllMs);
}
