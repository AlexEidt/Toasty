#include <stdint.h>
#include "board.h"
#include "bitboard.h"
#include "move.h"

int extract_moves_pawns(Bitboard board, int8_t offset, Move* moves, int start, Flag flag) {
    while (board != 0) {
        uint8_t pos = LSB(board);
        board = clear_bit(board, pos);
        Move* move = &moves[start++];
        move->to = pos; move->from = pos - offset; move->flags = flag;
    }

    return start;
}

int extract_moves_pawns_promotions(Bitboard board, int8_t offset, Move* moves, int start, Flag flag) {
    while (board != 0) {
        uint8_t pos = LSB(board);
        board = clear_bit(board, pos);
        Move* move;
        move = &moves[start++];
        move->to = pos; move->from = pos - offset; move->flags = ADD_PROMOTED_PIECE(QUEEN) | flag;
        move = &moves[start++];
        move->to = pos; move->from = pos - offset; move->flags = ADD_PROMOTED_PIECE(KNIGHT) | flag;
        move = &moves[start++];
        move->to = pos; move->from = pos - offset; move->flags = ADD_PROMOTED_PIECE(BISHOP) | flag;
        move = &moves[start++];
        move->to = pos; move->from = pos - offset; move->flags = ADD_PROMOTED_PIECE(ROOK) | flag;
    }

    return start;
}

int gen_pawn_pushes(Board* board, Move* moves, int index) {
    Bitboard pawns = get_pieces(board, PAWN, board->active_color);
    Bitboard empty = ~get_all_pieces(board);

    Bitboard single_pushes, double_pushes;
    int8_t offset;

    if (WHITE_TO_MOVE(board)) {
        single_pushes = ((pawns & ~RANK7) << 8) & empty;
        double_pushes = ((single_pushes & RANK3) << 8) & empty;
        offset = 8;
    } else {
        single_pushes = ((pawns & ~RANK2) >> 8) & empty;
        double_pushes = ((single_pushes & RANK6) >> 8) & empty;
        offset = -8;
    }

    index = extract_moves_pawns(single_pushes, offset, moves, index, QUIET);
    index = extract_moves_pawns(double_pushes, offset * 2, moves, index, PAWN_DOUBLE | QUIET);

    return index;
}

int gen_pawn_captures(Board* board, Move* moves, int index) {
    Bitboard pawns = get_pieces(board, PAWN, board->active_color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    if (WHITE_TO_MOVE(board)) {
        Bitboard valid_pawns = pawns & ~RANK7;
        Bitboard capture_left = ((valid_pawns & ~FILEA) << 9) & enemies;
        index = extract_moves_pawns(capture_left, 9, moves, index, CAPTURE);
        Bitboard capture_right = ((valid_pawns & ~FILEH) << 7) & enemies;
        index = extract_moves_pawns(capture_right, 7, moves, index, CAPTURE);
    } else {
        Bitboard valid_pawns = pawns & ~RANK2;
        Bitboard capture_left = ((valid_pawns & ~FILEH) >> 9) & enemies;
        index = extract_moves_pawns(capture_left, -9, moves, index, CAPTURE);
        Bitboard capture_right = ((valid_pawns & ~FILEA) >> 7) & enemies;
        index = extract_moves_pawns(capture_right, -7, moves, index, CAPTURE);
    }

    return index;
}

int gen_pawn_promotions_quiets(Board* board, Move* moves, int index) {
    Bitboard pawns = get_pieces(board, PAWN, board->active_color);
    Bitboard empty = ~get_all_pieces(board);

    Bitboard promotions;
    int8_t offset;

    if (WHITE_TO_MOVE(board)) {
        promotions = ((pawns & RANK7) << 8) & empty;
        offset = 8;
    } else {
        promotions = ((pawns & RANK2) >> 8) & empty;
        offset = -8;
    }

    index = extract_moves_pawns_promotions(promotions, offset, moves, index, QUIET);

    return index;
}

int gen_pawn_promotions_captures(Board* board, Move* moves, int index) {
    Bitboard pawns = get_pieces(board, PAWN, board->active_color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    if (WHITE_TO_MOVE(board)) {
        Bitboard valid_pawns = pawns & RANK7;
        Bitboard capture_left = ((valid_pawns & ~FILEA) << 9) & enemies;
        index = extract_moves_pawns_promotions(capture_left, 9, moves, index, CAPTURE);
        Bitboard capture_right = ((valid_pawns & ~FILEH) << 7) & enemies;
        index = extract_moves_pawns_promotions(capture_right, 7, moves, index, CAPTURE);
    } else {
        Bitboard valid_pawns = pawns & RANK2;
        Bitboard capture_left = ((valid_pawns & ~FILEH) >> 9) & enemies;
        index = extract_moves_pawns_promotions(capture_left, -9, moves, index, CAPTURE);
        Bitboard capture_right = ((valid_pawns & ~FILEA) >> 7) & enemies;
        index = extract_moves_pawns_promotions(capture_right, -7, moves, index, CAPTURE);
    }

    return index;
}

int gen_pawn_en_passant(Board* board, Move* moves, int index) {
    Bitboard pawns = get_pieces(board, PAWN, board->active_color);
    Bitboard en_passant = 1 << board->en_passant;

    if (WHITE_TO_MOVE(board)) {
        Bitboard capture_left = ((pawns & ~FILEA) << 9) & en_passant;
        index = extract_moves_pawns(capture_left, 9, moves, index, CAPTURE | EN_PASSANT);
        Bitboard capture_right = ((pawns & ~FILEH) << 7) & en_passant;
        index = extract_moves_pawns(capture_right, 7, moves, index, CAPTURE | EN_PASSANT);
    } else {
        Bitboard capture_left = ((pawns & ~FILEH) >> 9) & en_passant;
        index = extract_moves_pawns(capture_left, -9, moves, index, CAPTURE | EN_PASSANT);
        Bitboard capture_right = ((pawns & ~FILEA) >> 7) & en_passant;
        index = extract_moves_pawns(capture_right, -7, moves, index, CAPTURE | EN_PASSANT);
    }

    return index;
}

int extract_moves(Bitboard board, int8_t init, Move* moves, int start, Flag flag) {
    while (board != 0) {
        uint8_t pos = LSB(board);
        board = clear_bit(board, pos);
        Move* move = &moves[start++];
        move->to = pos; move->from = init; move->flags = flag;
    }

    return start;
}

int gen_knight_moves(Board* board, Move* moves, int index) {
    Bitboard knights = get_pieces(board, KNIGHT, board->active_color);
    Bitboard empty = ~get_all_pieces(board);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    while (knights != 0) {
        uint8_t pos = LSB(knights);
        knights = clear_bit(knights, pos);
        index = extract_moves(knight_moves[pos] & empty, pos, moves, index, QUIET);
        index = extract_moves(knight_moves[pos] & enemies, pos, moves, index, CAPTURE);
    }

    return index;
}

int gen_king_moves(Board* board, Move* moves, int index) {
    Bitboard king = get_pieces(board, KING, board->active_color);
    Bitboard empty = ~get_all_pieces(board);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    uint8_t pos = LSB(king);
    index = extract_moves(king_moves[pos] & empty, pos, moves, index, QUIET);
    index = extract_moves(king_moves[pos] & enemies, pos, moves, index, CAPTURE);

    return index;
}

int gen_rook_moves(Board* board, Move* moves, int index) {
    return gen_cardinal_moves(board, moves, index, ROOK);
}

int gen_bishop_moves(Board* board, Move* moves, int index) {
    return gen_intercardinal_moves(board, moves, index, BISHOP);
}

int gen_queen_moves(Board* board, Move* moves, int index) {
    index = gen_cardinal_moves(board, moves, index, QUEEN);
    index = gen_intercardinal_moves(board, moves, index, QUEEN);
    return index;
}

int gen_castle_moves(Board* board, Move* moves, int index) {
    switch_ply(board);
    Bitboard attacks = gen_attacks(board);
    switch_ply(board);

    Bitboard king = get_pieces(board, KING, board->active_color);
    if (king & attacks == 0) { // If king is not in check.
        int index = board->active_color >> 2; // Maps White to 0, Black to 1.
        Bitboard kingside = castling[index][KINGSIDE_PATH];
        Bitboard queenside = castling[index][QUEENSIDE_PATH];
        Bitboard queenside_path_rook = castling[index][QUEENSIDE_PATH_TO_ROOK];
        Bitboard king_pos = castling[index][KING_POSITION];
        Bitboard king_dst_kingside = castling[index][KING_DST_KINGSIDE];
        Bitboard king_dst_queenside = castling[index][KING_DST_QUEENSIDE];

        Bitboard us = get_pieces_color(board, board->active_color);
        Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

        if (can_castle_kingside(board, board->active_color)) {
            if ((kingside & attacks & us & enemies) == 0) {
                Move* move = &moves[index++];
                move->to = king_dst_kingside; move->from = king_pos; move->flags = CASTLE_KINGSIDE;
            }
        }
        if (can_castle_queenside(board, board->active_color)) {
            if ((queenside & attacks & us & enemies) == 0 && (queenside_path_rook & us & enemies) == 0) {
                Move* move = &moves[index++];
                move->to = king_dst_queenside; move->from = king_pos; move->flags = CASTLE_QUEENSIDE;
            }
        }
    }

    return index;
}

int gen_cardinal_moves(Board* board, Move* moves, int index, Piece piece) {
    Piece color = board->active_color;
    Bitboard cardinal = get_pieces(board, piece, color);
    Bitboard us = get_pieces_color(board, color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    while (cardinal != 0) {
        uint8_t pos = LSB(cardinal);
        cardinal = clear_bit(cardinal, pos);

        int ai, qi;
        Bitboard ray;
    
        ray = rook_moves[pos][NORTH];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= rook_moves[ai >= qi ? ai : qi - 8][NORTH];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = rook_moves[pos][EAST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= rook_moves[ai >= qi ? ai : qi + 1][EAST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = rook_moves[pos][SOUTH];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= rook_moves[ai <= qi ? ai : qi + 8][SOUTH];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = rook_moves[pos][WEST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= rook_moves[ai <= qi ? ai : qi - 1][WEST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);
    }

    return index;
}

int gen_intercardinal_moves(Board* board, Move* moves, int index, Piece piece) {
    Piece color = board->active_color;
    Bitboard intercardinal = get_pieces(board, piece, color);
    Bitboard us = get_pieces_color(board, color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    while (intercardinal != 0) {
        uint8_t pos = LSB(intercardinal);
        intercardinal = clear_bit(intercardinal, pos);

        int ai, qi;
        Bitboard ray;

        ray = bishop_moves[pos][SOUTHEAST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= bishop_moves[ai >= qi ? ai : qi + 9][SOUTHEAST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = bishop_moves[pos][SOUTHWEST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= bishop_moves[ai >= qi ? ai : qi + 7][SOUTHWEST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = bishop_moves[pos][NORTHEAST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= bishop_moves[ai <= qi ? ai : qi - 7][NORTHEAST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);

        ray = bishop_moves[pos][NORTHWEST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= bishop_moves[ai <= qi ? ai : qi - 9][NORTHWEST];
        index = extract_moves(ray & enemies, pos, moves, index, CAPTURE);
        index = extract_moves(ray & ~enemies, pos, moves, index, QUIET);
    }

    return index;
}

Bitboard gen_cardinal_attacks(Board* board, Piece piece) {
    Bitboard attacks = 0;

    Piece color = board->active_color;
    Bitboard cardinal = get_pieces(board, piece, color);
    Bitboard us = get_pieces_color(board, color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    while (cardinal != 0) {
        uint8_t pos = LSB(cardinal);
        cardinal = clear_bit(cardinal, pos);

        int ai, qi;
        Bitboard ray;
    
        ray = rook_moves[pos][NORTH];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= rook_moves[ai >= qi ? ai : qi - 8][NORTH];
        attacks |= ray;

        ray = rook_moves[pos][EAST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= rook_moves[ai >= qi ? ai : qi + 1][EAST];
        attacks |= ray;

        ray = rook_moves[pos][SOUTH];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= rook_moves[ai <= qi ? ai : qi + 8][SOUTH];
        attacks |= ray;

        ray = rook_moves[pos][WEST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= rook_moves[ai <= qi ? ai : qi - 1][WEST];
        attacks |= ray;
    }

    return attacks;
}

Bitboard gen_intercardinal_attacks(Board* board, Piece piece) {
    Bitboard attacks = 0;

    Piece color = board->active_color;
    Bitboard intercardinal = get_pieces(board, piece, color);
    Bitboard us = get_pieces_color(board, color);
    Bitboard enemies = get_pieces_color(board, OPPOSITE(board->active_color));

    while (intercardinal != 0) {
        uint8_t pos = LSB(intercardinal);
        intercardinal = clear_bit(intercardinal, pos);

        int ai, qi;
        Bitboard ray;
    
        ray = bishop_moves[pos][SOUTHEAST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= bishop_moves[ai >= qi ? ai : qi + 9][SOUTHEAST];
        attacks |= ray;

        ray = bishop_moves[pos][SOUTHWEST];
        ai = MSB(ray & enemies);
        qi = MSB(ray & us);
        ray ^= bishop_moves[ai >= qi ? ai : qi + 7][SOUTHWEST];
        attacks |= ray;

        ray = bishop_moves[pos][NORTHEAST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= bishop_moves[ai <= qi ? ai : qi - 7][NORTHEAST];
        attacks |= ray;

        ray = bishop_moves[pos][NORTHWEST];
        ai = LSB(ray & enemies);
        qi = LSB(ray & us);
        ray ^= bishop_moves[ai <= qi ? ai : qi - 9][NORTHWEST];
        attacks |= ray;
    }

    return attacks;
}

Bitboard gen_attacks(Board* board) {
    Bitboard attacks = 0;

    Piece active = board->active_color;
    Piece inactive = OPPOSITE(active);
    Bitboard empty = ~get_all_pieces(board);

    // Pawn Attacks.
    Bitboard pawns = get_pieces(board, PAWN, active);
    if (WHITE_TO_MOVE(board)) {
        attacks |= (pawns & ~FILEA) << 9;
        attacks |= (pawns & ~FILEH) << 7;
    } else {
        attacks |= (pawns & ~FILEH) >> 9;
        attacks |= (pawns & ~FILEA) >> 7;
    }

    // Knight Attacks.
    Bitboard knights = get_pieces(board, KNIGHT, active);
    while (knights != 0) {
        uint8_t pos = LSB(knights);
        knights = clear_bit(knights, pos);
        attacks |= knight_moves[pos];
    }

    // King Attacks.
    Bitboard king = get_pieces(board, KING, active);
    attacks |= king_moves[LSB(king)];

    // Sliding Attacks.
    attacks |= gen_cardinal_attacks(board, ROOK);
    attacks |= gen_intercardinal_attacks(board, BISHOP);
    attacks |= gen_cardinal_attacks(board, QUEEN);
    attacks |= gen_intercardinal_attacks(board, QUEEN);
    
    return attacks;
}

int gen_legal_moves(Board* board, Move* moves) {
    int index = 0;

    index = gen_pawn_pushes(board, moves, index);
    index = gen_pawn_captures(board, moves, index);
    index = gen_pawn_promotions_quiets(board, moves, index);
    index = gen_pawn_promotions_captures(board, moves, index);
    index = gen_pawn_en_passant(board, moves, index);

    index = gen_knight_moves(board, moves, index);
    index = gen_king_moves(board, moves, index);
    index = gen_rook_moves(board, moves, index);
    index = gen_bishop_moves(board, moves, index);
    index = gen_queen_moves(board, moves, index);

    index = gen_castle_moves(board, moves, index);

    Piece active = board->active_color;

    int size = 0;
    for (int i = 0; i < index; i++) {
        Move* move = &moves[i];
        make_move(board, move);
        Bitboard king = get_pieces_color(board, active);
        Bitboard attacks = gen_attacks(board);
        // If king is not in check after making the move, add the move.
        if (king & attacks == 0) {
            moves[size++] = *move;
        }
        undo_move(board, move);
    }

    return size;
}
