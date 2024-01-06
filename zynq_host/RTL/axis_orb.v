
module axis_orb(
    input wire s_axis_aclk,
    input wire s_axis_aresetn,
    input wire [31:0] s_axis_tdata,
    input wire [3:0] s_axis_tkeep,
    input wire s_axis_tlast,
    output reg s_axis_tready,
    input wire s_axis_tvalid,
    output wire [31:0] m_axis_tdata,
    output reg [3:0] m_axis_tkeep,
    output reg m_axis_tlast,
    input m_axis_tready,
    output reg m_axis_tvalid,
    output reg [2:0] state_reg,
    
    output wire h_cnt_IRQ,
	output [1:0] wsta,
	output [9 :0] vga_hs_cnt,
	output [9 :0] vga_vs_cnt,
	output [1 :0] buf_data_state,
    //
	output reg [39:0] r_match
    );
    
    assign h_cnt_IRQ = ((vga_hs_cnt == 10'd719 )&&(DMA_rst == 1'd1));

    reg [1:0] clk50M;	//100 data_rom, 50 sys
	always@(posedge s_axis_aclk)
	begin
        if (!DMA_rst)
		    clk50M <=  2'b10;
        else
		    clk50M <= clk50M + 2'b01;
	end

    wire match_en = (r_match == match_data) ? 1'b0 : 1'b1;
	always@(posedge s_axis_aclk)
	begin
        if (!DMA_rst)
            r_match <= 40'd0;
        else    
            r_match <= (match_en) ? match_data : 40'd0;
    end        
        assign	m_axis_tdata[19:0] = (clk50M == 1'd0) ? r_match[39:20] : r_match[19:0]; 
    // assign	m_axis_tdata[19:0] = (clk50M == 1'd0) ? match_data[39:20] : match_data[19:0]; 
	
	   //     // Instantiate ORB_matching module
     top ORB (
         .reset(DMA_rst), // '0' reset
         .video_clk(clk50M[1]), // Connect to axi_clk
	 	.mode_sw(1'd0),	//8 bits
	 	.star_up_sw(1'd1),
	 	.video_gray_out(s_axis_tdata[7:0]),
	 	.delay_video_data(s_axis_tdata[31:24]),
        
	 	.rout(m_axis_tdata[31:28]), // Connect to g_out
	 	.gout(m_axis_tdata[27:24]), // Connect to g_out
        .bout(m_axis_tdata[23:20]), // Connect to b_out
	 	.o_video_minus(open), 	//m_axis_data[7:0]),
	 	//
		
	 	.o_vga_hs_cnt(vga_hs_cnt),
	 	.o_vga_vs_cnt(vga_vs_cnt),
	 	.o_buf_data_state(buf_data_state),
	 	.o_match_data(match_data),
	 	.signal_test(open)
     );

	assign DMA_rst = (s_axis_tvalid == 1'b1);  //((s_axis_tkeep == 4'b1111));// || 

//    reg [2:0] state_reg;
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