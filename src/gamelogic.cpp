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
    std::vector<std::pair<int, int>> emptyCells;
    for (int r = 0; r < rows_; ++r)
        for (int c = 0; c < cols_; ++c)
            if (grid_[r][c] == 0) {
                emptyCells.emplace_back(r, c);
            }    
    return emptyCells;
}

// put value in a random empty cell
void GameLogic::placeTile(int value) {
    auto emptyCellList = emptyCells();
    if (emptyCellList.empty()) {return;}

    std::uniform_int_distribution<std::size_t> dist(0, emptyCellList.size() - 1);
    auto [r, c] = emptyCellList[dist(RNG_)];
    grid_[r][c] = value;
}

// place 2 or 4 in a empty cell
void GameLogic::spawnRandomTile() {
    std::uniform_int_distribution<int> chance(1, 100);
    
    if (chance(RNG_) <= possibOfTwo_) {placeTile(2);}
    else {placeTile(4);}
}

// slide and merge
// slide and merge a line
void GameLogic::slideAndMergeLine(std::vector<int> &line, int &gained, bool &moved) {
    const std::vector<int> before = line; // line before sliding
    gained = 0;

    // remove tiles which are 0
    std::vector<int> compact;
    compact.reserve(line.size());
    for (int t : line) {
        if (t != 0) {
            compact.push_back(t);
        }
    }    

    // merge equal tiles (front first)
    std::vector<int> merged;
    merged.reserve(compact.size());
    for (std::size_t i = 0; i < compact.size(); ++i) {
        if (i + 1 < compact.size() && compact[i] == compact[i + 1]) {
            int sum = compact[i] * 2;
            merged.push_back(sum);
            gained += sum;
            ++i;
        } else {
            merged.push_back(compact[i]);
        }
    }

    // pad with zeros
    merged.resize(line.size(), 0);

    moved = (merged != before);
    line.swap(merged);
}

// slide and merge the grid
bool GameLogic::slideAndMerge(Direction dir, int &gained) {
    bool movedAny = false;
    gained = 0;

    // if there is a sliding in a row
    if (dir == Direction::Left || dir == Direction::Right) {
        for (int r = 0; r < rows_; ++r) {
            
            // reassign the horizontal order according to the sliding
            std::vector<int> line(cols_);
            for (int c = 0; c < cols_; ++c) {

                if (dir == Direction::Left) {
                    int idx =  c;
                    line[c] = grid_[r][idx];
                } else {
                    int idx = cols_ - 1 - c;
                    line[c] = grid_[r][idx];
                }    
            }

            int gain = 0;
            bool moved = false;
            slideAndMergeLine(line, gain, moved);
            gained += gain;
            movedAny = movedAny || moved;

            // write the new line to the grid
            for (int c = 0; c < cols_; ++c) {
                if (dir == Direction::Left) {
                    int idx = c;
                    grid_[r][idx] = line[c];    
                } else {
                    int idx = cols_ - 1 - c;
                    grid_[r][idx] = line[c];
                }
            }
        }

    // if there is a sliding in a column    
    } else {
        for (int c = 0; c < cols_; ++c) {

            // reassign the vertical order according to the sliding
            std::vector<int> line(rows_);
            for (int r = 0; r < rows_; ++r) {
                int idx = (dir == Direction::Up) ? r : (rows_ - 1 - r);
                line[r] = grid_[idx][c];
            }

            int g = 0;
            bool moved = false;
            slideAndMergeLine(line, g, moved);
            gained += g;
            movedAny = movedAny || moved;

            for (int r = 0; r < rows_; ++r) {
                if (dir == Direction::Up) {
                    int idx = r;
                    grid_[idx][c] = line[r];                            
                } else {
                    int idx = rows_ - 1 - r;
                    grid_[idx][c] = line[r];    
                }
            }
            
        }
    }

    return movedAny;
}

// return value at specified cell
int GameLogic::valueAt(int r, int c) const {
    return grid_[r][c];
}

// apply slide in dir and its effects, and returns true if valid-move
bool GameLogic::move(Direction dir) {
    // for undo
    auto snapshotOfGrid = grid_;
    int snapshotOfScore = score_;

    int gained = 0;
    bool moved = slideAndMerge(dir, gained);

    // invalid move
    if (!moved) {
        grid_.swap(snapshotOfGrid);
        return false;
    }

    history_.push_back({std::move(snapshotOfGrid), snapshotOfScore});
    score_ += gained;

    spawnRandomTile();

    return true;
}

// undo check (look into history)
bool GameLogic::canUndo() const {
    return !history_.empty();
}

// undo
void GameLogic::undo() {
    if (history_.empty()) {return;}

    auto &last = history_.back();
    grid_ = last.first;
    score_ = last.second;

    history_.pop_back();
}

// restart a new game
void GameLogic::restart() {
    // assign tiles 0
    for (auto &row : grid_) {
        std::fill(row.begin(), row.end(), 0);
    }

    score_ = 0;
    history_.clear();

    // two 2 each in random places
    placeTile(2);
    placeTile(2);
}

// game state queries
// return true if reached the target
bool GameLogic::hasWon() const {
    for (auto &row : grid_)
        for (int t : row)
            if (t >= target_) {return true;}
    return false;
}

// return true if no valid moves
bool GameLogic::isGameOver() const {
    return validMoves().empty();
}

// give directions movable
std::vector<Direction> GameLogic::validMoves() const {
    std::vector<Direction> validMovesList;

    // try every direction on a mock game
    for (Direction d : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        GameLogic gameOfTry = *this;
        int gained = 0;
        if (gameOfTry.slideAndMerge(d, gained)){
            validMovesList.push_back(d);
        }    
    }

    return validMovesList;
}
