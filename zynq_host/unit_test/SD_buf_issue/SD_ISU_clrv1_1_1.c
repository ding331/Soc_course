
/***************************** Include Files *********************************/
#include "xaxidma.h"
#include "xparameters.h"
#include "xdebug.h"
#include "sleep.h"
#include "ff.h"

#if defined(XPAR_UARTNS550_0_BASEADDR)
#include "xuartns550_l.h"       /* to use uartns550 */
#endif

/******************** Constant Definitions **********************************/

#ifndef SDT
#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID

#ifdef XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR		XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#elif defined (XPAR_MIG7SERIES_0_BASEADDR)
#define DDR_BASE_ADDR	XPAR_MIG7SERIES_0_BASEADDR
#elif defined (XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR)
#define DDR_BASE_ADDR	XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR
#elif defined (XPAR_PSU_DDR_0_S_AXI_BASEADDR)
#define DDR_BASE_ADDR	XPAR_PSU_DDR_0_S_AXI_BASEADDR
#endif

#else

#ifdef XPAR_MEM0_BASEADDRESS
#define DDR_BASE_ADDR		XPAR_MEM0_BASEADDRESS
#endif
#endif

#ifndef DDR_BASE_ADDR
#warning CHECK FOR THE VALID DDR ADDRESS IN XPARAMETERS.H, \
DEFAULT SET TO 0x01000000
#define MEM_BASE_ADDR		0x01000000
#else
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)
#endif
                                            //mem base is mtf important
#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00100000)   
#define RX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00300000)
#define RX_BUFFER_HIGH		(MEM_BASE_ADDR + 0x004FFFFF)

#define MAX_PKT_LEN		720*480*4	//0x20	//8* 4 = 16*2 byte
//720*480*4
#define TEST_START_VALUE	0xC

#define NUMBER_OF_TRANSFERS	10
#define POLL_TIMEOUT_COUNTER    1000000U

/************************** Function Prototypes ******************************/
#if (!defined(DEBUG))
extern void xil_printf(const char *format, ...);
#endif

#ifndef SDT
int XAxiDma_SimplePollExample(u16 DeviceId);
#else
int XAxiDma_SimplePollExample(UINTPTR BaseAddress);
#endif
static int CheckData(void);

/************************** Variable Definitions *****************************/
/*
 * Device instance definitions
 */
static FATFS fatfs;
#define imageSize 720*480 	//720*480
// Function prototype
static int SD_Init();

// Define a global variable to keep track of the file read pointer position
UINT file_pointer = 0; // Keep track of the file pointer

// Function prototypes
u32 SD_Transfer_read(char *FileName, u32 DestinationAddress, UINT ByteLength);

char value[691200];//2*imageSize]; // Adjust the size to match the desired read length

static u16 row = 0;
static u32 page = 0;
XAxiDma AxiDma;
XAxiDma AxiDma2;

int main()
{
	int Status;

	xil_printf("\r\n--- Entering main() --- \r\n");

	/* Run the poll example for simple transfer */
#ifndef SDT
	Status = XAxiDma_SimplePollExample(DMA_DEV_ID);
#else
	Status = XAxiDma_SimplePollExample(XPAR_XAXIDMA_0_BASEADDR);
#endif

	if (Status != XST_SUCCESS) {
		xil_printf("XAxiDma_SimplePoll Example Failed\r\n");
		return XST_FAILURE;
	}

	xil_printf("Successfully ran XAxiDma_SimplePoll Example\r\n");

	xil_printf("--- Exiting main() --- \r\n");

	return XST_SUCCESS;

}

#ifndef SDT
int XAxiDma_SimplePollExample(u16 DeviceId)
#else
int XAxiDma_SimplePollExample(UINTPTR BaseAddress)
#endif
{
	XAxiDma_Config *CfgPtr;
	XAxiDma_Config *CfgPtr2;
	int Status;
	int Tries = NUMBER_OF_TRANSFERS;
	int Index = 0;
	u8 *TxBufferPtr;
	u8 *RxBufferPtr;
	// u8 Value;
	int TimeOut = POLL_TIMEOUT_COUNTER;

	TxBufferPtr = (u8 *)TX_BUFFER_BASE;
	RxBufferPtr = (u8 *)RX_BUFFER_BASE;

	/* Initialize the XAxiDma device.
	 */
#ifndef SDT
	CfgPtr = XAxiDma_LookupConfig(DeviceId);	//RX
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}
	CfgPtr2 = XAxiDma_LookupConfig(XPAR_AXIDMA_1_DEVICE_ID);

	if (!CfgPtr2) {
		xil_printf("No config found for %d\r\n", XPAR_AXIDMA_1_DEVICE_ID);
		return XST_FAILURE;
	}
#else
	CfgPtr = XAxiDma_LookupConfig(BaseAddress);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", BaseAddress);
		return XST_FAILURE;
	}
	CfgPtr2 = XAxiDma_LookupConfig(BaseAddress);
#endif

	Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
 	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(&AxiDma2, CfgPtr2);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	if (XAxiDma_HasSg(&AxiDma)) {
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	/* Disable interrupts, we use polling mode
	 */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
			    XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma2, XAXIDMA_IRQ_ALL_MASK,
			    XAXIDMA_DMA_TO_DEVICE);

	SD_Init();

	SD_Transfer_read("road720.bin", (u32)(value + imageSize), imageSize);
        file_pointer += imageSize;  //**important
	//1st back frame
	while (page < 1) {
		row = 0;
			if ((page & 1U) == 0U)//(page & 0x1 = 0)	//if lowest but of u32 page is 0
			{
				SD_Transfer_read("road720.bin", (u32)(value), imageSize);	//current
			for (int Index = 0; Index < MAX_PKT_LEN; Index += 1) {
				//lit endien
				TxBufferPtr[Index * 4] = value[Index];// & 0xFF; // Extracting 8 bits from value[Index]
				// Assigning 0xFF to the second and third bytes of each u32 value
				TxBufferPtr[(Index * 4) + 1] = 0; // 0xFF in the second byte
				TxBufferPtr[(Index * 4) + 2] = 0; // 0xFF in the third byte
				// Extractin(g 8 bits )from value[Index + 345600]
				TxBufferPtr[(Index * 4) + 3] = value[Index + 345600];// & 0xFF;				
				}
			}
			else
			{
			SD_Transfer_read("road720.bin", (u32)(value + imageSize), imageSize);	//current
					// Divide u32 array into u8
					// for (Index = 0; Index < MAX_PKT_LEN; Index++) {
					// 	// Convert u32 value to little-endian and assign 0 to the second and third bytes
					// 	TxBufferPtr[0] = (uint8_t)(value & 0xFF);          // First byte (LSB)
					// 	TxBufferPtr[1] = (uint8_t)((value >> 8) & 0xFF);   // Second byte (0)
					// 	TxBufferPtr[2] = (uint8_t)((value >> 16) & 0xFF);  // Third byte (0)
					// 	TxBufferPtr[3] = (uint8_t)((value >> 24) & 0xFF);  // Fourth byte (MSB)
					// }
			}
			xil_printf("(u8)frd[0] %x \r\n",(u8)value[0]);
			xil_printf("&frd[0] %x \r\n",&value[0]);
			xil_printf(" frd[0]      & 0xFF %x \r\n", value[0] & 0xFF);
			xil_printf(" frd[0]>>8   & 0xFF %x \r\n", value[0]>>8 & 0xFF);
			xil_printf(" frd[0]>>16  & 0xFF %x \r\n", value[0]>>16 & 0xFF);
			xil_printf(" frd[0]>>24  & 0xFF %x \r\n", value[0]>>24 & 0xFF);
			xil_printf(" frd[1]      & 0xFF %x \r\n", value[1]    & 0xFF);
			xil_printf(" frd[1]>>8   & 0xFF %x \r\n", value[1]>>8& 0xFF);
			xil_printf(" frd[1]>>16  & 0xFF %x \r\n", value[1]>>16 & 0xFF);
			xil_printf(" frd[1]>>24  & 0xFF %x \r\n", value[1]>>24 & 0xFF);
			xil_printf("(u8*)frd[0] %x \r\n",(u8*)value[0]);
			xil_printf("frd[86400] %x \r\n",value[86400]);
			xil_printf("&frd[86400] %x \r\n",&value[86400]);
			// Update the file pointer
			file_pointer += imageSize;
	/* Flush the buffers before the DMA transfer, in case the Data Cache
	 * is enabled
	 */
		Xil_DCacheFlushRange((UINTPTR)TxBufferPtr, MAX_PKT_LEN);
		Xil_DCacheFlushRange((UINTPTR)RxBufferPtr, MAX_PKT_LEN);

		// for (Index = 0; Index < Tries; Index ++) {
			XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBufferPtr,
							MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);

			XAxiDma_SimpleTransfer(&AxiDma2, (UINTPTR) TxBufferPtr,
							MAX_PKT_LEN, XAXIDMA_DMA_TO_DEVICE);

			/*Wait till tranfer is done or 1usec * 10^6 iterations of timeout occurs*/
			while (TimeOut) {
				if (!(XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)) &&
					!(XAxiDma_Busy(&AxiDma2, XAXIDMA_DMA_TO_DEVICE))) {

					break;
				}
				TimeOut--;
				usleep(1U);
			}
		// }

		page+= 1;
	}

	/* Test finishes successfully
	 */
	return XST_SUCCESS;
}

static int CheckData(void)
{
	u8 *RxPacket;
	int Index = 0;
	u8 Value;

	RxPacket = (u8 *) RX_BUFFER_BASE;
	Value = TEST_START_VALUE;

	/* Invalidate the DestBuffer before receiving the data, in case the
	 * Data Cache is enabled
	 */
	Xil_DCacheInvalidateRange((UINTPTR)RxPacket, MAX_PKT_LEN);

	for (Index = 0; Index < MAX_PKT_LEN; Index++) {
		if (RxPacket[Index] != Value) {
			xil_printf("Data error %d: %x/%x\r\n",
				   Index, (unsigned int)RxPacket[Index],
				   (unsigned int)Value);

			return XST_FAILURE;
		}
		Value = (Value + 1) & 0xFF;
	}

	return XST_SUCCESS;
}
u32 SD_Transfer_read(char *FileName, u32 DestinationAddress, UINT ByteLength) {
    FIL fil;
    FRESULT rc;
    UINT br;

    f_open(&fil, FileName, FA_READ);

    // Move the file pointer
    f_lseek(&fil, file_pointer);

    // Read data from the file
    f_read(&fil, (void *) DestinationAddress, ByteLength, &br);

    // Close the file
    f_close(&fil);

    return rc;
}


int SD_Init()
{
    FRESULT rc;

    rc = f_mount(&fatfs,"",0);
    if(rc)
    {
        xil_printf("ERROR : f_mount returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}
//
//int SD_Transfer_read(char *FileName,u32 DestinationAddress,u32 ByteLength)
//{
//    FIL fil;
//    FRESULT rc;
//    UINT br;
//    u32 file_size = 0;
//    rc = f_open(&fil,FileName,FA_READ);
//    if(rc)
//    {
//        xil_printf("ERROR : f_open returned %d\r\n",rc);
//        return XST_FAILURE;
//    }
//
//	file_size = f_size(&fil);
//	printf("%lx\n", file_size);
//
//    rc = f_lseek(&fil, 0);
//    if(rc)
//    {
//        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
//        return XST_FAILURE;
//    }
//    rc = f_read(&fil, (void*)DestinationAddress,ByteLength,&br);
//    if(rc)
//    {
//        xil_printf("ERROR : f_read returned %d\r\n",rc);
//        return XST_FAILURE;
//    }
//    rc = f_close(&fil);
//    if(rc)
//    {
//        xil_printf(" ERROR : f_close returned %d\r\n", rc);
//        return XST_FAILURE;
//    }
//    return XST_SUCCESS, file_size;
//}

int SD_Transfer_write(char *FileName,u32 SourceAddress,u32 ByteLength)
{
    FIL fil;
    FRESULT rc;
    UINT bw;

    rc = f_open(&fil,FileName,FA_CREATE_ALWAYS | FA_WRITE);
    if(rc)
    {
        xil_printf("ERROR : f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_lseek(&fil, 0);
    if(rc)
    {
        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_write(&fil,(void*) SourceAddress,ByteLength,&bw);
    if(rc)
    {
        xil_printf("ERROR : f_write returned %d\r\n", rc);
        return XST_FAILURE;
    }
    rc = f_close(&fil);
    if(rc){
        xil_printf("ERROR : f_close returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}
