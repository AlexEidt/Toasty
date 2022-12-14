#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include "tinycthread.h"
#include "search.h"
#include "opening.h"
#include "board.h"
#include "evaluate.h"
#include "move.h"
#include "hashmap.h"

int timer(void* arg) {
    sleep(SEARCH_TIMEOUT);
    *(bool*) arg = true;
    return 0;
}

void start_timer(bool* stop) {
    thrd_t thrd;
    thrd_create(&thrd, timer, stop);
}

bool select_move(Board* board, HashMap* hashmap, Move* move) {
    if (IN_OPENING_BOOK(board)) {
        // If an opening could be found, make that move.
        if (select_opening(board, move)) {
            sleep(SEARCH_TIMEOUT);
            return true;
        }
    }

    hashmap_clear(hashmap);

    bool stop = false;
    start_timer(&stop);

    Move selected;

    int score = 0;
    int depth = 1;
    while (!stop) {
        // MTDF
        int upper = INT_MAX;
        int lower = INT_MIN;

        while (lower < upper && !stop) {
            int beta = MAX(score, lower + 1);
            score = search_moves(board, &stop, hashmap, depth, beta - 1, beta, &selected);
            if (score < beta) {
                upper = score;
            } else {
                lower = score;
            }
        }
        depth++;
    }

    *move = selected;

    Move moves[MAX_MOVES];
	return gen_moves(board, moves) > 0;
}

int search_moves(Board* board, bool* stop, HashMap* hashmap, int depth, int alpha, int beta, Move* selected) {
    Move moves[MAX_MOVES];
    int n_moves = gen_moves(board, moves);

    order_moves(board, moves, n_moves);

    Move best = {0, 0, 0};

    const Board copy = *board;
    for (int i = 0; i < n_moves && !*stop; i++) {
        Move* move = &moves[i];
        make_move(board, move);
        int eval = -alpha_beta(board, stop, hashmap, depth - 1, 1, -beta, -alpha);
        *board = copy; // Undo move.

        if (eval > alpha) {
            alpha = eval;
            best = *move;
        }
    }

    if (best.to != best.from) {
        *selected = best;
    }

    return alpha;
}

int alpha_beta(Board* board, bool* stop, HashMap* hashmap, int depth, int ply, int alpha, int beta) {
    if (*stop) return 0;
    if (!is_legal(board)) return INF;

    if (ply > 0) {
        alpha = MAX(alpha, -CHECKMATE + ply);
        beta = MIN(beta, CHECKMATE - ply);
        if (alpha >= beta) return alpha;
    }

    uint64_t board_hash = hash(board);
    int score, flag;
    if (flag = hashmap_get(hashmap, board_hash, depth, &score)) {
        if (flag == BOUND_EXACT || (flag == BOUND_UPPER && score <= alpha) || (flag == BOUND_LOWER && score >= beta)) {
            return score;
        }
    }

    if (depth <= 0) {
        // Once depth of 0 is reached, search all remaining captures to reach a stable board state.
        int eval = quiescence(board, alpha, beta);
        hashmap_set(hashmap, board_hash, eval, depth, BOUND_EXACT);
        return eval;
    }

    // Null Move Pruning.
    switch_ply(board);
    uint8_t en_passant = board->en_passant;
    board->en_passant = 0;
    int eval = -alpha_beta(board, stop, hashmap, depth - 2, ply + 2, -beta, -beta + 1);
    board->en_passant = en_passant;
    switch_ply(board);

    if (eval >= beta) {
        hashmap_set(hashmap, board_hash, beta, depth, BOUND_LOWER);
        return beta;
    }

    Move moves[MAX_MOVES];
    int n_moves = gen_moves(board, moves);
    if (n_moves == 0) {
        if (is_in_check(board)) {
            return -CHECKMATE + ply;
        }
        return 0;
    }

    order_moves(board, moves, n_moves);
    const Board copy = *board;

    for (int i = 0; i < n_moves && !*stop; i++) {
        Move* move = &moves[i];
        make_move(board, move);
        int eval = -alpha_beta(board, stop, hashmap, depth - 1, ply + 1, -beta, -alpha);
        *board = copy; // Undo move.

        if (eval >= beta) {
            hashmap_set(hashmap, board_hash, beta, depth, BOUND_LOWER);
            return beta;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }

    hashmap_set(hashmap, board_hash, alpha, depth, BOUND_UPPER);

    return alpha;
}

int quiescence(Board* board, int alpha, int beta) {
    int eval = evaluate(board);

    if (eval >= beta) return beta;
    if (eval > alpha) alpha = eval;

    Move moves[MAX_MOVES];
    int n_moves = gen_captures(board, moves);
    order_moves(board, moves, n_moves);

    const Board copy = *board;

    for (int i = 0; i < n_moves; i++) {
        make_move(board, &moves[i]);
        eval = -quiescence(board, -beta, -alpha);
        *board = copy; // Undo move.

        if (eval >= beta) return beta;
        if (eval > alpha) alpha = eval;
    }

    return alpha;
}

void order_moves(Board* board, Move* moves, int size) {
    int scores[MAX_MOVES];
    Move best;

    for (int i = 0; i < size; i++) {
        scores[i] = -score_move(board, &moves[i]);
    }

    // Sort moves based on their scores.
    for (int i = 1; i < size; i++) {
        int j = i;
        while (j > 0 && scores[j - 1] > scores[j]) {
            int temp = scores[j];
            scores[j] = scores[j - 1];
            scores[j - 1] = temp;

            Move temp_move = moves[j];
            moves[j] = moves[j - 1];
            moves[j - 1] = temp_move;
            
            j--;
        }
    }
}