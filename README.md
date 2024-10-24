# bitboard_move_generation
Program that generates all possible chess moves in a given position. Uses bitboards and pseudolegal move generation and achieves a perft speed of 13 million nodes per second.

Has 8 occupancy bitboards, for the white pieces, black pieces and pawns, bishops, knights, rooks, queens and kings.
Knight and King moves are generated via lookup tables. Sliding piece moves are generated with hyperbolic quintessence.
