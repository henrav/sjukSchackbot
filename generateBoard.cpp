//
// Created by Henrik Ravnborg on 2024-03-09.
//
#include "ChessBoard.cpp"




class generateBoard {
    sf::Texture textures[12];
    sf::RenderWindow window;
    sf::Sprite *selectedPiece = nullptr;
    std::vector<int> pickUpPos;
    std::vector<Move> moves;
    ChessBoard *chessBoard;

public:
    generateBoard(ChessBoard* board) : chessBoard(board) {}
    struct ChessPiece {
        sf::Sprite sprite;
        int type;
        bool isWhite;
    };
    std::vector<ChessPiece> pieces;

    void movePiece() {
        if (selectedPiece != nullptr) { // Check if a piece has been selected
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            // Adjust position so the mouse cursor is at the center of the piece
            selectedPiece->setPosition(static_cast<float>(mousePos.x - 50), static_cast<float>(mousePos.y - 50));
        }
    }

    void run(ChessBoard *board) {
        window.create(sf::VideoMode(800, 800), "Chess");
        loadTextures();
        drawPieces(*board);
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
                if (event.type == sf::Event::MouseButtonPressed){
                    if (event.mouseButton.button == sf::Mouse::Left){
                        selectPiece(*board);
                    }
                }
                if (event.type == sf::Event::MouseMoved){
                    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)){
                        movePiece();
                    }
                }
                if (event.type == sf::Event::MouseButtonReleased){
                    placePiece();
                }
            }

            window.clear();
            drawBoard();
            draw();
            window.display();
        }
    }
private:
    void draw(){
        for (const auto & piece : pieces) {
            window.draw(piece.sprite);
        }
    }

    void drawBoard() {
        int tileSize = 100; // Assuming an 800x800 window, this makes each tile 100x100 pixels.
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
                tile.setPosition(x * tileSize, y * tileSize);
                tile.setFillColor((x + y) % 2 == 0 ? sf::Color::White : sf::Color(80, 80, 80));
                window.draw(tile);
            }
        }
    }
    void loadTextures() {
        // Example path - adjust according to your project structure
        std::string basePath = "/Users/henrikravnborg/CLionProjects/untitled7/bilder/";
        std::vector<std::string> fileNames = {
                "VitBonde.png", "VitHäst.png", "VitLöpare.png", "VittTorn.png", "VitDrottning.png", "VitKung.png",
                "SvartBonde.png", "SvartHäst.png", "SvartLöpare.png", "SvartTorn.png", "SvartDrottning.png", "SvartKung.png"
        };

        for (int i = 0; i < 12; ++i) {
            if (!textures[i].loadFromFile(basePath + fileNames[i])) {
                std::cerr << "Failed to load " << fileNames[i] << std::endl;
            }
        }
    }
    void drawPieces(const ChessBoard &board) {
        int tileSize = 100; // Same as in drawBoard method

        // Iterate through each square on the board
        for (int square = 0; square < 64; ++square) {
            int x = square % 8; // Column
            int y = square / 8; // Row
            uint64_t mask = 1ULL << square;
            ChessPiece piece;

            piece.sprite.setPosition(x * 100 + 12, y * 100 + 12); // Adjust position based on your board setup
            piece.sprite.setScale(0.5, 0.5); // Scale if necessary
            piece.sprite.setTexture(textures[getTextureForPiece(mask)]);
            pieces.push_back(piece);

            // Determine which piece is on this square and set the correct texture

        }
    }

    void selectPiece(ChessBoard &board) {
        int tileSize = 100;
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        int file = mousePos.x / tileSize;
        int rank = mousePos.y / tileSize;
        uint64_t mask = 1ULL << (rank * 8 + file);
        //determine the board that the piece is in
        // Reset selectedPiece
        selectedPiece = nullptr;

        for (auto &piece : pieces) {
            // Convert piece's sprite position back to board coordinates to find the selected piece
            sf::Vector2f pos = piece.sprite.getPosition();
            int pieceFile = (pos.x - 12) / tileSize; // Adjust for any offset you've applied
            int pieceRank = (pos.y - 12) / tileSize;

            if (file == pieceFile && rank == pieceRank) {
                selectedPiece = &piece.sprite; // Point selectedPiece to the sprite of the selected piece
                std::cout << "Selected piece at " << file << ", " << rank << std::endl;
                if (board.whitePieces & mask) {
                    ChessBoard::PieceType type;
                    if (board.whitePawns & mask) {
                        type = chessBoard->Pawn;
                        pickUpPos.push_back(file);
                        pickUpPos.push_back(rank);
                    } else if (board.whiteKnights & mask) {
                        type = chessBoard->Knight;
                        pickUpPos.push_back(file);
                        pickUpPos.push_back(rank);
                    } // Add additional conditions for other piece types

                    std::vector<Move> possibleMoves = chessBoard->generateMovesForPiece(mask, type); // 'true' indicates white
                } else if (board.blackPieces & mask) {
                    ChessBoard::PieceType type;
                    if (board.blackPawns & mask) {
                        type = ChessBoard::Pawn;
                        pickUpPos.push_back(file);
                        pickUpPos.push_back(rank);
                    } else if (board.blackKnights & mask) {
                        type = ChessBoard::Knight;
                        pickUpPos.push_back(file);
                        pickUpPos.push_back(rank);
                    } // Add additional conditions for other piece types

                    std::vector<Move> possibleMoves = chessBoard->generateMovesForPiece(mask, type); // 'false' indicates black
                }
                break;
            }
        }
    }

    void placePiece() {
        if (!selectedPiece) return; // No piece selected, nothing to do.

        int tileSize = 100;
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        int file = mousePos.x / tileSize;
        int rank = mousePos.y / tileSize;
        int rankTaken = pickUpPos[1];
        int fileTaken = pickUpPos[0];

        // Call playerMove on the chessBoard with the starting and ending positions.
        bool moveWasSuccessful = chessBoard->playerMove(rankTaken, fileTaken, rank, file);

        if (moveWasSuccessful) {
            // Move was valid and executed, update visual representation accordingly.
            selectedPiece->setPosition(file * tileSize + 12, rank * tileSize + 12);
            // Possibly update any GUI elements that reflect the current state of the game.
            clearSelectedPiece();
        } else {
            // Move was invalid. You could revert the piece to its original position,
            // display an error message, or otherwise indicate the move was not allowed.
            selectedPiece->setPosition(fileTaken * tileSize + 12, rankTaken * tileSize + 12);
            clearSelectedPiece();
        }

        // Clear or reset any temporary state as necessary.
        clearSelectedPiece();

    }


     int getTextureForPiece(uint64_t piece) {
        if (piece & chessBoard->whitePawns) return 0;
        if (piece & chessBoard->whiteKnights) return 1;
        if (piece & chessBoard->whiteBishops) return 2;
        if (piece & chessBoard->whiteRooks) return 3;
        if (piece & chessBoard->whiteQueens) return 4;
        if (piece & chessBoard->whiteKing) return 5;
        if (piece & chessBoard->blackPawns) return 6;
        if (piece & chessBoard->blackKnights) return 7;
        if (piece & chessBoard->blackBishops) return 8;
        if (piece & chessBoard->blackRooks) return 9;
        if (piece & chessBoard->blackQueens) return 10;
        if (piece & chessBoard->blackKing) return 11;
        return -1;
    }
    void clearSelectedPiece() {
        selectedPiece = nullptr;
        pickUpPos.clear();
    }


};