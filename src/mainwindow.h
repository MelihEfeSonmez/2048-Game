#pragma once

#include <QWidget>
#include <vector>
#include "gamelogic.h"

// forward declarations (full Qt headers are in the .cpp)
class QLabel;
class QPushButton;
class QFrame;
class QTimer;
class QButtonGroup;
class QKeyEvent;
class QResizeEvent;

// the three game modes
enum class GameMode { Normal, Unlimited, Hard };

// MainWindow: the view + controller.
// owns a GameLogic, draws it as a grid of labels, turns input into moves.
// all game rules live in GameLogic; this class only does UI and events.
// switching mode keeps the current board and score.
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // WASD / arrow keys, U = undo, R = restart
    void keyPressEvent(QKeyEvent *event) override;
    // keep the overlay over the board on resize
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onUndo();
    void onRestart();
    void onOverlayButton();       // overlay button: continue (unlimited) or restart
    void onModeChanged(int id);   // a mode button was clicked
    void onHardTimeout();         // 5s passed in Hard mode -> random move
    void updateHardCountdown();   // refresh the "x.x s" label while Hard timer runs

private:
    // setup
    void buildUi();

    // gameplay flow
    void startGame();             // reset the board for the current mode
    void applyMove(Direction dir);// do one slide and react to the result
    void evaluateEndStates();     // check win / game-over

    // rendering
    void updateView();            // refresh tiles, score, best, controls
    void refreshTiles();
    void showOverlay(const QString &message, const QString &buttonText);
    void hideOverlay();
    void positionOverlay();
    void setModeButtonsEnabled(bool enabled);
    void lockModesToUnlimited();  // after a normal/hard win: only Unlimited stays on

    // hard-mode timer
    void restartHardTimerIfNeeded();
    void stopHardTimer();

    // colour + font for a tile of the given value
    static QString tileStyle(int value);

    // model & state
    GameLogic board_;
    GameMode  mode_           = GameMode::Normal;
    bool      inputLocked_    = false; // true once the game ends (until restart)
    bool      winAcknowledged_= false; // show the win overlay only once
    bool      overlayContinue_= false; // overlay button means "keep playing", not restart
    int       bestScore_      = 0;     // best score this session (not saved to disk)

    // widgets
    QLabel       *scoreValue_   = nullptr;
    QLabel       *bestValue_    = nullptr;
    QFrame       *boardFrame_   = nullptr;
    std::vector<QLabel*> tiles_;        // tile labels, row-major

    QPushButton  *undoButton_    = nullptr;
    QPushButton  *restartButton_ = nullptr;
    QButtonGroup *modeGroup_     = nullptr;

    QFrame       *overlay_       = nullptr; // win / game-over panel over the grid
    QLabel       *overlayLabel_  = nullptr;
    QPushButton  *overlayButton_ = nullptr;

    QTimer       *hardTimer_     = nullptr; // 5s single-shot: fires the auto move
    QTimer       *countdownTimer_= nullptr; // repeating: refreshes the seconds label
    QLabel       *hardLabel_     = nullptr; // shows remaining seconds in Hard mode
};