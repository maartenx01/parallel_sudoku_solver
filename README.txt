For this project, I have implmented a 9x9 sudoku solver. The workhouse for this
solver uses a recursive backtracking algorithm. For each empty cell, the algorithm
tries a number and, if the number is valid, recursively calls itself and searches
deeper in a Depth First Search manner. This is implemented in the solver function.
I implemented a parallel solver (main_pool.cpp) and it uses a customized thread pool.
The methods divide up the search space at the first level and launches the solver 
function on the partitioned search space (subtrees) with an available thread in the 
thread pool C++ features demonstrated in this project include classes, threads, tuples, 
mutex, condition variables, namespace, iostream, operator overloading, definition 
guard, and lambda functions. The shortcomings of this project include some raw pointers
and memory management issue. Debugging the thread pool took way longer than expected.
Unfortuantely, I didn't get a chance to put in RAII memory management best practices.

API details:
Usage: exe <some_grid.txt>
$ ./exe some_grid.txt