// Alex Eidt
// Pumpking Chess Engine.

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

extern "C" {
	#include "Pumpking/board.h"
	#include "Pumpking/move.h"
}

#define CAPTURE_AUDIO 0
#define END_AUDIO 1
#define MOVE_AUDIO 2
#define WARNING_AUDIO 3

class Chess : public olc::PixelGameEngine {
public:
	Chess() {
		sAppName = "Pumpking";
	}

private:
	int screenWidth, screenHeight;

	Board* chessboard;

	std::unordered_map<byte, std::unordered_map<byte, olc::Decal*>> pieces;
	std::vector<int> audio;

	// Moves the player can make on every square of the board.
	std::vector<Move*> possible[64];
	// Previously selected square.
	int selectedSource;
	int selectedDestination;

	// Moves the player can make.
	Move moves[256];

	// Size of one square on the chess board.
	int unit;
	// Size of a chess piece sprite in pixels.
	float pieceSize;
	// Colors for the chess board.
	olc::Pixel light, dark, background, highlight;

	// Flags for Pawn Promotions.
	bool isPromotion;
	Piece promoted;

	bool gameOver;
	Piece winner; // WHITE, BLACK, or DRAW.

	// Used to track duplicate move destination spots to avoid redrawing circles.
	std::unordered_map<int, int> possible_spots;

public:
	bool OnUserCreate() override {
		std::filesystem::path dir (std::filesystem::current_path().string());
		std::filesystem::path spriteDir ("Pieces");
		std::filesystem::path audioDir ("Audio");

		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);

		std::vector<std::string> files;
		for (const auto& file : std::filesystem::directory_iterator(dir / audioDir)) {
			files.push_back(file.path().string());
		}

		std::sort(files.begin(), files.end());
		// Load Audio wav files from the "audio" directory into the audio vector.
		for (const auto& file : files) {
			int id = olc::SOUND::LoadAudioSample(file);
			if (id == -1) throw std::runtime_error("Failed to load audio sample: " + file);
			audio.push_back(id);
		}

		// Load Sprite png files from the "pieces" directory into the pieces vector.
		for (const auto& file : std::filesystem::directory_iterator(dir / spriteDir)) {
			std::string filepath = file.path().string();
			olc::Sprite* sprite = new olc::Sprite(filepath);
			if (sprite == nullptr) throw std::runtime_error("Failed to load piece: " + filepath);

			std::string piece = file.path().stem().string();
			Piece type;
			switch (piece[0]) {
				case 'p': type = PAWN; break;
				case 'n': type = KNIGHT; break;
				case 'b': type = BISHOP; break;
				case 'r': type = ROOK; break;
				case 'q': type = QUEEN; break;
				case 'k': type = KING; break;
				default: throw std::runtime_error("Invalid piece type: " + piece[0]);
			}

			Piece color;
			switch (piece[1]) {
				case 'b': color = BLACK; break;
				case 'w': color = WHITE; break;
				default: throw std::runtime_error("Invalid color: " + piece[1]);
			}

			pieces[color][type] = new olc::Decal(sprite);
		}

		srand(std::time(0));

		screenWidth = ScreenWidth();
		screenHeight = ScreenHeight();
		unit = std::min(screenWidth, screenHeight) / 10;
		pieceSize = pieces[WHITE][ROOK]->sprite->width;

		light = {235, 236, 208};
		dark = {119, 149, 86};
		background = {49, 46, 43};
		highlight = {246, 246, 105};

		isPromotion = false;
		promoted = 0;

		gameOver = false;
		winner = 0;
		
		selectedSource = -1;
		selectedDestination = -1;

		chessboard = new Board();
		board_from_fen(chessboard, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		DrawBoard();
		GenerateMoves();

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override {
		DrawPieces();

		if (gameOver) {
			std::string text;
			switch (winner) {
				case WHITE:
					text = "White Wins!";
					break;
				case BLACK:
					text = "Black Wins!";
					break;
				case DRAW:
					text = "Draw";
					break;
			}
			DrawStringDecal({10, 10}, text, {255, 255, 255}, {5, 5});
			return true;
		}

		if (isPromotion) {
			DrawPromotion();
			// Check if piece is a Queen, Bishop, Rook or Knight.
			if (promoted != 0) {
				isPromotion = false;
				HandlePromotion();
				promoted = 0;
			}
			return true;
		}
		
		// If the user clicks on a piece, show possible moves and allow user to make a move.
		if (GetMouse(0).bPressed) {
			olc::vi2d mouse = {GetMouseX(), GetMouseY()};
			int file = 7 - ((mouse.x - unit) / unit);
			int rank = 7 - ((mouse.y - unit) / unit);

			if (file >= 0 && file <= 7 && rank >= 0 && rank <= 7) {
				DrawBoard();
				DrawPieces();

				int index = rank * 8 + file;

				Move* selected = nullptr;
				if (selectedSource != -1) {
					for (const auto& move : possible[selectedSource]) {
						if (move->to == index) {
							selected = move;
							break;
						}
					}
				}

				if (selected != nullptr) {
					selectedDestination = index;
					MakeMove(selected, index);
				} else {
					selectedSource = index;
					ShowPossibleMoves(index);
				}
			}
		}

		return true;
	}

	bool OnUserDestroy() {
		olc::SOUND::DestroyAudio();

		return true;
	}

	// Integer to vector. Flattened index to rank and file.
	olc::vi2d Itof(int index) {
		return {7 - index & 7, 7 - index / 8};
	}

	// Show Possible moves from a given source square.
	void ShowPossibleMoves(int source) {
		SetPixelMode(olc::Pixel::ALPHA);
		possible_spots.clear();
		for (const auto& move : possible[source]) {
			// Check for duplicates destination spots.
			if (possible_spots.find(move->to) != possible_spots.end()) continue;
			possible_spots[move->to] = 0;

			olc::vi2d rf = Itof(move->to);
			olc::vi2d center = {rf.x * unit + unit * 3 / 2, rf.y * unit + unit * 3 / 2};
			if (IS_CAPTURE(move->flags) && !IS_EN_PASSANT(move->flags)) {
				FillCircle(center, unit / 2.1, {0, 0, 0, 100});
				FillCircle(center, unit / 2.75, (rf.x + rf.y) % 2 != 0 ? dark : light);
			} else {
				FillCircle(center, unit / 5, {0, 0, 0, 100});
			}
		}
		SetPixelMode(olc::Pixel::NORMAL);
	}

	// Make a move to the selected destination.
	void MakeMove(Move* move, int destination) {
		// Highlight the source and destination squares of the most recently made move.
		olc::vi2d rfs = Itof(selectedSource), rfd = Itof(destination);
		FillRect({unit * rfs.x + unit, unit * rfs.y + unit}, {unit, unit}, highlight);
		FillRect({unit * rfd.x + unit, unit * rfd.y + unit}, {unit, unit}, highlight);

		// If the move was a pawn promotion, show the pawn promotion GUI to allow user to select the
		// piece they want to promote to (Queen, Bishop, Rook, Knight).
		if (IS_PROMOTION(move->flags)) {
			isPromotion = true;
		} else {
			if (IS_CAPTURE(move->flags) || IS_CASTLE(move->flags)) {
				olc::SOUND::PlaySample(audio[CAPTURE_AUDIO]);
			} else {
				olc::SOUND::PlaySample(audio[MOVE_AUDIO]);
			}

			make_move(chessboard, move);
			switch_ply(chessboard);
			GenerateMoves();
		}
	}

	// Once the user selects a piece to promote, make the corresponding move on the board.
	void HandlePromotion() {
		for (const auto& move : possible[selectedSource]) {
			if (move->to == selectedDestination && IS_PROMOTION(move->flags) && PROMOTED_PIECE(move->flags) == promoted) {
				if (IS_CAPTURE(move->flags)) {
					olc::SOUND::PlaySample(audio[CAPTURE_AUDIO]);
				} else {
					olc::SOUND::PlaySample(audio[MOVE_AUDIO]);
				}

				make_move(chessboard, move);
				switch_ply(chessboard);

				GenerateMoves();
				DrawBoard();

				// Highlight the source and destination squares of the most recently made move.
				olc::vi2d rfs = Itof(selectedSource), rfsd = Itof(selectedDestination);
				FillRect({unit * rfs.x + unit, unit * rfs.y + unit}, {unit, unit}, highlight);
				FillRect({unit * rfsd.x + unit, unit * rfsd.y + unit}, {unit, unit}, highlight);
				break;
			}
		}
	}

	// Draw Screen for Pawn Promotions allowing user to selected promoted piece.
	void DrawPromotion() {
		int cx = screenWidth / 2;
		int cy = screenHeight / 2;
		int trim = unit / 5;
		int size = 4 * unit - 2 * trim;
		FillRect({cx - size / 2 - trim, cy - size / 2 - trim}, {size + 2 * trim, size + 2 * trim}, {0, 0, 0});
		FillRect({cx - size / 2, cy - size / 2}, {size, size}, {245, 245, 245});

		olc::vf2d scale = {(float) size / 2 / pieceSize, (float) size / 2 / pieceSize};

		olc::vf2d top_left = {(float) cx - size / 2, (float) cy - size / 2};
		olc::vf2d top_right = {(float) cx, (float) cy - size / 2};
		olc::vf2d bottom_left = {(float) cx - size / 2, (float) cy};
		olc::vf2d bottom_right = {(float) cx, (float) cy};

		auto overlap = [](olc::vf2d p1, olc::vf2d p2, int size) {
			return p2.x <= p1.x && p1.x <= p2.x + size && p2.y <= p1.y && p1.y <= p2.y + size;
		};
		
		olc::vf2d mouse = {(float) GetMouseX(), (float) GetMouseY()};

		bool tl = overlap(mouse, top_left, size / 2);
		bool tr = overlap(mouse, top_right, size / 2);
		bool bl = overlap(mouse, bottom_left, size / 2);
		bool br = overlap(mouse, bottom_right, size / 2);

		olc::vf2d sizevf = scale * pieceSize;

		olc::Pixel normal = {255, 255, 255, 255};
		olc::Pixel tint = {255, 255, 255, 100};

		DrawDecal(top_left, pieces[chessboard->active_color][QUEEN], scale, tl ? tint : normal);
		DrawDecal(top_right, pieces[chessboard->active_color][ROOK], scale, tr ? tint : normal);
		DrawDecal(bottom_left, pieces[chessboard->active_color][BISHOP], scale, bl ? tint : normal);
		DrawDecal(bottom_right, pieces[chessboard->active_color][KNIGHT], scale, br ? tint : normal);

		if (GetMouse(0).bPressed) {
			if (tl) promoted = QUEEN;
			else if (tr) promoted = ROOK;
			else if (bl) promoted = BISHOP;
			else if (br) promoted = KNIGHT;
		}
	}

	// Draws the chess board.
	void DrawBoard() {
		Clear(background);
		int trim = unit / 4;
		FillRect({unit - trim, unit - trim}, {8 * unit + 2 * trim, 8 * unit + 2 * trim}, {0, 0, 0});

		// Draw Chess Board.
		for (int file = 0; file < 8; file++) {
			for (int rank = 0; rank < 8; rank++) {
				bool color = (file + rank) % 2 != 0;
				olc::vf2d pos = {(float) rank * unit + unit, (float) file * unit + unit};
				FillRect(pos, {unit, unit}, color ? dark : light);
			}
		}
	}

	// Draw the pieces on the board.
	void DrawPieces() {
		olc::vf2d scale = {(float) unit / pieceSize, (float) unit / pieceSize};
		for (int file = 0; file < 8; file++) {
			for (int rank = 0; rank < 8; rank++) {
				// If the promotion screen is up, then do not draw pieces on ranks 3-6 and files c-f
				// since this area is covered by the promotion selection GUI.
				if (isPromotion && file >= 2 && file <= 5 && rank >= 2 && rank <= 5) continue;
				int index = file * 8 + rank;
				Piece piece = get_piece(chessboard, index);
				if (piece != 0) {
					Piece color = get_color(chessboard, index);
					olc::vi2d rf = Itof(index);
					DrawDecal({(float) rf.x * unit + unit, (float) rf.y * unit + unit}, pieces[color][piece], scale);
				}
			}
		}
	}

	// Generate all possible moves for the current player.
	void GenerateMoves() {
		for (int i = 0; i < 64; i++) {
			possible[i].clear();
		}

		int legal = gen_legal_moves(chessboard, moves);

		if (legal == 0) {
			gameOver = true;
			winner = OPPOSITE(chessboard->active_color);
			return;
		}

		for (int i = 0; i < legal; i++) {
			Move* move = &moves[i];
			possible[move->from].push_back(move);
		}
	}
};

int main() {
	Chess demo;
	if (demo.Construct(800, 800, 1, 1))
		demo.Start();

	return 0;
}