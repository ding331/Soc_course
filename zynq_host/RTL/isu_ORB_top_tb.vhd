library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.std_logic_textio.ALL;
library STD;
use STD.textio.ALL;

entity tb_top is
end tb_top;

architecture testbench of tb_top is

    -- Component declaration for the unit under test (UUT)
    component top
        port(
            reset          : in std_logic;
            video_clk      : in std_logic;
            mode_sw        : in std_logic;
            star_up_sw     : in std_logic;
            video_gray_out : in std_logic_vector(7 downto 0);
            delay_video_data : in std_logic_vector(7 downto 0);
            rout           : out std_logic_vector(3 downto 0);
            gout           : out std_logic_vector(3 downto 0);
            bout           : out std_logic_vector(3 downto 0);
            o_video_minus  : out std_logic_vector(7 downto 0);
            o_vga_hs_cnt   : out std_logic_vector(9 downto 0);
            o_vga_vs_cnt   : out std_logic_vector(9 downto 0);
            o_buf_data_state : out std_logic_vector(1 downto 0);
            o_match_data   : out std_logic_vector(39 downto 0);
            signal_test    : out std_logic
        );
    end component;

    -- Testbench signals
    signal i_clk         : std_logic := '0';
    signal i_rst    : std_logic := '0';
    signal mode_sw_tb  : std_logic := '0';
    signal star_up_sw_tb : std_logic := '0';
    signal video_gray_out_tb : std_logic_vector(7 downto 0) := (others => '0');
    signal delay_video_data_tb : std_logic_vector(7 downto 0) := (others => '0');
    signal rout_tb     : std_logic_vector(3 downto 0);
    signal gout_tb     : std_logic_vector(3 downto 0);
    signal bout_tb     : std_logic_vector(3 downto 0);
    signal o_video_minus_tb : std_logic_vector(7 downto 0);
    signal o_vga_hs_cnt_tb : std_logic_vector(9 downto 0);
    signal o_vga_vs_cnt_tb : std_logic_vector(9 downto 0);
    signal o_buf_data_state_tb : std_logic_vector(1 downto 0);
    signal o_match_data_tb : std_logic_vector(39 downto 0);
    signal signal_test_tb : std_logic;
signal i_enable : std_logic;
signal cnt : integer;
constant clk_period : time := 10 ns;
begin

    -- Instantiate the unit under test (UUT)
    UUT : top
        port map(
            reset          => i_rst,
            video_clk      => i_clk,
            mode_sw        => mode_sw_tb,
            star_up_sw     => star_up_sw_tb,
            video_gray_out => video_gray_out_tb,
            delay_video_data => delay_video_data_tb,
            rout           => rout_tb,
            gout           => gout_tb,
            bout           => bout_tb,
            o_video_minus  => o_video_minus_tb,
            o_vga_hs_cnt   => o_vga_hs_cnt_tb,
            o_vga_vs_cnt   => o_vga_vs_cnt_tb,
            o_buf_data_state => o_buf_data_state_tb,
            o_match_data   => o_match_data_tb,
            signal_test    => signal_test_tb
        );
clk_process :process
begin
    i_clk <= '0';
    wait for clk_period/2;
    i_clk <= '1';
    wait for clk_period/2;
end process;
process
begin
    i_rst <= '1';
    i_enable <= '0';
    wait for clk_period/2;
    i_rst <= '0';  
    i_enable <= '1';  
    wait for 1000000000 * clk_period;
end process;

    -- Stimulus process
    process
    begin
        i_rst <= '1';  -- Activate reset
        wait for 10 ns;
        i_rst <= '0';  -- Deactivate reset

        -- Apply your test stimuli here

        wait;
    end process;
    cnt_process:process(i_clk,i_rst)
begin
    if i_rst = '1' then 
        cnt <= 0;
    elsif rising_edge(i_clk)then
        if cnt < 720*480 then
            cnt <= cnt +1;
        end if;
    end if;
end process;
  -- Data source for gradient
  gradient_input: process
    file input_file : text;
    variable l: LINE;
    variable read_data: integer;
  begin
    file_open(input_file, "D:\zynq\road720.bin", read_mode);
      while not endfile(input_file) loop
          readline(input_file, l);
          if endfile(input_file) then
            exit;
          end if;
          wait for clk_period/2;
          read(l, read_data);
          video_gray_out_tb <= std_logic_vector(to_unsigned(read_data,8));
          wait for clk_period/2;
      end loop;
      file_close(input_file);
  end process;
--output_data
--output_process:process
--    file output_file : text;
--    variable l : line;
--    variable write_data : integer;
--    variable data : integer;
--begin
--    file_open(output_file, "D:/viv_prj/axis_ORB/axis_ORB.srcs/out.txt", write_mode);
--   while True loop
--        if cnt = img_width*img_height then
--            file_close(output_file);
--        else
--            wait for clk_period/2;        
--            write(output_file, integer'image(to_integer(unsigned(rout_tb))));
--            write(output_file, ",");
--            writeline(output_file, l);
--            wait for clk_period/2;        
--        end if;
--    end loop;    
--end process;

end testbench;