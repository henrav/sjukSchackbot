//
// Created by Henrik Ravnborg on 2024-03-09.
//

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <cstdint>
struct Move {
    uint64_t* pieceMoved;    // Pointer to the bitboard of the piece that moved
    uint64_t fromSquare;     // Bitboard representation of the starting square
    uint64_t toSquare;       // Bitboard representation of the destination square
    uint64_t* pieceCaptured; // Pointer to the bitboard of the captured piece, if any
    bool capture;            // Indicates if the move was a capture

    Move() : pieceMoved(nullptr), fromSquare(0), toSquare(0), pieceCaptured(nullptr), capture(false) {}

    Move(uint64_t* moved, uint64_t from, uint64_t to, uint64_t* captured = nullptr, bool cap = false)
            : pieceMoved(moved), fromSquare(from), toSquare(to), pieceCaptured(captured), capture(cap) {}



};

class ChessBoard {
public:
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
    uint64_t* previousMovePieceBitboard{};
    uint64_t previousMoveFrom{};
    uint64_t previousMoveTo{};

    enum PieceType {
        Pawn, Knight, Bishop, Rook, Queen, King, None
    };

    ChessBoard() {
        resetBoard();
    }

    std::vector<Move> moveHistory;

    std::vector<Move> generateMovesForPiece(uint64_t pieceBitboard, PieceType type) {
        std::vector<Move> possibleMoves;
        if (type == Knight) {
            possibleMoves = generateKnightMoves(pieceBitboard); // Assuming isWhite is determined beforehand
        }
        // Other piece types would be handled similarly
        return possibleMoves;
    }



    [[nodiscard]] PieceType getPieceTypeOnSquare(int theSquare) {
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
    bool isSquareOccupiedByWhite(int square) {
        uint64_t mask = 1ULL << square;
        return (whitePieces & mask) != 0;
    }


    //check if square is occupied
    bool isSquareOccupied(int square) {
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
    void movePiece(uint64_t& pieceBitboard, uint64_t fromSquare, uint64_t toSquare, uint64_t* capturedPieceBitboard = nullptr) {
        // Log the move before making it
        bool capture = capturedPieceBitboard && (*capturedPieceBitboard & toSquare);
        moveHistory.emplace_back(&pieceBitboard, fromSquare, toSquare, capturedPieceBitboard, capture);

        // Make the move
        pieceBitboard &= ~fromSquare;
        pieceBitboard |= toSquare;

        // If there's a capture, remove the captured piece
        if (capture) {
            *capturedPieceBitboard &= ~toSquare;
        }

        updateOccupiedSquares();
        printBoard();
    }

    //reset the previous move
    void resetPreviousMove() {
        if (!moveHistory.empty()) {
            Move& lastMove = moveHistory.back();

            // Undo the move
            *lastMove.pieceMoved &= ~lastMove.toSquare; // Remove the piece from its destination
            *lastMove.pieceMoved |= lastMove.fromSquare; // Place the piece back to its starting square

            // If there was a capture, restore the captured piece
            if (lastMove.capture && lastMove.pieceCaptured) {
                *lastMove.pieceCaptured |= lastMove.toSquare;
            }

            updateOccupiedSquares();
            moveHistory.pop_back(); // Remove the last move from history
        }
    }

    // Function to print the board - mainly for debugging purposes
    void printBoard()  {
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file < 8; file++) {
                uint64_t position = 1ULL << (rank * 8 + file);
                if (occupiedSquares & position) {
                    std::cout << "1 ";
                } else {
                    std::cout << ". ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    bool isSquareInBounds(int square, int square1) {
        return square >= 0 && square < 64 && square1 >= 0 && square1 < 64;
    }

    int bitScanForward(uint64_t position) {
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
        const std::vector<int> offsets = {-17, -15, -10, -6, 6, 10, 15, 17};
        bool isWhite = (whitePieces & knightPosition) != 0;
        uint64_t ownPieces = isWhite ? this->whitePieces : this->blackPieces;
        uint64_t enemyPieces = isWhite ? this->blackPieces : this->whitePieces;
        uint64_t* knightBitboard = isWhite ? &this->whiteKnights : &this->blackKnights;

        for (int offset : offsets) {
            int targetSquare = startSquare + offset;
            if (isSquareInBounds(startSquare, targetSquare)) {
                uint64_t targetBitboard = 1ULL << targetSquare;
                bool isTargetSquareOccupied = this->occupiedSquares & targetBitboard;
                bool isTargetSquareOccupiedByOwn = ownPieces & targetBitboard;

                // Proceed if the target square is not occupied by a piece of the same color.
                if (!isTargetSquareOccupiedByOwn) {
                    uint64_t* capturedPieceBitboard = nullptr;
                    bool isCapture = isTargetSquareOccupied && (enemyPieces & targetBitboard);

                    // Determine captured piece bitboard if it's a capture.
                    if (isCapture) {
                        PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                        capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                    }

                    // Create and add the move.
                    moves.emplace_back(knightBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);
                }
            }
        }

        return moves;
    }

    bool playerMove(int startRank, int startFile, int targetRank, int targetFile) {
        uint64_t fromMask = 1ULL << (startRank * 8 + startFile);
        uint64_t toMask = 1ULL << (targetRank * 8 + targetFile);

        // Determine the piece at the start position
        PieceType pieceType = getPieceTypeOnSquare(bitScanForward(fromMask)); // Ensure this method exists and works as expected.
        if (pieceType == None) return false; // No piece to move

        // Generate possible moves for the piece
        std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);

        for (const Move& move : possibleMoves) {
            if (move.toSquare == toMask) {
                // The move is valid, execute it
                uint64_t* pieceBitboard = getBitboardPointerByPieceType(pieceType, isSquareOccupiedByWhite(fromMask));
                uint64_t* capturedPieceBitboard = isSquareOccupied(toMask) ? getBitboardPointerByPieceType(
                        getPieceTypeOnSquare(bitScanForward(toMask)), !isSquareOccupiedByWhite(fromMask)) : nullptr;

                movePiece(*pieceBitboard, fromMask, toMask, capturedPieceBitboard);
                // Note: Visual and state updates related to the GUI should be handled in the calling function or component.

                return true; // Move was successfully applied
            }
        }

        return false; // No valid move was executed
    }













};