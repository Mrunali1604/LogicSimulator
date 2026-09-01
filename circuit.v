// Combinational Section: 4 Distinct Gate Types
and g1 (w_and, a, b);        // AND Gate
or  g2 (w_or,  c, d);     // OR Gate
xor g3 (w_xor, a, b);        // XOR Gate
not g4 (w_not, a);           // NOT Gate

// Sequential Section: 3-Stage DFF Shift Register
dff ff1 (q1, clk, d_in);     // Stage 1: Captures d_in on rising clk
dff ff2 (q2, clk, q1);       // Stage 2: Captures q1 on rising clk
dff ff3 (q3, clk, q2);       // Stage 3: Captures q2 on rising clk