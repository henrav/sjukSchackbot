//
// Created by Henrik Ravnborg on 2024-03-09.
//
#include <thread>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <cstdint>
static bool update = false;
struct Move {
    uint64_t* pieceMoved;    // Pointer to the bitboard of the piece that moved
    uint64_t fromSquare;     // Bitboard representation of the starting square
    uint64_t toSquare;       // Bitboard representation of the destination square
    uint64_t* pieceCaptured;// Pointer to the bitboard of the captured piece, if any
    bool capture;            // Indicates if the move was a capture


    Move() : pieceMoved(nullptr), fromSquare(0), toSquare(0), pieceCaptured(nullptr), capture(false){}

    Move(uint64_t* moved, uint64_t from, uint64_t to, uint64_t* captured = nullptr, bool cap = false, int score = 0)
            : pieceMoved(moved), fromSquare(from), toSquare(to), pieceCaptured(captured), capture(cap) {}

};

class ChessBoard {
public:
    const int pawnValue = 15;
    const int knightValue = 45;
    const int bishopValue = 45;
    const int rookValue = 70;
    const int queenValue = 130;

    // Additional score for capturing a more valuable piece with a less valuable piece
    const int captureBonus = 30;
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
    bool whitesTurn = true;

    enum PieceType {
        Pawn, Knight, Bishop, Rook, Queen, King, None
    };

    ChessBoard() {
        resetBoard();
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
        //Board();
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

            moveHistory.pop_back(); // Remove the last move from history
            updateOccupiedSquares();
        }
        //printBoard();
    }

    // Function to print the board - mainly for debugging purposes
    void printBoard() const  {
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
        const std::vector<int> offsets = {-17, -15, -10, -6, 6, 10, 15, 17};
        bool isWhite = (whitePieces & knightPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t* knightBitboard = isWhite ? &whiteKnights : &blackKnights;

        for (int offset : offsets) {
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
            // Ensure target square is not occupied by own piece; allow moves to empty squares or squares with enemy pieces.
            if (ownPieces & targetBitboard) continue;

            bool isCapture = (enemyPieces & targetBitboard) != 0;
            uint64_t* capturedPieceBitboard = nullptr;
            if (isCapture) {
                capturedPieceBitboard = getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite);
            }

            moves.emplace_back(knightBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);
        }

        return moves;
    }





    std::vector<Move> generatePawnMoves(uint64_t pawnPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(pawnPosition);
        bool isWhite = (whitePieces & pawnPosition) != 0;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t* pawnBitboard = isWhite ? &whitePawns : &blackPawns;
        uint64_t occupiedSquares = whitePieces | blackPieces;
        int startRow = startSquare / 8;

        // Define movement direction and starting row for pawn double move based on color
        int singleMoveOffset = isWhite ? 8 : -8;
        int doubleMoveOffset = isWhite ? 16 : -16;
        int doubleMoveStartRow = isWhite ? 1 : 6;
        int attackOffsets[] = {isWhite ? 7 : -9, isWhite ? 9 : -7};

        // Single move
        int targetSquare = startSquare + singleMoveOffset;
        if (targetSquare >= 0 && targetSquare < 64 && !(occupiedSquares & (1ULL << targetSquare))) {
            moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << targetSquare);

            // Double move
            int doubleMoveSquare = startSquare + doubleMoveOffset;
            if (startRow == doubleMoveStartRow && !(occupiedSquares & (1ULL << doubleMoveSquare))) {
                moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << doubleMoveSquare);
            }
        }

        // Attack moves
        for (int offset : attackOffsets) {
            targetSquare = startSquare + offset;
            if (targetSquare >= 0 && targetSquare < 64 && (enemyPieces & (1ULL << targetSquare))) {
                int targetCol = targetSquare % 8;
                int startCol = startSquare % 8;
                // Ensure attack move does not "wrap" around the board
                if (abs(targetCol - startCol) == 1) {
                    moves.emplace_back(pawnBitboard, 1ULL << startSquare, 1ULL << targetSquare, getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite), true);
                }
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
        uint64_t* rookBitboard = isWhite ? &whiteRooks : &blackRooks;
        int offsets[] = {1, -1, 8, -8};

        int startRow = startSquare / 8, startCol = startSquare % 8;

        for (int offset : offsets) {
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
                uint64_t* capturedPieceBitboard = nullptr;
                if (isCapture) {
                    PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                    capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                }

                moves.emplace_back(rookBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);

                if (isCapture) {
                    // Capture move; no further moves in this direction
                    break;
                }

                targetSquare += offset; // Continue in the same direction
            }
        }

        return moves;
    }



    std::vector<Move> generateBishopMoves(uint64_t bishopPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(bishopPosition);
        int startRow = startSquare / 8, startCol = startSquare % 8;
        bool isWhite = (whitePieces & bishopPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t* bishopBitboard = isWhite ? &whiteBishops : &blackBishops;
        int offsets[] = {7, 9, -7, -9};

        for (int offset : offsets) {
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
                uint64_t* capturedPieceBitboard = nullptr;
                if (isCapture) {
                    capturedPieceBitboard = getBitboardPointerByPieceType(getPieceTypeOnSquare(targetSquare), !isWhite);
                }

                moves.emplace_back(bishopBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);

                if (isCapture) break; // Capture stops further traversal in this direction

                // Prepare for the next iteration
                targetSquare += offset; // Continue in the same diagonal direction
                // Ensure the next target doesn't wrap the board
                if (abs((targetSquare / 8) - targetRow) != 1) break;
            }
        }

        return moves;
    }



    static bool isValidRookMove(int startSquare, int targetSquare){
        int startRank = startSquare / 8, startFile = startSquare % 8;
        int targetRank = targetSquare / 8, targetFile = targetSquare % 8;

        // Ensure the move stays within the board
        if (targetSquare < 0 || targetSquare >= 64) return false;

        // Rook moves must have the same rank or file
        return startRank == targetRank || startFile == targetFile;
    }

    static bool isValidDiagonalMove(int startSquare, int targetSquare){
        int startRank = startSquare / 8, startFile = startSquare % 8;
        int targetRank = targetSquare / 8, targetFile = targetSquare % 8;

        // Ensure the move stays within the board
        if (targetSquare < 0 || targetSquare >= 64) return false;

        // Diagonal moves must have the same rank and file difference magnitude
        return abs(startRank - targetRank) == abs(startFile - targetFile);
    }

    std::vector<Move> generateQueenMoves(uint64_t queenPosition) {
        std::vector<Move> moves;
        int startSquare = bitScanForward(queenPosition);
        bool isWhite = (whitePieces & queenPosition) != 0;
        uint64_t ownPieces = isWhite ? whitePieces : blackPieces;
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t* queenBitboard = isWhite ? &whiteQueens : &blackQueens;
        // Combine bishop and rook offsets for queen movement
        int offsets[] = {7, 9, -7, -9, 1, -1, 8, -8};

        for (int offset : offsets) {
            int targetSquare = startSquare + offset;

            while (true) { // Loop until break
                int targetRow = targetSquare / 8;
                int targetCol = targetSquare % 8;

                // Validate movement within board bounds and for diagonals, ensure proper row and column difference
                if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8 ||
                    (abs(offset) == 7 || abs(offset) == 9) && abs(targetRow - startSquare / 8) != abs(targetCol - startSquare % 8)) {
                    break;
                }

                uint64_t targetBitboard = 1ULL << targetSquare;
                if (ownPieces & targetBitboard) {
                    // Own piece is blocking further movement
                    break;
                }

                bool isCapture = enemyPieces & targetBitboard;
                uint64_t* capturedPieceBitboard = nullptr;
                if (isCapture) {
                    PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                    capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
                }

                // Adjust score based on position; for simplicity, this example keeps the logic as is

                moves.emplace_back(queenBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);

                if (isCapture) {
                    // Stop movement after capture
                    break;
                }

                targetSquare += offset; // Proceed to the next square in the current direction
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
        uint64_t* kingBitboard = isWhite ? &whiteKing : &blackKing;

        // Assuming 'isSquareInBounds' checks if targetSquare is a valid board position.
        // If its logic is based on 'startSquare' and 'targetSquare', consider optimizing it
        // to avoid costly computations, especially if it's called in a tight loop.

        int offsets[] = {1, -1, 8, -8, 7, 9, -7, -9};

        for (int offset : offsets) {
            int targetSquare = startSquare + offset;

            // Continue if the target square is not on the board
            if (!isSquareInBounds(startSquare, targetSquare)) continue;

            uint64_t targetBitboard = 1ULL << targetSquare;
            // Skip if the target square is occupied by own piece
            if (ownPieces & targetBitboard) continue;

            bool isCapture = enemyPieces & targetBitboard;
            uint64_t* capturedPieceBitboard = nullptr;
            if (isCapture) {
                PieceType pieceType = getPieceTypeOnSquare(targetSquare);
                capturedPieceBitboard = getBitboardPointerByPieceType(pieceType, !isWhite);
            }
            moves.emplace_back(kingBitboard, 1ULL << startSquare, 1ULL << targetSquare, capturedPieceBitboard, isCapture);
        }
        return moves;
    }



    bool playerMove(int startRank, int startFile, int targetRank, int targetFile) {
        uint64_t fromMask = 1ULL << (startRank * 8 + startFile);
        uint64_t toMask = 1ULL << (targetRank * 8 + targetFile);
        bool isWhite = (whitePieces & fromMask) != 0;
        if (isWhite != whitesTurn) return false; // It's not the player's turn
        PieceType pieceType = getPieceTypeOnSquare(bitScanForward(fromMask)); // Ensure this method exists and works as expected.
        if (pieceType == None) return false; // No piece to move
        // First, check if the king is in check
        uint64_t king = isWhite ? whiteKing : blackKing;
        if (isKingInCheck(true)) {
            // Generate all possible moves for the player
            std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);
            // Filter out moves that don't resolve the check

            std::vector<Move> movesThatResolveCheck = filterMovesThatResolveCheck(possibleMoves, whitesTurn);
            // Check if the move is valid
            for (const Move& move : movesThatResolveCheck) {
                if (move.toSquare == toMask) {
                    // The move is valid, execute it
                    uint64_t* pieceBitboard = getBitboardPointerByPieceType(pieceType, isSquareOccupiedByWhite(bitScanForward(fromMask)));
                    uint64_t* capturedPieceBitboard = isSquareOccupied(bitScanForward(fromMask)) ? getBitboardPointerByPieceType(getPieceTypeOnSquare(bitScanForward(toMask)), !isSquareOccupiedByWhite(bitScanForward(fromMask))) : nullptr;
                    movePiece(*pieceBitboard, fromMask, toMask, capturedPieceBitboard);
                    // Note: Visual and state updates related to the GUI should be handled in the calling function or component.
                    whitesTurn = !whitesTurn;
                    return true; // Move was successfully applied
                }
            }
            return false;
            // No valid move executed, either the move didn't resolve the check or wasn't valid
        }else{
            std::cout << "King is not in check" << std::endl;
            // Generate possible moves for the piece
            std::vector<Move> possibleMoves = generateMovesForPiece(fromMask, pieceType);

            for (const Move& move : possibleMoves) {
                movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured); // Apply the move temporarily

                // Check if OUR king is in check after this move
                bool ourKingInCheckAfterMove = isKingInCheck( true);

                resetPreviousMove(); // Undo the move

                // Only proceed if our king isn't in check as a result of this move
                if (!ourKingInCheckAfterMove && move.toSquare == toMask) {
                    // The move is valid and does not place our king in check
                    uint64_t* pieceBitboard = getBitboardPointerByPieceType(pieceType, isWhite);
                    uint64_t* capturedPieceBitboard = isSquareOccupied(bitScanForward(toMask)) ? getBitboardPointerByPieceType(getPieceTypeOnSquare(bitScanForward(toMask)), !isWhite) : nullptr;

                    movePiece(*pieceBitboard, fromMask, toMask, capturedPieceBitboard);
                    whitesTurn = !whitesTurn;
                    generateBotMoves();
                    return true; // Successfully executed move
                }
            }
        }
        // Determine the piece at the start position

        return false; // No valid move was executed
    }

    bool isKingInCheck(bool isWhite) {
        uint64_t enemyPieces = isWhite ? blackPieces : whitePieces;
        uint64_t king = isWhite ? whiteKing : blackKing;
        // Check if any enemy piece can attack the king
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (enemyPieces & position) {
                PieceType pieceType = getPieceTypeOnSquare(i);
                if (pieceType == isWhite? blackPawns : whitePawns){
                    int kingPosition = bitScanForward(king);
                    int pawnRow = i / 8, pawnCol = i % 8;
                    int kingRow = kingPosition / 8, kingCol = kingPosition % 8;
                    // Calculate row and column differences between the pawn and king
                    int rowDiff = pawnRow - kingRow;
                    int colDiff = abs(pawnCol - kingCol);

                    // For white king, check black pawns; for black king, check white pawns
                    if ((isWhite && rowDiff == -1 && colDiff == 1) || (!isWhite && rowDiff == 1 && colDiff == 1)) {
                        return true; // Pawn can attack the king diagonally
                    }
                }
                std::vector<Move> possibleMoves = generateMovesForPiece(position, pieceType);
                for (const Move& move : possibleMoves) {
                    if (move.toSquare == king) {
                        return true;
                    }
                }
            }
        }
    }

    bool checkIfMoveResultsInCheck(Move move, bool isWhite) {
        // Simulate the move
        movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
        // Check if the move results in check
        bool isCheck = isWhite ? isKingInCheck(whiteKing) : isKingInCheck(blackKing);

        // Reset the move
        resetPreviousMove();

        return isCheck;
    }

    bool resolveKingCheck(Move move, bool isWhite){
        // Simulate the move
        movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);

        // Check if the move results in check
        bool isCheck = isKingInCheck(isWhite);

        // Reset the move
        resetPreviousMove();

        return isCheck;

    }

    std::vector<Move> filterMovesThatResolveCheck(const std::vector<Move>& possibleMoves, bool isWhite) {
        std::vector<Move> movesThatResolveCheck;
        for (const Move& move : possibleMoves) {
            // Simulate the move
            movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
            //printBoard();
            // If the move results in the king no longer being in check, keep it
            if (!isKingInCheck( isWhite)) {
                movesThatResolveCheck.push_back(move);
            }

            // Undo the move
            resetPreviousMove();
        }

        return movesThatResolveCheck;
    }

    bool knightMoveValid(int startSquare, int endSquare){
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
        for (const Move& move : possibleMoves) {
            movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
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
        }else{
            //simulateMove
            movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
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

    void generateBotMoves() {
        Move bestMove;
        int bestScore = INT_MIN;

        for (int i = 0; i < 64; i++) {
            if (blackPieces & (1ULL << i)) {
                auto moves = generateMovesForPiece(1ULL << i, getPieceTypeOnSquare(i));
                for (auto& move : moves) {
                    movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
                    if (!isKingInCheck(false)) {  // Check for no check on black
                        movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
                        int score = alphaBeta(INT_MIN, INT_MAX, 4, false);
                        if (score > bestScore) {
                            bestScore = score;
                            bestMove = move;
                        }
                        resetPreviousMove();
                    }
                    resetPreviousMove();
                }
            }
        }

        if (bestMove.pieceMoved) {  // Check if a move was found
            movePiece(*bestMove.pieceMoved, bestMove.fromSquare, bestMove.toSquare, bestMove.pieceCaptured);
            whitesTurn = !whitesTurn;  // Switch turn
        }
    }


    std::vector<Move> generateAllPossibleMoves() {
        //all possible moves for the current player which is black
        std::vector<Move> allPossibleMoves;
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (blackPieces & position) {
                allPossibleMoves.insert(allPossibleMoves.end(), generateMovesForPiece(position, getPieceTypeOnSquare(i)).begin(), generateMovesForPiece(position, getPieceTypeOnSquare(i)).end());
            }
        }
    }
    const std::vector<int> pawnPositionalValue = {
            0, 0, 0, 0, 0, 0, 0, 0, // Rank 1 (from white's perspective)
            1, 2, 3, 5, 5, 3, 2, 1, // Rank 2
            1, 1, 2, 3, 3, 2, 1, 1, // Rank 3
            0, 0, 0, 2, 2, 0, 0, 0, // Rank 4
            1, 1, 2, 3, 3, 2, 1, 1, // Rank 5
            1, 2, 3, 5, 5, 3, 2, 1, // Rank 6
            7, 7, 7, 7, 7, 7, 7, 7, // Rank 7
    };

// Positional values for knights
    const std::vector<int> knightPositionalValue = {
            -10, -4, -2, -2, -2, -2, -4, -10,
            -5 , 0 , 0 , 0 , 0 , 0 , 0 , -5,
            -5 , 0 , 5 , 5 , 5 , 5 , 0 , -5,
            -5 , 0 , 5 , 10, 10, 5 , 0 , -5,
            -5 , 0 , 5 , 10, 10, 5 , 0 , -5,
            -5 , 0 , 5 , 5 , 5 , 5 , 0 , -5,
            -5 , 0 , 0 , 0 , 0 , 0 , 0 , -5,
            -10, -4, -2, -5, -5, -2, -4, -10
    };

    bool isPieceThreatened(int targetPosition, bool isPieceWhite) {
        uint64_t enemyPieces = isPieceWhite ? blackPieces : whitePieces;
        // Calculate row and column for targetPosition for pawn attack validation
        int targetRow = targetPosition / 8, targetCol = targetPosition % 8;

        // Iterate through all squares to check for threats
        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            if (enemyPieces & position) {
                PieceType pieceType = getPieceTypeOnSquare(i);
                // Special handling for pawn threats
                if ((isPieceWhite && pieceType == Pawn && (blackPieces & position)) ||
                    (!isPieceWhite && pieceType == Pawn && (whitePieces & position))) {
                    int pawnRow = i / 8, pawnCol = i % 8;
                    // Calculate differences between the pawn and target position
                    int rowDiff = pawnRow - targetRow;
                    int colDiff = abs(pawnCol - targetCol);

                    // Check if pawn can attack the target position
                    if ((isPieceWhite && rowDiff == 1 && colDiff == 1) ||
                        (!isPieceWhite && rowDiff == -1 && colDiff == 1)) {
                        return true; // Pawn can attack the target diagonally
                    }
                } else {
                    // For other pieces, generate all possible moves and check if any targets the given position
                    std::vector<Move> possibleMoves = generateMovesForPiece(position, pieceType);
                    for (const Move& move : possibleMoves) {
                        if (bitScanForward(move.toSquare) == targetPosition) {
                            return true; // The target can be captured by an enemy piece
                        }
                    }
                }
            }
        }
        return false; // If no enemy moves can capture the target, it is not threatened
    }



    int evaluateBoard(bool isWhite) {
        int score = 0;

        // Basic piece values and other constants remain the same

        for (int i = 0; i < 64; i++) {
            uint64_t position = 1ULL << i;
            bool isPieceWhite = (whitePieces & position) != 0;

            int pieceValue = 0; // Initialize to 0 for each square

            if (whitePawns & position || blackPawns & position) {
                pieceValue = pawnValue + pawnPositionalValue[i];
            } else if (whiteKnights & position || blackKnights & position) {
                pieceValue = knightValue + knightPositionalValue[i];
            }
            // Extend this logic for bishops, rooks, queens

            // Adjust the score based on the piece's color relative to the side being evaluated
            score += (isPieceWhite == isWhite ? pieceValue : -pieceValue);
            if (isPieceThreatened(i, isPieceWhite)) {
                score -= (isPieceWhite == isWhite ? pieceValue * 2 : 0); // Double deduction for threatened piece.
            }
        }

        // No need to iterate again; all necessary calculations are done in a single pass

        return score;
    }







    int alphaBeta(int alpha, int beta, int depth, bool isMaximizer){
        if (depth == 0){
            return evaluateBoard(false);
        }
        if (isMaximizer){
            int maxEval = INT_MIN;
            for (int i = 0; i < 64; i++) {
                uint64_t position = 1ULL << i;
                if (blackPieces & position) {
                    auto moves = generateMovesForPiece(position, getPieceTypeOnSquare(i));
                    for (auto& move : moves) {
                        movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
                        int eval = alphaBeta(alpha, beta, depth - 1, false);
                        maxEval = std::max(maxEval, eval);
                        alpha = std::max(alpha, eval);
                        resetPreviousMove();
                        if (alpha >= beta){
                            break;
                        }
                    }
                }
            }
            return maxEval;
        }else{
            int minEval = INT_MAX;
            for (int i = 0; i < 64; i++) {
                uint64_t position = 1ULL << i;
                if (whitePieces & position) {
                    auto moves = generateMovesForPiece(position, getPieceTypeOnSquare(i));
                    for (auto& move : moves) {
                        movePiece(*move.pieceMoved, move.fromSquare, move.toSquare, move.pieceCaptured);
                        int eval = alphaBeta(alpha, beta, depth - 1, true);
                        minEval = std::min(minEval, eval);
                        beta = std::min(beta, eval);
                        resetPreviousMove();
                        if (beta <=  alpha){
                            break;
                        }
                    }
                }
            }
            return minEval;

        }

    }

    static int calculateCapturedPieceScore(PieceType pieceType) {
        switch (pieceType) {
            case Pawn:
                return 1;
            case Knight:
            case Bishop:
                return 3;
            case Rook:
                return 5;
            case Queen:
                return 9;
            default:
                return 0;
        }
    }
};

