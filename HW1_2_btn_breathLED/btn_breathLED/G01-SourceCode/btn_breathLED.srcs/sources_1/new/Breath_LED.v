
module Breath_LED
#(parameter btn_sensity = 20,
parameter   DELAY100 = 100,
parameter maxLimit = 1300,
parameter minLimit = 500)
(
    input           clk,    //24Mhz
    input           rst_n,
   input           upCount,
   input           downCount,
    
    output          led_out
);

//parameter   threshold = 1000;
// parameter   threshold = 100;//just test
wire            delay_1us;
wire            delay_1ms;
wire            delay_1s;
reg             pwm;
reg     [7:0]   cnt1us;
reg     [10:0]  cnt1us;
reg     [10:0]  cnt1sec;
reg             display_state;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@


reg [25:0]clkDivCnt;
reg limitTrigCnt;
reg [23:0]threshold;       //1 PWM spend time
always @(posedge clk or negedge rst_n)begin //divClk to count
begin
    if(!rst_n)
         begin		
             clkDivCnt <= 25'd0;
			 limitTrigCnt <= 1'd0;
		end
	else
		begin
			if (clkDivCnt[btn_sensity] == 1'b1)// == 25'd10000)	//let lim change trig high
			begin
				limitTrigCnt <= 1'd1;
				clkDivCnt <= 1'd0;
				end
			else
			 begin
				limitTrigCnt <= 1'd0;
				clkDivCnt <= clkDivCnt + 25'd1;
			end
		end 
	end 
end 

reg Btn_ctrl_threshold_state;
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
		Btn_ctrl_threshold_state <= 1'b0;
	else
	begin
		if (limitTrigCnt == 1'd1)
		begin
			if(upCount == 1'b1)
			begin
				if (threshold < maxLimit )
					Btn_ctrl_threshold_state <= =1'b1;
				end
			else
				if (threshold  > minLimit)
					Btn_ctrl_threshold_state <= =1'b0;
				end
		end	
	end	
end		
		
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
        threshold <= 23'd1000;
    else begin
		// if (limitTrigCnt == 1'd1)
		// begin
			// if(upCount == 1'b1)
			// begin
			case(Btn_ctrl_threshold_state)
				1'b1:
			     // threshold <= (threshold < maxLimit ) ? threshold + 23'd1 : threshold;
//				if (threshold < maxLimit )
					threshold <= threshold + 23'd1;
//				else
//					threshold <= threshold;
			// end 
			// else if(downCount == 1'b1)
			// begin
				// if (threshold > minLimit )
				1'b0:
					threshold <= threshold - 23'd1;
				// else
					// threshold <= threshold;
			// end 
			// else
				default:
				threshold <= threshold;
		end
		// else
			// threshold <= threshold;
	end
end
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//延時1us
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
        cnt1us <= 8'b0;
    else if(cnt1us == DELAY100 - 1'b1)
        cnt1us <= 8'b0;
    else 
        cnt1us <= cnt1us + 1'b1;
end

assign delay_1us = (cnt1us == DELAY100 - 1'b1)? 1'b1:1'b0;

//延時1ms
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
        cnt1us <= 10'b0;
    else if(delay_1us == 1'b1)begin
        if(cnt1us == threshold - 1'b1)    
            cnt1us <= 10'b0;
        else 
            cnt1us <= cnt1us + 1'b1;
    end
    else 
        cnt1us <= cnt1us;
end
assign delay_1ms = ((delay_1us == 1'b1) && (cnt1us == threshold - 1'b1))? 1'b1:1'b0;
   
//延時1s
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
        cnt1sec <= 10'b0;
    else if(delay_1ms)
    begin
         if(cnt1sec == threshold - 1'b1)
            cnt1sec <= 10'b0;
        else 
            cnt1sec <= cnt1sec + 1'b1;
     end
    else 
        cnt1sec <= cnt1sec;    
end

assign delay_1s = ((delay_1ms == 1'b1) && (cnt1sec == threshold - 1'b1))? 1'b1:1'b0;

//state change
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
        display_state <= 1'b0;
    else if(delay_1s)//每一秒切換一次led燈顯示狀態
        display_state <= ~display_state;
    else 
        display_state <= display_state;
end

//pwm信號的產生
always @(posedge clk or negedge rst_n)begin
    if(!rst_n)
            pwm <= 1'b0;
    else 
        case(display_state)
            1'b0: pwm <= (cnt1us < cnt1sec)? 1'b1:1'b0;
            1'b1: pwm <= (cnt1us < cnt1sec)? 1'b0:1'b1;
        default: pwm <= pwm;
        endcase
end


assign led_out = pwm;

endmodule 
