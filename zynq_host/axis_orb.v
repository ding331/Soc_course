
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

   input sw_minus,
   input sw_soft_KP,
   input [3:0] sw_threshold,
   input [2:0] btnMode,
   input [3:0] btn_ROI,

   output h_cnt_IRQ,
   output [7 :0] harris_out2,
   output [9 :0] vga_hs_cnt,
   output [9 :0] vga_vs_cnt,
   output wire [ 9:0] x1,
   output wire [ 9:0] y1,
   output wire [ 9:0] x2,
   output wire [ 9:0] y2,
   output wire [ 1:0] o_match_sel,
   output wire [ 19:0] o_cyclePerframe,
   output wire o_FPS_en,


   output wire [39:0] o_r_match_xy,
   output wire [43:0] o_threshold,
   output wire [63:0] ILA_brief_code
   );
wire DMA_rst;
assign DMA_rst = (s_axis_tvalid == 1'b1);  //((s_axis_tkeep == 4'b1111));// ||

assign h_cnt_IRQ = ((vga_hs_cnt == 10'd719)&&(DMA_rst == 1'd1));

reg [1:0] cnt50M;   //100 data_rom, 50 sys
always@(posedge s_axis_aclk)
begin
   if (!DMA_rst)
       cnt50M <=  2'b10;
   else
       cnt50M <= cnt50M + 2'b01;
end

wire [10:0] range_total_cnt;
wire [7:0] minus_data, video_minus_s, video_minus, erotion_data, erotion_data1;
wire [3:0] r_out, g_out, b_out;
wire [7:0] video_gray_out;
wire [7:0] delay_video_data;
assign delay_video_data = s_axis_tdata[31:24];
assign video_gray_out = s_axis_tdata[7:0];
//wire [1:0] soft_KP = s_axis_tdata[16:15];   //KP_p, KP_n
wire [7:0] frame = s_axis_tdata[14:8];
reg [7:0] reg_frame;

wire [63:0] breif_data_1;
wire [19:0] o_breif_data_xy;
assign ILA_brief_code = breif_data_1;
wire [39:0] match_xy;
reg [19:0] cyclePerframe;
reg FPS_en;
assign o_cyclePerframe = cyclePerframe;
assign o_FPS_en = FPS_en;

always@(posedge s_axis_aclk) //* 5ns
begin
    if (reg_frame != frame)        //if SD, DMA new frame
    begin
       FPS_en <= 1'd1;
       reg_frame <= frame;
    end
    else if(vga_hs_cnt >= 20'd718 && vga_vs_cnt == 20'd479)   //DMA_RX => lwip (overlook lwip latency)
    begin
       FPS_en <= 1'd0;
       reg_frame <= reg_frame;
    end
    else
    begin
       FPS_en <= FPS_en;
       reg_frame <= reg_frame;
    end
end
always@(posedge cnt50M[1]) //* 5ns
begin
    if (FPS_en)    //DMA new frame
    begin
        cyclePerframe <= cyclePerframe + 20'd1;
    end
     else
     begin
        cyclePerframe <= 20'd0;
    end
end

       // Instantiate ORB_matching module
top ORB (
   .sw_minus(sw_minus),
   .sw_soft_KP(sw_soft_KP),
   .soft_KP(soft_KP),
   .sw_threshold(sw_threshold),
   .btn_ROI(btn_ROI),
   .reset(DMA_rst), // '0' reset
   .video_clk(cnt50M[1]), // Connect to axi_clk
   .mode_sw(1'd0), //8 bits
   .star_up_sw(1'd1),
   .video_gray_out(video_gray_out),
   .delay_video_data(delay_video_data),
   .rout(r_out), //(m_axis_tdata[31:28]), // Connect to g_out
   .gout(g_out), //(m_axis_tdata[27:24]), // Connect to g_out
   .bout(b_out), //(m_axis_tdata[23:20]), // Connect to b_out
   .o_minus_data(minus_data),
   .o_video_minus(video_minus),    //m_axis_tdata[7:0]),
   .o_video_minus_s(video_minus_s),
   .o_vga_hs_cnt(vga_hs_cnt),
   .o_vga_vs_cnt(vga_vs_cnt),

   .o_threshold(threshold),
   .o_erotion_data(erotion_data),
   .o_harris_out2(harris_out2),
   .o_range_total_cnt(range_total_cnt),
   .o_breif_data_1(breif_data_1),
   .o_match_data(match_xy),
   .signal_test(open)
);
////////////////////////////////

assign x1 = match_xy[9:0];
assign y1 = match_xy[19:10];
assign x2 = match_xy[29:20];
assign y2 = match_xy[39:30];

reg [1:0] match_sel;       //move to VITIS
reg [39:0] r_match_xy;
always@(posedge cnt50M[1]) //* pixel_clk
begin
    if (!DMA_rst)
    begin
        match_sel <= 2'd0;
        r_match_xy <= 40'd0;
    end
    else
    begin
       if (( r_match_xy != match_xy) && (match_sel != 2'd1) )   // && (r_match_xy == 40'd0 )
       begin
           match_sel <= 2'd1;  //0->1, 2->1
//            r_match_xy <= match_xy;
       end
       else if( match_sel == 2'd1 ) //&& r_match_xy != 40'd0))   //1->2
          begin
           match_sel <= 2'd2;
//            r_match_xy <= match_xy;
           end
       else    //2->0, 0->0
           begin
           match_sel <= 2'd0;
           r_match_xy <= 40'd0;
           end
       r_match_xy <= match_xy;
   end
end
assign o_match_sel = match_sel;
assign o_r_match_xy  = r_match_xy ;

reg [31:0] ORB_result;
always@(s_axis_aclk) begin  //non blocking
   begin
       case(btnMode)
           3'b000 :
               ORB_result[31:24] <= {r_out , g_out};
           3'b001 :
               ORB_result[31:24] <= harris_out2;  //delay
           3'b010 :
               ORB_result[31:24] <= erotion_data;
           3'b011 :
               ORB_result[31:24] <= video_gray_out;
           3'b100 :
               ORB_result[31:24] <= video_minus;
           3'b101 :
               ORB_result[31:24] <= breif_data_1[63:32];
           3'b110 :
               ORB_result[31:24] <= breif_data_1[31:0];
            3'b111 :
                ORB_result[31:24] <= minus_data;
           default :
               ORB_result[31:24] <= ORB_result[31: 24];
       endcase
       ORB_result[23:20] <= (b_out);

    if (vga_hs_cnt >= 20'd718 && vga_vs_cnt == 20'd479)
        begin
            ORB_result[19:0] <=  cyclePerframe + 20'd3;
        end    
    else 
        begin
            case(match_sel)
               2'b00 :
                   ORB_result[19:0] <= 20'd0;
               2'b01 ://              //[19:0] <= x1, y1       x2, y2
                   ORB_result[19:0] <= {match_xy[9:0], match_xy[19:10]};
               2'b10 :
                   ORB_result[19:0] <= {match_xy[29:20], match_xy[39:30]};
               2'b11 :
                   ORB_result[19:0] <= ORB_result[19:0];
              default :
                   ORB_result[19:0] <= ORB_result[19:0];
           endcase
       end
   end
end

assign m_axis_tdata = ORB_result;

//    ( match_xy[39:30]) : (match_xy[19:10]) ;
    //x1, y1            x2, y2
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
