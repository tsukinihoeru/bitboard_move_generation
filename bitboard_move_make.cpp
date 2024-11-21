//
//  bitboard_move_make.cpp
//  InvincibleSummer
//
//  Created by Harry Chiu on 11/15/24.
//

#include "bitboard_gen.h"

void Bitboard_Gen::make_move(uint16_t move){
    pre_update_hash();
    int ep_target = 0;
    int captured_piece = 0;
    
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    
    uint8_t new_castling_rights = handle_castling_rights(source, dest);
    
    if(flag == QUIET_FLAG){
        move_piece(source, dest);
    }
    else if(flag == DOUBLE_PAWN_PUSH_FLAG){
        move_piece(source, dest);
        ep_target = dest;
    }
    else if(flag == CAPTURE_FLAG){
        captured_piece = mailbox[dest];
        remove_piece(dest);
        move_piece(source, dest);
    }
    else if(flag == KINGSIDE_CASTLE_FLAG){
        move_piece(source, dest);
        move_piece(source + 3, source + 1);
    }
    else if(flag == QUEENSIDE_CASTLE_FLAG){
        move_piece(source, dest);
        move_piece(source - 4, source - 1);
    }
    else if(flag == EN_PASSANT_FLAG){
        captured_piece = mailbox[game_history[ply].ep_target];
        move_piece(source, dest);
        remove_piece(game_history[ply].ep_target);
    }
    else if((flag & 12) == 8){
        remove_piece(source);
        add_piece(current_side, flag - 5, dest);
    }
    else if((flag & 12) == 12){
        captured_piece = mailbox[dest];
        remove_piece(source);
        remove_piece(dest);
        add_piece(current_side, flag - 9, dest);
    }
    ply++;
    game_history[ply] = game_state(new_castling_rights, captured_piece, ep_target);
    post_update_hash();
    hash_history[ply] = zobrist_hash;
}

void Bitboard_Gen::unmake_move(uint16_t move){
    pre_update_hash();
    
    int source = (move >> 10) & 0x3f;
    int dest = (move >> 4) & 0x3f;
    int flag = move & 0x0f;
    
    move_piece(dest, source);
    
    if(flag == CAPTURE_FLAG){
        add_piece(game_history[ply].captured, dest);
    }
    else if(flag == KINGSIDE_CASTLE_FLAG){
        add_piece(mailbox[source + 1], source + 3);
        remove_piece(source + 1);
    }
    else if(flag == QUEENSIDE_CASTLE_FLAG){
        add_piece(mailbox[source - 1], source - 4);
        remove_piece(source -1);
    }
    else if(flag == EN_PASSANT_FLAG){
        add_piece(game_history[ply].captured, game_history[ply - 1].ep_target);
    }
    else if((flag & 12) == 8){
        remove_piece(source);
        add_piece(!current_side, PAWN_BOARD, source);
    }
    else if((flag & 12) == 12){
        remove_piece(source);
        add_piece(game_history[ply].captured, dest);
        add_piece(!current_side, PAWN_BOARD, source);
    }
    ply--;
    post_update_hash();
}

void Bitboard_Gen::make_null_move(){
    pre_update_hash();
    ply++;
    game_history[ply] = game_state(game_history[ply - 1].castling_rights, 0, 0);
    post_update_hash();
}

void Bitboard_Gen::unmake_null_move(){
    pre_update_hash();
    ply--;
    post_update_hash();
}
inline void Bitboard_Gen::pre_update_hash(){
    zobrist_hash ^= zobrist_keys.castling[game_history[ply].castling_rights];
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
}
inline void Bitboard_Gen::post_update_hash(){
    current_side = !current_side;
    zobrist_hash ^= zobrist_keys.color;
    zobrist_hash ^= zobrist_keys.castling[game_history[ply].castling_rights];
    zobrist_hash ^= zobrist_keys.ep_squares[game_history[ply].ep_target];
}

uint8_t Bitboard_Gen::handle_castling_rights(int source, int dest){
    uint8_t new_castling_rights = game_history[ply].castling_rights;
    switch (source){
        case 0: //a1
            new_castling_rights &= ~WQS_CASTLING_RIGHTS;
            break;
        case 4: //e1
            new_castling_rights &= ~(WQS_CASTLING_RIGHTS | WKS_CASTLING_RIGHTS);
            break;
        case 7: //h1
            new_castling_rights &= ~WKS_CASTLING_RIGHTS;
            break;
        case 56: //a8
            new_castling_rights &= ~BQS_CASTLING_RIGHTS;
            break;
        case 60: //e8
            new_castling_rights &= ~(BQS_CASTLING_RIGHTS | BKS_CASTLING_RIGHTS);
            break;
        case 63: //h8
            new_castling_rights &= ~BKS_CASTLING_RIGHTS;
            break;
    }
    switch (dest){
        case 0: //a1
            new_castling_rights &= ~WQS_CASTLING_RIGHTS;
            break;
        case 4: //e1
            new_castling_rights &= ~(WQS_CASTLING_RIGHTS | WKS_CASTLING_RIGHTS);
            break;
        case 7: //h1
            new_castling_rights &= ~WKS_CASTLING_RIGHTS;
            break;
        case 56: //a8
            new_castling_rights &= ~BQS_CASTLING_RIGHTS;
            break;
        case 60: //e8
            new_castling_rights &= ~(BQS_CASTLING_RIGHTS | BKS_CASTLING_RIGHTS);
            break;
        case 63: //h8
            new_castling_rights &= ~BKS_CASTLING_RIGHTS;
            break;
    }
    return new_castling_rights;
}
