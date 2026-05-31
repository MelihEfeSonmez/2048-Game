#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <vector>
#include <utility>
#include <random>

enum class Direction {Up, Down, Left, Right};

// game logic class 
class GameLogic {

    private:
        // ---DATA FIELDS---
        int rows_, cols_, target_, possibOfTwo_;
        int score_;

        // grid representation
        std::vector<std::vector<int>> grid_;

        // history of grid, score snapshot for undo
        std::vector<std::pair<std::vector<std::vector<int>>, int>> history_;

        // random source
        std::mt19937 RNG_;

        // ---FUNCTIONS---
        std::vector<std::pair<int, int>> emptyCells() const; // give empty cells
        void placeTile(int value); // put value in a random empty cell
        void spawnRandomTile(); // place 2 or 4 in a empty cell

        // slide and merge
        bool slideAndMerge(Direction dir, int &gained); // slide and merge the grid
        static void slideAndMergeLine(std::vector<int> &line, int &gained, bool &moved); // slide and merge a line

    public:
        // ---CONSTRUCTOR---
        GameLogic(int rows, int cols, int target, int possibOfTwo);

        // ---GETTERS---
        int getRows() const {return rows_;}
        int getCols() const {return cols_;}
        int getTarget() const {return target_;}
        int getScore() const {return score_;}

        // ---FUNCTIONS---
        // return value at specified cell
        int valueAt(int r, int c) const;

        // apply slide in dir and its effects, and returns true if valid-move
        bool move(Direction dir);

        // undo check and undo
        bool canUndo() const;
        void undo();

        // restart a new game
        void restart();

        // game state queries
        bool hasWon() const; // return true if reached the target
        bool isGameOver() const; // return true if no valid moves
        std::vector<Direction> validMoves() const;// give directions movable
};

#endif
