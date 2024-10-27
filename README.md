# bitboard_move_generation
Program that generates all possible chess moves in a given position. Uses bitboards and pseudolegal move generation and achieves a perft speed of 13 million nodes per second.

Has 8 occupancy bitboards, for the white pieces, black pieces and pawns, bishops, knights, rooks, queens and kings.
Knight and King moves are generated via lookup tables. Sliding piece moves are generated with hyperbolic quintessence.
Uses Zobrist Hashing for later implementation of transposition tables.

This move generation program will be the backbone of my chess engine project. To use the move generation for your own engine, simply call the generate_moves function to get all pseudolegal moves, and check for legality with the is_in_check() function. The moves are encoded as a uint16_t, where the first 6 bits source, the following 6 bits are the destination, and the last 4 bits are the special flags for promotions and captures.
