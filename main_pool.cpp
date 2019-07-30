#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include "board_pool.h"

using mpcs51044::Board;

int main(int argc, char **argv) {

    mpcs51044::Board board;

    mpcs51044::Board * ptr = &board;

    //*ptr = board;

    mpcs51044::ThreadPool pool;

    std::string line;
    std::ifstream file;
    file.open (argv[1]);
    for (int i = 0; i < SIZE; i++) {
        getline(file, line);
        for (int j = 0; j < SIZE; j++) {
            board.grid[i][j] = ((int) line[j] - 48);
        }

    }
    file.close();

    std::cout << "Input Board" << std::endl;
    std::cout << board;


    //board.threadPoolSolve();

    std::vector<Board *> ptr_vec;
    for (int row = 0; row < SIZE; row++) {
        for (int column = 0; column < SIZE; column++) {

            if (ptr->grid[row][column] == EMPTY) {
                // try number 

                for (int number = 1; number <= SIZE; number++) {
                    Board * trial = (Board *) malloc(sizeof(Board)); 
                    ptr_vec.push_back(trial);
                    *trial = *ptr;
                    trial->grid[row][column] = number;
                    //printf("waiting0\n");
                    pool.schedule(trial, ptr); 
                    //printf("waiting\n");
                    
                }
            }
        }
    }
    std::cout << "Solved Board\n";
    pool.wait();

    std::cout << board;


    return 0;

}