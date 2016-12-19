--rx component of uartControl

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity rx is
port (clk: in std_logic;
		rxLine: in std_logic;
		data: out std_logic_vector(7 downto 0);
		busyFlag: out std_logic);
end entity rx;

architecture behavior of rx is
signal databuff: std_logic_vector(9 downto 0);
signal rxFlag: std_logic := '0';
signal prescl: integer range 0 to 5208:= 0;
signal index: integer range 0 to 9:= 0;

begin

busyFlag <= rxFlag; --sets busy flag each time rxFlag is set
process(clk) is
begin
	 if(rising_edge(clk)) then
		--if rxLine recieves a 0 and the rxLine is not currently busy, then we need to start transmission (recieving data)
		if(rxFlag = '0' and rxLine = '0') then
			index <= 0;
			prescl <= 0;
			rxFlag <= '1';
		end if;
		
		if(rxFlag <= '1') then
			--read in value
			databuff(index) <= rxLine;
			
			if(prescl < 5207) then --this divides clock rate
				prescl <= prescl + 1;
			else
				prescl <= 0;
			end if;
			
			if(prescl = 2500) then --read and write in slightly different intervals
				if (index<9) then
					index <= index + 1;
				else
					--make sure data packet is valid
					if(databuff(0) = '0' and databuff(9) = '1') then
						data <= databuff(8 downto 1);
					else
						--set to all 0 if data is not valid
						data <= (others => '0');
					end if;
				rxFlag <= '0';
				end if;
			end if;
		end if;
	 end if;
end process;

end architecture behavior;