-- VHDL structural netlist
-- Combinational: (in1, in2..., out)
g1: AND2 port map (a, b, w_and);
g2: OR2  port map (c, d_in, w_or);
g3: XOR2 port map (a, b, w_xor);
g4: NOT1 port map (a, w_not);

-- Sequential: (clk, d, q)
ff1: DFF port map (clk, d_in, q1);
ff2: DFF port map (clk, q1, q2);
ff3: DFF port map (clk, q2, q3);