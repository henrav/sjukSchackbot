

#include <cstdint>
#include <iostream>
#include "generateBoard.cpp"


int main() {
    ChessBoard board;
    generateBoard bräda(&board);
    board.printBoard(); // Print initial board setup
    bräda.run(&board);

    return 0;
}

