#include "mainwindow.h"

#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTimer>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QRandomGenerator>

namespace {
// game config: grid size, target tile and spawn chances live here
// so they can be changed in one place (N, M, K, P, Q).

// grid size: N rows x M cols (default 4 x 4)
constexpr int N = 4;
constexpr int M = 4;

// target / winning tile (default 2048)
constexpr int K = 2048;

// spawn chances in percent: P for a 2, Q for a 4 (must sum to 100)
constexpr int P = 90;
constexpr int Q = 10;

static_assert(P + Q == 100, "P and Q must sum to 100.");
static_assert(N >= 1 && M >= 1, "Grid must have at least one cell.");

// board look
constexpr int kCellSize    = 90;   // tile size in pixels
constexpr int kCellSpacing = 10;   // gap between tiles

// mode button style (idle / selected / disabled)
const char *kModeStyle =
    "QPushButton { background-color:#bbada0; color:#f9f6f2; border:none;"
    " border-radius:4px; padding:6px 14px; font-weight:bold; }"
    "QPushButton:checked { background-color:#8f7a66; }"
    "QPushButton:disabled { background-color:#cdc1b4; color:#f9f6f2; }";

// highlighted mode button: same strong tone as Undo/Restart, used for the
// Unlimited button after a win so it does not look greyed out
const char *kModeHighlightStyle =
    "QPushButton { background-color:#8f7a66; color:#f9f6f2; border:none;"
    " border-radius:4px; padding:6px 14px; font-weight:bold; }";

// undo / restart button style
const char *kActionStyle =
    "QPushButton { background-color:#8f7a66; color:#f9f6f2; border:none;"
    " border-radius:4px; padding:8px 16px; font-weight:bold; }"
    "QPushButton:disabled { background-color:#cdc1b4; color:#f9f6f2; }";
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      board_(N, M, K, P)
{
    buildUi();

    // window keeps keyboard focus so arrow/WASD keys come here,
    // not to a focused button
    setFocusPolicy(Qt::StrongFocus);

    startGame();
}


// UI construction

void MainWindow::buildUi()
{
    setStyleSheet("MainWindow { background-color:#faf8ef; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(14);

    // header: title + score/best boxes
    auto *header = new QHBoxLayout();

    auto *titleBox = new QVBoxLayout();
    auto *title = new QLabel("2048", this);
    title->setStyleSheet("font-size:48px; font-weight:bold; color:#776e65;");
    auto *subtitle = new QLabel(
        QString("Join the tiles, get to %1!").arg(K), this);
    subtitle->setStyleSheet("font-size:13px; color:#776e65;");
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    header->addLayout(titleBox);
    header->addStretch();

    // helper to build one score box (caption above a value)
    auto makeScoreBox = [this](const QString &caption, QLabel *&valueOut) {
        auto *box = new QFrame(this);
        box->setStyleSheet("QFrame { background-color:#bbada0; border-radius:6px; }");
        box->setMinimumWidth(80);
        auto *v = new QVBoxLayout(box);
        v->setContentsMargins(14, 6, 14, 6);
        v->setSpacing(0);
        auto *cap = new QLabel(caption, box);
        cap->setAlignment(Qt::AlignCenter);
        cap->setStyleSheet("color:#eee4da; font-size:11px; font-weight:bold;");
        valueOut = new QLabel("0", box);
        valueOut->setAlignment(Qt::AlignCenter);
        valueOut->setStyleSheet("color:white; font-size:20px; font-weight:bold;");
        v->addWidget(cap);
        v->addWidget(valueOut);
        return box;
    };

    header->addWidget(makeScoreBox("SCORE", scoreValue_));
    header->addSpacing(8);
    header->addWidget(makeScoreBox("BEST", bestValue_));
    root->addLayout(header);

    // mode buttons (Normal / Unlimited / Hard)
    auto *modeRow = new QHBoxLayout();
    modeGroup_ = new QButtonGroup(this);
    modeGroup_->setExclusive(true);

    const char *modeNames[] = {"Normal", "Unlimited", "Hard"};
    for (int i = 0; i < 3; ++i) {
        auto *btn = new QPushButton(modeNames[i], this);
        btn->setCheckable(true);
        btn->setFocusPolicy(Qt::NoFocus); // let key events reach the window
        btn->setStyleSheet(kModeStyle);
        modeGroup_->addButton(btn, i);
        modeRow->addWidget(btn);
    }
    modeGroup_->button(0)->setChecked(true);
    modeRow->addStretch();
    root->addLayout(modeRow);
    connect(modeGroup_, &QButtonGroup::idClicked, this, &MainWindow::onModeChanged);

    // undo / restart buttons
    auto *actionRow = new QHBoxLayout();

    undoButton_ = new QPushButton("\u21B6 Undo (U)", this);
    restartButton_ = new QPushButton("\u21BB Restart (R)", this);
    for (QPushButton *b : {undoButton_, restartButton_}) {
        b->setFocusPolicy(Qt::NoFocus);
        b->setStyleSheet(kActionStyle);
        actionRow->addWidget(b);
    }
    actionRow->addStretch();
    root->addLayout(actionRow);
    connect(undoButton_, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(restartButton_, &QPushButton::clicked, this, &MainWindow::onRestart);

    // hard-mode countdown label (only visible in Hard mode)
    hardLabel_ = new QLabel("", this);
    hardLabel_->setStyleSheet("color:#776e65; font-size:13px; font-weight:bold;");
    root->addWidget(hardLabel_);
    hardLabel_->hide();

    // board grid
    boardFrame_ = new QFrame(this);
    boardFrame_->setStyleSheet("QFrame { background-color:#bbada0; border-radius:8px; }");
    auto *grid = new QGridLayout(boardFrame_);
    grid->setContentsMargins(kCellSpacing, kCellSpacing, kCellSpacing, kCellSpacing);
    grid->setSpacing(kCellSpacing);

    const int rows = board_.getRows();
    const int cols = board_.getCols();
    tiles_.assign(static_cast<std::size_t>(rows * cols), nullptr);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            auto *cell = new QLabel(boardFrame_);
            cell->setFixedSize(kCellSize, kCellSize);
            cell->setAlignment(Qt::AlignCenter);
            cell->setStyleSheet(tileStyle(0));
            grid->addWidget(cell, r, c);
            tiles_[static_cast<std::size_t>(r * cols + c)] = cell;
        }
    }
    root->addWidget(boardFrame_, 0, Qt::AlignCenter);

    // win / game-over overlay (child of the board, hidden at start)
    overlay_ = new QFrame(boardFrame_);
    overlay_->setStyleSheet(
        "QFrame { background-color:rgba(238,228,218,0.85); border-radius:8px; }");
    auto *ov = new QVBoxLayout(overlay_);
    ov->setAlignment(Qt::AlignCenter);
    overlayLabel_ = new QLabel("", overlay_);
    overlayLabel_->setAlignment(Qt::AlignCenter);
    overlayLabel_->setStyleSheet("font-size:40px; font-weight:bold; color:#776e65;");
    overlayButton_ = new QPushButton("Try Again", overlay_);
    overlayButton_->setFocusPolicy(Qt::NoFocus);
    overlayButton_->setStyleSheet(kActionStyle);
    ov->addWidget(overlayLabel_);
    ov->addSpacing(12);
    ov->addWidget(overlayButton_, 0, Qt::AlignCenter);
    overlay_->hide();
    // one slot handles the button; it checks overlayContinue_ to decide
    // between "keep playing" and "restart"
    connect(overlayButton_, &QPushButton::clicked, this, &MainWindow::onOverlayButton);

    // hard-mode timer
    hardTimer_ = new QTimer(this);
    hardTimer_->setSingleShot(true);
    hardTimer_->setInterval(5000); // 5 seconds (Hard mode)
    connect(hardTimer_, &QTimer::timeout, this, &MainWindow::onHardTimeout);

    // separate timer just to refresh the seconds label; does not affect play
    countdownTimer_ = new QTimer(this);
    countdownTimer_->setInterval(100); // refresh 10x per second
    connect(countdownTimer_, &QTimer::timeout, this, &MainWindow::updateHardCountdown);
}


// Gameplay flow

void MainWindow::startGame()
{
    board_.restart();
    inputLocked_     = false;
    winAcknowledged_ = false;
    overlayContinue_ = false;
    setModeButtonsEnabled(true);   // a fresh game re-enables mode switching
    hideOverlay();
    updateView();
    restartHardTimerIfNeeded();
    setFocus(); // grab keyboard focus back after a button click
}

void MainWindow::applyMove(Direction dir)
{
    if (inputLocked_)
        return;

    // move() returns false on an invalid move: nothing changes,
    // no new tile, no timer reset
    if (!board_.move(dir))
        return;

    updateView();
    evaluateEndStates();
    restartHardTimerIfNeeded();
}

void MainWindow::evaluateEndStates()
{
    // win: a tile reached the target. shown in every mode, but only once.
    if (!winAcknowledged_ && board_.hasWon()) {
        winAcknowledged_ = true;
        inputLocked_     = true;   // pause input while the message is up
        stopHardTimer();
        lockModesToUnlimited();    // once won, only Unlimited stays available
        if (mode_ == GameMode::Unlimited) {
            // unlimited: closing the message keeps the same board
            overlayContinue_ = true;
            showOverlay("You Win!", "Keep going");
        } else {
            // normal / hard: game ends. only forward move is Unlimited.
            overlayContinue_ = false;
            showOverlay("You Win!", "Play Again");
        }
        updateView();
        return;
    }

    // loss: no valid move left in any direction (can happen in any mode)
    if (board_.isGameOver()) {
        inputLocked_     = true;
        overlayContinue_ = false;
        stopHardTimer();
        setModeButtonsEnabled(false);  // after a loss only Restart is offered
        showOverlay("Game Over", "Try Again");
        updateView();
    }
}


// Rendering

void MainWindow::updateView()
{
    refreshTiles();

    scoreValue_->setText(QString::number(board_.getScore()));

    // best score for this session only; an undo that lowers the
    // score does not lower the best
    if (board_.getScore() > bestScore_)
        bestScore_ = board_.getScore();
    bestValue_->setText(QString::number(bestScore_));

    // undo only when there is history and the game is still live
    undoButton_->setEnabled(board_.canUndo() && !inputLocked_);
}

void MainWindow::refreshTiles()
{
    const int cols = board_.getCols();
    for (int r = 0; r < board_.getRows(); ++r) {
        for (int c = 0; c < cols; ++c) {
            const int v = board_.valueAt(r, c);
            QLabel *cell = tiles_[static_cast<std::size_t>(r * cols + c)];
            cell->setText(v == 0 ? QString() : QString::number(v));
            cell->setStyleSheet(tileStyle(v));
        }
    }
}

void MainWindow::showOverlay(const QString &message, const QString &buttonText)
{
    overlayLabel_->setText(message);
    overlayButton_->setText(buttonText);
    positionOverlay();
    overlay_->show();
    overlay_->raise();
}

void MainWindow::hideOverlay()
{
    if (overlay_)
        overlay_->hide();
}

void MainWindow::positionOverlay()
{
    if (overlay_ && boardFrame_)
        overlay_->setGeometry(boardFrame_->rect());
}

void MainWindow::setModeButtonsEnabled(bool enabled)
{
    if (!modeGroup_)
        return;
    for (QAbstractButton *b : modeGroup_->buttons()) {
        b->setEnabled(enabled);
        b->setStyleSheet(kModeStyle); // clear any post-win highlight
    }
}

void MainWindow::lockModesToUnlimited()
{
    // after winning in normal/hard, the only forward move is Unlimited.
    // disable Normal (0) and Hard (2), keep Unlimited (1) on.
    // a Restart re-enables all of them via startGame().
    if (!modeGroup_)
        return;
    if (auto *b = modeGroup_->button(0)) b->setEnabled(false); // Normal
    if (auto *b = modeGroup_->button(2)) b->setEnabled(false); // Hard
    if (auto *b = modeGroup_->button(1)) {                     // Unlimited
        b->setEnabled(true);
        b->setStyleSheet(kModeHighlightStyle); // make it stand out like Restart
    }
}


// Hard-mode timer

void MainWindow::restartHardTimerIfNeeded()
{
    if (mode_ == GameMode::Hard && !inputLocked_) {
        hardTimer_->start();        // (re)start the 5s countdown
        countdownTimer_->start();   // refresh the label
        hardLabel_->show();
        updateHardCountdown();      // show the full time at once
    } else {
        stopHardTimer();
    }
}

void MainWindow::stopHardTimer()
{
    if (hardTimer_)      hardTimer_->stop();
    if (countdownTimer_) countdownTimer_->stop();
    if (hardLabel_)      hardLabel_->hide();
}

void MainWindow::updateHardCountdown()
{
    if (mode_ != GameMode::Hard || inputLocked_)
        return;

    // remainingTime() is -1 when inactive, and can read slightly above the
    // interval right after start(); clamp to [0, 5000] so the label never shows more than 5.0 s
    int ms = hardTimer_->remainingTime();
    if (ms < 0)    ms = 0;
    if (ms > 5000) ms = 5000;
    hardLabel_->setText(QString("Auto move in %1.%2 s")
                            .arg(ms / 1000)
                            .arg((ms % 1000) / 100));
}

void MainWindow::onHardTimeout()
{
    if (mode_ != GameMode::Hard || inputLocked_)
        return;

    // pick a random VALID direction so the auto move is never invalid
    const std::vector<Direction> moves = board_.validMoves();
    if (moves.empty())
        return; // no move possible: end-state check handles it

    const int idx = QRandomGenerator::global()->bounded(
        static_cast<int>(moves.size()));
    applyMove(moves[static_cast<std::size_t>(idx)]);
}


// Slots: buttons & mode switching

void MainWindow::onUndo()
{
    if (inputLocked_ || !board_.canUndo())
        return;
    board_.undo();
    updateView();
    restartHardTimerIfNeeded();
    setFocus();
}

void MainWindow::onRestart()
{
    startGame();
}

void MainWindow::onOverlayButton()
{
    if (overlayContinue_) {
        // unlimited win: close the message and keep playing this board
        overlayContinue_ = false;
        inputLocked_     = false;
        hideOverlay();
        updateView();
        restartHardTimerIfNeeded();
        setFocus();
    } else {
        // win in normal/hard, or any loss: start a fresh game
        onRestart();
    }
}

void MainWindow::onModeChanged(int id)
{
    GameMode requested = GameMode::Normal;
    if (id == 1)      requested = GameMode::Unlimited;
    else if (id == 2) requested = GameMode::Hard;

    // clicking the active mode does nothing: buttons switch mode,
    // they do not restart
    if (requested == mode_)
        return;

    mode_ = requested;

    // switching mode keeps the board and score; never a new game.
    // if a win message was up, switching resumes play (a loss stays locked).
    if (!board_.isGameOver()) {
        inputLocked_     = false;
        overlayContinue_ = false;
        hideOverlay();
    }

    restartHardTimerIfNeeded();
    updateView();
    setFocus();
}


// Event handlers

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        // restart and undo keys
        case Qt::Key_R: onRestart(); return;
        case Qt::Key_U: onUndo();    return;

        // slide keys: arrows and WASD
        case Qt::Key_Up:    case Qt::Key_W: applyMove(Direction::Up);    return;
        case Qt::Key_Down:  case Qt::Key_S: applyMove(Direction::Down);  return;
        case Qt::Key_Left:  case Qt::Key_A: applyMove(Direction::Left);  return;
        case Qt::Key_Right: case Qt::Key_D: applyMove(Direction::Right); return;

        default: QWidget::keyPressEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (overlay_ && overlay_->isVisible())
        positionOverlay();
}


// Tile appearance

QString MainWindow::tileStyle(int value)
{
    QString bg;
    QString fg = "#776e65"; // dark text on the light low tiles

    switch (value) {
        case 0:    bg = "#cdc1b4"; break; // empty cell
        case 2:    bg = "#eee4da"; break;
        case 4:    bg = "#ede0c8"; break;
        case 8:    bg = "#f2b179"; fg = "#f9f6f2"; break;
        case 16:   bg = "#f59563"; fg = "#f9f6f2"; break;
        case 32:   bg = "#f67c5f"; fg = "#f9f6f2"; break;
        case 64:   bg = "#f65e3b"; fg = "#f9f6f2"; break;
        case 128:  bg = "#edcf72"; fg = "#f9f6f2"; break;
        case 256:  bg = "#edcc61"; fg = "#f9f6f2"; break;
        case 512:  bg = "#edc850"; fg = "#f9f6f2"; break;
        case 1024: bg = "#edc53f"; fg = "#f9f6f2"; break;
        case 2048: bg = "#edc22e"; fg = "#f9f6f2"; break;
        default:   bg = "#3c3a32"; fg = "#f9f6f2"; break; // 4096 and up
    }

    // smaller font as the number gets longer so it still fits
    int fontPx = 32;
    if (value >= 100)   fontPx = 28;
    if (value >= 1000)  fontPx = 24;
    if (value >= 10000) fontPx = 20;

    return QString("QLabel { background-color:%1; color:%2; border-radius:6px;"
                   " font-weight:bold; font-size:%3px; }")
        .arg(bg, fg).arg(fontPx);
}