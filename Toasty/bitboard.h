#ifndef BITBOARD_H_
#define BITBOARD_H_

#include <stdint.h>

typedef uint64_t Bitboard;

#define LSB(x) (__builtin_ctzll(x))
#define MSB(x) (63 - __builtin_clzll(x))
#define COUNT(x) (__builtin_popcountll(x))

#define ADD_BIT(board, pos) ((board) |= (1ULL << (pos)))
#define CLEAR_BIT(board, pos) ((board) &= ~(1ULL << (pos)))

#define RANK1 0b0000000000000000000000000000000000000000000000000000000011111111ULL
#define RANK2 0b0000000000000000000000000000000000000000000000001111111100000000ULL
#define RANK3 0b0000000000000000000000000000000000000000111111110000000000000000ULL
#define RANK4 0b0000000000000000000000000000000011111111000000000000000000000000ULL
#define RANK5 0b0000000000000000000000001111111100000000000000000000000000000000ULL
#define RANK6 0b0000000000000000111111110000000000000000000000000000000000000000ULL
#define RANK7 0b0000000011111111000000000000000000000000000000000000000000000000ULL
#define RANK8 0b1111111100000000000000000000000000000000000000000000000000000000ULL

#define FILEA 0b1000000010000000100000001000000010000000100000001000000010000000ULL
#define FILEB 0b0100000001000000010000000100000001000000010000000100000001000000ULL
#define FILEC 0b0010000000100000001000000010000000100000001000000010000000100000ULL
#define FILED 0b0001000000010000000100000001000000010000000100000001000000010000ULL
#define FILEE 0b0000100000001000000010000000100000001000000010000000100000001000ULL
#define FILEF 0b0000010000000100000001000000010000000100000001000000010000000100ULL
#define FILEG 0b0000001000000010000000100000001000000010000000100000001000000010ULL
#define FILEH 0b0000000100000001000000010000000100000001000000010000000100000001ULL

#define H1 0
#define G1 1
#define F1 2
#define E1 3
#define D1 4
#define C1 5
#define B1 6
#define A1 7
#define H2 8
#define G2 9
#define F2 10
#define E2 11
#define D2 12
#define C2 13
#define B2 14
#define A2 15
#define H3 16
#define G3 17
#define F3 18
#define E3 19
#define D3 20
#define C3 21
#define B3 22
#define A3 23
#define H4 24
#define G4 25
#define F4 26
#define E4 27
#define D4 28
#define C4 29
#define B4 30
#define A4 31
#define H5 32
#define G5 33
#define F5 34
#define E5 35
#define D5 36
#define C5 37
#define B5 38
#define A5 39
#define H6 40
#define G6 41
#define F6 42
#define E6 43
#define D6 44
#define C6 45
#define B6 46
#define A6 47
#define H7 48
#define G7 49
#define F7 50
#define E7 51
#define D7 52
#define C7 53
#define B7 54
#define A7 55
#define H8 56
#define G8 57
#define F8 58
#define E8 59
#define D8 60
#define C8 61
#define B8 62
#define A8 63

#define Q1 0b1111000011110000111100001111000000000000000000000000000000000000ULL
#define Q2 0b0000111100001111000011110000111100000000000000000000000000000000ULL
#define Q3 0b0000000000000000000000000000000011110000111100001111000011110000ULL
#define Q4 0b0000000000000000000000000000000000001111000011110000111100001111ULL

#define SOUTH 0
#define WEST 1
#define NORTH 2
#define EAST 3

#define SOUTHEAST 0
#define SOUTHWEST 1
#define NORTHWEST 2
#define NORTHEAST 3

#define KINGSIDE_PATH 0
#define QUEENSIDE_PATH 1
#define QUEENSIDE_PATH_TO_ROOK 2
#define KING_POSITION 3
#define KING_DST_KINGSIDE 4
#define KING_DST_QUEENSIDE 5

Bitboard get_blocker(Bitboard mask, int square);
void init_magic_tables();
void init_rook_table();
void init_bishop_table();

extern const Bitboard KING_MOVES[64];
extern const Bitboard KNIGHT_MOVES[64];
extern const Bitboard ROOK_MOVES[65][4];
extern const Bitboard BISHOP_MOVES[65][4];
extern const Bitboard ROOK_MAGIC[64];
extern const Bitboard BISHOP_MAGIC[64];
extern const int ROOK_OFFSET[64];
extern const int BISHOP_OFFSET[64];
extern const Bitboard ROOK_BLOCKER_MASK[64];
extern const Bitboard BISHOP_BLOCKER_MASK[64];
extern Bitboard ROOK_TABLE[64][4096];
extern Bitboard BISHOP_TABLE[64][512];
extern const Bitboard CASTLING[2][6];

#endif