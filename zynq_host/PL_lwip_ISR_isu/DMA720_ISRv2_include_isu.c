
/***************************** Include Files *********************************/
#include "xaxidma.h"
#include "xparameters.h"
#include "xdebug.h"
#include "sleep.h"
#include "ff.h"
#include "xscugic.h"

/******************** Constant Definitions **********************************/
//DEFINE STATEMENTS TO INCREASE SPEED
#undef LWIP_TCP
#undef LWIP_DHCP
#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif
#include "lwip/udp.h"
#include "xil_cache.h"
// Include our own definitions
#include "includes.h"

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
/************************** Variable Definitions *****************************/
/*
 * Device instance definitions
 */
static FATFS fatfs;
#define imageSize 720*480	//720*480
// Function prototype
static int SD_Init();

// Define a global variable to keep track of the file read pointer position
UINT file_pointer = 0; // Keep track of the file pointer

// Function prototypes
u32 SD_Transfer_read(char *FileName, u32 DestinationAddress, UINT ByteLength);

char value[691200];//2*imageSize]; // Adjust the size to match the desired read length

static u32 row = 0;
static u32 page = 0;
XAxiDma AxiDma;
XAxiDma AxiDma2;
XScuGic IntcInstance;
//static void imageProcISR(void *CallBackRef);

/* defined by each RAW mode application */
void print_app_header();
int start_application();
//int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
void lwip_init();

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
/* set up netif stuctures */
static struct netif server_netif;
struct netif *echo_netif;

// Global Variables to store results and handle data flow
int			Centroid;

// Global variables for data flow
volatile u8		IndArrDone;
volatile u32		EthBytesReceived;
volatile u8		SendResults;
volatile u8   		DMA_TX_Busy;
volatile u8		Error;

// Global Variables for Ethernet handling
u16_t    		RemotePort = 8;
struct ip4_addr  	RemoteAddr;
struct udp_pcb 		send_pcb;

void
print_ip(char *msg, struct ip4_addr *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(struct ip4_addr *ip, struct ip4_addr *mask, struct ip4_addr *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

/* print_app_header: function to print a header at start time */
void print_app_header()
{
	xil_printf("\n\r\n\r------lwIP UDP GetCentroid Application------\n\r");
	xil_printf("UDP packets sent to port 7 will be processed\n\r");
}

int main()
{

	int Status = 0;

	// //Interrupt Controller Configuration
	// XScuGic_Config *IntcConfig;
	// IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	// status =  XScuGic_CfgInitialize(&IntcInstance, IntcConfig, IntcConfig->CpuBaseAddress);

	// if(status != XST_SUCCESS){
	// 	xil_printf("Interrupt controller initialization failed..");
	// 	return -1;
	// }

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
	struct ip4_addr ipaddr, netmask, gw /*, Remotenetmask, Remotegw*/;
	struct pbuf * psnd;
	err_t udpsenderr;
	int status = 0;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x10 };

	/* Use the same structure for the server and the echo server */
	echo_netif = &server_netif;
	init_platform();

	/* initialize IP addresses to be used */
	IP4_ADDR(&ipaddr,  10, 0,   0, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      10, 0,   0,  1);

	IP4_ADDR(&RemoteAddr,  10, 0,   0, 77);
	//IP4_ADDR(&Remotenetmask, 255, 255, 255,  0);
	//IP4_ADDR(&Remotegw,      10, 0,   0,  1);

	print_app_header();

	/* Initialize the lwip for UDP */
	lwip_init();

  	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(echo_netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(echo_netif);

	xil_printf("Zedboard IP settings: \r\n");
	print_ip_settings(&ipaddr, &netmask, &gw);
	xil_printf("Remote IP settings: \r\n");
	//print_ip_settings(&RemoteAddr, &Remotenetmask, &Remotegw);
	print_ip("Board IP: ", &RemoteAddr);

	/* start the application (web server, rxtest, txtest, etc..) */
	status = start_application();
	if (status != 0){
		xil_printf("Error in start_application() with code: %d\n\r", status);
		// goto ErrorOrDone;
	}
	xil_printf("\r\n---web done, Entering DMA() --- \r\n");

	static u8* lwip_buf;
	int detect_cnt = 0;
	XAxiDma_Config *CfgPtr;
	XAxiDma_Config *CfgPtr2;
	int Status;
	int Tries = NUMBER_OF_TRANSFERS;
	int Index = 0;
	u8 *TxBufferPtr;
	u8 *RxBufferPtr;
	u8 Value;
	int TimeOut = POLL_TIMEOUT_COUNTER;

	TxBufferPtr = (u8 *)TX_BUFFER_BASE;
	RxBufferPtr = (u8 *)RX_BUFFER_BASE;
	// Initialize memory to all zeros
	for(Index = 0; Index < MAX_PKT_LEN; Index ++){
		TxBufferPtr[Index] = 0x00;
		RxBufferPtr[Index] = 0x00;
	}

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
	//1st back frame
	while (page < 2) {	//&& send done
		row = 0;
			if ((page & 1U) == 0U)//(page & 0x1 = 0)	//if lowest but of u32 page is 0
			{
				SD_Transfer_read("road720.bin", (u32)(value), imageSize);	//current
					// Divide u32 array into u8
					for (Index = 0; Index < MAX_PKT_LEN; Index++) {
						if (Index % 4 == 0) {
							TxBufferPtr[Index] = (u8)(value[Index / 4] & 0xFF); // Extracting the lowest byte of each u32 value
						} else if (Index % 4 == 1 || Index % 4 == 2) {
							TxBufferPtr[Index] = (u8)0; // Assigning 0 to the second and third bytes of each u32 value
						} else if (Index % 4 == 3) {
							TxBufferPtr[Index] = (u8)(value[(Index + imageSize) / 4] & 0xFF); // Extracting the lowest byte of the next u32 value
						}
					}
				}
			else
			{
			SD_Transfer_read("road720.bin", (u32)(&value + imageSize), imageSize);	//current
					// Divide u32 array into u8
					for (Index = 0; Index < MAX_PKT_LEN; Index++) {
						if (Index % 4 == 0) {
							TxBufferPtr[Index] = (u8)(value[Index + imageSize / 4] & 0xFF); // Extracting the lowest byte of each u32 value
						} else if (Index % 4 == 1 || Index % 4 == 2) {
							TxBufferPtr[Index] = (u8)0; // Assigning 0 to the second and third bytes of each u32 value
						} else if (Index % 4 == 3) {
							TxBufferPtr[Index] = (u8)(value[(Index) / 4] & 0xFF); // Extracting the lowest byte of the next u32 value
						}
					}
			}
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
		// }	//DDR send 2 hos
			while (Error==0) {
				if (TcpFastTmrFlag) {
					tcp_fasttmr();
					TcpFastTmrFlag = 0;
					//SendResults = 1;
					//xil_printf("*");
				}
				if (TcpSlowTmrFlag) {
					tcp_slowtmr();
					TcpSlowTmrFlag = 0;
					SendResults = 1;
				}
				//xemacif_input(echo_netif);
				//transfer_data();
				/* Receive packets */
				xemacif_input(echo_netif);

				/* Send results back from time to time */
				if (SendResults == 1){
					SendResults = 0;
					psnd = pbuf_alloc(PBUF_TRANSPORT, (720+4+50)*sizeof(u32), PBUF_REF);
		            memcpy(lwip_buf, &row, 4);

			for (int pixel = 1; pixel < 721; ++pixel) {
				lwip_buf[3 * pixel + 1] = (RxBufferPtr[pixel] >> 28) & 0xFF;
				lwip_buf[3 * pixel + 2] = (RxBufferPtr[pixel] >> 20) & 0xFF;
				lwip_buf[3 * pixel + 3] = (RxBufferPtr[pixel] >> 12) & 0xFF;

				if ((RxBufferPtr[pixel] & 0xFFFFF) != 0) {
					lwip_buf[4 + 720 + detect_cnt] = (RxBufferPtr[pixel] & 0xFFFF) & 0xFF;
					detect_cnt += 1;
				}
			}

		            udpsenderr = udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);
					xil_printf(".");
					if (udpsenderr != ERR_OK){
						xil_printf("UDP Send failed with Error %d\n\r", udpsenderr);
					}
					pbuf_free(psnd);
				}
		    }
		page+= 1;
		detect_cnt = 0;
	}

	/* Test finishes successfully
	 */
	cleanup_platform();
	return XST_SUCCESS;
}

//u8* lwip_buf;
//static void imageProcISR((void) *CallBackRef){	//(u32*)&RX_buf[720]
//	static int detec_cnt = 0;
//	// xil_printf("v: %x\n\r", row);
//	XScuGic_Disable(&IntcInstance,XPAR_FABRIC_AXI_ORB720_0_ORB_INTR_INTR);
//
//    /* {u32 [720]  divide}*/
//
//    	/* receive and process packets */
//	while (Error==0) {
//		if (TcpFastTmrFlag) {
//			tcp_fasttmr();
//			TcpFastTmrFlag = 0;
//			//SendResults = 1;
//			//xil_printf("*");
//		}
//		if (TcpSlowTmrFlag) {
//			tcp_slowtmr();
//			TcpSlowTmrFlag = 0;
//			SendResults = 1;
//		}
//		//xemacif_input(echo_netif);
//		//transfer_data();
//		/* Receive packets */
//		xemacif_input(echo_netif);
//
//		/* Send results back from time to time */
//		if (SendResults == 1){
//			SendResults = 0;
//			psnd = pbuf_alloc(PBUF_TRANSPORT, (720+4+50)*sizeof(u32), PBUF_REF);
//            memcpy(lwip_buf, &row, 4);
//
//	for (int pixel = 1; pixel < 721; ++pixel) {
//		lwip_buf[3 * pixel + 1] = (RxBufferPtr[pixel] >> 28) & 0xFF;
//		lwip_buf[3 * pixel + 2] = (RxBufferPtr[pixel] >> 20) & 0xFF;
//		lwip_buf[3 * pixel + 3] = (RxBufferPtr[pixel] >> 12) & 0xFF;
//
//		if ((RxBufferPtr[pixel] & 0xFFFFF) != 0) {
//			lwip_buf[4 + 720 + detect_cnt] = (RxBufferPtr[pixel] & 0xFFFF) & 0xFF;
//			detect_cnt += 1;
//		}
//	}
//
//            udpsenderr = udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);
//			xil_printf(".");
//			if (udpsenderr != ERR_OK){
//				xil_printf("UDP Send failed with Error %d\n\r", udpsenderr);
//			}
//			pbuf_free(psnd);
//		}
//    }
//
//	// o_grw(7 downto 0) lw
//	row = row +1 ;
//	XScuGic_Enable(&IntcInstance,XPAR_FABRIC_AXI_ORB720_0_ORB_INTR_INTR);
//}

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
