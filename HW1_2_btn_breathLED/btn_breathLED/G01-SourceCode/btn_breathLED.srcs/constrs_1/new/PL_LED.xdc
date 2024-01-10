#set_property PACKAGE_PIN Y9 [get_ports {clock}];  # "GCLK"
set_property -dict {PACKAGE_PIN Y9 IOSTANDARD LVCMOS33} [get_ports clk]

set_property IOSTANDARD LVCMOS33 [get_ports led_out]
set_property IOSTANDARD LVCMOS33 [get_ports rst_n]
set_property PACKAGE_PIN F22 [get_ports rst_n]
set_property PACKAGE_PIN T22 [get_ports led_out]

set_property IOSTANDARD LVCMOS33 [get_ports upCount]
set_property PACKAGE_PIN T18 [get_ports upCount]
set_property IOSTANDARD LVCMOS33 [get_ports downCount]
set_property PACKAGE_PIN R16 [get_ports downCount]



