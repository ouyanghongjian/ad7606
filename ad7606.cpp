
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include "ad7606.h"



#define DllExport

#define ARRAY_SIZE  (1000)

//#define DEVICE_CTX(dev) ((dev)->ctx)
//#define HANDLE_CTX(handle) (DEVICE_CTX((handle)->dev))


void sync_transfer_wait_for_completion(struct libusb_transfer *transfer);
void GetSampleData(int readsize);
bool UIS_Rx(HANDLE pDevHand,BYTE* pbyRxBuf,DWORD wRxSize,WORD wTimeOut);
bool UIS_Tx(HANDLE pDevHand,BYTE* pbyTxBuf,DWORD wTxSize,WORD wTimeOut);

DEV_STATUS *theseDev = NULL;
libusb_context *ctx;
ADC_RESULT *adc_results;
int id = 0;


static void milliseconds_delay(unsigned long mSec){
    struct timeval tv;
    tv.tv_sec=mSec/1000;
    tv.tv_usec=(mSec*1000)%1000000;
    int err;
    do{
        err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}


bool UIS_Rx(HANDLE pDevHand,BYTE* pbyRxBuf,DWORD wRxSize,WORD wTimeOut)
{
    int r;
    int rx = 0;
    //printf( "%s\n","UIS_Rx");
    r = libusb_bulk_transfer(pDevHand,BULK_IN_EP,pbyRxBuf,(int)wRxSize,&rx,wTimeOut);
    return !r;
}

bool UIS_Tx(HANDLE pDevHand,BYTE* pbyTxBuf,DWORD wTxSize,WORD wTimeOut)
{
    int r;
    int tx = 0;
    //printf( "%s\n","UIS_Tx");
    r = libusb_bulk_transfer(pDevHand,BULK_OUT_EP,pbyTxBuf,(int)wTxSize,&tx,wTimeOut);
    return !r;
}


void M3F20xm_CancelInterruptTransfer(libusb_transfer* pIntTrans)
{
  	if(pIntTrans)
  	{
      	libusb_cancel_transfer(pIntTrans);
  	}
}

HANDLE  M3F20xm_OpenDevice(libusb_context *pCtx)
   /*++
Routine Description:
   open a USBIO dev
Arguments:
   void
Return Value:
  a usb dev index
--*/
{
    int r;
	HANDLE pDevHand;
    printf( "%s\n","M3F20xm_USBIO_OpenDevice");
    pDevHand = libusb_open_device_with_vid_pid(pCtx,USBD_VID,USBD_PID);
   	if(pDevHand==NULL)
    {
       printf("libusb_open_device_with_vid_pid error\n");
       return NULL;
     }

    r=libusb_claim_interface(pDevHand,0);
    if(r<0)
    {
        printf("cannot claim interface\n");
		return NULL;
    }

   return pDevHand;
}

int  M3F20xm_CloseDevice()
   /*++
Routine Description:
   close a USBIO dev
Arguments:
   byIndex - device No.
Return Value:
  none
--*/
{
   int re ;
   printf( "USBIO %s\n","M3F20xm_CloseDevice");

   M3F20xm_CancelInterruptTransfer(theseDev->pIntTrans);

   
   libusb_release_interface(theseDev->pHandle,0);
   //libusb_attach_kernel_driver(pDevHand,0);
   libusb_close(theseDev->pHandle);

   return 0;

}



int M3F20xm_ADCSetConfig(HANDLE pDevHand, ADC_CONFIG* pCfg)
{
 	printf("%s\n", "M3F20xm_ADCSetConfig");
	int bResult;
	BYTE buff[64];
	buff[0] = IOCTL_SET_ADC >> 8;
	buff[1] = IOCTL_SET_ADC & 0xFF;

	memcpy(&buff[8], pCfg, sizeof(ADC_CONFIG));
	bResult = UIS_Tx(pDevHand, buff, 64, 100);
	return bResult;
}

int M3F20xm_ADCGetConfig(HANDLE pDevHand, ADC_CONFIG* pCfg)
{
	printf("%s\n","M3F20xm_ADCGetConfig");
	int bResult;
	BYTE buff[64];
	buff[0] = IOCTL_GET_ADC >> 8;
	buff[1] = IOCTL_GET_ADC & 0xFF;
	bResult = UIS_Tx(pDevHand, buff, 64, 100);
	if (!bResult)
	{
		printf("%s fail\n", "UIS_Tx");		
		return FALSE;
	}
	bResult = UIS_Rx(pDevHand, buff, 64, 100);
	if (bResult && buff[0] == DEV_ADC)
	{
		//*pbySelect         = buff[1];		
		memcpy(pCfg, &buff[1], sizeof(ADC_CONFIG));
	}
	else
	{
		bResult = false;
		printf("%s fail\n", "UIS_Rx");
	}
	return bResult;
}



int  M3F20xm_ADCStart(HANDLE pDevHand)
{
  	//printf( "%s\n","M3F20xm_ADCStart");
 	BYTE buff[64]; 	
	buff[0] = DEV_ADC;
	buff[1] = OP_START;
 
	if (!UIS_Tx(pDevHand, buff, 64, 100))
	{
		printf("M3F20xm_ADCStart UIS_Tx fail\n");
		return false;
	}
   	return true;
 }


int M3F20xm_ADCStop(HANDLE pDevHand)
{
    bool bResult;
    //printf( "%s\n","M3F20xm_ADCStop");
  	BYTE buff[64];
	buff[0] = DEV_ADC;
	buff[1] = OP_STOP;
	
	if (!UIS_Tx(pDevHand, buff, 64, 100))
	{
		printf("M3F20xm_ADCStop UIS_Tx fail\n");
		return false;
	}
    return true;
}


int M3F20xm_ADCRead(HANDLE pDevHand,WORD* lpReadBuffer)
{
  printf("%s++\n", "M3F20xm_ADCRead");
	//	bool bResult;
	BYTE buff[64];
	buff[0] = DEV_ADC;
	buff[1] = OP_READ;
	buff[2] = 0;
	buff[3] = 0;
	buff[4] = 0;
	buff[5] = 0;
	buff[6] = 0;
	buff[7] = 0;
	if (!UIS_Tx(pDevHand, buff, 64, 100))
	{
		printf("UIS_Tx fail\n");
		return false;
	}
	if (!UIS_Rx(pDevHand, (BYTE*)lpReadBuffer, 16, 100))
	{
		printf("UIS_Rx fail\n");
		return false;
	}
	return true;
}


void  AsyncTransferCallBack(struct libusb_transfer *transfer)
{
    //printf("AsyncTransferCallBack\n");

    int *completed = (int*)transfer->user_data;
    *completed = 1;
    //printf("actual_length=%d\n", transfer->actual_length);
}


void *IntReadThread(void *arg)
{
    int r;
    int readSize;    
	int completed = 0;
	//printf("IntReadThread\n");
	
    HANDLE pDev = theseDev->pHandle;
    theseDev->pIntTrans = libusb_alloc_transfer(0);
	if (!theseDev->pIntTrans)
	{
		printf("libusb_alloc_transfer fail\n");
		return 0;
	}
	libusb_fill_bulk_transfer(theseDev->pIntTrans, pDev, INT_IN_EP, (BYTE*)theseDev->RxBuf, TRIG_SIZE,AsyncTransferCallBack, &completed, 0);
	theseDev->pIntTrans->type = LIBUSB_TRANSFER_TYPE_INTERRUPT;

	r = libusb_submit_transfer(theseDev->pIntTrans);
	if (r < 0) {
		libusb_free_transfer(theseDev->pIntTrans);
		theseDev->pIntTrans = NULL;
		return 0;
	}
	//printf("Enter loop \n");
	
   	while(1)
   	{
 		sync_transfer_wait_for_completion(theseDev->pIntTrans);
 		readSize = theseDev->pIntTrans->actual_length;
 		//printf("readsize =%d,status = %d\n",readSize,theseDev->pIntTrans->status);
 		switch (theseDev->pIntTrans->status)
		{
 			case LIBUSB_TRANSFER_COMPLETED:
 				completed = 0;
 				GetSampleData(readSize);
 				r = libusb_submit_transfer(theseDev->pIntTrans);
 				if (r < 0) 
 					break;
 		 		else
 					continue;
 			case LIBUSB_TRANSFER_TIMED_OUT:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_TIMEOUT;
 				break;
 			case LIBUSB_TRANSFER_STALL:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_PIPE;
 				break;
 			case LIBUSB_TRANSFER_OVERFLOW:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_OVERFLOW;
 				break;
 			case LIBUSB_TRANSFER_NO_DEVICE:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_NO_DEVICE;
 				break;
 			case LIBUSB_TRANSFER_ERROR:
 			case LIBUSB_TRANSFER_CANCELLED:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_IO;
 				break;
 			default:
				printf("status = %d\n",theseDev->pIntTrans->status);
 				r = LIBUSB_ERROR_OTHER;
 		}
 
 		libusb_free_transfer(theseDev->pIntTrans);
 		theseDev->pIntTrans = NULL;
 		break;
  	}
}

void sync_transfer_wait_for_completion(struct libusb_transfer *transfer)
{
	int r;
	struct timeval tv;
	
	int* completed = (int*)transfer->user_data;

	//printf("sync_transfer_wait_for_completion\n");

	while (!*completed) 
	{
		//r = libusb_handle_events_completed(ctx, completed);
		tv.tv_sec = 0;
		tv.tv_usec = 50000;
		r = libusb_handle_events_timeout_completed(ctx, &tv, completed);
		if (r < 0) {
			if (r == LIBUSB_ERROR_INTERRUPTED)
				continue;
			printf("libusb_handle_events failed, cancelling transfer and retrying");
			libusb_cancel_transfer(transfer);
			continue;
		}
	}
}

#if 1
void GetSampleData(int readsize)
{
	float realVol,MaxVol;

	//printf("GetSampleData\n");
	if(readsize <= 0)
	{
		return;
	}
	
	if(theseDev->cfg.byADCOptions & 0x10)
   	{
   		MaxVol = 10;
   	}
    else
   	{
   		MaxVol = 5;
   	}

	for(int i = 0; i < readsize / 16; i++,theseDev->dwCycle++)
    {
         //printf("cycles %d   id %d   ",theseDev.dwCycle + i, id);
         for(int j = 0; j < 5; j++)
         {
         	if(theseDev->RxBuf[i*8+j]&0x8000)
         	{
         		theseDev->RxBuf[i*8+j] = ~theseDev->RxBuf[i*8+j];
         		realVol = -1 * MaxVol * (theseDev->RxBuf[i*8+j] + 1) / 32768;
         	} 
         	else
         	{
         		realVol = MaxVol * (theseDev->RxBuf[i*8+j] + 1) / 32768;
         	}
			if(theseDev->dwCycle < ARRAY_SIZE)
			{
				*((float *)(&adc_results[theseDev->dwCycle]) + j) = realVol;
			}
         	//printf("% 2.6f,   ",realVol);
         }
         //printf("\n");
    }
    //theseDev->dwCycle += readsize / 16;
    if(theseDev->cfg.dwMaxCycles && theseDev->cfg.dwMaxCycles == theseDev->dwCycle)
    {  
    	theseDev->sampeled = 0; 
        //printf("sample is end!\n");
   	}		
}
#endif

#if 0
void GetSampleData(int readsize)
{
	float realVol,MaxVol;

	if(readsize <= 0)
	{
		return;
	}
	if(theseDev->cfg.byADCOptions & 0x10)
   	{
   		MaxVol = 10;
   	}
    else
   	{
   		MaxVol = 5;
   	}

	for(int i = 0; i < readsize / 16; i++)
    {
         printf("cycles %d   ",theseDev->dwCycle + i);
         for(int j = 0; j < 8;j++)
         {
         	if(theseDev->RxBuf[i*8+j]&0x8000)
         	{
         		theseDev->RxBuf[i*8+j] = ~theseDev->RxBuf[i*8+j];
         		realVol = -1 * MaxVol * (theseDev->RxBuf[i*8+j] + 1) / 32768;
         	} 
         	else
         	{
         		realVol = MaxVol * (theseDev->RxBuf[i*8+j] + 1) / 32768;
         	}
         	printf("% 2.6f,   ",realVol);
         }
         printf("\n");
    }
    theseDev->dwCycle += readsize / 16;
    if(theseDev->cfg.dwMaxCycles && theseDev->cfg.dwMaxCycles == theseDev->dwCycle)
    {    
		theseDev->sampeled = 0; 
        printf("sample is end!\n");
   	}
	printf("cfg.dwMaxCycles: %d  dwCycle: %d\n", theseDev->cfg.dwMaxCycles, theseDev->dwCycle);
	milliseconds_delay(2);
}
#endif



extern "C" { 


int init_m3f20xm()
{
	int r;
	HANDLE pDevHandle = NULL;


	adc_results = (ADC_RESULT *)malloc(sizeof(ADC_RESULT)*ARRAY_SIZE);
	if(adc_results == NULL)
	{
		printf("malloc adc_results failed\n");
		return -1;
	}

	theseDev = (DEV_STATUS *)malloc(sizeof(DEV_STATUS));
	if(theseDev == NULL)
	{
		printf("malloc theseDev failed\n");
		return -1;
	}

	r= libusb_init(&ctx);
   	if(r<0)
   	{
       printf("libusb_init error\n");
       return -1;
   	}
   	else
   	{
       libusb_set_debug(ctx,3);
   	}
    theseDev->pHandle = M3F20xm_OpenDevice(ctx);
    if(theseDev->pHandle == NULL)
    {
		printf("M3F20xm_OpenDevice fail\n");
		libusb_exit(ctx);	
        return -1;
	}

   	theseDev->sampeled = 0;
   	theseDev->a_thread = -1;
   	theseDev->dwCycle = 0;

   	r = pthread_create(&theseDev->a_thread, NULL, IntReadThread, NULL);
   	if (r != 0)
   	{
      	printf( "Create Thread_main failed\n");
      	M3F20xm_CloseDevice();
      	libusb_exit(ctx);
      	return -1;
   	} 

	theseDev->cfg.byADCOptions |= 0x10;     //
  	theseDev->cfg.byTrigOptions |= 0x01;    //period sample
  	theseDev->cfg.wTrigSize = TRIG_SIZE;
  	theseDev->cfg.wPeriod = 100;    //100us
  	theseDev->cfg.dwMaxCycles = ARRAY_SIZE; //continueous sampling
	if(!M3F20xm_ADCSetConfig(theseDev->pHandle,&theseDev->cfg))
	{
		printf("call M3F20xm_ADCSetConfig fail\n"); 
		M3F20xm_CloseDevice();
		libusb_exit(ctx);
		return -1; 
	}	
	/*
	 M3F20xm_ADCGetConfig(theseDevs[dev1].pHandle,&theseDevs[dev1].cfg);
	 printf("dev%d-ADC Op = %02x\n",dev1,theseDevs[dev1].cfg.byADCOptions);
	 printf("dev%d-ADC Trigop = %02x\n",dev1,theseDevs[dev1].cfg.byTrigOptions);
	 printf("dev%d-ADC wPeriod = %d\n",dev1,theseDevs[dev1].cfg.wPeriod);
	 printf("dev%d-ADC wTrigSize = %d\n",dev1,theseDevs[dev1].cfg.wTrigSize);
	 */
  	return 0;
}


void stop_adc()
{
	if(theseDev == NULL || theseDev->pHandle == NULL)
	{
		printf("theseDev or pHandle is NULL\n");
		return;
	}

	M3F20xm_ADCStop(theseDev->pHandle);
}


void release_m3f20xm()
{
	if(theseDev == NULL || theseDev->pHandle == NULL)
	{
		printf("theseDev or pHandle is NULL\n");
		return;
	}

	M3F20xm_CloseDevice();
	libusb_exit(ctx);
}


ADC_RESULT * get_adc_value()
{
	struct timeval tv;

	if(adc_results == NULL || theseDev == NULL || theseDev->pHandle == NULL)
	{
		printf("theseDev or pHandle is NULL\n");
		return adc_results;
	}

	while(1)
	{
		if(!theseDev->sampeled)
		{	
			id = 0;
			theseDev->dwCycle = 0;
			theseDev->sampeled = 1;
			//gettimeofday(&tv,NULL);
			//printf("tv.tv_sec:%lu tv.tv_usec:%lu\n", tv.tv_sec, tv.tv_usec);
			M3F20xm_ADCStart(theseDev->pHandle);
			break;
		}
		else
		{			
			milliseconds_delay(2);
		}
	}

	while(1)
	{
		if(theseDev->sampeled)
		{
			milliseconds_delay(2);
		}
		else
		{
			//gettimeofday(&tv,NULL);
			//printf("tv.tv_sec:%lu tv.tv_usec:%lu\n", tv.tv_sec, tv.tv_usec);
			break;
		}
	}
	return adc_results;
}


}

#if 0
int main(int argc, char *argv[])
{
	int r;
	ADC_RESULT *p;

	r = init_m3f20xm();
	if(r != 0)
	{
		exit(115);
	}

	for(int k=0; k<1; k++)
	{
		for(int j=0;j<1;j++)
		{

			p = get_adc_value();

			for(int i = 0; i<ARRAY_SIZE; i++)
			{
				printf("id: %d  ua: %2.6f  ub: %2.6f  uc: %2.6f  i0 %2.6f\n",i,(p+i)->ua,(p+i)->ub,(p+i)->uc,(p+i)->i0);
			}
			milliseconds_delay(10);
		}
		stop_adc();
		milliseconds_delay(100);
	}

	release_m3f20xm();
	milliseconds_delay(2000);
}
#endif




