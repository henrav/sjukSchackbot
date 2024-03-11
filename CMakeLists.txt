cmake_minimum_required(VERSION 3.25)
project(untitled7)

set(CMAKE_CXX_STANDARD 17)

add_executable(untitled7 main.cpp Run.cpp Run.h generateBoard.cpp generateBoard.h ChessBoard.cpp ChessBoard.h)
find_package (SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
include_directories (${SFML_INCLUDE_DIRS})
target_link_libraries (untitled7 sfml-system sfml-window sfml-graphics sfml-audio sfml-network)