
module axis_orb(
    input wire s_axis_aclk,
    input wire s_axis_aresetn,
    input [31:0] s_axis_tdata,
    input wire [3:0] s_axis_tkeep,
    input wire s_axis_tlast,
    output reg s_axis_tready,
    input wire s_axis_tvalid,
    output wire [31:0] m_axis_tdata,    //wire m_axis_tdata
    output reg [3:0] m_axis_tkeep,
    output reg m_axis_tlast,
    input m_axis_tready,
    output reg m_axis_tvalid,
    output reg [2:0] state_reg,
    
    input [3:0] sw_threshold, 
    input [2:0] btnMode, 
    output h_cnt_IRQ,
	output [9 :0] vga_hs_cnt,
	output [9 :0] vga_vs_cnt
    );
wire DMA_rst;
assign DMA_rst = (s_axis_tvalid == 1'b1);  //((s_axis_tkeep == 4'b1111));// || 

assign h_cnt_IRQ = ((vga_hs_cnt == 10'd719)&&(DMA_rst == 1'd1));

reg [1:0] cnt50M;	//100 data_rom, 50 sys
always@(posedge s_axis_aclk)
begin
    if (!DMA_rst)
        cnt50M <=  2'b10;
    else
        cnt50M <= cnt50M + 2'b01;
end
wire [39:0] match_data;
// wire match_en;       //move to VITIS
// assign match_en = (match_data == match_data) ? 1'b0 : 1'b1;
// always@(posedge s_axis_aclk)
// begin
//     if (!DMA_rst)
//         match_data <= 40'd0;
//     else    
//         match_data <= (match_en == 1'b1) ? match_data : match_data;   //refresh : save
// end 
wire [10:0] range_total_cnt;
wire [7:0] minus_data, video_minus_s, video_minus, erotion_data, erotion_data1;
wire [3:0] r_out, g_out, b_out;
wire [7:0] video_gray_out;
wire [7:0] delay_video_data;
assign delay_video_data = s_axis_tdata[31:24];
assign video_gray_out = s_axis_tdata[7:0];
        // Instantiate ORB_matching module
top ORB (
    .sw_threshold(sw_threshold),
    .reset(DMA_rst), // '0' reset
    .video_clk(cnt50M[1]), // Connect to axi_clk
    .mode_sw(1'd0),	//8 bits
    .star_up_sw(1'd1),
    .video_gray_out(video_gray_out),
    .delay_video_data(delay_video_data),
    .rout(r_out), //(m_axis_tdata[31:28]), // Connect to g_out
    .gout(g_out), //(m_axis_tdata[27:24]), // Connect to g_out
    .bout(b_out), //(m_axis_tdata[23:20]), // Connect to b_out
    .o_minus_data(minus_data),
    .o_video_minus(video_minus), 	//m_axis_tdata[7:0]),
    .o_video_minus_s(video_minus_s),
    .o_vga_hs_cnt(vga_hs_cnt),
    .o_vga_vs_cnt(vga_vs_cnt),

    .o_erotion_data(erotion_data),
    .o_erotion_data1(erotion_data1),
    .o_range_total_cnt(range_total_cnt),
    .o_match_data(match_data),
    .signal_test(open)
);
////////////////////////////////
// wire [31:0] ORB_result;     //{RGB , match x, y}
// assign ORB_result[31:28] =  (btnMode == 2'b00) ? r_out : 
//                             (btnMode == 2'b01) ? erotion_data[7:4]:  
//                             (btnMode == 2'b10) ? video_gray_out[7:4]:  
//                             (btnMode == 2'b11) ? delay_video_data[7:4]: video_gray_out[7:4];
// assign ORB_result[27:24] =  (btnMode == 2'b00) ? g_out : 
//                             (btnMode == 2'b01) ? erotion_data[3:0]:  
//                             (btnMode == 2'b10) ? video_gray_out[3:0]:  
//                             (btnMode == 2'b11) ? delay_video_data[3:0]: video_gray_out[3:0];
// // assign ORB_result[31:24] =  (btnMode == 2'b00) ? {r_out, g_out} : 
// //                             (btnMode == 2'b01) ? erotion_data:  
// //                             (btnMode == 2'b10) ? video_gray_out:  
// //                             (btnMode == 2'b11) ? delay_video_data: video_gray_out;

// assign ORB_result[23:20] = (b_out);
//                                              //VITIS xor detect mtach
// assign ORB_result[19:10] =  (cnt50M == 2'b00) ? (match_data[9:0]   ) : 
//                             (cnt50M == 2'b01) ? (match_data[29:20] ) :                                        
//                             (cnt50M == 2'b10) ? (match_data[9:0]   ) :                                        
//                             (cnt50M == 2'b11) ? (match_data[29:20] ) : (match_data[9:0]) ; 
// assign ORB_result[9:0] =    (cnt50M == 2'b00) ? ( match_data[19:10]) : 
//                             (cnt50M == 2'b01) ? ( match_data[39:30]) :                                        
//                             (cnt50M == 2'b10) ? ( match_data[19:10]) :                                        
//                             (cnt50M == 2'b11) ? ( match_data[39:30]) : (match_data[19:10]) ; 
//                         //                                   //x1, y1            x2, y2
reg [31:0] ORB_result;
always@(s_axis_aclk) begin  //non blocking
    case(btnMode)
        3'b000 :
            ORB_result[31:24] = {r_out , g_out};
        3'b001 :
            ORB_result[31:24] = erotion_data1;  //delay
        3'b010 :
            ORB_result[31:24] = erotion_data;
        3'b011 :
            ORB_result[31:24] = delay_video_data;
        3'b100 :
            ORB_result[31:24] = video_gray_out;
        3'b101 :
            ORB_result[31:24] = video_minus;
        3'b110 :
            ORB_result[31:24] = video_minus_s;
        3'b111 :
            ORB_result[31:24] = minus_data;
        default :
            ORB_result[31:24] = ORB_result[31: 24];
    endcase
    ORB_result[23:20] = (b_out);
    ORB_result[19:10] = (cnt50M == 1'd0) ? (match_data[9:0]) : (match_data[29:20]); 
                                  //[19:0] = x1, y1            x2, y2
    ORB_result[9:0] = (cnt50M == 1'd0) ? (match_data[19:10]) : (match_data[39:30]); 
end
assign m_axis_tdata = ORB_result;
   
////////////////////////////////

reg [31:0] tdata;
reg tlast;

parameter init               = 3'd0;
parameter SetSlaveTready     = 3'd1;
parameter CheckSlaveTvalid   = 3'd2;
parameter ProcessTdata       = 3'd3;
parameter CheckTlast         = 3'd4;

always @ (posedge s_axis_aclk)
    begin
        // Default outputs            
        m_axis_tvalid <= 1'b0;
        
        if (s_axis_aresetn == 1'b0)
            begin
                tlast <= 1'b0;
                tdata[31:0] <= 32'd0;
                s_axis_tready <= 1'b0;
                // m_axis_tdata[31:0] <= 32'd0;
                m_axis_tkeep <= 4'h0;
                m_axis_tlast <= 1'b0;
                state_reg <= init;
            end
        else
            begin
                case(state_reg) 
                    init : // 0 
                        begin
                            tlast <= 1'b0;
                            tdata[31:0] <= 32'd0;
                            s_axis_tready <= 1'b0;
                            // m_axis_tdata[31:0] <= 32'd0;
                            m_axis_tkeep <= 4'h0;
                            m_axis_tlast <= 1'b0;
                            state_reg <= SetSlaveTready;
                        end 
                        
                    SetSlaveTready : // 1
                        begin
                            s_axis_tready <= 1'b1;
                            state_reg <= CheckSlaveTvalid;
                        end 
                        
                    CheckSlaveTvalid : // 2
                        begin
                            if (s_axis_tkeep == 4'hf && s_axis_tvalid == 1'b1)
                                begin
                                    s_axis_tready <= 1'b0;
                                    tlast <= s_axis_tlast;
                                    tdata[31:0] <= s_axis_tdata[31:0];
                                    state_reg <= ProcessTdata;
                                end
                            else
                                begin 
                                    tdata[31:0] <= 32'd0;
                                    state_reg <= CheckSlaveTvalid;
                                end 
                        end
                        
                    ProcessTdata : // 3
                        begin 
                            m_axis_tkeep <= 4'hf;
                            m_axis_tlast <= tlast;
                            m_axis_tvalid <= 1'b1;
                            // m_axis_tdata[31:0] <= tdata[31:0];
                            
                            if (m_axis_tready == 1'b1)
                                begin 
                                    state_reg <= CheckTlast;
                                end 
                            else
                                begin 
                                    state_reg <= ProcessTdata;
                                end 
                        end
                        
                    CheckTlast : // 4
                        begin 
                            if (m_axis_tlast == 1'b1)
                                begin				
                                    state_reg <= init;
                                end
                            else if (m_axis_tready == 1'b1)
                                begin
                                    state_reg <= SetSlaveTready;
                                end
                            else 
                                begin 
                                    state_reg <= CheckTlast;
                                end 
                        end 
                        
                endcase 
            end
    end
    
endmodule