module Breath_LED_tb(

    );
	
	
	reg clk;
	reg rst_n;
	wire led_out;
	
	
	//generate clock
	initial begin
		clk = 1;
		forever 
			#50 clk = ~clk;
	end
	
	//initialization
	initial begin
		rst_n = 0;
		#100
		rst_n = 1;
		
	end
	
	//instantiation
	Breath_LED u_led(
	.clk(clk),
	.rst_n(rst_n),
	.led_out(led_out)
	);
	
	
	
endmodule