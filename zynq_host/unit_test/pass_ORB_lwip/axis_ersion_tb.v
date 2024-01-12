
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;
use IEEE.std_logic_textio.ALL;
library STD;
use STD.textio.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity vga_act_DMA_tb is
end vga_act_DMA_tb;

architecture Behavioral of vga_act_DMA_tb is
    -- Component declaration
    component axis_orb
        port (
            s_axis_aclk : in std_logic;
            s_axis_aresetn : in std_logic;
            s_axis_tdata : in std_logic_vector(31 downto 0);
            s_axis_tkeep : in std_logic_vector(3 downto 0);
            s_axis_tlast : in std_logic;
            s_axis_tready : out std_logic;
            s_axis_tvalid : in std_logic;
            m_axis_tdata : out std_logic_vector(31 downto 0);
            m_axis_tkeep : out std_logic_vector(3 downto 0);
            m_axis_tlast : out std_logic;
            m_axis_tready : in std_logic;
            m_axis_tvalid : out std_logic;
            state_reg : out std_logic_vector(2 downto 0);
            h_cnt_IRQ : out std_logic;
            wsta : out std_logic_vector(1 downto 0);
            vga_hs_cnt : out std_logic_vector(9 downto 0);
            vga_vs_cnt : out std_logic_vector(9 downto 0);
            buf_data_state : out std_logic_vector(1 downto 0);
            range_total_cnt : out std_logic_vector(10 downto 0);
            bntMode : in std_logic;
            r_match : out std_logic_vector(39 downto 0)
        );
    end component;

    -- Signals declaration
    signal s_axis_aclk : std_logic := '0';
    signal s_axis_aresetn : std_logic := '0';
    signal s_axis_tdata : std_logic_vector(31 downto 0) := (others => '0');
    signal s_axis_tkeep : std_logic_vector(3 downto 0) := (others => '0');
    signal s_axis_tlast : std_logic := '0';
    signal s_axis_tready : std_logic := '0';
    signal s_axis_tvalid : std_logic := '0';
    signal m_axis_tdata : std_logic_vector(31 downto 0);
    signal m_axis_tkeep : std_logic_vector(3 downto 0) := (others => '0');
    signal m_axis_tlast : std_logic := '0';
    signal m_axis_tready : std_logic := '0';
    signal m_axis_tvalid : std_logic := '0';
    signal state_reg : std_logic_vector(2 downto 0) := (others => '0');
    
    signal h_cnt_IRQ : std_logic := '0';
    signal wsta : std_logic_vector(1 downto 0) := (others => '0');
    signal vga_hs_cnt : std_logic_vector(9 downto 0) := (others => '0');
    signal vga_vs_cnt : std_logic_vector(9 downto 0) := (others => '0');
    signal buf_data_state : std_logic_vector(1 downto 0) := (others => '0');
    signal range_total_cnt : std_logic_vector(10 downto 0) := (others => '0');
    
    signal bntMode : std_logic := '1';
    signal r_match : std_logic_vector(39 downto 0) := (others => '0');
    
    signal reset_done : boolean := false;
begin
    -- Component instantiation
    u1: axis_orb
        port map (
            s_axis_aclk => s_axis_aclk,
            s_axis_aresetn => s_axis_aresetn,
            s_axis_tdata => s_axis_tdata,
            s_axis_tkeep => s_axis_tkeep,
            s_axis_tlast => s_axis_tlast,
            s_axis_tready => s_axis_tready,
            s_axis_tvalid => s_axis_tvalid,
            m_axis_tdata => m_axis_tdata,
            m_axis_tkeep => m_axis_tkeep,
            m_axis_tlast => m_axis_tlast,
            m_axis_tready => m_axis_tready,
            m_axis_tvalid => m_axis_tvalid,
            state_reg => state_reg,
            h_cnt_IRQ => h_cnt_IRQ,
            wsta => wsta,
            vga_hs_cnt => vga_hs_cnt,
            vga_vs_cnt => vga_vs_cnt,
            buf_data_state => buf_data_state,
            range_total_cnt => range_total_cnt,
            bntMode => bntMode,
            r_match => r_match
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
cnt_process:process(i_clk,i_rst)
begin
    if i_rst = '1' then 
        cnt <= 0;
    elsif rising_edge(i_clk)then
        if cnt < img_width*img_height then
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
    file_open(input_file, "C:\Users\abc78\lab\project\FPGA_Canny\Canny.srcs\input_data.txt", read_mode);
      while not endfile(input_file) loop
          readline(input_file, l);
          if endfile(input_file) then
            exit;
          end if;
          wait for clk_period/2;
          read(l, read_data);
          i_img_data <= std_logic_vector(to_unsigned(read_data,8));
          wait for clk_period/2;
      end loop;
      file_close(input_file);
  end process;
// --output_data
// output_process:process
//     file output_file : text;
//     variable l : line;
//     variable write_data : integer;
//     variable data : integer;
// begin
//     file_open(output_file, "d:/viv_prj/axis_orb/axis_orb.srcs/result.txt", write_mode);
//     while True loop
//         if cnt = img_width*img_height then
//             file_close(output_file);
//         else
//             wait for clk_period/2;
//             if o_img_data = '1' then
//                             data := 255;
//             else
//                             data := 0;
//             end if;
//             write(output_file, integer'image(data));
//             write(output_file, ",");
//             writeline(output_file, l);
//             wait for clk_period/2;        
//         end if;
//     end loop;    
// end process;
--debug
Sobel_process:process
    file output_file : text;
    variable l : line;
    variable write_data : integer;
begin
    file_open(output_file, "d:\viv_prj\axis_orb\axis_orb.srcs\result.txt", write_mode);
    while True loop
        if cnt = img_width*img_height then
            file_close(output_file);
        else
            wait for clk_period/2;        
            write(output_file, integer'image(to_integer(unsigned(CN_SB_data_out))));
            write(output_file, ",");
            writeline(output_file, l);
            wait for clk_period/2;        
        end if;
    end loop;    
end process;
// NMS_process:process
//     file output_file : text;
//     variable l : line;
//     variable write_data : integer;
// begin
//     file_open(output_file, "d:/viv_prj/axis_orb/axis_orb.srcs/result.txt", write_mode);
//     while True loop
//         if cnt = img_width*img_height then
//             file_close(output_file);
//         else
//             wait for clk_period/2;        
//             write(output_file, integer'image(to_integer(unsigned(CN_NMS_data_out))));
//             write(output_file, ",");
//             writeline(output_file, l);
//             wait for clk_period/2;        
//         end if;
//     end loop;    
// end process;
end Behavioral;