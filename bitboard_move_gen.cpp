//
//  bitboard_gen.cpp
//  Bitboard_Movegen
//
//  Created by Harry Chiu on 10/17/24.
//
#include "bitboard_gen.h"

#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define reverse(x) OSSwapInt64(x)
#elif defined(__linux__)
    #include <byteswap.h>
    #define reverse(x) __bswap_64(x)
#elif defined(_WIN32)
    #include <stdlib.h>
    #define reverse(x) _byteswap_uint64(x)
#else
    #error "Byte swapping function not defined for this platform."
#endif

//generates all pseudolegal moves
int Bitboard_Gen::generate_moves(uint16_t * m_list){
    move_list = m_list;
    occupied_board = bitboards[WHITE] | bitboards[BLACK];
    empty_board = ~occupied_board;
    generate_attacked_squares();
    
    if(current_side){
        add_black_pawn_moves();
        add_black_castle_moves();
    }
    else{
        add_white_pawn_moves();
        add_white_castle_moves();
    }
    
    add_knight_moves();
    add_king_moves();
    add_diag_moves();
    add_orthog_moves();
    
    return (int) (move_list - m_list);
}

inline void Bitboard_Gen::add_black_pawn_moves(){
    U64 side_pawn_board = bitboards[current_side] & bitboards[PAWN_BOARD];
    //get all possible pawn pushes except for ones that are promotions
    U64 board = (side_pawn_board >> 8) & empty_board & (~rank_masks[0]);
    U64 double_pawn_pushes = (board >> 8) & empty_board & rank_masks[4];
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest + 8) << 10) | (dest << 4) | QUIET_FLAG;
    }
    while(double_pawn_pushes){
        int dest = pop_lsb(&double_pawn_pushes);
        *move_list++ = ((dest + 16) << 10) | (dest << 4) | DOUBLE_PAWN_PUSH_FLAG;
    }
    //get all pawn captures except promos
    board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (~rank_masks[0]);
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest + 9) << 10) | (dest << 4) | CAPTURE_FLAG;
    }
    board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (~rank_masks[0]);
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest + 7) << 10) | (dest << 4) | CAPTURE_FLAG;
    }
    
    //PROMOTIONS::
    board = (side_pawn_board >> 8) & empty_board & (rank_masks[0]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_moves(dest + 8, dest);
    }
    board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (rank_masks[0]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_cap_moves(dest + 9, dest);
    }
    board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (rank_masks[0]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_cap_moves(dest + 7, dest);
    }
    
    //EN PASSANT
    board = ep_target_lookup[game_history[ply].ep_target] & side_pawn_board;
    while(board){
        int source = pop_lsb(&board);
        *move_list++ = (source << 10) | ((game_history[ply].ep_target - 8) << 4) |EN_PASSANT_FLAG;
    }
}

inline void Bitboard_Gen::add_white_pawn_moves(){
    U64 side_pawn_board = bitboards[current_side] & bitboards[PAWN_BOARD];
    U64 board = (side_pawn_board << 8) & empty_board & (~rank_masks[7]);
    U64 double_pawn_pushes = (board << 8) & empty_board & rank_masks[3];
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest - 8) << 10) | (dest << 4) | QUIET_FLAG;
    }while(double_pawn_pushes){
        int dest = pop_lsb(&double_pawn_pushes);
        *move_list++ = ((dest - 16) << 10) | (dest << 4) | DOUBLE_PAWN_PUSH_FLAG;
    }
    board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (~rank_masks[7]);
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest - 9) << 10) | (dest << 4) | CAPTURE_FLAG;
    }
    board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (~rank_masks[7]);
    while(board){
        int dest = pop_lsb(&board);
        *move_list++ = ((dest - 7) << 10) | (dest << 4) | CAPTURE_FLAG;
    }
    //PROMOTIONS:
    board = (side_pawn_board << 8) & empty_board & (rank_masks[7]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_moves(dest - 8, dest);
    }
    board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (rank_masks[7]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_cap_moves(dest - 9, dest);
    }
    board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (rank_masks[7]);
    while(board){
        int dest = pop_lsb(&board);
        add_promo_cap_moves(dest - 7, dest);
    }
    board = ep_target_lookup[game_history[ply].ep_target] & side_pawn_board;
    while(board){
        int source = pop_lsb(&board);
        *move_list++ = (source << 10) | ((game_history[ply].ep_target + 8) << 4) | EN_PASSANT_FLAG;
    }
}

inline void Bitboard_Gen::add_black_castle_moves(){
    //black ks castle logic, first line: if h8 and e8 has been undisturbed
    //2nd line: if not in check and not castling through check
    //3d line: squares between are empty
    if((game_history[ply].castling_rights & BKS_CASTLING_RIGHTS)
       && !(enemy_attacked_board & (occupy_square[60] | occupy_square[61] | occupy_square[62]))
       && !(occupied_board & (occupy_square[61] | occupy_square[62]))){
        *move_list++ = (60 << 10) | (62 << 4) |KINGSIDE_CASTLE_FLAG;
    }
    //black qs castle
    if((game_history[ply].castling_rights & BQS_CASTLING_RIGHTS)
       && !(enemy_attacked_board & (occupy_square[60] | occupy_square[59] | occupy_square[58]))
       && !(occupied_board & (occupy_square[57] | occupy_square[58] | occupy_square[59]))){
        *move_list++ = (60 << 10) | (58 << 4) |QUEENSIDE_CASTLE_FLAG;
    }
}

inline void Bitboard_Gen::add_white_castle_moves(){
    //white castle logic, if confused, read the black castle comments above
    if((game_history[ply].castling_rights & WKS_CASTLING_RIGHTS)
       && !(enemy_attacked_board & (occupy_square[4] | occupy_square[5] | occupy_square[6]))
       && !(occupied_board & (occupy_square[5] | occupy_square[6]))){
        *move_list++ = (4 << 10) | (6 << 4) | KINGSIDE_CASTLE_FLAG;
    }
    //white qs castle
    if((game_history[ply].castling_rights & WQS_CASTLING_RIGHTS)
       && !(enemy_attacked_board & (occupy_square[2] | occupy_square[3] | occupy_square[4]))
       && !(occupied_board & (occupy_square[1] | occupy_square[2] | occupy_square[3]))){
        *move_list++ = (4 << 10) | (2 << 4) |QUEENSIDE_CASTLE_FLAG;
    }
}

inline void Bitboard_Gen::add_knight_moves(){
    U64 knight_board = bitboards[current_side] & bitboards[KNIGHT_BOARD];
    while(knight_board){
        int source = pop_lsb(&knight_board);
        U64 quiet_dests = knight_move_lookup[source] & empty_board;
        add_quiet_moves(source, quiet_dests);
        //intersect target moves with opposing pieces
        U64 capture_dests = knight_move_lookup[source] & bitboards[!current_side];
        add_capture_moves(source, capture_dests);
    }
}

inline void Bitboard_Gen::add_king_moves(){
    int king_source = get_square_index(bitboards[current_side] & bitboards[KING_BOARD]);
    U64 king_quiet_dests = king_move_lookup[king_source] & empty_board & (~enemy_attacked_board);
    add_quiet_moves(king_source, king_quiet_dests);
    //intersect target moves with opposing pieces
    U64 king_capture_dests = king_move_lookup[king_source] & bitboards[!current_side]  & (~enemy_attacked_board);
    add_capture_moves(king_source, king_capture_dests);
}

inline void Bitboard_Gen::add_diag_moves(){
    U64 diag_board = bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(diag_board){
        int source = pop_lsb(&diag_board);
        U64 res = hyp_quint(source, diagonal_masks[source_to_diagonal[source]]);
        res |= hyp_quint(source, antidiagonal_masks[source_to_antidiagonal[source]]);
        add_quiet_moves(source, res & empty_board);
        add_capture_moves(source, res & bitboards[!current_side]);
    }
}

inline void Bitboard_Gen::add_orthog_moves(){
    U64 orthog_board = bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(orthog_board){
        int source = pop_lsb(&orthog_board);
        U64 res = hyp_quint(source, file_masks[source_to_file[source]]);
        res |= hyp_quint_horiz(source, rank_masks[source_to_rank[source]]);
        add_quiet_moves(source, res & empty_board);
        add_capture_moves(source, res & bitboards[!current_side]);
    }
}



//generates only capture moves, for the quiesence search
int Bitboard_Gen::generate_captures(uint16_t * m_list){
    move_list = m_list;
    occupied_board = bitboards[WHITE] | bitboards[BLACK];
    empty_board = ~occupied_board;
    generate_attacked_squares();
    
    U64 side_pawn_board = bitboards[current_side] & bitboards[PAWN_BOARD];
    
    //only initializing one destboard
    U64 board;
    
    //Pawn captures
    if(current_side){
        //get all pawn captures except promos
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (~rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest + 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS::
        board = (side_pawn_board >> 9) & bitboards[WHITE] & (~file_masks[7]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            add_promo_cap_moves(dest + 9, dest);
        }
        board = (side_pawn_board >> 7) & bitboards[WHITE] & (~file_masks[0]) & (rank_masks[0]);
        while(board){
            int dest = pop_lsb(&board);
            add_promo_cap_moves(dest + 7, dest);
        }
    }
    else{
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 9) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (~rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            *move_list++ = ((dest - 7) << 10) | (dest << 4) | CAPTURE_FLAG;
        }
        
        //PROMOTIONS:
        board = (side_pawn_board << 9) & bitboards[BLACK] & (~file_masks[0]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            add_promo_cap_moves(dest - 9, dest);
        }
        board = (side_pawn_board << 7) & bitboards[BLACK] & (~file_masks[7]) & (rank_masks[7]);
        while(board){
            int dest = pop_lsb(&board);
            add_promo_cap_moves(dest - 7, dest);
        }
    }
    //knight captures
    board = bitboards[current_side] & bitboards[KNIGHT_BOARD];
    while(board){
        int source = pop_lsb(&board);
        //intersect target moves with opposing pieces
        U64 capture_dests = knight_move_lookup[source] & bitboards[!current_side];
        add_capture_moves(source, capture_dests);
    }
    
    //generate all king moves, no need to loop since there is only one king
    //once attacked squares are calculated, king goes to only unattacked squares
    int king_source = get_square_index(bitboards[current_side] & bitboards[KING_BOARD]);
    //intersect target moves with opposing pieces
    U64 king_capture_dests = king_move_lookup[king_source] & bitboards[!current_side]  & (~enemy_attacked_board);
    add_capture_moves(king_source, king_capture_dests);
    
    //generate all bishop queen sliding moves
    board = bitboards[current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, diagonal_masks[source_to_diagonal[source]]);
        res |= hyp_quint(source, antidiagonal_masks[source_to_antidiagonal[source]]);
        add_capture_moves(source, res & bitboards[!current_side]);
    }
    
    //generate all rook and queen moves
    board = bitboards[current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(board){
        int source = pop_lsb(&board);
        U64 res = hyp_quint(source, file_masks[source_to_file[source]]);
        res |= hyp_quint_horiz(source, rank_masks[source_to_rank[source]]);
        add_capture_moves(source, res & bitboards[!current_side]);
    }
    return (int) (move_list - m_list);
}

//generates all squares attacked by side
void Bitboard_Gen::generate_attacked_squares(){
    enemy_attacked_board = 0;
    //bulk process all pawn attacks
    U64 side_pawn_board = bitboards[!current_side] & bitboards[PAWN_BOARD];
    if(!current_side){
        enemy_attacked_board |= (side_pawn_board >> 9) & (~file_masks[7]);
        enemy_attacked_board |= (side_pawn_board >> 7) & (~file_masks[0]);
    }else{
        enemy_attacked_board |= (side_pawn_board << 9) & (~file_masks[0]);
        enemy_attacked_board |= (side_pawn_board << 7) & (~file_masks[7]);
    }
    
    //get all knight moves
    U64 side_knight_board = bitboards[!current_side] & bitboards[KNIGHT_BOARD];
    while(side_knight_board){
        int source = pop_lsb(&side_knight_board);
        enemy_attacked_board |= knight_move_lookup[source];
    }
    
    //get the king moves, errors if no king
    int king_source = get_square_index(bitboards[!current_side] & bitboards[KING_BOARD]);
    enemy_attacked_board |= king_move_lookup[king_source];

    //get all diag sliders
    U64 side_diagonal_board = bitboards[!current_side] & (bitboards[BISHOP_BOARD] | bitboards[QUEEN_BOARD]);
    while(side_diagonal_board){
        int source = pop_lsb(&side_diagonal_board);
        enemy_attacked_board |= hyp_quint(source, diagonal_masks[source_to_diagonal[source]]);
        enemy_attacked_board |= hyp_quint(source, antidiagonal_masks[source_to_antidiagonal[source]]);
    }
    
    U64 side_orthog_board = bitboards[!current_side] & (bitboards[ROOK_BOARD] | bitboards[QUEEN_BOARD]);
    while(side_orthog_board){
        int source = pop_lsb(&side_orthog_board);
        enemy_attacked_board |= hyp_quint(source, file_masks[source_to_file[source]]);
        enemy_attacked_board |= hyp_quint_horiz(source, rank_masks[source_to_rank[source]]);
    }
    
}

//only works for vertical and diag
U64 Bitboard_Gen::hyp_quint(int source, U64 mask) {
    return (((mask & occupied_board) - occupy_square[source] * 2) ^
            reverse(reverse(mask & occupied_board) - reverse(occupy_square[source]) * 2)) & mask;
}

//only works for horizontal
U64 Bitboard_Gen::hyp_quint_horiz(int source, U64 mask) {
    return (((mask & occupied_board) - occupy_square[source] * 2) ^
            mirror(mirror(mask & occupied_board) - mirror(occupy_square[source]) * 2)) & mask;
}

inline void Bitboard_Gen::add_quiet_moves(int source, U64 dests){
    while(dests){
        int dest = pop_lsb(&dests);
        *move_list++ = (source << 10) | (dest << 4) | QUIET_FLAG;
    }
}

inline void Bitboard_Gen::add_capture_moves(int source, U64 dests){
    while(dests){
        int dest = pop_lsb(&dests);
        *move_list++ = (source << 10) | (dest << 4) | CAPTURE_FLAG;
    }
}

inline void Bitboard_Gen::add_promo_moves(int source, int dest){
    *move_list++ = (source << 10) | (dest << 4) | KNIGHT_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | BISHOP_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | ROOK_PROMO_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | QUEEN_PROMO_FLAG;
}

inline void Bitboard_Gen::add_promo_cap_moves(int source, int dest){
    *move_list++ = (source << 10) | (dest << 4) | KNIGHT_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | BISHOP_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | ROOK_PROMO_CAP_FLAG;
    *move_list++ = (source << 10) | (dest << 4) | QUEEN_PROMO_CAP_FLAG;
}
