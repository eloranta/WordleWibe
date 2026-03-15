#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stringsView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    connect(ui->runAgainButton, &QPushButton::clicked, this, &MainWindow::resetGame);
    initializeLetterBoxes();
    resetGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadStrings()
{
    const QString filePath = QCoreApplication::applicationDirPath() + "/wordle_strings.txt";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->stringsView->setPlainText(tr("Could not open:\n%1").arg(filePath));
        statusBar()->showMessage(tr("wordle_strings.txt not found in exe directory"));
        return;
    }

    m_candidateWords = QString::fromUtf8(file.readAll()).toUpper().split('\n', Qt::SkipEmptyParts);
    for (QString &word : m_candidateWords) {
        word = word.trimmed();
    }
    m_candidateWords.removeAll(QString());
    refreshStringsView();
    statusBar()->showMessage(tr("Loaded wordle_strings.txt from exe directory"));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        applyFilters();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::MouseButtonPress) {
        return QMainWindow::eventFilter(watched, event);
    }

    QLabel *label = qobject_cast<QLabel *>(watched);
    if (!label || !m_letterBoxes.contains(label)) {
        return QMainWindow::eventFilter(watched, event);
    }

    const auto state = static_cast<LetterState>(label->property("letterState").toInt());
    setLetterBoxState(label, nextLetterState(state));

    return true;
}

void MainWindow::applyFilters()
{
    QString lettersToExclude;
    QVector<QPair<int, QString>> wrongPositionLetters;
    QVector<QPair<int, QString>> correctPositionLetters;
    QString lettersKnownToBePresent;

    for (int i = 0; i < m_letterBoxes.size(); ++i) {
        QLabel *label = m_letterBoxes.at(i);
        const auto state = static_cast<LetterState>(label->property("letterState").toInt());
        const QString letter = label->text().trimmed().toUpper();

        if (state == LetterState::NotInWord) {
            if (!letter.isEmpty() && !lettersKnownToBePresent.contains(letter) && !lettersToExclude.contains(letter)) {
                lettersToExclude += letter;
            }
        } else if (state == LetterState::WrongPosition && !letter.isEmpty()) {
            wrongPositionLetters.append({i, letter});
            if (!lettersKnownToBePresent.contains(letter)) {
                lettersKnownToBePresent += letter;
            }
        } else if (state == LetterState::CorrectPosition && !letter.isEmpty()) {
            correctPositionLetters.append({i, letter});
            if (!lettersKnownToBePresent.contains(letter)) {
                lettersKnownToBePresent += letter;
            }
        }
    }

    QString filteredLettersToExclude;
    for (const QChar &letter : lettersToExclude) {
        if (!lettersKnownToBePresent.contains(letter)) {
            filteredLettersToExclude += letter;
        }
    }
    lettersToExclude = filteredLettersToExclude;

    if (lettersToExclude.isEmpty() && wrongPositionLetters.isEmpty() && correctPositionLetters.isEmpty()) {
        statusBar()->showMessage(tr("No gray, orange, or green letters to filter"));
        return;
    }

    QStringList filteredWords;
    filteredWords.reserve(m_candidateWords.size());

    for (const QString &word : m_candidateWords) {
        bool shouldRemoveWord = false;

        for (const QChar &letter : lettersToExclude) {
            if (word.contains(letter)) {
                shouldRemoveWord = true;
                break;
            }
        }

        if (!shouldRemoveWord) {
            for (const auto &entry : wrongPositionLetters) {
                if (word.mid(entry.first, 1) == entry.second || !word.contains(entry.second)) {
                    shouldRemoveWord = true;
                    break;
                }
            }
        }

        if (!shouldRemoveWord) {
            for (const auto &entry : correctPositionLetters) {
                if (word.mid(entry.first, 1) != entry.second) {
                    shouldRemoveWord = true;
                    break;
                }
            }
        }

        if (!shouldRemoveWord) {
            filteredWords.append(word);
        }
    }

    const int removedCount = m_candidateWords.size() - filteredWords.size();
    m_candidateWords = filteredWords;
    refreshStringsView();
    QStringList ruleSummary;
    if (!lettersToExclude.isEmpty()) {
        ruleSummary.append(tr("gray %1").arg(lettersToExclude));
    }
    if (!wrongPositionLetters.isEmpty()) {
        QStringList orangeRules;
        for (const auto &entry : wrongPositionLetters) {
            orangeRules.append(tr("%1@%2").arg(entry.second).arg(entry.first + 1));
        }
        ruleSummary.append(tr("orange %1").arg(orangeRules.join(',')));
    }
    if (!correctPositionLetters.isEmpty()) {
        QStringList greenRules;
        for (const auto &entry : correctPositionLetters) {
            greenRules.append(tr("%1@%2").arg(entry.second).arg(entry.first + 1));
        }
        ruleSummary.append(tr("green %1").arg(greenRules.join(',')));
    }

    statusBar()->showMessage(tr("Removed %1 words using %2").arg(removedCount).arg(ruleSummary.join("; ")));
}

void MainWindow::refreshStringsView()
{
    ui->stringsView->setPlainText(m_candidateWords.join('\n'));
}

void MainWindow::initializeLetterBoxes()
{
    m_letterBoxes = {ui->letterBox1, ui->letterBox2, ui->letterBox3, ui->letterBox4, ui->letterBox5};

    const QString initialWord = QStringLiteral("CRANE");
    for (int i = 0; i < m_letterBoxes.size(); ++i) {
        QLabel *label = m_letterBoxes.at(i);
        label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        label->setText(initialWord.mid(i, 1));
        label->installEventFilter(this);
        setLetterBoxState(label, LetterState::NotInWord);
    }
}

void MainWindow::resetGame()
{
    const QString initialWord = QStringLiteral("CRANE");
    for (int i = 0; i < m_letterBoxes.size(); ++i) {
        QLabel *label = m_letterBoxes.at(i);
        label->setText(initialWord.mid(i, 1));
        setLetterBoxState(label, LetterState::NotInWord);
    }

    loadStrings();
    statusBar()->showMessage(tr("Reset to initial word list and starting state"));
}

void MainWindow::setLetterBoxState(QLabel *label, LetterState state)
{
    label->setProperty("letterState", static_cast<int>(state));

    QString backgroundColor;
    switch (state) {
    case LetterState::NotInWord:
        backgroundColor = QStringLiteral("#787c7e");
        break;
    case LetterState::WrongPosition:
        backgroundColor = QStringLiteral("#c9b458");
        break;
    case LetterState::CorrectPosition:
        backgroundColor = QStringLiteral("#6aaa64");
        break;
    }

    label->setStyleSheet(QStringLiteral(
        "QLabel {"
        " background-color: %1;"
        " color: white;"
        " border: 2px solid #3a3a3c;"
        " font-size: 24px;"
        " font-weight: 700;"
        "}"
    ).arg(backgroundColor));
}

MainWindow::LetterState MainWindow::nextLetterState(LetterState state) const
{
    switch (state) {
    case LetterState::NotInWord:
        return LetterState::WrongPosition;
    case LetterState::WrongPosition:
        return LetterState::CorrectPosition;
    case LetterState::CorrectPosition:
        return LetterState::NotInWord;
    }

    return LetterState::NotInWord;
}
