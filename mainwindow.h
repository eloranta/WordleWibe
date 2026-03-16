#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum class LetterState {
        NotInWord,
        WrongPosition,
        CorrectPosition
    };

    void loadStrings();
    void applyFilters();
    void refreshStringsView();
    void initializeLetterBoxes();
    void resetGame();
    void setCurrentCandidateWord(const QString &word);
    void setLetterBoxState(QLabel *label, LetterState state);
    LetterState nextLetterState(LetterState state) const;

    Ui::MainWindow *ui;
    QVector<QLabel *> m_letterBoxes;
    QStringList m_candidateWords;
};
#endif // MAINWINDOW_H
