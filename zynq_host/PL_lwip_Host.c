/***************************** Include Files *********************************/
#include "xaxidma.h"
#include "xparameters.h"
//DEFINE STATEMENTS TO INCREASE SPEED
#include "xdebug.h"
#include "sleep.h"
#include "ff.h"
#include "xscugic.h"

#undef LWIP_TCP
#undef LWIP_DHCP
#include <stdio.h>
#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/udp.h"
#include "xil_cache.h"

// Include our own definitions
//#include "includes.h"

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
#define BUFFER_BASE		(MEM_BASE_ADDR + 0x00451810)		//**

#define MAX_PKT_LEN		720*480*4	//0x20	//8* 4 = 16*2 byte
//720*480*4

#define POLL_TIMEOUT_COUNTER    1000000U
#define lwip_len    2264*24+4 +4 	//4+720*3+100 +4
#define btnMode 0   //PL switch
#define brief32_Mode 0   //lwip[32] or lwip[12] & lwip[20]

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
static UINT file_pointer = 0; // Keep track of the file pointer
// Function prototypes
u32 SD_Transfer_read(char *FileName, u32 DestinationAddress, UINT ByteLength);
static unsigned char value[691200];//2*imageSize]; // Adjust the size to match the desired read length

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

// Global variables for data flow
volatile u8		IndArrDone;
volatile u32		EthBytesReceived;
volatile u8		SendResults;
volatile u8   		DMA_TX_Busy;
volatile u8		Error;

// Global Variables for Ethernet handling
u16_t    		RemotePort = 7;
struct ip4_addr  	RemoteAddr;
struct udp_pcb 		send_pcb;

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
	static int row = 0;
	static int page = 1;
	static int detect_cnt = 0;
	static int lwip_cnt = 0;
	XAxiDma_Config *CfgPtr;
	XAxiDma_Config *CfgPtr2;
	int Status;
	int Index = 0;
	u8 *TxBufferPtr;
	u8 *RxBufferPtr;
	int TimeOut = POLL_TIMEOUT_COUNTER;

	TxBufferPtr = (u8 *)TX_BUFFER_BASE;
	RxBufferPtr = (u8 *)RX_BUFFER_BASE;
	// // Initialize memory to all zeros
	// for(Index = 0; Index < MAX_PKT_LEN; Index ++){
	// 	TxBufferPtr[Index] = 0x00;
	// 	RxBufferPtr[Index] = 0x00;
	// }
	u8 *lwip_buf;
	lwip_buf = (u8 *)BUFFER_BASE;
	u32 *RxBufferPtr_32;
//	u32 *lwip_buf_32;
	// lwip_buf_32 = (u32 *)BUFFER_BASE;

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
	IP4_ADDR(&ipaddr,  192, 168,   1, 100);		//192, 168, 178, 148
	IP4_ADDR(&netmask, 255, 255, 255,  0);//
	IP4_ADDR(&gw,      192, 168,   1,  1);		//192, 168, 178, 254

	IP4_ADDR(&RemoteAddr,  192, 168,  1, 77);	//**
	//IP4_ADDR(&Remotenetmask, 255, 255, 255,  0);
	//IP4_ADDR(&Remotegw,   192, 168,  1,  1);

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
	// XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
	// 		    XAXIDMA_DEVICE_TO_DMA);
	// XAxiDma_IntrDisable(&AxiDma2, XAXIDMA_IRQ_ALL_MASK,
	// 		    XAXIDMA_DMA_TO_DEVICE);

	SD_Init();
	u32 PL_FPS;

	//1st front frame
	SD_Transfer_read("road720.bin.bin", (u32)(value + imageSize), imageSize);
	file_pointer += imageSize;

	while (page < 62) {
		// row = 0;
		if ((page & 1U) == 0U)//(page & 0x1 = 0)	//if lowest but of u32 page is 0
		{
			SD_Transfer_read("road720.bin.bin", (u32)(value), imageSize);	//current
			for (int Index = 0; Index < MAX_PKT_LEN/4; Index += 1) {
				//lit endien
				TxBufferPtr[Index * 4] = value[Index];	// 嚙緹嚙箭嚙緻嚙箠
				// Assigning 0xFF to the second and third bytes of each u32 value
				TxBufferPtr[(Index * 4) + 1] = page & 0xFF; // 0xFF in the second byte
				TxBufferPtr[(Index * 4) + 2] = 0; // 0xFF in the third byte
				// Extractin(g 8 bits )from value[Index + 345600]
				TxBufferPtr[(Index * 4) + 3] = value[Index + 345600];// 345600~2*345600
				// if (print_flag == 1 && TxBufferPtr[Index * 4] != 0)	//嚙緹嚙瑾嚙箠 [7:0] != 0
				// 	xil_printf("&1st now frame[0] != at %x is %x \r\n", &TxBufferPtr[Index * 4], (u8)TxBufferPtr[Index * 4]);
				// 	print_flag = 0;
				// if (TxBufferPtr[(Index * 4) + 3] != 0)	//嚙箴嚙瑾嚙箠
				// 	xil_printf("&1st now frame[0] != at %x is %x \r\n", &TxBufferPtr[(Index * 4) + 3], (u8)TxBufferPtr[Index * 4 + 3]);
				// 	print_flag = 0;
			}
		}
		else
		{
			SD_Transfer_read("road720.bin.bin", (u32)(value + imageSize), imageSize);	//current
			for (int Index = 0; Index < MAX_PKT_LEN/4; Index += 1) {

				TxBufferPtr[Index * 4] = value[Index + 345600];// 嚙緹嚙箭嚙緻嚙箠 s_data[7:0]
				// Assigning 0xFF to the second and third bytes of each u32 value
				TxBufferPtr[(Index * 4) + 1] = page & 0xFF; // 0xFF in the second byte
				TxBufferPtr[(Index * 4) + 2] = 0; // 0xFF in the third byte
				// Extractin(g 8 bits )from value[Index + 345600]
				TxBufferPtr[(Index * 4) + 3] = value[Index];// s_data[31:24]
			// 	if (print_flag == 0 && TxBufferPtr[Index * 4] != 0)	//嚙緹嚙瑾嚙箠 [7:0] != 0
			// 		xil_printf("&2ed now [345600] at %x is %x \r\n", &TxBufferPtr[Index * 4], (u8)TxBufferPtr[Index * 4]);
			}
		}
		// xil_printf("(u8)frd[0] %x \r\n",(u8)value[0]);
		// xil_printf("&frd[0] %x \r\n",&value[0]);
		// xil_printf("&frd[345600] %x \r\n",&value[345600]);
		// Update the file pointer
		file_pointer += imageSize;
	/* Flush the buffers before the DMA transfer, in case the Data Cache
	 * is enabled
	 */
		Xil_DCacheFlushRange((UINTPTR)TxBufferPtr, MAX_PKT_LEN);
		// Xil_DCacheFlushRange((UINTPTR)RxBufferPtr, MAX_PKT_LEN);
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
					xil_printf("DMA fine\n");
					break;
				}
				TimeOut--;
				usleep(1U);
			}
			//			for(int i = 0; i < page; i++)
//			xil_printf("PL_FPS addr at 0x%x . PL_FPS(5ns*cycle): %x \n",
//					(RX_BUFFER_BASE + 720*480*4 -4), (PL_FPS)&0xFFFFF);//low 20 bit
			xil_printf("PL_FPS addr at 0x%x . PL_FPS(cycle): %x \n",
				(&(RxBufferPtr[720*480*4-8])), (RxBufferPtr[720*480*4-8])&0xFFFFF );//low 20 bit

			PL_FPS = (RxBufferPtr[720*480*4-8]&0xFFFFF);

			//tidy
			// xil_printf("DMA done");
		RxBufferPtr_32 = (u32 *)RX_BUFFER_BASE;		//isu
		u32 row_decnt = 0;
		u32 r_20bit = 0;	//match[39~20]
		for (row = 0; row < 480; row++) {   //ini row
            // RxBufferPtr_32 = (RX_BUFFER_BASE + (u32)(720* row));   //?
			// xil_printf("&RxBufferPtr_32 sh  %x \r\n", &RxBufferPtr_32);
			// xil_printf("RxBufferPtr_32 to  %x \r\n", RxBufferPtr_32);
			row_decnt = 0;
			for (int n = 0; n < 4; n++) {		//row
				lwip_buf[(row % 24)*2264 + n] = (uint8_t)((row >> 8*n) & 0xFF);          //
			}
			for (int n = 0; n < 100; n++) {		//match[40]
				lwip_buf[2264*(row% 24 +1) -100 + n] = 0;          //
			}
			for (int j = 0; j < 720 ; j++) {	//#define btnMode 0
				if (brief32_Mode == 1){		//ILA brief[64],
					if( RxBufferPtr_32[j+720*row] != RxBufferPtr_32[j+720*row + 1]){	//&& brief cnt < 2260/4
						lwip_buf[2264 * (row% 24) + 4 + 4*j]     =  (u8)((RxBufferPtr_32[j+720*row]>>24));
						lwip_buf[2264 * (row% 24) + 4 + 4*j + 1] =  (u8)((RxBufferPtr_32[j+720*row]>>16)& 0x0F);
						lwip_buf[2264 * (row% 24) + 4 + 4*j + 2] =  (u8)((RxBufferPtr_32[j+720*row]>>8)& 0x00F);
						lwip_buf[2264 * (row% 24) + 4 + 4*j + 3] =  (u8)((RxBufferPtr_32[j+720*row])& 0x000F);
//						xil_printf("page: %d, x:%d, y:%d, brf != 0 : %x \r\n", page, j, row+1, (RxBufferPtr_32[j+720*row + 1]) );
					}
				} else {
				if (btnMode == 0) {	//ORB ...444					//1 u32 = 1 pixel
						lwip_buf[2264 * (row% 24) + 4 + 3 * j] =     (u8)((RxBufferPtr_32[j+720*row] >> 24) & 0xF0); // 31 downto 28 bits with 4 bits 0
						lwip_buf[2264 * (row% 24) + 4 + 3 * j + 1] = (u8)((RxBufferPtr_32[j+720*row] >> 20) & 0x0F0); // 27 downto 24 bits with 4 bits 0
						lwip_buf[2264 * (row% 24) + 4 + 3 * j + 2] = (u8)((RxBufferPtr_32[j+720*row] >> 16) & 0x00F0); // 23 downto 20 bits with 4 bits 0
					}
				else {	// erosion, bypass DMA_TX[7:0], bypass DMA_TX[31:24]	ILA check null RTL
						lwip_buf[2264 * (row% 24) + 4 + 3 * j]     = (u8)((RxBufferPtr_32[j+720*row] >> 24) & 0xFF); // 31 downto 24
						lwip_buf[2264 * (row% 24) + 4 + 3 * j + 1] = (u8)((RxBufferPtr_32[j+720*row] >> 24) & 0xFF); // 31 downto 24
						lwip_buf[2264 * (row% 24) + 4 + 3 * j + 2] = (u8)((RxBufferPtr_32[j+720*row] >> 24) & 0xFF); // 31 downto 24
					}
				// 	2164~2264 2264+4+2160=4428~4528 4528+2164=6692
				//嚙磐 RxBufferPtr_32[j] 嚙踝蕭嚙瘠嚙踝蕭20bits != 0 嚙璀嚙篁嚙踝蕭C嚙踝蕭嚙箭嚙踝蕭}...
				if ( (((RxBufferPtr_32[j+720*row] << 12) & 0xFFFFF) != 0)
					&& ( (((RxBufferPtr_32[j+720*row] << 12) & 0xFFFFF) != 0) != r_20bit) //2->1->2
					&& ( (((RxBufferPtr_32[j+720*row+1])<< 12 & 0xFFC00)>>12 ) != 0 && (RxBufferPtr_32[j+720*row+1] << 22) != 0 )//1st frame match[20] er
	//				&& (row_decnt < 20)
					)
				{
					//嚙瘠嚙踝蕭u嚙踝蕭嚙�20 matchxy, 嚙踝蕭嚙磕嚙盤20嚙諉則嚙誼梧蕭
					lwip_buf[(2264* (row% 24) +1) -100 + row_decnt * 5]     = (u8)((RxBufferPtr_32[j+720*row] >> 12) & 0xFF); // 嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙箴8bits [20:12
					lwip_buf[(2264* (row % 24) +1) -100 + row_decnt * 5]     = (u8)((RxBufferPtr_32[j+720*row] >> 12) & 0xFF); // 嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙箴8bits [20:12
					lwip_buf[(2264* (row % 24) +1) -100 + row_decnt * 5 + 1] = (u8)((RxBufferPtr_32[j+720*row] >> 4) & 0xFF); // 嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭嚙踝蕭8bits
					lwip_buf[(2264* (row % 24) +1) -100 + row_decnt * 5 + 2] = (u8)(((RxBufferPtr_32[j+720*row] << 4) & 0xF0) | ((RxBufferPtr_32[j+720*row + 1] >> 16) & 0x0F)); // RxBufferPtr_32[j*2]嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙諒恬蕭4bits | (RxBufferPtr_32[j*2+1] & 0xFFFFF)嚙踝蕭嚙箴4bits
					lwip_buf[(2264* (row % 24) +1) -100 + row_decnt * 5 + 3] = (u8)((RxBufferPtr_32[j+720*row + 1] >> 8) & 0xFF); // 嚙踝蕭RxBufferPtr[j*2+1]嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭嚙踝蕭8bits
					lwip_buf[(2264* (row % 24) +1) -100 + row_decnt * 5 + 4] = (u8)(RxBufferPtr_32[j+720*row + 1] & 0xFF); // 嚙踝蕭RxBufferPtr[j*2+1]嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭8bits
					//matchxy 嚙踝蕭嚙衛湛蕭嚙箭480嚙踝蕭(嚙盤嚙誼梧蕭嚙踝蕭D) issue: 嚙瞋ini嚙赭為0
//					xil_printf("\n detect_cnt/20 : %x",(u32)(detect_cnt/20) );
//					lwip_buf[(2264* ((u32)(detect_cnt/20) % 24) +1) -100 + row_decnt * 5]     = (u8)((RxBufferPtr_32[j+720*row] >> 12) & 0xFF); // 嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙箴8bits [20:12
//					lwip_buf[(2264* ((u32)(detect_cnt/20) % 24) +1) -100 + row_decnt * 5 + 1] = (u8)((RxBufferPtr_32[j+720*row] >> 4) & 0xFF); // 嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭嚙踝蕭8bits
//					lwip_buf[(2264* ((u32)(detect_cnt/20) % 24) +1) -100 + row_decnt * 5 + 2] = (u8)(((RxBufferPtr_32[j+720*row] << 4) & 0xF0) | ((RxBufferPtr_32[j+720*row + 1] >> 16) & 0x0F)); // RxBufferPtr_32[j*2]嚙踝蕭嚙諒低20bits嚙踝蕭嚙賦的嚙諒恬蕭4bits | (RxBufferPtr_32[j*2+1] & 0xFFFFF)嚙踝蕭嚙箴4bits
//					lwip_buf[(2264* ((u32)(detect_cnt/20) % 24) +1) -100 + row_decnt * 5 + 3] = (u8)((RxBufferPtr_32[j+720*row + 1] >> 8) & 0xFF); // 嚙踝蕭RxBufferPtr[j*2+1]嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭嚙踝蕭8bits
//					lwip_buf[(2264* ((u32)(detect_cnt/20) % 24) +1) -100 + row_decnt * 5 + 4] = (u8)(RxBufferPtr_32[j+720*row + 1] & 0xFF); // 嚙踝蕭RxBufferPtr[j*2+1]嚙諒低20bits嚙踝蕭嚙賦的嚙踝蕭8bits
					if (row_decnt == 1)	//check lwip_buf[] with host.py
					{	// {x1, y1}, {x2, y2}
//						xil_printf("\n page: %d, cur_row: %d, \n pos: %d, %d, match: , %d, %d", page, row, (RxBufferPtr_32[j+720*row] >> 10) & 0x3FF, (RxBufferPtr_32[j+720*row] ) & 0x3FF,
//																						(RxBufferPtr_32[j+720*row+1] >> 10) & 0x3FF, (RxBufferPtr_32[j+720*row+1] & 0x3FF ) );
//						xil_printf("\n  : %x", lwip_buf[(2264* ((u32)(detect_cnt/20)% 24) +1) -100 + row_decnt * 5]);
//						xil_printf("\n  : %x", lwip_buf[(2264* ((u32)(detect_cnt/20)% 24) +1) -100 + row_decnt * 5+1]);
//						xil_printf("\n  : %x", lwip_buf[(2264* ((u32)(detect_cnt/20)% 24) +1) -100 + row_decnt * 5+2]);
//						xil_printf("\n  : %x", lwip_buf[(2264* ((u32)(detect_cnt/20)% 24) +1) -100 + row_decnt * 5+3]);
//						xil_printf("\n  : %x", lwip_buf[(2264* ((u32)(detect_cnt/20)% 24) +1) -100 + row_decnt * 5+4]);
					}

					r_20bit = ((RxBufferPtr_32[j+720*row +1]));// << 12) & 0xFFFFF);
					row_decnt += 1;
					detect_cnt += 1;
					}
				}
				lwip_buf[2264 * (row% 24 + 1) + 4] = PL_FPS;	//last u32 lwip_buf[] for FPS

			}

			Xil_DCacheFlushRange((UINTPTR)lwip_buf, lwip_len);	//(4 + 720*3 + 100)/4);	//*
			// xil_printf("...Xil_DCacheFlushRlwip_bufange");

			u32 *lwip_buf_32 = (u32*)BUFFER_BASE;	//lwip_buf_32 = (u32*)BUFFER_BASE;
		// }	//DDR send 2 host
			if ((row+1) % 24 == 0 )//24 row per pack
			{
				//  xil_printf("&lwip_buf[0] 0x %x \r\n\n",&lwip_buf[0]);

				// while (Error==0) {
				if (Error==0) {
					if (TcpFastTmrFlag) {
						tcp_fasttmr();
						TcpFastTmrFlag = 0;
						// SendResults = 1;
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
					// xil_printf("err echo");

					xemacif_input(echo_netif);
					// xil_printf("echo ...");

					/* Send results back from time to time */
					if (SendResults == 1){
						SendResults = 0;									//***
						// memcpy(&lwip_buf[0], &row, 4);
						psnd = pbuf_alloc(PBUF_TRANSPORT, (lwip_len)*sizeof(u8), PBUF_REF);

						psnd->payload = lwip_buf_32;//BUFFER_BASE;	//&lwip_buf_32;

						udpsenderr = udp_sendto(&send_pcb, psnd, &RemoteAddr, RemotePort);
						xil_printf(".");
						if (udpsenderr != ERR_OK){
							xil_printf("UDP Send failed with Error %d\n\r", udpsenderr);
							row -= 24;
						}
						else
						{
							if (lwip_cnt == 0)	//1st pac , Host lost
								{row -= 24;}
							lwip_cnt += 1;
						}
						pbuf_free(psnd);	//one page done

					}
					else
					{
						xil_printf("SendResults Error == 1\n");
						row -= 24;
					}
				}
				else
				{
					xil_printf("Error == 1");
					row -= 24;
				}
				// xil_printf("lwip_buf[1] 0x %x \r\n",lwip_buf[1]);
				// xil_printf("lwip_buf[2] 0x %x \r\n",lwip_buf[2]);
				//  xil_printf("lwip_buf[4] 0x %x \r\n", lwip_buf[4]);

				// xil_printf("row detect_Cnt = %x \r\n",detect_cnt);
				// detect_cnt = 0;
			}
			usleep(20000);	//pac lost per 20 time
		}
		xil_printf("page = %d \r\n",page);
		xil_printf("\n page lwip_cnt = %d \r\n",lwip_cnt);
		lwip_cnt = 0;
		xil_printf("page detect_Cnt = %d \r\n",detect_cnt);
		page+= 1;
		detect_cnt = 0;
	}
	xil_printf("tot page = %d \r\n",page);

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

void print_ip(char *msg, struct ip4_addr *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(struct ip4_addr *ip, struct ip4_addr *mask, struct ip4_addr *gw)
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

