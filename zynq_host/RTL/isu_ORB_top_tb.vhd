library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use IEEE.std_logic_textio.ALL;

entity vga_act_DMA_tb is
end vga_act_DMA_tb;

architecture Behavioral of vga_act_DMA_tb is
    signal s_axis_aclk, s_axis_aresetn, s_axis_tdata, s_axis_tvalid, s_axis_tlast : std_logic;
    signal s_axis_tkeep : std_logic_vector(3 downto 0);
    signal m_axis_tdata, m_axis_tready, m_axis_tlast : std_logic;
    signal m_axis_tvalid, m_axis_tkeep : std_logic_vector(3 downto 0);
    signal h_cnt_IRQ : std_logic;
    signal wsta : std_logic_vector(1 downto 0);
    signal vga_hs_cnt, vga_vs_cnt : std_logic_vector(9 downto 0);
    signal buf_data_state : std_logic_vector(1 downto 0);
    signal range_total_cnt : std_logic_vector(10 downto 0);
    signal bntMode : std_logic;
    signal r_match : std_logic_vector(39 downto 0);

    -- Add any other signals you may need for simulation here

    -- Add component instantiation here (if required)
    -- Replace the following line with the actual instantiation of your 'axis_orb' module
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

    -- Add stimulus process here
    process
    begin
        -- Initialize inputs
        s_axis_aclk <= '0';
        s_axis_aresetn <= '0';
        s_axis_tdata <= (others => '0');
        s_axis_tkeep <= (others => '0');
        s_axis_tlast <= '0';
        s_axis_tvalid <= '0';
        m_axis_tready <= '0';
        bntMode <= '1';
        r_match <= (others => '0');

        -- Add any other initialization here

        wait for 10 ns; -- Wait for initial signals to settle

        -- Stimulus
        s_axis_aresetn <= '1'; -- Release reset
        wait for 10 ns;
        s_axis_tvalid <= '1'; -- Assert valid
        wait for 10 ns;
        s_axis_tvalid <= '0'; -- Deassert valid
        wait for 10 ns;

        -- Add any other stimulus here

        -- Add clock generation here if not provided by external source
        for i in 1 to 100 loop
            s_axis_aclk <= not s_axis_aclk;
            wait for 5 ns; -- Adjust the clock period as needed
        end loop;

        wait;

    end process;

    -- Add any other processes or assertions here

end Behavioral;
