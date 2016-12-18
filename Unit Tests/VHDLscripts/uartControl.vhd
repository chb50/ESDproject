-- uart controller for interfacing with arduino
--arduino will select from various display patterns based on the switches selected on the FPGA

entity uartControl is
port (switches: in std_logic_vector(7 downto 0);
		CLOCK_50: in std_logic;
		UART_RXD: in std_logic;
		UART_TXD: out std_logic;
		--not sure if this will be top level, but if so need to add these signals
		LEDR: out std_logic_vector(7 downto 0); --shows which value will be sent next
		LEDG: out std_logic_vector(7 downto 0)); --displays acknowledgement from arduino about which display pattern to use
end entity uartControl;

architecture behavior of uartControl is
begin
end architecture behavior;