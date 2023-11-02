#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_io.h"   //这个头文件下面包含很重要的IO读写函数

#include "xil_printf.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xgpio.h"

#include "sleep.h"
#define DDR_BASE_ADDRESS    0x00110100	//0x00100000
#define DDR_SIZE            3600000	//<16MB

//parameter definitions
#define INTC_DEVICE_ID    XPAR_PS7_SCUGIC_0_DEVICE_ID  //DEVICE_ID用來初始化函數，索引數組元素  XPAR_PS7_SCUGIC_0_DEVICE_ID = 0
//參數是系統的中斷的設備ID基地址的宏定義，也就是中斷的基地址
#define LED_DEVICE_ID     XPAR_AXI_GPIO_1_DEVICE_ID   //XPAR_AXI_GPIO_1_DEVICE_ID = 1
#define BTNS_DEVICE_ID    XPAR_AXI_GPIO_0_DEVICE_ID   //XPAR_AXI_GPIO_0_DEVICE_ID = 0
#define INTC_GPIO_INTERRUPT_ID    XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define BTN_INT    XGPIO_IR_CH1_MASK
#define DELAY 1000

XGpio LED;
XGpio BTNInst;
XScuGic    INTCInst;  //產生一箇中斷控制器實例結構體

//Function protype
static void BTN_Intr_Handler(void *baseaddr_p);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId,XGpio *GpioInstancePtr);

u32 offset = 0;
//static int btn_value;
u32 btn_value= 0;
void BTN_Intr_Handler(void *InstancePtr) //中斷處理函數
{
     unsigned char led_val=0;
//     xil_printf( "BTN pressed");
    //Ignore additional button presses
    if((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) != BTN_INT)
     {
        return;
       //Disable GPIO interrupts
        XGpio_InterruptDisable(&BTNInst,BTN_INT);
    }
    btn_value = XGpio_DiscreteRead(&BTNInst,1);	//從按鍵處 讀數據
     printf( "BTN is %d\n", btn_value);
    switch(btn_value)
    {
    	case 1: led_val = 0x01;break;  //led[7:0]=0x01
    	case 2: led_val = 0x02;break;
    	case 4: led_val = 0x03;break; //亮1和0 b0011
    	case 8: led_val = 0x04;break; //亮2 b0100
    	case 16: led_val = 0x05;break; //亮0和2 b0101
    	default:break;
    }
     if (btn_value != 0)
    	offset = 0;
      XGpio_DiscreteWrite(&LED,1,led_val); //直接寫入 led
      xil_printf( "LED is %d\r\n", XGpio_DiscreteRead(&LED, 1) );
    //Acknowledge GPIO interrupts
     (void)XGpio_InterruptClear(&BTNInst,BTN_INT);

		 //Enable GPIO interrupts
     XGpio_InterruptEnable(&BTNInst,BTN_INT);

  }
 int main(void)
 {
	 init_platform();

	 for(u32 i=0; i< 500000; i++)
		{
			 Xil_Out32(DDR_BASE_ADDRESS + i*4, 0x00ffff00);
		 }
     int status;
      //按鍵初始化
      status = XGpio_Initialize(&BTNInst,BTNS_DEVICE_ID);
      if(status != XST_SUCCESS) return XST_FAILURE;
    //初始化LED
      status = XGpio_Initialize(&LED,LED_DEVICE_ID);
      if(status != XST_SUCCESS) return XST_FAILURE;

      //設置按鍵IO的方向爲輸入
      XGpio_SetDataDirection(&BTNInst,1,0xff);//通道1；設置方向 0 輸出 1輸入 0xFF表示8位都是輸入
      //設置LED IO 的方向爲輸出
      XGpio_SetDataDirection(&LED,1,0X00);//通道1；設置方向 0 輸出 1輸入, 0x00表示8位都是輸出

     //初始化中斷控制器
      status = IntcInitFunction(INTC_DEVICE_ID,&BTNInst);
      if(status != XST_SUCCESS) return XST_FAILURE;

      while(1){
		  for(;offset < 900000;offset++) {
			  xil_printf( "The data at Address: %x is %x \n\r",DDR_BASE_ADDRESS + offset*4, Xil_In32(DDR_BASE_ADDRESS + offset* 4) );
			 if (btn_value != 0)
			{
				offset = 0;
			}
			 usleep(DELAY);
		  }
			offset = 0;
      }
      return 0;
 }
 int IntcInitFunction(u16 DeviceId,XGpio *GpioInstancePtr) //初始化中斷
 {
      XScuGic_Config *IntcConfig; //中斷控制器配置實例結構體
      int status;
     //Interrupt controller initialization
      IntcConfig = XScuGic_LookupConfig(DeviceId); //找到scugic實體
      status = XScuGic_CfgInitialize(&INTCInst,IntcConfig,IntcConfig->CpuBaseAddress); //GIC初始化
     if(status != XST_SUCCESS) return XST_FAILURE;
     //Call interrupt setup function
      status = InterruptSystemSetup(&INTCInst);  //調用中斷系統啓動~中斷註冊函數Xil_ExceptionRegisterHandler
    if(status != XST_SUCCESS) return XST_FAILURE;

      //Register GPIO interrupt handler
    //設置中斷服務程序入口地址--調用中斷處理函數   //連接到我們自己定義的中斷處理函數BTN_Intr_Handler
    status = XScuGic_Connect(&INTCInst,INTC_GPIO_INTERRUPT_ID,(Xil_ExceptionHandler)BTN_Intr_Handler,(void *)GpioInstancePtr);
      if(status != XST_SUCCESS) return XST_FAILURE;
     //Enable GPIO interrupts
     XGpio_InterruptEnable(GpioInstancePtr,1); //相應GPIO中斷允許
     XGpio_InterruptGlobalEnable(GpioInstancePtr);//GPIO全局中斷允許


     //Enable GPIO interrupts in the controller
     XScuGic_Enable(&INTCInst,INTC_GPIO_INTERRUPT_ID); //GIC允許 	//使能我們設立的中斷實例
     return XST_SUCCESS;
 }

 //Xilinx提供的通用異常處理程序，中斷觸發之後統一由XScuGic_InterruptHandler先處理，然後在HandlerTable中查找相應的處理函數
 int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)  //中斷系統啓動~中斷註冊函數Xil_ExceptionRegisterHandler
 {
     //Register GIC interrupt handler
     Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler,XScuGicInstancePtr);
     Xil_ExceptionEnable(); //使能異常處理
     return XST_SUCCESS;
}
