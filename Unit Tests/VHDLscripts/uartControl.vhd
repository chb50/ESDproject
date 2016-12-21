-- uart controller for interfacing with arduino
--arduino will select from various display patterns based on the switches selected on the FPGA
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity uartControl is
port (SW: in std_logic_vector(8 downto 0);
		CLOCK_50: in std_logic;
		UART_RXD: in std_logic;
		UART_TXD: out std_logic;
		--not sure if this will be top level, but if so need to add these signals
		LEDR: out std_logic_vector(7 downto 0); --shows which value will be sent next
		LEDG: out std_logic_vector(7 downto 0)); --displays acknowledgement from arduino about which display pattern to use
end entity uartControl;

architecture behavior of uartControl is

--Components--

component tx is
port(clk: in std_logic;
		startBit: in std_logic; --startBit must be 0 to initiate data transfer
		data: in std_logic_vector(7 downto 0);
		busyFlag: out std_logic;
		txLine: out std_logic);
end component tx;

component rx is
port (clk: in std_logic;
		rxLine: in std_logic;
		data: out std_logic_vector(7 downto 0);
		busyFlag: out std_logic);
end component rx;

--Signals--

signal uartEnable: std_logic:= SW(8); --switch 8 used to enable uart controller

signal txData: std_logic_vector(7 downto 0);
signal txStart: std_logic:= '0';
signal txBusy: std_logic; --signals if tx busy

signal rxData: std_logic_vector(7 downto 0);
signal rxBusy: std_logic;

begin

txConnect: tx port map (clk => CLOCK_50, startBit => txStart, data => txData, busyFlag => txBusy, txLine => UART_TXD);
rxConnect: rx port map (clk => CLOCK_50, rxLine => UART_RXD, data => rxData, busyFlag => rxBusy);

--Added by Akshaya 
connect: process (CLOCK_50) is 
begin
	if (rising_edge(CLOCK_50)) then
		if (uartEnable = '1' and txBusy = '0') then
			txStart <= '1';			--indicate that you are sending bits
			txData <= sw(7 downto 0);
		else
			txStart <= '0'; 			--indicate no bits are transmitted *CHECK THIS! DO WE REALLY NEED IT?
										--wait for the busyFlag to not be busy
		end if;
	end if;
end process;


process(CLOCK_50) is
begin
	if (rising_edge(CLOCK_50)) then
		if (uartEnable = '1' and rxBusy = '0') then
				LEDG <= rxData;
		end if;
	end if;
	
end process;

LEDR <= sw(7 downto 0);	

end architecture behavior;