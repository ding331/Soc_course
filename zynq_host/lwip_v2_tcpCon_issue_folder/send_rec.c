void send_received_data()

{

    #if __arm__

    int copy = 3;

    #else

    int copy = 0;

    #endif

    err_t err;

    int Status;

    struct tcp_pcb *tpcb = connected_pcb;



    /*initial the first axdma transmission, only excuse once*/

    if(!first_trans_start)

    {

    Status = XAxiDma_SimpleTransfer(&AxiDma, (u32)RxBufferPtr[0],

    (u32)(PAKET_LENGTH), XAXIDMA_DEVICE_TO_DMA);

    if (Status != XST_SUCCESS)

    {

    xil_printf("axi dma failed! 0 %d\r\n", Status);

    return;

    }

    /*set the flag, so this part of code will not excuse again*/

    first_trans_start = 1;

    }



    /*if the last axidma transmission is done, the interrupt triggered, then start TCP transmission*/

    if(packet_trans_done)

    {



    if (!connected_pcb)

    return;



    /* if tcp send buffer has enough space to hold the data we want to transmit from PL, then start tcp transmission*/

    if (tcp_sndbuf(tpcb) > SEND_SIZE)

    {

    /*transmit received data through TCP*/

    err = tcp_write(tpcb, RxBufferPtr[packet_index & 1], SEND_SIZE, copy);

    if (err != ERR_OK) {

    xil_printf("txperf: Error on tcp_write: %d\r\n", err);

    connected_pcb = NULL;

    return;

    }

    err = tcp_output(tpcb);

    if (err != ERR_OK) {

    xil_printf("txperf: Error on tcp_output: %d\r\n",err);

    return;

    }



    packet_index++;

    /*clear the axidma done flag*/

    packet_trans_done = 0;



    /*initial the other axidma transmission when the current transmission is done*/

    Status = XAxiDma_SimpleTransfer(&AxiDma, (u32)RxBufferPtr[(packet_index + 1)&1],

    (u32)(PAKET_LENGTH), XAXIDMA_DEVICE_TO_DMA);

    if (Status != XST_SUCCESS)

    {

    xil_printf("axi dma %d failed! %d \r\n", (packet_index + 1), Status);

    return;

    }



    }

    }

}

static err_t

tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)

{

xil_printf("txperf: Connected to iperf server\r\n");



/* store state */

connected_pcb = tpcb;



/* set callback values & functions */

tcp_arg(tpcb, NULL);

tcp_sent(tpcb, tcp_sent_callback);



tcp_client_connected = 1;

/* initiate data transfer */

return ERR_OK;

}

