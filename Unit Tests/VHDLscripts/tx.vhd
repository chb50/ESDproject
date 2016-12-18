--TX signal control to be a component of the uartController
entity tx is
port(clk: in std_logic;)
		startBit: in std_logic; --startBit must be 0 to initiate data transfer
		data: in std_logic_vector(7 downto 0);
		busyFlag: out std_logic;
		txLine: out std_logic);
end entity tx;

architecture behavior of tx is

--prescaler is used to scale the clock signal to 9.6 KHz
--this is the rate that data will be transmitted and recieved by the rs232 port
signal prescl: integer range 0 to 5208 := 0;
signal index: integer range 0 to 9; --how we will address the data buffer
signal databuff: std_logic_vector(9 downto 0); --bit 0 is the start bit, bit 9 is the end bit
signal txFlag: std_logic; --used to start transmittion process

begin

process(clk) is
begin
end process

end architecture behavior;