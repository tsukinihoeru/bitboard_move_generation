//
//  bitboard_utility.cpp
//  InvincibleSummer
//
//  Created by Harry Chiu on 11/15/24.
//

#include "bitboard_gen.h"

void Bitboard_Gen::move_piece(int source, int dest){
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[source]][source] ^ zobrist_keys.piecesquare[mailbox[source]][dest];
    U64 mask = occupy_square[source] | occupy_square[dest];
    bitboards[mailbox[source] & 1] ^= mask;
    bitboards[mailbox[source] >> 1] ^= mask;
    mailbox[dest] = mailbox[source];
    mailbox[source] = 0;
}

void Bitboard_Gen::add_piece(int piece_color, int piece_type, int square_index){
    bitboards[piece_type] |= occupy_square[square_index];
    bitboards[piece_color] |= occupy_square[square_index];
    mailbox[square_index] = piece_color + (piece_type << 1);
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[square_index]][square_index];
}

void Bitboard_Gen::add_piece(int piece, int square_index){
    bitboards[piece & 1] |= occupy_square[square_index];
    bitboards[piece >> 1] |= occupy_square[square_index];
    mailbox[square_index] = piece;
    zobrist_hash ^= zobrist_keys.piecesquare[piece][square_index];
}

void Bitboard_Gen::remove_piece(int source){
    zobrist_hash ^= zobrist_keys.piecesquare[mailbox[source]][source];
    bitboards[mailbox[source] & 1] &= ~occupy_square[source];
    bitboards[mailbox[source] >> 1] &= ~occupy_square[source];
    mailbox[source] = 0;
    
}

U64 Bitboard_Gen::mirror(U64 x) {
   U64 k1 = (0x5555555555555555);
   U64 k2 = (0x3333333333333333);
   U64 k4 = (0x0f0f0f0f0f0f0f0f);
   x = ((x >> 1) & k1) +  2*(x & k1);
   x = ((x >> 2) & k2) +  4*(x & k2);
   x = ((x >> 4) & k4) + 16*(x & k4);
   return x;
}

U64 Bitboard_Gen::perft(int depth){
    if (depth == 0){
        return 1ULL;
    }
    U64 nodes = 0;
    uint16_t move_list[256];
    int num_moves;
    num_moves = generate_moves(move_list);
    for(int i = 0; i < num_moves; i++){
        make_move(move_list[i]);
        if(!is_move_legal()){
            nodes += perft(depth - 1);
        }
        unmake_move(move_list[i]);
    }
    return nodes;
}



bool Bitboard_Gen::is_move_legal(){
    int king_source = get_square_index(bitboards[!current_side] & bitboards[KING_BOARD]);
    occupied_board = bitboards[WHITE] | bitboards[BLACK];
    //potential squares the king can be attacked by pawns
    U64 board = pawn_capture_lookup[!current_side][king_source];
    if(board & bitboards[current_side] & bitboards[PAWN_BOARD])
        return true;
    
    //knights and kings are easy
    if(knight_move_lookup[king_source] & bitboards[current_side] & bitboards[KNIGHT_BOARD])
        return true;
    if(king_move_lookup[king_source]  & bitboards[current_side] & bitboards[KING_BOARD])
        return true;
    
    //check diagonal rays
    board = hyp_quint(king_source, diagonal_masks[source_to_diagonal[king_source]]);
    board |= hyp_quint(king_source, antidiagonal_masks[source_to_antidiagonal[king_source]]);
    if(board & bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    
    //check orthogonal rays
    board = hyp_quint(king_source, file_masks[source_to_file[king_source]]);
    board |= hyp_quint_horiz(king_source, rank_masks[source_to_rank[king_source]]);
    if(board & bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    return false;
}

bool Bitboard_Gen::position_in_check(){
    int king_source = get_square_index(bitboards[current_side] & bitboards[KING_BOARD]);
    occupied_board = bitboards[WHITE] | bitboards[BLACK];
    //potential squares the king can be attacked by pawns
    U64 board = pawn_capture_lookup[current_side][king_source];
    if(board & bitboards[!current_side] & bitboards[PAWN_BOARD])
        return true;
    
    //knights and kings are easy
    if(knight_move_lookup[king_source] & bitboards[!current_side] & bitboards[KNIGHT_BOARD])
        return true;
    if(king_move_lookup[king_source]  & bitboards[!current_side] & bitboards[KING_BOARD])
        return true;
    
    //check diagonal rays
    board = hyp_quint(king_source, diagonal_masks[source_to_diagonal[king_source]]);
    board |= hyp_quint(king_source, antidiagonal_masks[source_to_antidiagonal[king_source]]);
    if(board & bitboards[!current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    
    //check orthogonal rays
    board = hyp_quint(king_source, file_masks[source_to_file[king_source]]);
    board |= hyp_quint_horiz(king_source, rank_masks[source_to_rank[king_source]]);
    if(board & bitboards[!current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD])){
        return true;
    }
    return false;
}

char print_piece_arr[16] = {'0', '1', '2', '3', 'P', 'p', 'B', 'b', 'N', 'n', 'R', 'r', 'Q', 'q', 'K', 'k'};

void Bitboard_Gen::print_board(){
    for(int i = 7; i >= 0; i--){
        for(int j = 0; j < 8; j++){
            if(mailbox[i * 8 + j])
                std::cout << print_piece_arr[mailbox[i * 8 + j]];
            else
                std::cout << ".";
        }std::cout << std::endl;
    }
}

void Bitboard_Gen::print_bit_boards(){
    for(int i = 0; i < 8; i++){
        if(bitboards[i]){
            print_u64(bitboards[i]);
            std::cout << "\n\n";
        }
    }
}

void Bitboard_Gen::print_u64(U64 bitboard){
    for(int i = 7; i >= 0; i--){
        for(int j = 0; j < 8; j++){
            U64 index = 8 * i + j;
            U64 res = 1;
            res = res << index;
            if(res & bitboard){
                std::cout << 1;
            }else{
                std::cout << 0;
            }std::cout << " ";
        }std::cout << '\n';
    }
}

std::string Square [64] {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

void print_move(uint16_t move){
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    std::cout<< Square[source] << Square[dest] << flag;
}

void Bitboard_Gen::clear_board(){
    for(int i = 0; i < 64; i++)
        mailbox[i] = 0;
    for(int i = 0; i < 8; i++){
        bitboards[i] = 0;
    }
}

bool Bitboard_Gen::check_consistency(){
    U64 board;
    for(uint16_t i = PAWN_BOARD; i <= KING_BOARD; i++){
        for(uint16_t side = 0; side < 2; side++){
            int piece = side + (i << 1);
            board = bitboards[i] & bitboards[side];
            while(board){
                int source = pop_lsb(&board);
                if(mailbox[source] != piece)
                    return false;
            }
        }
    }
    return true;
}
