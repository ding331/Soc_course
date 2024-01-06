#include "xaxidma.h"
#include "xparameters.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xdebug.h"
#include "sleep.h"

//a[345600]未定位址，ILA TLAST無觸發	//345600*32bit < 2^24(DMA buffer)

#define frameSize 345600	//720*480

u32 checkHalted(u32 baseAddress,u32 offset);
	XAxiDma myDma;
	XAxiDma myDma2;

int main(){

	static u32 a[frameSize] = {0};	// = {1,2,3,4,5,6,7,8, 1,2,3,4,5,6,7,8, 1,2,3,4,5,6,7,8, 1,2,3,4,5,6,7,8};

	u32 b[frameSize];
    u32 status;

	XAxiDma_Config *myDmaConfig;
	XAxiDma_Config *myDmaConfig2;
	myDmaConfig = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_1_BASEADDR);
	status = XAxiDma_CfgInitialize(&myDma, myDmaConfig);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
	myDmaConfig2 = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);

	status = XAxiDma_CfgInitialize(&myDma2, myDmaConfig2);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
	XAxiDma_IntrDisable(&myDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&myDma2, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DEVICE_TO_DMA);
        //初始化DMA，初不初始化無所謂，跟進去就知道本質都在寫寄存器
	XAxiDma_Reset(&myDma);
	XAxiDma_Reset(&myDma2);
	/* Enable all interrupts */
	XAxiDma_IntrEnable(&myDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrEnable(&myDma2, XAXIDMA_IRQ_ALL_MASK,	XAXIDMA_DEVICE_TO_DMA);


    while(!XAxiDma_ResetIsDone(&myDma)&&!XAxiDma_ResetIsDone(&myDma2));
        //打印寄存器
        //0x4040 0000是DMA的基地址，在block design的address中可以看到，後面是偏移地址，
        //window->address editor
	xil_printf("CR is %d\r\n", Xil_In32(0x40400000 + 0x30));
	xil_printf("SR is %d\r\n", Xil_In32(0x40400000 + 0x34));
	xil_printf("LENGTH is %d\r\n", Xil_In32(0x40400000 + 0x58));
    //     //開始傳輸
	// Xil_Out32((0x40400000 + 0x30),1); //start dma
	// Xil_Out32((0x40400000 + 0x48),XPAR_AXI_DMA_0_BASEADDR); //set da address
	// Xil_Out32((0x40400000 + 0x58),16383); //set length

//	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR,0x4);
//	xil_printf("Status before data transfer %0x\n",status);

	for (int Index = 0; Index < 5; Index ++) {
	Xil_DCacheFlushRange((u32)a[0],frameSize);

	XAxiDma_SimpleTransfer(&myDma2, (u32)b[0], frameSize*sizeof(u32),XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_SimpleTransfer(&myDma, (u32)a[0], frameSize*sizeof(u32),XAXIDMA_DMA_TO_DEVICE);//typecasting in C/C++

	Xil_DCacheFlushRange((u32)b[0],frameSize);
//再次打印，看寄存器的狀態
	xil_printf("CR is %d\r\n", Xil_In32(0x40400000 + 0x30));
	xil_printf("SR is %d\r\n", Xil_In32(0x40400000 + 0x34));
	xil_printf("LENGTH is %d\r\n", Xil_In32(0x40400000 + 0x58));

	while ((XAxiDma_Busy(&myDma2, XAXIDMA_DEVICE_TO_DMA))||(XAxiDma_Busy(&myDma, XAXIDMA_DMA_TO_DEVICE)))
		{};
		XAxiDma_Reset(&myDma);
		XAxiDma_Reset(&myDma2);
	}

	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
    	print("DMA initialization success..\n");

	print("DMA transfer success..\n");

}


u32 checkHalted(u32 baseAddress,u32 offset){
	u32 status;
	status = (XAxiDma_ReadReg(baseAddress,offset))&XAXIDMA_HALTED_MASK;
	return status;
}
