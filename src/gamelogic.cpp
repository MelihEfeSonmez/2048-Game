    #include "gamelogic.h"

    #include <algorithm>

    // ---CONSTRUCTOR---
    GameLogic::GameLogic(int rows, int cols, int target, int possibOfTwo) {
        rows_ = rows;
        cols_ = cols;
        target_ = target;
        possibOfTwo_ = possibOfTwo;
        score_ = 0;

        grid_.assign(rows, std::vector<int>(cols, 0));

        std::random_device rd;
        RNG_.seed(rd());

        restart();

    }

    // give empty cells
    std::vector<std::pair<int, int>> GameLogic::emptyCells() const {

    }

    // put value in a random empty cell
    void GameLogic::placeTile(int value) {

    }

    // place 2 or 4 in a empty cell
    void GameLogic::spawnRandomTile() {

    }

    // slide and merge
    // slide and merge the grid
    bool GameLogic::slideAndMerge(Direction dir, int &gained) {

    }

    // slide and merge a line
    void GameLogic::slideAndMergeLine(std::vector<int> &line, int &gained, bool &moved) {

    }

    // return value at specified cell
    int GameLogic::valueAt(int r, int c) const {

    }

    // apply slide in dir and its effects, and returns true if valid-move
    bool GameLogic::move(Direction dir) {

    }

    // undo check
    bool GameLogic::canUndo() const {

    }

    // undo
    void GameLogic::undo() {

    }

    // restart a new game
    void GameLogic::restart() {

    }

    // game state queries
    // return true if reached the target
    bool GameLogic::hasWon() const {

    }

    // return true if no valid moves
    bool GameLogic::isGameOver() const {

    }

    // return true if one valid move exists
    bool GameLogic::hasAnyMove() const {

    }

    // give directions movable
    std::vector<Direction> GameLogic::validMoves() const {

    }