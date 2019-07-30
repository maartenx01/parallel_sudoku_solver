#ifndef BOARD_H
#define BOARD_H


#include <iostream>
#include <thread>
#include <future>
#include <tuple>
#include <vector>
#include <queue>
#include <future>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <functional>
#include <utility>
#include <memory>

using std::queue;
using std::packaged_task;
using std::mutex;
using std::condition_variable;
using std::thread;
using std::max;
using std::vector;
using std::unique_lock;
using std::lock_guard;
using std::move;
using std::ref;
using std::make_shared;

//#include "my_thread_pool.h"


#define SIZE 9
#define EMPTY 0



namespace mpcs51044 {


class Board {
private:
    bool rowConstraint();
    bool columnConstraint();
    bool blockConstraint();
public:

    int grid[SIZE][SIZE];
    Board() {
        for (int i = 0; i < SIZE; i++) {
		    for (int j = 0; j < SIZE; j++) {
			    this->grid[i][j] = 0;
		    }
	    }  
    };
    bool boardSolution();
    bool asyncSolve();
    bool threadPoolSolve();
    friend std::ostream & operator<< (std::ostream &os, Board b);
    Board(const Board &b) { 
        for (int i = 0; i < SIZE; i++) {
		    for (int j = 0; j < SIZE; j++) {
			    this->grid[i][j] = b.grid[i][j];
		    }
	    }    
    }
    Board& operator= (const Board& b) {
        for (int i = 0; i < SIZE; i++) {
		    for (int j = 0; j < SIZE; j++) {
			    this->grid[i][j] = b.grid[i][j];
		    }
	    } 
        return *this;
    }
};


std::tuple<bool, Board> solve(Board &board);
bool boardValid(Board board, int row, int column, int number);
bool rowValid(Board board, int row, int number);
bool columnValid(Board board, int column, int number);
bool blockValid(Board board, int row, int column, int number);

bool boardValid(Board board,int row, int column, int number) {
    return rowValid(board, row, number) && columnValid(board, column, number) && blockValid(board, row, column, number);
}

std::tuple<bool, Board> solve(Board &board) {
    for (int row = 0; row < SIZE; row++) {
        for (int column = 0; column < SIZE; column++) {

            if (board.grid[row][column] == EMPTY) {
                // try number    
                for (int number = 1; number <= SIZE; number++) {

                    if (boardValid(board, row, column, number)) {

                        board.grid[row][column] = number;

                        // go deeper                        
                        if (std::get<0>(solve(board))) {
                            return {true, board};
                        } else {
                            board.grid[row][column] = EMPTY;
                        }
                    }
                }
                return {false, board};
            }
        }
    }
    return {true, board};
}



bool rowValid(Board board, int row, int number) {
    for (int i = 0; i < SIZE; i++) 
        if (board.grid[row][i] == number)
            return false;
    return true;
}

bool columnValid(Board board, int column, int number) {
    for (int i = 0; i < SIZE; i++) 
        if (board.grid[i][column] == number)
            return false;
    return true;
}

bool blockValid(Board board, int row, int column, int number) {
    int r = row/3;
    int c = column/3;
    for (int i = (3*r); i < ((3*r)+3); i++)
        for (int j = (3*c); j < ((3*c)+3); j++)
            if (board.grid[i][j] == number)
                return false;
    return true;
}

void indirection(Board *brd, Board *mstr) {
    //printf("HEre");
    std::tuple<bool,mpcs51044::Board> tup;

    tup = solve(*brd);
    if (std::get<0>(tup))
        *mstr = std::get<1>(tup);

}


class ThreadPool {
private:

    bool finished = false;
    queue<std::tuple<Board *,
                     Board *> > q;
    condition_variable cv;
    mutex qlock;
    mutex wlock;
    condition_variable cvw;
    int work_pending = 0;
    // ~ThreadPool() {
    //     wait();
    //     finished = true;
    //     cv.notify_all();
    //     for (auto &tid : vec) {
    //         tid.join();
    //     }
    // }


public:
    std::vector<std::thread> vec;
    ThreadPool() {
        int num_threads = std::thread::hardware_concurrency();
        //printf("constructor");
        //std::vector<std::future<std::tuple<bool,Board> > > handle(SIZE);
        
        for (int i = 0; i < num_threads; i++) {
            vec.push_back(std::thread([this](){run();}));
        }
    }
    ~ThreadPool() {
        wait();
        finished = true;
        cv.notify_all();
        for (auto &tid : vec) {
            tid.join();
        }
    }
/*
    void schedule(std::function<void(std::function<std::tuple<bool,Board>(Board &)>, Board*, Board*)> indirection,
                  std::function<std::tuple<bool,Board>(Board &)> slv,
                  Board *brd,
                  Board *mstr) {
        {
            unique_lock<mutex> ulock(qlock);
            q.push({indirection,slv,brd,mstr});
            cv.notify_one();
        }
    }
    */
    void schedule(
                  Board *brd,
                  Board *mstr) {
        {
            //printf("schedule\n");
            unique_lock<mutex> ulock(qlock);
            q.push({brd,mstr});
            {
                unique_lock<mutex> wl(wlock);
                work_pending++;
            }
            cv.notify_one();
        }
    }

    void wait(){
        unique_lock<mutex> ql(qlock);
        cvw.wait(ql, [&](){return !work_pending;});
    }
    void run() {
        //printf("Im thread: \n");
        Board *brd;
        Board *mstr;
        std::tuple<
                    Board *,
                    Board *> tup;
        while(true) {
            {
                unique_lock<mutex> ulock(qlock);
                //printf("waiting WORK\n;");
                cv.wait(ulock, [&](){return (!q.empty() || finished);});
                if (finished) 
                    break;
                //printf("GETTING WORK\n;");
           
                tup = q.front();
                q.pop();
                //indirection = std::get<0>(tup);
                //slv = std::get<1>(tup);
                brd = std::get<0>(tup);
                mstr = std::get<1>(tup);
                
            }
            indirection(brd,mstr);
            {
                
                unique_lock<mutex> wl(wlock);
                if(!--work_pending) {
                    cvw.notify_one();
                }

            }
        }
    }

    void done() {
        {
            unique_lock<mutex> ulock(qlock);
            finished = true;
        }
        cv.notify_all();
    }

};

/*
bool Board::threadPoolSolve() {
    //printf("board\n");

    mpcs51044::ThreadPool pool;

    //printf("post construction\n");
    std::vector<std::future<std::tuple<bool,Board> > > handle(SIZE);
    std::vector<Board *> ptr_vec;
    for (int row = 0; row < SIZE; row++) {
        for (int column = 0; column < SIZE; column++) {

            if (this->grid[row][column] == EMPTY) {
                // try number 

                for (int number = 1; number <= SIZE; number++) {
                    Board * trial = (Board *) malloc(sizeof(Board)); 
                    ptr_vec.push_back(trial);
                    *trial = *this;
                    trial->grid[row][column] = number;
                    //printf("waiting0\n");
                    pool.schedule(trial, this); 
                    //printf("waiting\n");
                    
                }
                //for (int i = 0; i < ptr_vec.size(); i++) {
                //    free(ptr_vec[i]);
                //}
            }
        }
    }
    std::cout << "Solved Board\n";
    std::cout << *this;
    pool.wait();
    if (this->boardSolution())
        return true;
    else
        return false;
    
    
        //return true;
}
*/
/*
bool Board::asyncSolve() {
    std::vector<std::future<std::tuple<bool,Board> > > handle(SIZE);
    for (int row = 0; row < SIZE; row++) {
        for (int column = 0; column < SIZE; column++) {

            if (this->grid[row][column] == EMPTY) {
                // try number 

                for (int number = 1; number <= SIZE; number++) {
                    Board trial = *this;
                    trial.grid[row][column] = number;
                    handle[number-1] = std::async(solve, trial); 
                }
                for (int number = 1; number <= SIZE; number++) {
                    auto t = handle[number-1].get();
                    if (std::get<0>(t) == true) {
                        Board b = std::get<1>(t);
                        for (int i = 0; i < SIZE; i++) {
		                    for (int j = 0; j < SIZE; j++) {
			                    this->grid[i][j] = b.grid[i][j];
		                    }
	                    }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
*/


inline std::ostream & operator<< (std::ostream &os, Board b) {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			os << b.grid[i][j];
		}
		os << std::endl;
	}
    os << std::endl;

	return os;
}

bool Board::boardSolution() {
    return rowConstraint() && columnConstraint() && blockConstraint();
}

bool Board::rowConstraint() {
    bool row[SIZE] = {false};
    for (int i = 0; i < SIZE; i++) {
        bool list[SIZE] = {false};
        for (int j = 0; j < SIZE; j++) {
            int val = this->grid[i][j];
            if (val == 1 || val == 2 || val == 3 || val == 4 || val == 5 ||
                            val == 6 || val == 7 || val == 8 || val == 9) {
                if (list[val-1] == true) {
                    return false;
                } else {
                    list[val-1] = true;
                }
            } 
        }
        row[i] = list[0] && list[1] && list[2] && list[3] && list[4] &&
                            list[5] && list[6] && list[7] && list[8];
    }
    return row[0] && row[1] && row[2] && row[3] && row[4] &&
                     row[5] && row[6] && row[7] && row[8];
}

bool Board::columnConstraint() {
    bool column[SIZE] = {false};
    for (int j = 0; j < SIZE; j++) {
        bool list[SIZE] = {false};
        for (int i = 0; i < SIZE; i++) {
            int val = this->grid[i][j];
            if (val == 1 || val == 2 || val == 3 || val == 4 || val == 5 ||
                            val == 6 || val == 7 || val == 8 || val == 9) {
                if (list[val-1] == true) {
                    return false;
                } else {
                    list[val-1] = true;
                }
            } 
        }
        column[j] = list[0] && list[1] && list[2] && list[3] && list[4] &&
                               list[5] && list[6] && list[7] && list[8];
    }
    return column[0] && column[1] && column[2] && column[3] && column[4] &&
                        column[5] && column[6] && column[7] && column[8];
}

bool Board::blockConstraint() {
    bool block[SIZE] = {false};
    for (int g = 0; g < 3; g++) {
        for (int h = 0; h < 3; h++) {
            bool list[SIZE] = {false};
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int val = this->grid[(g*3)+i][(h*3)+j];
                    if (val == 1 || val == 2 || val == 3 || val == 4 || val == 5 ||
                                    val == 6 || val == 7 || val == 8 || val == 9) {
                        if (list[val-1] == true) {
                            return false;
                        } else {
                            list[val-1] = true;
                        }
                    }
                    block[g*3+h] = list[0] && list[1] && list[2] && list[3] && list[4] &&
                                              list[5] && list[6] && list[7] && list[8];
                }
            }     
        }
    }
    return block[0] && block[1] && block[2] && block[3] && block[4] &&
                       block[5] && block[6] && block[7] && block[8];
}




}
#endif