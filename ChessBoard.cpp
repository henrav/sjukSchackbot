//
// Created by Henrik Ravnborg on 2024-03-09.
//
#include <thread>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstdint>
#include <algorithm>
#include <iostream>
using namespace std;

static bool update = false;

struct Move {
    uint64_t* pieceMoved;    // Pointer to the bitboard of the piece that moved
    uint64_t fromSquare;     // Bitboard representation of the starting square
    uint64_t toSquare;       // Bitboard representation of the destination square
    uint64_t* pieceCaptured; // Pointer to the bitboard of the captured piece, if any
    bool capture;            // Indicates if the move was a capture
    bool castle;             // Indicates if the move was a castling move
    uint64_t kingToSquare;   // Bitboard representation of the king's destination square in a castling move
    uint64_t kingFromSquare; // Bitboard representation of the king's starting square in a castling move
    bool whiteKing;          // Indicates if it's a white king in a castling move
    bool promotion;          // Indicates if the move is a pawn promotion
    int score;               // Score of the move for evaluation

    // Default constructor
    Move() : pieceMoved(nullptr), fromSquare(0), toSquare(0), pieceCaptured(nullptr), capture(false), castle(false), kingToSquare(0), kingFromSquare(0), promotion(false), score(0) {}

    // Constructor for regular and capture moves
    Move(uint64_t* moved, uint64_t from, uint64_t to, uint64_t* captured = nullptr, bool cap = false, int moveScore = 0)
            : pieceMoved(moved), fromSquare(from), toSquare(to), pieceCaptured(captured), capture(cap), castle(false), kingToSquare(0), kingFromSquare(0), promotion(false), score(moveScore) {}

    // Constructor for castling moves
    Move(uint64_t* moved, uint64_t from, uint64_t to, bool cas, uint64_t kingTo, uint64_t kingFrom, bool whiteking, int moveScore = 0)
            : pieceMoved(moved), fromSquare(from), toSquare(to), pieceCaptured(nullptr), capture(false), castle(cas), kingToSquare(kingTo), kingFromSquare(kingFrom), promotion(false), whiteKing(whiteking), score(moveScore) {}

    // Constructor for pawn promotion moves
    Move(uint64_t* moved, uint64_t from, uint64_t to, uint64_t* captured = nullptr, bool cap = false, bool prom = true, int moveScore = 0)
            : pieceMoved(moved), fromSquare(from), toSquare(to), pieceCaptured(captured), capture(cap), castle(false), kingToSquare(0), kingFromSquare(0), promotion(prom), score(moveScore) {}
};


class ChessBoard {
public:

    enum NodeType {
        EXACT,
        LOWERBOUND,
        UPPERBOUND
    };
    struct TTEntry {
        uint64_t hashKey;
        int depth;
        int value;
        NodeType flag;
        Move bestMove;
    };

    struct TranspositionTable {
        static const size_t TABLE_SIZE = 1 << 20; // Example size, adjust as needed.
        std::vector<TTEntry> table;

        TranspositionTable() : table(TABLE_SIZE) {}

        void store(uint64_t hashKey, int depth, int value, NodeType flag, Move bestMove) {
            size_t index = hashKey % TABLE_SIZE; // Simple modulo for index calculation.
            // You could add more sophisticated collision handling here.
            table[index] = {hashKey, depth, value, flag, bestMove};
        }

        TTEntry *get(uint64_t hashKey) {
            size_t index = hashKey % TABLE_SIZE;
            TTEntry &entry = table[index];
            if (entry.hashKey == hashKey) {
                return &entry; // Found, return a pointer to the entry.
            }
            return nullptr; // Not found, return nullptr.
        }

        size_t indexFor(uint64_t hashKey) const {
            return hashKey % TABLE_SIZE; // Simple modulo hashing
        }
    };

    int roundnr = 0;
    uint64_t zobristTable[64][12];
    uint64_t sideToMoveHash;
    vector<Move> rootMoves;
    Move BestMover;
    vector<Move> quiesceMoves;
    std::map<int, uint64_t> northMoves, southMoves, eastMoves, westMoves;
    static const int pawnValue = 10;
    static const int knightValue = 28;
    static const int bishopValue = 28;
    static const int rookValue = 40;
    static const int queenValue = 70;
    const int singleMoveOffsetWhite = 8;
    const int singleMoveOffsetBlack = -8;
    const int doubleMoveOffsetWhite = 16;
    const int doubleMoveOffsetBlack = -16;
    const int doubleMoveStartRowWhite = 1;
    const int doubleMoveStartRowBlack = 6;
    const std::vector<int> attackOffsetsWhite = {7, 9};
    const std::vector<int> attackOffsetsBlack = {-9, -7};
    const std::vector<int> knightOffsets{-17, -15, -10, -6, 6, 10, 15, 17};
    const std::vector<int> pawnPositionalValue = {
            0, 0, 0, 0, 0, 0, 0, 0,
            3, 2, 1, -1, -1, -1, 1, 2,
            2, 2, 4, 6, 6, 4, 2, 2,
            1, 1, 2, 5, 5, 2, 1, 1,
            0, 0, 1, 3, 3, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            5, 5, 5, 5, 5, 5, 5, 5,
            0, 0, 0, 0, 0, 0, 0, 0,
    };


    // Positional values for knights
    const std::vector<int> knightPositionalValue = {
            -5, -2, -2, -2, -2, -2, -2, -5,
            -2, 0, 0, 3, 3, 0, 0, -2,
            -2, 0, 3, 6, 6, 3, 0, -2,
            -2, 3, 6, 8, 8, 6, 3, -2,
            -2, 3, 6, 8, 8, 6, 3, -2,
            -2, 0, 3, 6, 6, 3, 0, -2,
            -2, 0, 0, 3, 3, 0, 0, -2,
            -5, -2, -2, -2, -2, -2, -2, -5,
    };


    const std::vector<int> bishopPositionalValue = {
            -5, -2, -2, -2, -2, -2, -2, -5,
            -2, 0, 0, 0, 0, 0, 0, -2,
            -2, 0, 5, 5, 5, 5, 0, -2,
            -2, 0, 5, 8, 8, 5, 0, -2,
            -2, 0, 5, 8, 8, 5, 0, -2,
            -2, 0, 5, 5, 5, 5, 0, -2,
            -2, 0, 0, 0, 0, 0, 0, -2,
            -5, -2, -2, -2, -2, -2, -5, -5,
    };


    const std::vector<int> rookPositionalValue = {
            0, 0, 0, 5, 5, 0, 0, 0,
            5, 10, 10, 5, 5, 10, 10, 5,
            -5, 0, 0, 5, 5, 0, 0, -5,
            -5, 0, 0, 5, 5, 0, 0, -5,
            -5, 0, 0, 5, 5, 0, 0, -5,
            -5, 0, 0, 5, 5, 0, 0, -5,
            5, 10, 10, 10, 10, 10, 10, 5,
            0, 0, 0, 5, 5, 0, 0, 0,
    };


    const std::vector<int> queenPositionalValue = {
            -2, -2, -2, -2, -2, -2, -2, -2,
            -2, 0, 0, 0, 0, 0, 0, -2,
            -2, 0, 3, 3, 3, 3, 0, -2,
            -2, 0, 3, 5, 5, 3, 0, -2,
            -2, 0, 3, 5, 5, 3, 0, -2,
            -2, 0, 3, 3, 3, 3, 0, -2,
            -2, 0, 0, 0, 0, 0, 0, -2,
            -2, -2, -2, -2, -2, -2, -2, -2,
    };


    // Additional score for capturing a more valuable piece with a less valuable piece
    const int captureBonus = 2;
    uint64_t whitePawns{};
    uint64_t whiteRooks{};
    uint64_t whiteKnights{};
    uint64_t whiteBishops{};
    uint64_t whiteQueens{};
    uint64_t whiteKing{};

    uint64_t blackPawns{};
    uint64_t blackRooks{};
    uint64_t blackKnights{};
    uint64_t blackBishops{};
    uint64_t blackQueens{};
    uint64_t blackKing{};

    uint64_t occupiedSquares{}; // Represents all occupied squares
    uint64_t whitePieces{}; // Represents all white pieces
    uint64_t blackPieces{}; // Represents all black pieces
    uint64_t *previousMovePieceBitboard{};
    uint64_t previousMoveFrom{};
    uint64_t previousMoveTo{};
    bool whitesTurn = true;

    enum PieceType {
        Pawn, Knight, Bishop, Rook, Queen, King, None
    };

    ChessBoard() {
        resetBoard();
        precomputeMoves();

    }
    // Part of your ChessBoard or equivalent class


    void precomputeMoves() {
        for (int square = 0; square < 64; ++square) {
            uint64_t north = 0, south = 0, east = 0, west = 0;
            int row = square / 8, col = square % 8;

            // North Moves
            for (int r = row + 1; r < 8; ++r) {
                north |= 1ULL << (r * 8 + col);
            }

            // South Moves
            for (int r = row - 1; r >= 0; --r) {
                south |= 1ULL << (r * 8 + col);
            }

            // East Moves
            for (int c = col + 1; c < 8; ++c) {
                east |= 1ULL << (row * 8 + c);
            }

            // West Moves
            for (int c = col - 1; c >= 0; --c) {
                west |= 1ULL << (row * 8 + c);
            }

            northMoves[square] = north;
            southMoves[square] = south;
            eastMoves[square] = east;
            westMoves[square] = west;
        }
    }

    std::vector<Move> moveHistory;

    std::vector<Move> generateMovesForPiece(uint64_t pieceBitboard, PieceType type) {
        std::vector<Move> possibleMoves;
        switch (type) {
            case Pawn:
                possibleMoves = generatePawnMoves(pieceBitboard);
                break;
            case Knight:
                possibleMoves = generateKnightMoves(pieceBitboard);
                break;
            case Bishop:
                possibleMoves = generateBishopMoves(pieceBitboard);
                break;
            case Rook:
                possibleMoves = generateRookMoves(pieceBitboard);
                break;
            case Queen:
                possibleMoves = generateQueenMoves(pieceBitboard);
                break;
            case King:
                possibleMoves = generateKingMoves(pieceBitboard);
                break;
            default:
                break;

        }

        return possibleMoves;
    }

    uint64_t computeHash(bool isWhitesTurn) const {
        uint64_t hash = 0;

        for (int square = 0; square < 64; ++square) { // Iterate over all squares
            if (isSquareOccupied(square)) { // If the square is occupied
                PieceType pieceType = getPieceTypeOnSquare(square); // Get the piece type on this square
                bool isWhite = isSquareOccupiedByWhite(square); // Determine the color of the piece
                int pieceIndex = pieceType + (isWhite ? 0 : 6); // Calculate the piece index based on type and color

                // XOR the hash with the Zobrist key for the piece on this square
                hash ^= zobristTable[square][pieceIndex];
            }
        }

        // Include side to move in the hash
        if (isWhitesTurn) {
            hash ^= sideToMoveHash;
        }

        // Add other states like castling rights, en passant square, etc., to the hash as needed

        return hash;
    }


    [[nodiscard]] PieceType getPieceTypeOnSquare(int theSquare) const {
        uint64_t mask = 1ULL << theSquare;
        if ((whitePawns | blackPawns) & mask) return Pawn;
        if ((whiteKnights | blackKnights) & mask) return Knight;
        if ((whiteBishops | blackBishops) & mask) return Bishop;
        if ((whiteRooks | blackRooks) & mask) return Rook;
        if ((whiteQueens | blackQueens) & mask) return Queen;
        if ((whiteKing | blackKing) & mask) return King;
        return None;

    }

    //check if square is occupied by white piece
    bool isSquareOccupiedByWhite(int square) const {
        uint64_t mask = 1ULL << square;
        return (whitePieces & mask) != 0;
    }


    //check if square is occupied
    bool isSquareOccupied(int square) const {
        uint64_t mask = 1ULL << square;
        return (occupiedSquares & mask) != 0;
    }


    //reset the board to the initial position
    void resetBoard() {
        whitePawns = 0xFF00ULL;
        whiteRooks = 0x81ULL;
        whiteKnights = 0x42ULL;
        whiteBishops = 0x24ULL;
        whiteQueens = 0x8ULL;
        whiteKing = 0x10ULL;
        blackPawns = 0x00FF000000000000ULL;
        blackRooks = 0x8100000000000000ULL;
        blackKnights = 0x4200000000000000ULL;
        blackBishops = 0x2400000000000000ULL;
        blackQueens = 0x800000000000000ULL;
        blackKing = 0x1000000000000000ULL;
        updateOccupiedSquares();
        //printBoard();

    }

    //update the occupied squares
    void updateOccupiedSquares() {
        occupiedSquares = whitePawns | whiteRooks | whiteKnights | whiteBishops | whiteQueens | whiteKing
                          | blackPawns | blackRooks | blackKnights | blackBishops | blackQueens | blackKing;
        whitePieces = whitePawns | whiteRooks | whiteKnights | whiteBishops | whiteQueens | whiteKing;
        blackPieces = blackPawns | blackRooks | blackKnights | blackBishops | blackQueens | blackKing;

    }


    // Example move function: Move a piece from one square to another
    // This is a simplistic example. A real move function would need to be much more comprehensive.
    void movePiece(uint64_t &pieceBitboard, uint64_t fromSquare, uint64_t toSquare,
                   uint64_t *capturedPieceBitboard = nullptr) {
        // Log the move before making it
        bool capture = capturedPieceBitboard && (*capturedPieceBitboard & toSquare);
        moveHistory.emplace_back(&pieceBitboard, fromSquare, toSquare, capturedPieceBitboard, capture, false);

        // Make the move
        pieceBitboard &= ~fromSquare;
        pieceBitboard |= toSquare;

        // If there's a capture, remove the captured piece
        if (capture) {
            *capturedPieceBitboard &= ~toSquare;
        }

        updateOccupiedSquares();
        //Board();
    }

    void movePiece(Move move) {

        moveHistory.push_back(move);
        if (move.castle) {
            *move.pieceMoved &= ~move.fromSquare;
            *move.pieceMoved |= move.toSquare;
            if (move.whiteKing) {
                whiteKing &= ~move.kingFromSquare;
                whiteKing |= move.kingToSquare;
            } else {
                blackKing &= ~move.kingFromSquare;
                blackKing |= move.kingToSquare;
            }
        }/*
        else if (move.promotion){
            *move.pieceMoved &= ~move.fromSquare;
            // Add a queen to the destination square
            uint64_t* queenBitboard = whitePawns ? &whiteQueens : &blackQueens;
            *queenBitboard |= move.toSquare;
        }*/else {

            if (move.capture) {
                int score = getPieceValue(getPieceTypeOnSquare(bitScanForward(move.toSquare)));
                move.score += score;
                *move.pieceCaptured &= ~move.toSquare;
            }
            *move.pieceMoved &= ~move.fromSquare;
            *move.pieceMoved |= move.toSquare;

            /*
            if (move.capture) {
                if (isSquareThreatened(bitScanForward(move.toSquare), !isSquareOccupiedByWhite((move.toSquare)))) {
                    int score = getPieceValue(getPieceTypeOnSquare(bitScanForward(move.toSquare)));
                    int score2 = getPieceValue(getPieceTypeOnSquare(bitScanForward(move.fromSquare)));
                    if (score2 > score) {
                        move.score -= 10000;
                    }else{
                        move.score += score;
                    }
                }else{
                    move.score += getPieceValue(getPieceTypeOnSquare(bitScanForward(move.toSquare)));
                }
                *move.pieceCaptured &= ~move.toSquare;
            }
            *move.pieceMoved &= ~move.fromSquare;
            *move.pieceMoved |= move.toSquare;
             */
        }


        updateOccupiedSquares();
    }

    //reset the previous move
    void resetPreviousMove() {
        if (!moveHistory.empty()) {
            Move lastMove = moveHistory.back(); // Capture the last move for readability
            if (!lastMove.castle) {
                *lastMove.pieceMoved |= lastMove.fromSquare; // Restore the piece to its original square
                *lastMove.pieceMoved &= ~lastMove.toSquare; // Clear the piece's new square
                if (lastMove.capture) {
                    *lastMove.pieceCaptured |= lastMove.toSquare; // Restore the captured piece
                }
            } else {
                *lastMove.pieceMoved |= lastMove.fromSquare;
                *lastMove.pieceMoved &= ~lastMove.toSquare;
                if (lastMove.whiteKing) {
                    whiteKing |= lastMove.kingFromSquare;
                    whiteKing &= ~lastMove.kingToSquare;
                } else {
                    blackKing |= lastMove.kingFromSquare;
                    blackKing &= ~lastMove.kingToSquare;
                }
            }
            moveHistory.pop_back(); // Remove the last move from history after resetting it
            // Switch turn

            updateOccupiedSquares();
        }
    }



    void printBoard() const {
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file < 8; file++) {
                uint64_t position = 1ULL << (rank * 8 + file);
                if (occupiedSquares & position) {
                    PieceType pieceType = getPieceTypeOnSquare(rank * 8 + file);
                    bool isWhite = (whitePieces & position) != 0;
                    if (isWhite) {
                        switch (pieceType) {
                            case Pawn:
                                std::cout << " P ";
                                break;
                            case Knight:
                                std::cout << " N ";
                                break;
                            case Bishop:
                                std::cout << " B ";
                                break;
                            case Rook:
                                std::cout << " R ";
                                break;
                            case Queen:
                                std::cout << " Q ";
                                break;
                            case King:
                                std::cout << " K ";
                                break;
                            default:
                                std::cout << " w ";
                        }
                    } else {
                        switch (pieceType) {
                            case Pawn:
                                std::cout << " p ";
                                break;
                            case Knight:
                                std::cout << " n ";
                                break;
                            case Bishop:
                                std::cout << " b ";
                                break;
                            case Rook:
                                std::cout << " r ";
                                break;
                            case Queen:
                                std::cout << " q ";
                                break;
                            case King:
                                std::cout << " k ";
                                break;
                            default:
                                std::cout << " b ";
                        }
                    }
                } else {
                    std::cout << " . ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    static bool isSquareInBounds(int square, int square1) {
        return square >= 0 && square < 64 && square1 >= 0 && square1 < 64;
    }

    static int bitScanForward(uint64_t position) {
        return __builtin_ffsll(position) - 1;
    }

    uint64_t *getBitboardPointerByPieceType(PieceType type, bool b) {
        switch (type) {
            case Pawn:
                return b ? &whitePawns : &blackPawns;
            case Knight:
                return b ? &whiteKnights : &blackKnights;
            case Bishop:
                return b ? &whiteBishops : &blackBishops;
            case Rook:
                return b ? &whiteRooks : &blackRooks;
            case Queen:
                return b ? &whiteQueens : &blackQueens;
            case King:
                return b ? &whiteKing : &blackKing;
            default:
                return nullptr;
        }
    }

    std::vector<Move> generateKnightMoves(uint64_t knightPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(knightPosition);
        bool isWhite = (whitePieces & knightPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *knightBitboard = isWhite ? &whiteKnights : &blackKnights;

        for (int offset: knightOffsets) {
            int targetSquare = startSquare + offset;

            // Calculate row and column to prevent moves that "wrap" around the board.
            int startRow = startSquare / 8, targetRow = targetSquare / 8;
            int startCol = startSquare % 8, targetCol = targetSquare % 8;

            // Ensure move does not "wrap" around the board and stays within bounds.
            if (abs(startRow - targetRow) > 2 || abs(startCol - targetCol) > 2) continue;
            if (abs(startCol - targetCol) == 1 && abs(startRow - targetRow) != 2) continue;
            if (abs(startRow - targetRow) == 1 && abs(startCol - targetCol) != 2) continue;
            if (targetSquare < 0 || targetSquare >= 64) continue;

            uint64_t targetBitboard = 1ULL << targetSquare;
            if (ownPieces & targetBitboard) continue; // Skip if own piece occupies the target square

            bool isCapture = (enemyPieces & targetBitboard) != 0;
            uint64_t *capturedPieceBitboard = nullptr;
            if (isCapture) {
                capturedPieceBitboard = getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite);
            }

            moves.emplace_back(knightBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard,
                               isCapture, false);
        }

        return moves;
    }

    // Other member functions...

    bool isPromotionSquare(int square, bool isWhite) {
        return isWhite ? square / 8 == 7 : square / 8 == 0;
    }


    std::vector<Move> generatePawnMoves(uint64_t pawnPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(pawnPosition);
        bool isWhite = (whitePieces & pawnPosition) != 0;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *pawnBitboard = isWhite ? &whitePawns : &blackPawns;
        uint64_t occupiedSquares = whitePieces | blackPieces;

        // Directional and starting row settings for pawn moves
        int singleMoveOffset = isWhite ? singleMoveOffsetWhite : singleMoveOffsetBlack; // Moving up or down the board
        int doubleMoveOffset = isWhite ? doubleMoveOffsetWhite : doubleMoveOffsetBlack; // Two squares forward
        int doubleMoveStartRow = isWhite ? doubleMoveStartRowWhite
                                         : doubleMoveStartRowBlack; // Starting row for a double move
        std::vector<int> attackOffsets = isWhite ? std::vector<int>{7, 9} : std::vector<int>{-7,
                                                                                             -9}; // Diagonal attacks

        // Single forward move
        int targetSquare = startSquare + singleMoveOffset;
        if (targetSquare >= 0 && targetSquare < 64 && !(occupiedSquares & (1ULL << targetSquare))) {
            moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << targetSquare, nullptr, false, false);

            if (startSquare / 8 == doubleMoveStartRow &&
                !(occupiedSquares & (1ULL << (startSquare + doubleMoveOffset)))) {
                moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << (startSquare + doubleMoveOffset), nullptr,
                                   false, false);
            }
        }

        // Attack moves
        for (int offset: attackOffsets) {
            targetSquare = startSquare + offset;
            // Check valid square range, prevent wrap-around, and ensure there is an enemy piece to capture
            if (targetSquare >= 0 && targetSquare < 64 &&
                ((startSquare % 8 != 0) || (offset != -9 && offset != 7)) && // Not wrapping left for left attacks
                ((startSquare % 8 != 7) || (offset != -7 && offset != 9)) && // Not wrapping right for right attacks
                (enemyPieces & (1ULL << targetSquare))) {
                // Capture move
                moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << targetSquare,
                                   getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite), true,
                                   false);
            }
        }

        return moves;
    }


    std::vector<Move> generateRookMoves(uint64_t rookPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(rookPosition);
        bool isWhite = (whitePieces & rookPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *rookBitboard = isWhite ? &whiteRooks : &blackRooks;
        uint64_t allPieces = whitePieces | blackPieces;
        int offsets[] = {1, -1, 8, -8}; // Right, Left, Down, Up

        int startRow = startSquare / 8, startCol = startSquare % 8;

        for (int offset: offsets) {
            int targetSquare = startSquare;
            do {
                targetSquare += offset;
                if (targetSquare < 0 || targetSquare >= 64) break; // Check board boundaries

                // Calculate the row and column to prevent wrapping
                int targetRow = targetSquare / 8, targetCol = targetSquare % 8;

                // Prevent wrapping around the board
                if (offset == 1 || offset == -1) {
                    if (targetRow != startRow) break; // Horizontal move, ensure row doesn't change
                } else {
                    if (targetCol != startCol) break; // Vertical move, ensure column doesn't change
                }

                uint64_t targetBitboard = 1ULL << targetSquare;
                if (targetBitboard & allPieces) { // If any piece is on the target square
                    if (targetBitboard & enemyPieces) { // If enemy piece, include as capture
                        PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                        uint64_t *capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                        moves.emplace_back(rookBitboard, 1ULL << startSquare, targetBitboard, capturedPieceBitboard,
                                           true, false);
                    }
                    break; // Stop if any piece is encountered
                } else {
                    moves.emplace_back(rookBitboard, 1ULL << startSquare, targetBitboard, nullptr, false, false);
                }
            } while (true);
        }
        /* OLD FUNCTION RETURN IF NEW STARTS LAGGING
        for (int offset: offsets) {
            int targetSquare = startSquare + offset;
            while (targetSquare >= 0 && targetSquare < 64) {
                int targetRow = targetSquare / 8, targetCol = targetSquare % 8;

                // Additional checks to prevent wrapping around the board
                if (offset == 1 || offset == -1) {
                    // Horizontal movement: Break if we change rows
                    if (targetRow != startRow) break;
                } else {
                    // Vertical movement: Break if we change columns
                    if (targetCol != startCol) break;
                }

                uint64_t targetBitboard = 1ULL << targetSquare;
                if (ownPieces & targetBitboard) {
                    // Own piece blocks the way, no further moves in this direction
                    break;
                }

                bool isCapture = enemyPieces & targetBitboard;
                uint64_t *capturedPieceBitboard = nullptr;
                if (isCapture) {
                    PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                    capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                }

                moves.emplace_back(rookBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard,
                                   isCapture, false);

                if (isCapture) {
                    // Capture move; no further moves in this direction
                    break;
                }

                targetSquare += offset; // Continue in the same direction
            }
        }
         */
// Move(uint64_t* moved, uint64_t from, uint64_t to, bool cas, uint64_t kingTo, uint64_t kingFrom, bool whiteking)
        //add castling as a move
        //if theres no pieces between the king and the rook
        if (isWhite) {
            //first check white rook at position 0, and if king is at position 4
            if (rookPosition == 0x1ULL && whiteKing == 0x10ULL) {
                //check if there are no pieces between the king and the rook
                if ((occupiedSquares & 0x60ULL) == 0) {
                    moves.emplace_back(&whiteRooks, 0x1ULL, 0x20ULL, true, 0x40ULL, 0x10ULL, true);
                }

            }
            //check if white rook is at position 7, and if king is at position 4
            if (rookPosition == 0x80ULL && whiteKing == 0x10ULL) {
                //check if there are no pieces between the king and the rook
                if ((occupiedSquares & 0x60ULL) == 0) {
                    moves.emplace_back(&whiteRooks, 0x80ULL, 0x10ULL, true, 0x40ULL, 0x10ULL, true);
                }
            }
        } else {
            if (rookPosition == 0x100000000000000ULL && blackKing == 0x1000000000000ULL) {
                //check if there are no pieces between the king and the rook
                if ((occupiedSquares & 0x7000000000000000ULL) == 0) {
                    moves.emplace_back(&blackRooks, 0x100000000000000ULL, 0x1000000000000ULL, true,
                                       0x4000000000000000ULL, 0x1000000000000ULL, false);
                }
            }
            //check if white rook is at position 7, and if king is at position 4
            if (rookPosition == 0x8000000000000000ULL && blackKing == 0x1000000000000ULL) {
                //check if there are no pieces between the king and the rook
                if ((occupiedSquares & 0xE000000000000000ULL) == 0) {
                    moves.emplace_back(&blackRooks, 0x8000000000000000ULL, 0x1000000000000ULL, true,
                                       0x2000000000000000ULL, 0x1000000000000ULL, false);
                }
            }


        }


        return moves;
    }
/*
    std::vector<Move> generateBishopMoves(uint64_t bishopPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(bishopPosition);
        bool isWhite = (whitePieces & bishopPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *bishopBitboard = isWhite ? &whiteBishops : &blackBishops;
        uint64_t allPieces = whitePieces | blackPieces;

        int directions[] = {7, 9, -7, -9}; // Diagonal directions
        for (int dir : directions) {
            int targetSquare = startSquare + dir;
            while (targetSquare >= 0 && targetSquare < 64 &&
                   !((1ULL << targetSquare) & ownPieces) && // Not landing on own piece
                   (((targetSquare / 8) - (startSquare / 8)) == ((targetSquare % 8) - (startSquare % 8)) ||
                    ((targetSquare / 8) - (startSquare / 8)) == -((targetSquare % 8) - (startSquare % 8)))) { // Diagonal movement
                // If path is blocked by any piece, stop
                if ((1ULL << targetSquare) & allPieces) {
                    if ((1ULL << targetSquare) & enemyPieces) { // Capture
                        uint64_t *capturedPieceBitboard = getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite);
                        moves.emplace_back(bishopBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, true, false);
                    }
                    break; // Blocker encountered, stop looking further in this direction
                } else { // No piece, add as a potential move
                    moves.emplace_back(bishopBitboard, 1ULL << startSquare, 1ULL << targetSquare, nullptr, false, false);
                }
                targetSquare += dir; // Move to the next square in the same direction
                // Ensure we don't wrap around the board
                if (abs((targetSquare % 8) - (startSquare % 8)) > 1) break;
            }
        }

        return moves;
    }
*/

    //OLD FUNCTION FOR BISHOP REVERT IF NEEDED
    //OLD FUNCTION FOR BISHOP REVERT IF NEEDED
    //OLD FUNCTION FOR BISHOP REVERT IF NEEDED
    //OLD FUNCTION FOR BISHOP REVERT IF NEEDED

    std::vector<Move> generateBishopMoves(uint64_t bishopPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(bishopPosition);
        int startRow = startSquare / 8, startCol = startSquare % 8;
        bool isWhite = (whitePieces & bishopPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *bishopBitboard = isWhite ? &whiteBishops : &blackBishops;
        int offsets[] = {7, 9, -7, -9};

        for (int offset: offsets) {
            int targetSquare = startSquare + offset;
            while (targetSquare >= 0 && targetSquare < 64) {
                int targetRow = targetSquare / 8, targetCol = targetSquare % 8;

                // Check for diagonal "wrapping"
                if (abs(targetRow - startRow) != abs(targetCol - startCol)) {
                    break; // This move attempts to "wrap" the board, so it's invalid
                }

                uint64_t targetBitboard = 1ULL << targetSquare;
                if (ownPieces & targetBitboard) break; // Own piece blocks further movement

                bool isCapture = enemyPieces & targetBitboard;
                uint64_t *capturedPieceBitboard = nullptr;
                if (isCapture) {
                    capturedPieceBitboard = getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite);
                }

                moves.emplace_back(bishopBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard,
                                   isCapture, false);

                if (isCapture) break; // Capture stops further traversal in this direction

                // Prepare for the next iteration
                targetSquare += offset; // Continue in the same diagonal direction
                // Ensure the next target doesn't wrap the board
                if (abs((targetSquare / 8) - targetRow) != 1) break;
            }
        }

        return moves;
    }


    static bool isValidRookMove(int startSquare, int targetSquare) {
        int startRank = startSquare / 8, startFile = startSquare % 8;
        int targetRank = targetSquare / 8, targetFile = targetSquare % 8;

        // Ensure the move stays within the board
        if (targetSquare < 0 || targetSquare >= 64) return false;

        // Rook moves must have the same rank or file
        return startRank == targetRank || startFile == targetFile;
    }

    static bool isValidDiagonalMove(int startSquare, int targetSquare) {
        int startRank = startSquare / 8, startFile = startSquare % 8;
        int targetRank = targetSquare / 8, targetFile = targetSquare % 8;

        // Ensure the move stays within the board
        if (targetSquare < 0 || targetSquare >= 64) return false;

        // Diagonal moves must have the same rank and file difference magnitude
        return abs(startRank - targetRank) == abs(startFile - targetFile);
    }

    std::vector<Move> generateQueenMoves(uint64_t queenPosition) {
        std::vector<Move> moves;

        // Determine if the queen is white or black based on the position
        bool isWhite = (whitePieces & queenPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *queenBitboard = isWhite ? &whiteQueens : &blackQueens;
        // Direction offsets for a queen (combines rook and bishop movements)
        int offsets[] = {7, 9, -7, -9, 1, -1, 8, -8};

        // Handle multiple queens on the bitboard
        while (queenPosition) {
            int startSquare = bitScanForward(queenPosition); // Find the least significant bit
            queenPosition &= queenPosition - 1; // Remove the least significant bit

            for (int offset: offsets) {
                int targetSquare = startSquare;
                int previousRow = startSquare / 8, previousCol = startSquare % 8;
                while (true) {
                    targetSquare += offset;
                    if (targetSquare < 0 || targetSquare >= 64) break; // Out of bounds

                    int targetRow = targetSquare / 8, targetCol = targetSquare % 8;

                    // Prevent wrapping across the board
                    if (abs(targetRow - previousRow) > 1 || abs(targetCol - previousCol) > 1) break;

                    uint64_t targetBitboard = 1ULL << targetSquare;
                    if (ownPieces & targetBitboard) {
                        // Blocked by own piece
                        break;
                    }

                    bool isCapture = enemyPieces & targetBitboard;
                    uint64_t *capturedPieceBitboard = nullptr;
                    if (isCapture) {
                        PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                        capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                    }

                    moves.emplace_back(queenBitboard, 1ULL << startSquare, 1ULL << targetSquare,
                                       capturedPieceBitboard, isCapture, false);

                    if (isCapture) {
                        break; // Stop at capture
                    }

                    // Update previous position for the next iteration
                    previousRow = targetRow;
                    previousCol = targetCol;
                }
            }
        }

        return moves;
    }


    std::vector<Move> generateKingMoves(uint64_t kingPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(kingPosition);
        bool isWhite = (whitePieces & kingPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t *kingBitboard = isWhite ? &whiteKing : &blackKing;


        int offsets[] = {1, -1, 8, -8, 7, 9, -7, -9};

        for (int offset: offsets) {
            int targetSquare = startSquare + offset;

            // Continue if the target square is not on the board
            if (!isSquareInBounds(startSquare, targetSquare)) continue;

            uint64_t targetBitboard = 1ULL << targetSquare;
            // Skip if the target square is occupied by own piece
            if (ownPieces & targetBitboard) continue;

            bool isCapture = enemyPieces & targetBitboard;
            uint64_t *capturedPieceBitboard = nullptr;
            if (isCapture) {
                PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
            }
            moves.emplace_back(kingBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard,
                               isCapture, false);
        }
        return moves;
    }


    bool playerMove(int startRank, int startFile, int targetRank, int targetFile) {
        int i = 0;
        uint64_t fromMask = 1ULL << (startRank * 8 + startFile);
        uint64_t toMask = 1ULL << (targetRank * 8 + targetFile);
        bool isWhite = (whitePieces & fromMask) != 0;
        if (isWhite != whitesTurn) return false;
        PieceType pieceType = getPieceTypeOnSquare(
                bitScanForward(fromMask));
        if (pieceType == None) return false; // No piece to move
        // First, check if the king is in chec
        uint64_t king = isWhite ? whiteKing : blackKing;
        if (isKingInCheck(true)) {
            std::cout << "King is in check!" << std::endl;
            // Generate all possible moves for the player
            std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);
            // Filter out moves that don't resolve the check

            std::vector<Move> movesThatResolveCheck = filterMovesThatResolveCheck(possibleMoves, whitesTurn);
            // Check if the move is valid
            for (const Move &move: movesThatResolveCheck) {
                if (move.toSquare == toMask) {
                    movePiece(move);
                    whitesTurn = !whitesTurn;
                    generateBotMoves();
                    return true; // Move was successfully applied
                }
            }
            return false;
            // No valid move executed, either the move didn't resolve the check or wasn't valid
        } else {

            // Generate possible moves for the piece
            std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);

            for (const Move &move: possibleMoves) {
                movePiece(move); // Apply the move temporarily

                // Check if OUR king is in check after this move
                bool ourKingInCheckAfterMove = isKingInCheck(true);

                resetPreviousMove(); // Undo the move

                // Only proceed if our king isn't in check as a result of this move
                if (!ourKingInCheckAfterMove && move.toSquare == toMask) {
                    // The move is valid and does not place our king in check
                    movePiece(move);
                    whitesTurn = !whitesTurn;
                    generateBotMoves();
                    i++;
                    return true; // Successfully executed move
                }
            }
        }
        // Determine the piece at the start position

        return false; // No valid move was executed
    }

    bool isKingInCheck(bool isWhite) {
        // Determine positions and enemy color
        if (isSquareThreatened(bitScanForward(isWhite ? (whiteKing) : (blackKing)), isWhite)) {
            return true;
        }
        return false;


    }

    int isKingInCheckInt(bool isWhite) {
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t king = isWhite ? whiteKing : blackKing;
        // Check if any enemy piece can attack the king
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (enemyPieces & position) {
                PieceType pieceType = getPieceTypeOnSquare(i);
                if (pieceType == isWhite ? blackPawns : whitePawns) {
                    int kingPosition = bitScanForward(king);
                    int pawnRow = i / 8, pawnCol = i % 8;
                    int kingRow = kingPosition / 8, kingCol = kingPosition % 8;
                    // Calculate row and column differences between the pawn and king
                    int rowDiff = pawnRow - kingRow;
                    int colDiff = abs(pawnCol - kingCol);

                    // For white king, check black pawns; for black king, check white pawns
                    if ((isWhite && rowDiff == -1 && colDiff == 1) || (!isWhite && rowDiff == 1 && colDiff == 1)) {
                        return 1; // Pawn can attack the king diagonally
                    }
                }
                std::vector<Move> possibleMoves = generateMovesForPiece(position, pieceType);
                for (const Move &move: possibleMoves) {
                    if (move.toSquare == king) {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    bool checkIfMoveResultsInCheck(Move move, bool isWhite) {
        // Simulate the move
        movePiece(move);
        // Check if the move results in check
        bool isCheck = isWhite ? isKingInCheck(whiteKing) : isKingInCheck(blackKing);

        // Reset the move
        resetPreviousMove();

        return isCheck;
    }

    bool resolveKingCheck(Move move, bool isWhite) {
        // Simulate the move
        movePiece(move);

        // Check if the move results in check
        bool isCheck = isKingInCheck(isWhite);

        // Reset the move
        resetPreviousMove();


        return isCheck;

    }

    std::vector<Move> filterMovesThatResolveCheck(const std::vector<Move> &possibleMoves, bool isWhite) {
        std::vector<Move> movesThatResolveCheck;
        for (const Move &move: possibleMoves) {
            // Simulate the move
            movePiece(move);

            if (!isKingInCheck(isWhite)) {
                movesThatResolveCheck.push_back(move);
            }

            // Undo the move
            resetPreviousMove();
        }

        return movesThatResolveCheck;
    }

    bool knightMoveValid(int startSquare, int endSquare) {
        int startFile = startSquare % 8, targetFile = endSquare % 8;
        int startRank = startSquare / 8, targetRank = endSquare / 8;

        // Check for horizontal wrap
        if (abs(startFile - targetFile) > 2) return false;
        // Check for vertical move without horizontal wrap
        if (abs(startRank - targetRank) > 2) return false;

        // Additional checks as needed...

        return true;
    }

    void simulateAndPrintAllPossibleMoves(uint64_t fromMask, PieceType pieceType) {
        std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);
        for (const Move &move: possibleMoves) {
            movePiece(move);
            //printBoard();
            resetPreviousMove();
        }
    }


    bool checkifMoveIsGood(const Move &move, bool white) {
        //apply all rules for the king and check and stuff
        // check if king is in check before move
        if (isKingInCheck(white)) {
            //check if move resolves the check
            if (resolveKingCheck(move, white)) {
                return true;
            }
        } else {
            //simulateMove
            movePiece(move);
            //check if king is in check after move
            if (isKingInCheck(white)) {
                resetPreviousMove();
                return false;
            }
            resetPreviousMove();
            return true;
        }
        return false;
    }

    std::vector<Move> generateMovesForColor(bool white) {
        std::vector<Move> allPossibleMoves;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (white) {
                if (whitePieces & position) {
                    for (auto &move: generateMovesForPiece(position, getPieceTypeOnSquare((i)))) {
                        movePiece(move);
                        if (!isKingInCheck(true)) {
                            allPossibleMoves.push_back(move);
                            if (move.capture) {
                                if (isSquareThreatened(bitScanForward(move.toSquare), false)) {
                                    move.score -= 100;
                                } else {
                                    move.score += 100;
                                }
                            }
                        }
                        resetPreviousMove();
                    }
                }
            } else {
                if (blackPieces & position) {
                    for (auto &move: generateMovesForPiece(position, getPieceTypeOnSquare((i)))) {
                        movePiece(move);
                        if (!isKingInCheck(false)) {
                            allPossibleMoves.push_back(move);
                            if (move.capture) {
                                if (isSquareThreatened(bitScanForward(move.toSquare), true)) {
                                    move.score -= 100;
                                } else {
                                    move.score += 100;
                                }
                            }
                        }
                        resetPreviousMove();
                    }
                }
            }
        }
        std::sort(allPossibleMoves.begin(), allPossibleMoves.end(), [](const Move &a, const Move &b) {
            return a.score > b.score;
        });
        //remove last 50% of moves
        return allPossibleMoves;
    }


    std::vector<Move> generateMovesForColoren(bool white) {
        std::vector<Move> allPossibleMoves;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (white) {
                if (whitePieces & position) {
                    for (auto &move: generateMovesForPiece(position, getPieceTypeOnSquare(i))) {
                        movePiece(move);
                        if (!isKingInCheck(true)) {
                            allPossibleMoves.push_back(move);
                        }
                        resetPreviousMove();
                    }
                }
            } else {
                if (blackPieces & position) {
                    for (auto &move: generateMovesForPiece(position, getPieceTypeOnSquare((i)))) {
                        movePiece(move);
                        if (!isKingInCheck(false)) {
                            allPossibleMoves.push_back(move);

                        }
                        resetPreviousMove();
                    }
                }
            }
        }

        return allPossibleMoves;
    }


    void generateBotMoves() {
        Move bestMove;  // This will be used to store the best move found
        int bestScore = INT_MIN;  // Initialize bestScore with the lowest possible value
        sf::Clock clock;
        rootMoves.clear();
        // int pvSearch(int depth, int alpha, int beta, bool isMaximizer, bool isRoot = false) {
/*
        alphaBetaNoTime(INT_MIN, INT_MAX, 5, true, true);
        std::cout << "Time taken: " << clock.getElapsedTime().asSeconds() << " seconds" << std::endl;
        std::cout << "Root moves: " << rootMoves.size() << std::endl;
        sort(rootMoves.begin(), rootMoves.end(), [](const Move &a, const Move &b) {
            return a.score > b.score;
        });
        */

        // Generate all possible moves for the current player
        alphaBetaNoTime(INT_MIN, INT_MAX, 4, true, true);
        sort(rootMoves.begin(), rootMoves.end(), [](const Move &a, const Move &b) {
            return a.score > b.score;
        });
        bestMove = rootMoves[0];
        cout << "Score: " << bestMove.score << endl;
        cout << "time taken: " << clock.getElapsedTime().asSeconds() << " seconds" << endl;
        movePiece(bestMove);



        // Clear the vector before each root-level search to start fresh.








        whitesTurn = !whitesTurn;





    }


    std::vector<Move> generateAllPossibleMoves() {
        //all possible moves for the current player which is black
        std::vector<Move> allPossibleMoves;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (blackPieces & position) {
                allPossibleMoves.insert(allPossibleMoves.end(),
                                        generateMovesForPiece(position, getPieceTypeOnSquare(i)).begin(),
                                        generateMovesForPiece(position, getPieceTypeOnSquare(i)).end());
            }
        }
    }


    int getPieceValue(PieceType type) {
        switch (type) {
            case Pawn:
                return pawnValue;
            case Knight:
                return knightValue;
            case Bishop:
                return bishopValue;
            case Rook:
                return rookValue;
            case Queen:
                return queenValue;
            default:
                return 0;
        }
    }


    bool isSquareThreatened(int targetPosition, bool isPieceWhite) {
        bool onAFile = targetPosition % 8 == 0;
        bool onHFile = targetPosition % 8 == 7;

        // Corrected enemy pawn attack positions based on piece color
        int pawnOffset1 = isPieceWhite ? 9
                                       : -7; // For white pieces, check black pawn attacks from +9; for black, check white pawn attacks from -7
        int pawnOffset2 = isPieceWhite ? 7 : -9; // For white, check +7; for black, check -9

        if ((!onHFile && isEnemyPawnAt(targetPosition + pawnOffset1, !isPieceWhite)) ||
            (!onAFile && isEnemyPawnAt(targetPosition + pawnOffset2, !isPieceWhite))) {
            return true;
        }

        if (isKnightThreatening(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy knight
        }


        if (isKingThreat(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy king
        }
        if (isDiagonalThreat(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy bishop or queen
        }
        if (isFileRankThreat(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy rook or queen
        }

        if (isQueenThreatFile(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy rook or queen
        }

        if (isQueenThreatDiagonal(targetPosition, !isPieceWhite)) {
            return true; // Threatened by an enemy rook or queen
        }

        return false; // No threats found



        // Additional checks for bishops, rooks, queens, and king would go here

        return false; // No threats found
    }

    bool isFileRankThreat(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> fileRankOffsets = {1, -1, 8, -8};
        for (int offset: fileRankOffsets) {
            int currentPosition = targetPosition;
            while (true) {
                int nextPosition = currentPosition + offset;
                if (nextPosition < 0 || nextPosition >= 64) break; // Out of bounds

                // Calculate row and column to prevent wrapping
                int currentRow = currentPosition / 8;
                int nextRow = nextPosition / 8;
                int currentCol = currentPosition % 8;
                int nextCol = nextPosition % 8;

                // Prevent horizontal movement from wrapping to the next line
                if ((offset == 1 || offset == -1) && currentRow != nextRow) break;

                // Update currentPosition for the next iteration
                currentPosition = nextPosition;

                uint64_t bitboardPosition = 1ULL << currentPosition;
                if ((whitePieces | blackPieces) & bitboardPosition) { // Any piece blocks the way
                    if (!isEnemyWhite && (blackRooks) & bitboardPosition) return true;
                    if (isEnemyWhite && (whiteRooks) & bitboardPosition) return true;
                    break; // Stop if blocked by any piece
                }
            }
        }
        return false; // If no threat found
    }

    bool isQueenThreatFile(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> fileRankOffsets = {1, -1, 8, -8};
        for (int offset: fileRankOffsets) {
            int currentPosition = targetPosition;
            while (true) {
                int nextPosition = currentPosition + offset;
                if (nextPosition < 0 || nextPosition >= 64) break; // Out of bounds

                // Calculate row and column to prevent wrapping
                int currentRow = currentPosition / 8;
                int nextRow = nextPosition / 8;
                int currentCol = currentPosition % 8;
                int nextCol = nextPosition % 8;

                // Prevent horizontal movement from wrapping to the next line
                if ((offset == 1 || offset == -1) && currentRow != nextRow) break;

                // Update currentPosition for the next iteration
                currentPosition = nextPosition;

                uint64_t bitboardPosition = 1ULL << currentPosition;
                if ((whitePieces | blackPieces) & bitboardPosition) { // Any piece blocks the way
                    if (!isEnemyWhite && (blackQueens) & bitboardPosition) return true;
                    if (isEnemyWhite && (whiteQueens) & bitboardPosition) return true;
                    break; // Stop if blocked by any piece
                }
            }
        }
        return false; // If no threat found
    }

    bool isKingThreat(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> kingOffsets = {1, -1, 8, -8, 7, 9, -7, -9};
        for (int offset: kingOffsets) {
            int currentPosition = targetPosition + offset;
            if (isOnBoard(currentPosition, targetPosition, offset)) {
                uint64_t bitboardPosition = 1ULL << currentPosition;
                if ((isEnemyWhite ? whiteKing : blackKing) & bitboardPosition) {
                    return true; // Threatened by the king
                }
            }
        }
        return false;
    }

    bool isKnightThreatening(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> knightMoves = {-17, -15, -10, -6, 6, 10, 15, 17}; // Knight move offsets from a position
        for (int move: knightMoves) {
            int newPosition = targetPosition + move;
            if (newPosition < 0 || newPosition >= 64) continue; // Out of bounds
            if (abs(targetPosition % 8 - newPosition % 8) > 2) continue;

            int rowDifference = abs((newPosition / 8) - (targetPosition / 8));
            int colDifference = abs((newPosition % 8) - (targetPosition % 8));

            // Check if the move is valid (doesn't wrap the board)
            if ((rowDifference == 2 && colDifference == 1) || (rowDifference == 1 && colDifference == 2)) {
                if ((!isEnemyWhite && (blackKnights & (1ULL << newPosition))) ||
                    (isEnemyWhite && (whiteKnights & (1ULL << newPosition)))) {
                    return true; // A knight is threatening the target position
                }
            }
        }
        return false; // No knight threat found
    }


    bool isDiagonalThreat(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> diagonalOffsets = {7, 9, -7, -9};
        int startRow = targetPosition / 8;
        int startColumn = targetPosition % 8;
        for (int offset: diagonalOffsets) {
            int currentPosition = targetPosition;
            while (true) {
                int nextPosition = currentPosition + offset;
                if (nextPosition < 0 || nextPosition >= 64) break; // Out of bounds

                int nextRow = nextPosition / 8;
                int nextColumn = nextPosition % 8;

                // Calculate row and column differences to prevent diagonal wrapping
                if (abs(nextRow - startRow) != abs(nextColumn - startColumn)) break;

                // Update start position for next iteration's comparison
                startRow = currentPosition / 8;
                startColumn = currentPosition % 8;

                currentPosition = nextPosition;

                uint64_t bitboardPosition = 1ULL << currentPosition;
                if ((whitePieces | blackPieces) & bitboardPosition) { // Any piece blocks the way
                    if (!isEnemyWhite && (blackBishops) & bitboardPosition) return true;
                    if (isEnemyWhite && (whiteBishops) & bitboardPosition) return true;
                    break; // Stop if blocked by any piece
                }
            }
        }
        return false; // If no threat found
    }

    bool isQueenThreatDiagonal(int targetPosition, bool isEnemyWhite) {
        const std::vector<int> diagonalOffsets = {7, 9, -7, -9};
        int startRow = targetPosition / 8;
        int startColumn = targetPosition % 8;
        for (int offset: diagonalOffsets) {
            int currentPosition = targetPosition;
            while (true) {
                int nextPosition = currentPosition + offset;
                if (nextPosition < 0 || nextPosition >= 64) break; // Out of bounds

                int nextRow = nextPosition / 8;
                int nextColumn = nextPosition % 8;

                // Calculate row and column differences to prevent diagonal wrapping
                if (abs(nextRow - startRow) != abs(nextColumn - startColumn)) break;

                // Update start position for next iteration's comparison
                startRow = currentPosition / 8;
                startColumn = currentPosition % 8;

                currentPosition = nextPosition;

                uint64_t bitboardPosition = 1ULL << currentPosition;
                if ((whitePieces | blackPieces) & bitboardPosition) { // Any piece blocks the way
                    if (!isEnemyWhite && (blackQueens) & bitboardPosition) return true;
                    if (isEnemyWhite && (whiteQueens) & bitboardPosition) return true;
                    break; // Stop if blocked by any piece
                }
            }
        }
        return false; // If no threat found
    }


    bool isOnBoard(int currentPosition, int originalPosition, int offset) {
        // Check if off the board vertically
        if (currentPosition < 0 || currentPosition >= 64) return false;

        // Calculate row and column to prevent wrapping
        int currentRow = currentPosition / 8, currentCol = currentPosition % 8;
        int originalRow = originalPosition / 8, originalCol = originalPosition % 8;

        // Prevent horizontal wrapping
        if (offset == 1 || offset == -1) {
            return currentRow == originalRow;
        }
        // Prevent vertical wrapping (technically unnecessary but included for completeness)
        if (offset == 8 || offset == -8) {
            return true; // Always true since vertical moves can't wrap
        }
        // Prevent diagonal wrapping
        return abs(currentRow - originalRow) == abs(currentCol - originalCol);
    }


    bool isEnemyPawnAt(int position, bool isEnemyWhite) {
        if (position < 0 || position >= 64) return false; // Out of bounds
        uint64_t bitboardPosition = 1ULL << position;
        return (isEnemyWhite ? whitePawns : blackPawns) & bitboardPosition;
    }

    bool isEnemyKnightAt(int position, bool isEnemyWhite) {
        if (position < 0 || position >= 64) return false; // Out of bounds
        uint64_t bitboardPosition = 1ULL << position;
        return (isEnemyWhite ? whiteKnights : blackKnights) & bitboardPosition;
    }

    bool hasPawnSupport(int targetPosition, bool isPieceWhite) {
        int pawnOffsets[2] = {isPieceWhite ? -7 : 7, isPieceWhite ? -9 : 9}; // Determine offsets based on color
        bool onAFile = targetPosition % 8 == 0;
        bool onHFile = targetPosition % 8 == 7;

        for (int offset: pawnOffsets) {
            int supportPosition = targetPosition + offset;
            // Edge of board adjustments
            if ((onAFile && offset == (isPieceWhite ? -9 : 7)) ||
                (onHFile && offset == (isPieceWhite ? -7 : 9)))
                continue;
            if (supportPosition < 0 || supportPosition >= 64) continue; // Out of bounds safety check

            uint64_t bitboardPosition = 1ULL << supportPosition;
            // Check if a pawn of the same color occupies the support position
            if ((isPieceWhite && (whitePawns & bitboardPosition)) ||
                (!isPieceWhite && (blackPawns & bitboardPosition))) {
                return true;
            }
        }
        return false;
    }


    int getPositionalValue(PieceType pieceType, int position, bool isWhite) {
        int index = isWhite ? position : (63 - position); // Flip position for black
        switch (pieceType) {
            case Pawn:
                return pawnPositionalValue[index];
            case Knight:
                return knightPositionalValue[index];
            case Bishop:
                return bishopPositionalValue[index];
            case Rook:
                return rookPositionalValue[index];
            case Queen:
                return queenPositionalValue[index];
            default:
                return 0;
        }
    }


    int evaluateBoard(bool isWhite) {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            PieceType pieceType = getPieceTypeOnSquare(i);
            bool isWhitePiece = isSquareOccupiedByWhite(i);
            if (isWhitePiece == isWhite && pieceType != None) {
                int pieceValue = getPieceValue(pieceType);
                score += pieceValue + getPositionalValue(pieceType, i, isWhite);

                if (isSquareThreatened(i, isWhite)) {
                    bool reverseThreatBackup = isSquareThreatened(i, isWhite);
                    bool pawnSupport = hasPawnSupport(i, isWhitePiece);
                    bool backup = reverseThreatBackup || pawnSupport;
                    int threatValue = getPieceValue(findMostSignificantThreateningPieceType(i, !isWhite));
                    // Adjust score based on threat and backup status; consider refining this logic
                    int difference = pieceValue - threatValue;
                    if (!backup) {
                        if (difference > 13) {
                            score -= pieceValue * 4;
                        } else {
                            score -= pieceValue * 2;
                        }
                    } else {
                        if (difference > 12) {
                            score -= pieceValue * 2;
                        }
                    }
                }
            } else if (pieceType != None) {
                score -= getPieceValue(pieceType);
            }
        }
        return score;
    }


    int evaluateBoardForWhitePieces() {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            PieceType pieceType = getPieceTypeOnSquare(i);
            bool isWhitePiece = isSquareOccupiedByWhite(i);

            // Focus on white pieces
            if (isWhitePiece && pieceType != None) {
                int pieceValue = getPieceValue(pieceType);
                int positionalValue = getPositionalValue(pieceType, i, isWhitePiece);
                score += pieceValue + positionalValue;

                // If the piece is threatened, adjust the score
                if (isSquareThreatened(i, isWhitePiece)) {
                    bool reverseThreatBackup = isSquareThreatened(i, !isWhitePiece);
                    bool pawnSupport = hasPawnSupport(i, isWhitePiece);
                    bool backup = reverseThreatBackup || pawnSupport;
                    if (!backup) {
                        score -= 15;
                    } else {
                        score -= 5;
                    }
                }
            }
            score -= getPieceValue(pieceType);
        }
        return score;
    }

    int shortEvalBoard(bool white) {
        int score = 0;
        int enemies = 0;
        int friendlys = 0;
        int TotalFriendlyPieceValue = 0;
        int TotalEnemyPieceValue = 0;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            PieceType pieceType = getPieceTypeOnSquare(i);
            bool isWhitePiece = isSquareOccupiedByWhite(i);
            if (isWhitePiece == white && pieceType != None) {
                score += getPositionalValue(pieceType, i, isWhitePiece);
                score += getPieceValue(pieceType) * 4;
                friendlys++;
            } else {
                score -= getPieceValue(pieceType);
                enemies++;
            }
        }
        score += (friendlys - enemies) * 2;
        return score;
    }


    void printPieceType(PieceType pieceType) {
        switch (pieceType) {
            case Pawn:
                std::cout << "Pawn";
                break;
            case Knight:
                std::cout << "Knight";
                break;
            case Bishop:
                std::cout << "Bishop";
                break;
            case Rook:
                std::cout << "Rook";
                break;
            case Queen:
                std::cout << "Queen";
                break;
            case King:
                std::cout << "King";
                break;
            default:
                std::cout << "None";
                break;
        }


    }


    PieceType findMostSignificantThreateningPieceType(int targetPosition, bool isEnemyWhite) {
        PieceType mostSignificantThreat = None; // Initialize as None, assuming no threat initially
        bool threatDetected = false;

        // Calculate enemy pawn attack positions with edge case handling
        int pawnOffset1 = !isEnemyWhite ? -7 : 9;
        int pawnOffset2 = !isEnemyWhite ? -9 : 7;
        bool onAFile = targetPosition % 8 == 0;
        bool onHFile = targetPosition % 8 == 7;

        if ((!onHFile && isEnemyPawnAt(targetPosition + pawnOffset1, !isEnemyWhite)) ||
            (!onAFile && isEnemyPawnAt(targetPosition + pawnOffset2, !isEnemyWhite))) {
            mostSignificantThreat = Pawn;
            threatDetected = true;
        }

        if (!threatDetected && isKnightThreatening(targetPosition, !isEnemyWhite)) {
            mostSignificantThreat = Knight;
            threatDetected = true;
        }

        if (!threatDetected && isDiagonalThreat(targetPosition, !isEnemyWhite)) {
            mostSignificantThreat = Bishop;
            threatDetected = true;
        }

        if (!threatDetected && isFileRankThreat(targetPosition, !isEnemyWhite)) {
            mostSignificantThreat = Rook;
            threatDetected = true;
        }

        if (!threatDetected && isQueenThreatFile(targetPosition, !isEnemyWhite)) {
            mostSignificantThreat = Queen;
            threatDetected = true;
        }

        if (!threatDetected && isQueenThreatDiagonal(targetPosition, !isEnemyWhite)) {
            mostSignificantThreat = Queen;
            threatDetected = true;
        }


    if (!threatDetected &&isKingThreat(targetPosition, !isEnemyWhite)) {
        mostSignificantThreat = King;
        threatDetected = true;
    }
    return mostSignificantThreat;
}
    int pvSearch(int alpha, int beta, int depth, bool isMaximizer) {
        if (depth == 0){
            return isMaximizer? -shortEvalBoard(false) : shortEvalBoard(false);
        }
        bool bSearchPV = true;
        int score = INT_MIN;
        auto moves = generateMovesForColor(isMaximizer);
        for (auto &move: moves){
            movePiece(move);
            if (bSearchPV){
                score = -pvSearch(-beta, -alpha, depth - 1, !isMaximizer);
            }else{
                score = -pvSearch(-alpha - 1, -alpha, depth - 1, !isMaximizer);
                if (alpha < score && score < beta){
                    score = -pvSearch(-beta, -alpha, depth - 1, !isMaximizer);
                }
            }
            resetPreviousMove();
            if (score >= beta){
                return beta;
            }
            if (score > alpha){
                alpha = score;
                bSearchPV = false;
            }
        }
    }






    /*
     function pvs(node, depth, α, β, color) is
    if depth = 0 or node is a terminal node then
        return color × the heuristic value of node
    for each child of node do
        if child is first child then
            score := −pvs(child, depth − 1, −β, −α, −color)
        else
            score := −pvs(child, depth − 1, −α − 1, −α, −color) (* search with a null window *)
            if α < score < β then
                score := −pvs(child, depth − 1, −β, −α, −color) (* if it failed high, do a full re-search *)
        α := max(α, score)
        if α ≥ β then
            break (* beta cut-off *)
    return α
     */

    vector<Move> orderingmoves(vector<Move> moves) {
        vector<Move> orderedMoves;
        for ( auto &move: moves) {
            movePiece(move);
            move.score = shortEvalBoard(false);
            resetPreviousMove();
        }
        sort(moves.begin(), moves.end(), [](const Move &a, const Move &b) {
            return a.score > b.score;
        });
        return orderedMoves;
    }

    int alphaBetaNoTime(int alpha, int beta, int depth, bool isMaximizer, bool root) {
        if (depth == 0) {
            return shortEvalBoard(false);
        }
        if (isMaximizer) {
            int maxEval = INT_MIN;
            auto moves = generateMovesForColor(false); // Generate moves for black
            for (auto& move : moves) {
                movePiece(move); // Apply the move
                int eval = alphaBetaNoTime(alpha, beta, depth - 1, false, false); // Recurse for minimizing player
                if (root) { // If this is the root, save the move and its score
                    move.score = eval; // Set the move's score
                    rootMoves.push_back(move); // Add the move to rootMoves
                }
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                resetPreviousMove(); // Undo the move
                if (alpha >= beta) break; // Alpha-beta pruning
            }
            return maxEval;
        } else {
            int minEval = INT_MAX;
            auto moves = generateMovesForColor(true); // Generate moves for white
            for (const auto& move : moves) {
                movePiece(move);
                int eval = alphaBetaNoTime(alpha, beta, depth - 1, true, false); // Recurse for maximizing player
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                resetPreviousMove();
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }








    int alphaBeta(int alpha, int beta, int depth, bool isMaximizer, sf::Clock clock, int time) {
        // Base case: if depth is 0, evaluate the board from black's perspective.
        if (clock.getElapsedTime().asSeconds() > time) {
            return shortEvalBoard(false);
        }
        if (depth == 0) {
            return shortEvalBoard(false);
        }

        if (isMaximizer) {
            int maxEval = INT_MIN;
            // Generate moves for black since the bot is black
            auto moves = generateMovesForColor(false);
            for (const auto &move: moves) {
                movePiece(move); // Apply the move to the board.
                int eval = alphaBeta(alpha, beta, depth - 1, false, clock, time);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                resetPreviousMove();

                if (alpha >= beta) {
                    break; // Alpha cut-off for pruning.
                }
            }
            return maxEval;
        } else {
            int minEval = INT_MAX;

            auto moves = generateMovesForColor(true);

            for (const auto &move: moves) {
                movePiece(move); // Apply the move to the board.
                int eval = alphaBeta(alpha, beta, depth - 1, true, clock, time);

                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);

                resetPreviousMove();

                if (beta <= alpha) {
                    break; // Beta cut-off for pruning.
                }
            }
            return minEval;
        }
    }

    vector<Move> generateCapturesForColor(bool white){
        vector<Move> moves = generateMovesForColor(false);
        vector<Move> movesss;
        for (auto &move: moves){
            if (move.capture){
                movesss.push_back(move);
            }
        }
        return movesss;
    }

    int quiesce(int alpha, int beta){
        int stand_pat = shortEvalBoard(false);

        if (stand_pat >= beta){
            return beta;
        }
        if (alpha < stand_pat){
            alpha = stand_pat;
        }
        vector<Move> moves = generateCapturesForColor(false);
        for (auto &move: moves){
            movePiece(move);
            int score = -quiesce(-beta, -alpha);
            resetPreviousMove();
            if (score >= beta){
                return beta;
            }
            if (score > alpha){
                alpha = score;
            }
        }
        return alpha;

    }
/*
    int alphaBetaan(int alpha, int beta, int depth, bool isMaximizer, sf::Clock clock) {
        // Base case: if depth is 0, evaluate the board from black's perspective.
        if (depth == 0) {
            return evaluateBoard(false) ;
        }

        if (isMaximizer) {
            int maxEval = INT_MIN;
            // Generate moves for black since the bot is black
            auto moves = generateMovesForColoren(false);
            for (const auto& move : moves) {
                movePiece(move); // Apply the move to the board.
                int eval = alphaBeta(alpha, beta, depth - 1, false, clock);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                resetPreviousMove();
                if (alpha >= beta) {
                    break; // Alpha cut-off for pruning.
                }
            }
            return maxEval;
        } else {
            int minEval = INT_MAX;
            // Generate moves for white since this is the opponent's turn
            auto moves = generateMovesForColoren(true);
            for (const auto& move : moves) {
                movePiece(move); // Apply the move to the board.
                int eval = alphaBeta(alpha, beta, depth - 1, true, clock);
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                resetPreviousMove();
                if (beta <= alpha) {
                    break; // Beta cut-off for pruning.
                }
            }
            return minEval;
        }
    }
    */




    static int calculateCapturedPieceScore(PieceType pieceType) {
        switch (pieceType) {
            case Pawn:
                return pawnValue;
            case Knight:
            case Bishop:
                return bishopValue;
            case Rook:
                return rookValue;
            case Queen:
                return queenValue;
            default:
                return 0;
        }
    }

    };

