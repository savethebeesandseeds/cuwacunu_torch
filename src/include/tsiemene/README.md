tsiemene means "thing"

tsi for short.

The right mental model: Kahn-style process networks, but with your waves

Think:

ports are FIFO streams

TSIs are deterministic processes consuming from input FIFOs and producing to output FIFOs

circuits are wires connecting FIFOs

This model has a beautiful property: determinism survives parallel scheduling, as long as:

each process is deterministic, and

each channel is FIFO