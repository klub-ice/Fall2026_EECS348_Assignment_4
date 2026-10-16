/*
    EECS 348 Assignment 4
    Author: Zoey Spies
    KUID: 3136594
    Creation Date: 10/14/2026
    Revision date: 10/15/2026
    Purpose:  Solve Sudoku puzzles using a backtracking algorithm.
    Collaborators: Copilot, Claude, EECS 348 notes and slides, EECS 388 notes and slides, geeksforgeeks.org, stackoverflow.com
*/

#include <bitset> // for counting bits in a bitmask
#include <cctype> // for std::tolower
#include <cstdlib> // for std::strtoull
#include <fstream> // for file I/O
#include <iostream> // for console I/O
#include <sstream> // for std::ostringstream
#include <string> // for std::string
#include <vector> // for std::vector

// ---------------------------------------------------------------------------
// Board: the 9x9 grid plus one bitmask per row, column and 3x3 block.
// ---------------------------------------------------------------------------
class Board { // represents a Sudoku board and tracks the state of the puzzle
public:
    static const int SIZE = 9; // size of the board (9x9)
    static const int BOX = 3; // size of a box (3x3)
    static const unsigned DIGIT_BITS = 0x3FEu;  // bits 1..9 set

    Board() { clear(); } // constructor initializes the board to empty and valid

    void clear() { // clears the board and resets all masks
        for (int r = 0; r < SIZE; ++r) { // for each row
            rowMask_[r] = colMask_[r] = boxMask_[r] = 0; // reset the masks
            for (int c = 0; c < SIZE; ++c) cells_[r][c] = 0; // reset the cells
        }
        valid_ = true; // reset the valid flag
    }

    bool loadFromFile(const std::string& path, std::string& err) { // loads a puzzle from a file and returns true if successful
        std::ifstream in(path.c_str()); // open the file for reading
        if (!in) { // if the file could not be opened
            err = "could not open file"; // set the error message
            return false; // return false to indicate failure
        }
        clear(); // clear the board before loading new data
        std::vector<int> values; // vector to hold the values read from the file
        std::string line; // string to hold each line read from the file
        while (std::getline(in, line)) { // read each line from the file
            if (isSeparatorLine(line)) continue; // skip separator lines
            for (std::size_t i = 0; i < line.size(); ++i) { // for each character in the line
                char ch = line[i]; // get the character
                if (ch >= '1' && ch <= '9') // if the character is a digit between 1 and 9
                    values.push_back(ch - '0'); // convert the character to an integer and add it to the values vector
                else if (ch == '0' || ch == '.' || ch == '_' || ch == '*' || // if the character is a placeholder for an empty cell
                         ch == 'x' || ch == 'X') // or a case-insensitive 'x'
                    values.push_back(0); // add a 0 to the values vector to represent an empty cell
            }
        }
        if (values.size() != static_cast<std::size_t>(SIZE * SIZE)) { // if the number of values read is not equal to 81
            std::ostringstream os; // create a string stream to build the error message
            os << "expected 81 cells but found " << values.size(); // add the number of values found to the error message
            err = os.str(); // set the error message
            return false; // return false to indicate failure
        }
        for (int i = 0; i < SIZE * SIZE; ++i) { // for each cell in the board
            int v = values[i], r = i / SIZE, c = i % SIZE; // calculate the row and column indices
            if (v == 0) continue; // skip empty cells
            if (canPlace(r, c, v)) // if the value can be placed in the cell
                place(r, c, v); // place the value in the cell and update the masks
            else // if the value cannot be placed in the cell
                valid_ = false; // set the valid flag to false to indicate that the board is invalid
        }
        return true; // return true to indicate success
    }

    bool isValid() const { return valid_; } // returns true if the board is valid (no conflicts)
    bool isEmpty(int r, int c) const { return cells_[r][c] == 0; } // returns true if the cell at (r, c) is empty

    bool canPlace(int r, int c, int v) const { // returns true if the value v can be placed at (r, c) without conflicts
        return (usedMask(r, c) & (1u << v)) == 0; // check if the bit for v is not set in the used mask
    }

    void place(int r, int c, int v) { // places the value v at (r, c) and updates the masks
        cells_[r][c] = v; // set the cell value
        unsigned bit = 1u << v; // calculate the bit for v
        rowMask_[r] |= bit; // set the bit for v in the row mask
        colMask_[c] |= bit; // set the bit for v in the column mask
        boxMask_[boxIndex(r, c)] |= bit; // set the bit for v in the box mask
    }

    void remove(int r, int c) { // removes the value at (r, c) and updates the masks
        unsigned bit = 1u << cells_[r][c]; // calculate the bit for the value being removed
        cells_[r][c] = 0; // set the cell to empty
        rowMask_[r] &= ~bit; // clear the bit for the value in the row mask
        colMask_[c] &= ~bit; // clear the bit for the value in the column mask
        boxMask_[boxIndex(r, c)] &= ~bit; // clear the bit for the value in the box mask
    }

    unsigned candidates(int r, int c) const { // returns a bitmask of possible candidates for the cell at (r, c)
        return ~usedMask(r, c) & DIGIT_BITS; // return the complement of the used mask ANDed with the digit bits to get the candidates
    }

    int countBits(unsigned m) const { // counts the number of bits set in the bitmask m
        return static_cast<int>(std::bitset<16>(m).count()); // use std::bitset to count the number of bits set in m
    }

    std::string toString(char blank = '_') const { // returns a string representation of the board, using blank for empty cells
        std::ostringstream os; // create a string stream to build the string representation
        for (int r = 0; r < SIZE; ++r) { // for each row
            for (int c = 0; c < SIZE; ++c) // for each column
                os << (cells_[r][c] ? static_cast<char>('0' + cells_[r][c]) : blank) << ' '; // output the cell value or blank
            os << '\n'; // output a newline after each row
        }
        return os.str(); // return the string representation of the board
    }

private:
    int cells_[SIZE][SIZE]; // 2D array to hold the values of the cells (0 for empty, 1-9 for filled)
    unsigned rowMask_[SIZE], colMask_[SIZE], boxMask_[SIZE]; // bitmasks to track which values are used in each row, column, and box
    bool valid_; // flag to indicate if the board is valid (no conflicts)

    int boxIndex(int r, int c) const { return (r / BOX) * BOX + (c / BOX); } // calculates the index of the 3x3 box for the cell at (r, c)

    unsigned usedMask(int r, int c) const { // returns a bitmask of values already used in the row, column, and box for the cell at (r, c)
        return rowMask_[r] | colMask_[c] | boxMask_[boxIndex(r, c)]; // return the union of the row, column, and box masks to get the used values
    }

    bool isSeparatorLine(const std::string& s) const { // returns true if the line is a separator line (contains only whitespace and divider characters)
        bool sawDivider = false; // flag to indicate if a divider character was seen
        for (std::size_t i = 0; i < s.size(); ++i) { // for each character in the line
            char ch = s[i]; // get the character
            if (ch == '-' || ch == '+' || ch == '=' || ch == '|') // if the character is a divider character
                sawDivider = true; // set the flag to true
            else if (!std::isspace(static_cast<unsigned char>(ch))) // if the character is not whitespace
                return false; // return false to indicate that the line is not a separator line
        }
        return sawDivider; // return true if a divider character was seen, false otherwise
    }
};

// ---------------------------------------------------------------------------
// SolutionListener: interface the solver reports each solution through.
// ---------------------------------------------------------------------------
class SolutionListener { // interface for receiving notifications of solutions found by the solver
public:
    virtual ~SolutionListener() {} // virtual destructor to allow proper cleanup of derived classes
    virtual void onSolution(const Board& board, unsigned long long number) = 0; // pure virtual function to be implemented by derived classes to handle a solution found by the solver
};

// ---------------------------------------------------------------------------
// Solver: recursive backtracking over the whole search tree.
// ---------------------------------------------------------------------------
class Solver { // implements a backtracking algorithm to find all solutions to a Sudoku puzzle
public:
    Solver(Board& board, unsigned long long limit = 0) // constructor initializes the solver with a reference to the board and an optional limit on the number of solutions to find
        : board_(board), limit_(limit), count_(0), limitHit_(false), listener_(NULL) {}

    unsigned long long solveAll(SolutionListener* listener) { // finds all valid solutions and reports each one to the listener
        count_ = 0; // reset the count before solving
        limitHit_ = false; // reset the limit flag before solving
        listener_ = listener; // store the listener for callback reporting
        if (board_.isValid()) search(); // begin the recursive search only if the board is currently valid
        return count_; // return the total number of solutions found
    }

    bool hitLimit() const { return limitHit_; } // returns true if the solver stopped because the user-specified limit was hit

private:
    static const int NO_CELL = -1; // sentinel value meaning no empty cell was found
    static const int MORE_THAN_ANY_CELL = Board::SIZE + 1; // starting value that is larger than any real candidate count

    Board& board_; // reference to the puzzle board being solved
    unsigned long long limit_, count_; // limit on solutions and count of solutions found so far
    bool limitHit_; // flag set when the search hits the solver limit
    SolutionListener* listener_; // callback object that receives each solved board

    void search() { // recursively chooses the next empty cell and tries each valid digit
        int bestR = NO_CELL, bestC = NO_CELL, bestCount = MORE_THAN_ANY_CELL; // best empty cell found so far and its candidate count
        unsigned bestMask = 0; // bitmask of valid digits for the best cell

        for (int r = 0; r < Board::SIZE; ++r) { // scan each row
            for (int c = 0; c < Board::SIZE; ++c) { // scan each column
                if (!board_.isEmpty(r, c)) continue; // skip filled cells
                unsigned m = board_.candidates(r, c); // get the candidate bitmask for this empty cell
                int n = board_.countBits(m); // count how many values could legally go here
                if (n < bestCount) { // keep the cell with the fewest candidates to prune the search faster
                    bestCount = n; // save the candidate count
                    bestR = r; // save the row index
                    bestC = c; // save the column index
                    bestMask = m; // save the valid candidate mask
                    if (n == 0) return; // no valid moves exist, so backtrack immediately
                }
            }
        }

        if (bestR == NO_CELL) { // no empty cells remain, so the board is a complete solution
            ++count_; // increment the total number of solutions found
            if (listener_) listener_->onSolution(board_, count_); // report the solution to the listener
            if (limit_ != 0 && count_ >= limit_) limitHit_ = true; // stop searching if the user limit has been reached
            return; // return to the previous recursive level
        }

        for (int v = 1; v <= Board::SIZE; ++v) { // try each digit from 1 to 9
            if (!(bestMask & (1u << v))) continue; // skip values that are not legal for this cell
            board_.place(bestR, bestC, v); // place the candidate on the board
            search(); // recurse deeper to continue solving
            board_.remove(bestR, bestC); // undo the placement before trying the next candidate
            if (limitHit_) return; // stop recursing once the limit has been reached
        }
    }
};

// ---------------------------------------------------------------------------
// SolutionPrinter: sends each solution to the console and the solution file.
// ---------------------------------------------------------------------------
class SolutionPrinter : public SolutionListener {
public:
    SolutionPrinter(std::ostream& console, std::ostream* file)
        : console_(console), file_(file) {}

    virtual void onSolution(const Board& board, unsigned long long number) {
        if (number == 1) console_ << "Solution:\n"; // print a header only before the first solution
        console_ << board.toString() << '\n'; // print the solved board
        if (file_) *file_ << board.toString() << '\n'; // also write the board to the output file
    }

private:
    std::ostream& console_; // stream used for text output to the user
    std::ostream* file_; // optional stream used to write to the solution file
};

// ---------------------------------------------------------------------------
// PuzzleRunner: handles one puzzle file from load to report.
// ---------------------------------------------------------------------------
class PuzzleRunner {
public:
    explicit PuzzleRunner(unsigned long long limit) : limit_(limit) {} // stores the optional per-puzzle solution limit

    void process(const std::string& path) const { // loads one puzzle file, solves it, and prints the result
        std::cout << "==================================================\n"; // print a header separator
        std::cout << "Puzzle file: " << path << "\n\n"; // show which puzzle is being processed

        Board board; // create a board object for the current puzzle
        std::string err; // holds any error returned by the board loader
        if (!board.loadFromFile(path, err)) { // try to read the puzzle from disk
            std::cout << "Error: " << err << "\n\n"; // print the file-loading error
            return; // stop processing this puzzle
        }

        std::cout << "Puzzle:\n" << board.toString() << '\n'; // print the loaded puzzle before solving

        std::string outName = solutionFileName(path); // determine where the solution output should be written
        std::ofstream out(outName.c_str()); // open the solution file for writing
        if (!out) std::cout << "Warning: could not create " << outName << "\n\n"; // warn if the file could not be opened

        SolutionPrinter printer(std::cout, out ? &out : NULL); // create the listener that prints every solution
        Solver solver(board, limit_); // create the solver with the puzzle limit
        unsigned long long count = solver.solveAll(&printer); // solve the puzzle and count how many solutions were found

        if (count == 0) { // no valid solution exists for this puzzle
            std::cout << "No solution found\n\n"; // tell the user nothing was found
            if (out) out << "No solution found\n"; // record that result in the output file
        } else { // at least one valid solution was found
            std::ostringstream summary; // build the result summary string
            summary << "Solutions found: " << count
                    << (solver.hitLimit() ? " (stopped at --limit)" : ""); // include the limit note only when the cap was reached
            std::cout << '\n' << summary.str() << "\n\n"; // print the summary to the console
            if (out) out << '\n' << summary.str() << '\n'; // write the summary to the output file
        }
        if (out) std::cout << "Output written to " << outName << "\n\n"; // tell the user where the output file was written
    }

private:
    unsigned long long limit_; // maximum number of solutions to search for before stopping

    std::string solutionFileName(const std::string& puzzlePath) const { // builds a matching solution filename from the original puzzle path
        std::size_t slash = puzzlePath.find_last_of("/\\"); // find the final directory separator, if any
        std::string dir = (slash == std::string::npos) ? "" : puzzlePath.substr(0, slash + 1); // keep the directory part if present
        std::string base = (slash == std::string::npos) ? puzzlePath : puzzlePath.substr(slash + 1); // keep just the filename

        std::string lower = base; // copy the base name for case-insensitive matching
        for (std::size_t i = 0; i < lower.size(); ++i)
            lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i]))); // normalize to lowercase

        std::size_t pos = lower.find("puzzle"); // look for the word "puzzle" in the filename
        if (pos != std::string::npos)
            base.replace(pos, 6, "solution"); // replace the puzzle part with solution
        else
            base = "solution_" + base; // otherwise create a new solution_ prefix name
        return dir + base; // return the fully qualified output path
    }
};

// ---------------------------------------------------------------------------
// SudokuApp: reads the command line and runs every puzzle file given.
// ---------------------------------------------------------------------------
class SudokuApp {
public:
    int run(int argc, char* argv[]) { // parses the command line and processes each puzzle file supplied
        std::vector<std::string> files; // stores the puzzle paths provided by the user
        unsigned long long limit = 0;  // 0 = no limit

        for (int i = 1; i < argc; ++i) { // skip the program name and inspect each argument
            std::string a = argv[i]; // store the current argument for easier checking
            if (a == "--limit" && i + 1 < argc)
                limit = std::strtoull(argv[++i], NULL, 10); // parse the user limit from the next argument
            else
                files.push_back(a); // treat every other argument as a puzzle file path
        }

        if (files.empty()) { // no files were provided on the command line
            std::string name; // holds the user-entered filename
            std::cout << "Enter puzzle file name: "; // prompt the user for a puzzle path
            if (!std::getline(std::cin, name) || name.empty()) { // stop if the input is missing or blank
                std::cerr << "No puzzle file given.\n"; // report the missing input
                return 1; // return an error code to the shell
            }
            files.push_back(name); // add the user-entered filename to the list
        }

        PuzzleRunner runner(limit); // create a runner configured with the command-line limit
        for (std::size_t i = 0; i < files.size(); ++i) runner.process(files[i]); // process each puzzle in turn
        return 0; // return success to the operating system
    }
};

int main(int argc, char* argv[]) {
    SudokuApp app;
    return app.run(argc, argv);
}