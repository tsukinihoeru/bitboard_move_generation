//
//  bitboard_init.cpp
//  InvincibleSummer
//
//  Created by Harry Chiu on 11/15/24.
//
#include "bitboard_gen.h"

Bitboard_Gen::Bitboard_Gen(){
    clear_board();
}

Bitboard_Gen::Bitboard_Gen(std::string fen){
    init_zobrist_keys();
    set_board(fen);
}

void Bitboard_Gen::set_board(std::string fen){
    
    int rank = 7;
    int file = 0;
    
    //wipe the board and fill it with 0s
    clear_board();
    zobrist_hash = 0;
    ply = 0;
    uint8_t castling_rights = 0;
    
    for(char& c : fen){
        if(c == '/'){
            rank--;
            file = 0;
        }else if(int (c) > int ('0') && int (c) < int('9')) { // c is between 1 and 8
            file += int (c) - int('0');
        }else if(!((rank * 16 + file) & 0x88)){
            int piece_type = fen_char_to_piece_type[(char) std::tolower(c)];
            int piece_color = isupper(c) ? WHITE : BLACK;
            add_piece(piece_color, piece_type, rank * 8 + file);
            file++;
        }else if(rank == 0 && file == 8){
            if(c == 'K'){
                castling_rights |= WKS_CASTLING_RIGHTS;
            }else if(c == 'Q'){
                castling_rights |= WQS_CASTLING_RIGHTS;
            }else if(c == 'k'){
                castling_rights |= BKS_CASTLING_RIGHTS;
            }else if(c == 'q'){
                castling_rights |= BQS_CASTLING_RIGHTS;
            }else if(c == 'w'){
                current_side = WHITE;
            }else if(c == 'b'){
                current_side = BLACK;
            }
        }
    }

    game_history[ply] = game_state(castling_rights, 0, 0);
    hash_history[ply] = zobrist_hash;
}

void Bitboard_Gen::init_zobrist_keys(){
    PRNG rng(1070372);
    
    for(int piece = 4; piece < 16; piece++){
        for(int square = 0; square < 64; square++){
            zobrist_keys.piecesquare[piece][square] = rng.rand64();
        }
    }for(int i = 0; i < 16; i++)
        zobrist_keys.castling[i] = rng.rand64();
    zobrist_keys.color = rng.rand64();
    for(int i = 0; i < 40; i++)
        zobrist_keys.ep_squares[i] = rng.rand64();
}
