
update_ip_catalog -rebuild -scan_changes
report_ip_status -name ip_status

regenerate_bd_layout
regenerate_bd_layout -routing

upgrade_ip -vlnv xilinx.com:user:data_processor:1.0 [get_ips  design_1_data_processor_0_1] -log ip_upgrade.log

save_bd_design
export_ip_user_files -of_objects [get_ips design_1_data_processor_0_1] -no_script -sync -force -quiet
generate_target all [get_files  D:/viv_prj/axis_gen_DMA/axis_gen_DMA.srcs/sources_1/bd/design_1/design_1.bd]

export_ip_user_files -of_objects [get_files D:/viv_prj/axis_gen_DMA/design_1.srcs/sources_1/bd/ORB_1/ORB_1.bd] -no_script -sync -force -quiet
export_simulation -of_objects [get_files D:/viv_prj/axis_gen_DMA/design_1.srcs/sources_1/bd/ORB_1/ORB_1.bd] -directory D:/viv_prj/axis_gen_DMA/design_1.ip_user_files/sim_scripts -ip_user_files_dir D:/viv_prj/axis_gen_DMA/design_1.ip_user_files -ipstatic_source_dir D:/viv_prj/axis_gen_DMA/design_1.ip_user_files/ipstatic -lib_map_path [list {modelsim=D:/viv_prj/axis_gen_DMA/design_1.cache/compile_simlib/modelsim} {questa=D:/viv_prj/axis_gen_DMA/design_1.cache/compile_simlib/questa} {riviera=D:/viv_prj/axis_gen_DMA/design_1.cache/compile_simlib/riviera} {activehdl=D:/viv_prj/axis_gen_DMA/design_1.cache/compile_simlib/activehdl}] -use_ip_compiled_libs -force -quiet
report_ip_status -name ip_status 
make_wrapper -files [get_files D:/viv_prj/axis_gen_DMA/design_1.srcs/sources_1/bd/ORB_1/ORB_1.bd] -top

launch_runs impl_1 -to_step write_bitstream -jobs 3