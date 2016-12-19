--TX signal control to be a component of the uartController

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tx is
port(clk: in std_logic;
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
signal txFlag: std_logic := '0'; --states if transmission is currently taking place

begin

busyFlag <= txFlag; --tells outside that tx is transmitting data

--only run transmition when txFlag = '0' and startbit = '1'
process(clk) is
begin
	if (rising_edge(clk)) then
		if (txFlag = '0' and startBit = '1') then
			txFlag <= '1';
			databuff(0) <= '0'; --indicates start of transmission
			databuff(9) <= '1'; --indicates end of transmission
		
			databuff(8 downto 1) <= data;
		end if;
		if(txFlag = '1') then --currently transmitting data
			if(prescl < 5207) then --scalar test for 50 MHz clk signal
				prescl <= prescl + 1;
			else
				prescl <= 0;
			end if;
			
			if(prescl = 2607) then 
				txLine <= databuff(index);
				if(index < 9) then
					index <= index + 1;
				else 
					txFlag <= '0';
					index <= 0;
				end if;
			end if;
		end if;
	end if;
end process;

end architecture behavior;