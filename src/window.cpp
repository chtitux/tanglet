/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "window.h"

#include "board.h"
#include "clock.h"
#include "language_dialog.h"
#include "locale_dialog.h"
#include "new_game_dialog.h"
#include "scores_dialog.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#include <ctime>

//-----------------------------------------------------------------------------

class Window::State {
public:
	State(Window* window)
		: m_window(window) { }

	virtual ~State() { }

	virtual void enter() { }
	virtual void newGame() { setState("NewGame"); }
	virtual void openGame() { setState("OpenGame"); }
	virtual void play() { }
	virtual void autoPause() { }
	virtual void autoResume() { }
	virtual void pause() { }
	virtual void resume() { }
	virtual void finish() { setState("Finish"); }

protected:
	void setPaused(bool paused) {
		m_window->m_board->setPaused(paused);
		m_window->m_pause_action->setChecked(paused);
	}

	void setContentsIndex(int index) {
		m_window->m_contents->setCurrentIndex(index);
	}

	void setState(const QString& state) {
		m_window->m_state = m_window->m_states.value(state);
		m_window->m_state->enter();
	}

private:
	Window* m_window;
};

//-----------------------------------------------------------------------------

class Window::NewGameState : public Window::State {
public:
	NewGameState(Window* window)
		: State(window) { }

	void enter() {
		m_next_state = "Play";
		setPaused(true);
		setContentsIndex(4);
	}

	void play() { setState(m_next_state); }
	void autoPause() { m_next_state = "AutoPause"; }
	void autoResume() { m_next_state = "Play"; }

private:
	QString m_next_state;
};

//-----------------------------------------------------------------------------

class Window::OpenGameState : public Window::State {
public:
	OpenGameState(Window* window)
		: State(window) { }

	void enter() {
		m_next_state = "Play";
		setPaused(true);
		setContentsIndex(2);
	}

	void play() { setState(m_next_state); }
	void autoPause() { m_next_state = "AutoPause"; }
	void autoResume() { m_next_state = "Play"; }

private:
	QString m_next_state;
};

//-----------------------------------------------------------------------------

class Window::PlayState : public Window::State {
public:
	PlayState(Window* window)
		: State(window) { }

	void enter() {
		setPaused(false);
		setContentsIndex(0);
	}

	void autoPause() { setState("AutoPause"); }
	void pause() { setState("Pause"); }
};

//-----------------------------------------------------------------------------

class Window::AutoPauseState : public Window::State {
public:
	AutoPauseState(Window* window)
		: State(window), m_count(0) { }

	void enter() {
		setPaused(true);
		setContentsIndex(1);
		m_count++;
	}

	void autoPause() {
		m_count++;
	}

	void autoResume() {
		m_count--;
		if (m_count < 1) {
			m_count = 0;
			setState("Play");
		}
	}

	void newGame() { m_count = 0; setState("NewGame"); }
	void openGame() { m_count = 0; setState("OpenGame"); }
	void pause() { m_count = 0; setState("Pause"); }
	void resume() { m_count = 0; setState("Play"); }
	void finish() { m_count = 0; setState("Finish"); }

private:
	int m_count;
};

//-----------------------------------------------------------------------------

class Window::PauseState : public Window::State {
public:
	PauseState(Window* window)
		: State(window) { }

	void enter() {
		setPaused(true);
		setContentsIndex(1);
	}

	void resume() { setState("Play"); }
};

//-----------------------------------------------------------------------------

class Window::FinishState : public Window::State {
public:
	FinishState(Window* window)
		: State(window) { }

	void enter() {
		setPaused(false);
		setContentsIndex(0);
	}
};

//-----------------------------------------------------------------------------

namespace
{
	class AboutDialog : public QDialog
	{
	public:
		AboutDialog(const QString& title, const QString& filename, QWidget* parent = 0);
	};

	AboutDialog::AboutDialog(const QString& title, const QString& filename, QWidget* parent)
		: QDialog(parent)
	{
		setWindowTitle(title);

		QTextEdit* text = new QTextEdit(this);
		text->setWordWrapMode(QTextOption::NoWrap);
		text->setReadOnly(true);

		QFile file(filename);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			text->setHtml("<pre>" + stream.readAll() + "</pre>");
			file.close();
		}

		QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
		buttons->setCenterButtons(style()->styleHint(QStyle::SH_MessageBox_CenterButtons));
		connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->addWidget(text);
		layout->addWidget(buttons);

		resize(700, 500);
	}
}

//-----------------------------------------------------------------------------

Window::Window()
: m_pause_action(0) {
	setWindowTitle(tr("Tanglet"));
	setWindowIcon(QIcon(":/tanglet.png"));

	// Create states
	m_states.insert("NewGame", new NewGameState(this));
	m_states.insert("OpenGame", new OpenGameState(this));
	m_states.insert("Play", new PlayState(this));
	m_states.insert("AutoPause", new AutoPauseState(this));
	m_states.insert("Pause", new PauseState(this));
	m_states.insert("Finish", new FinishState(this));
	m_state = m_states.value("NewGame");

	// Create widgets
	m_contents = new QStackedWidget(this);
	setCentralWidget(m_contents);

	m_board = new Board(this);
	m_contents->addWidget(m_board);
	connect(m_board, SIGNAL(started()), this, SLOT(gameStarted()));
	connect(m_board, SIGNAL(finished(int)), this, SLOT(gameFinished(int)));

	// Create pause screen
	m_pause_screen = new QLabel(tr("<p><b><big>Paused</big></b><br>Click to resume playing.</p>"), this);
	m_pause_screen->setAlignment(Qt::AlignCenter);
	m_pause_screen->installEventFilter(this);
	m_contents->addWidget(m_pause_screen);

	// Create open game screen
	QLabel* open_game_screen = new QLabel(tr("<p><b><big>Please wait</big></b><br>Loading game...</p>"), this);
	open_game_screen->setAlignment(Qt::AlignCenter);
	m_contents->addWidget(open_game_screen);

	// Create start screen
	QLabel* start_screen = new QLabel(tr("Click to start a new game."), this);
	start_screen->setAlignment(Qt::AlignCenter);
	start_screen->installEventFilter(this);
	m_contents->addWidget(start_screen);

	// Create new game screen
	QLabel* new_game_screen = new QLabel(tr("<p><b><big>Please wait</big></b><br>Generating a new board...</p>"), this);
	new_game_screen->setAlignment(Qt::AlignCenter);
	m_contents->addWidget(new_game_screen);

	// Create game menu
	QMenu* menu = menuBar()->addMenu(tr("&Game"));
	menu->addAction(tr("New &Game..."), this, SLOT(newGame()), tr("Ctrl+Shift+N"));
	menu->addAction(tr("&New Roll"), this, SLOT(newRoll()), QKeySequence::New);
	menu->addAction(tr("&Choose..."), this, SLOT(chooseGame()));
	menu->addAction(tr("&Share..."), this, SLOT(shareGame()));
	menu->addSeparator();
	QAction* end_action = menu->addAction(tr("&End"), this, SLOT(endGame()));
	end_action->setEnabled(false);
	connect(m_board, SIGNAL(pauseAvailable(bool)), end_action, SLOT(setEnabled(bool)));
	menu->addSeparator();
	m_pause_action = menu->addAction(tr("&Pause"));
	m_pause_action->setCheckable(true);
	m_pause_action->setShortcut(tr("Ctrl+P"));
	m_pause_action->setEnabled(false);
	connect(m_pause_action, SIGNAL(triggered(bool)), this, SLOT(setPaused(bool)));
	connect(m_board, SIGNAL(pauseAvailable(bool)), m_pause_action, SLOT(setEnabled(bool)));
	menu->addSeparator();
	m_details_action = menu->addAction(tr("&Details"), this, SLOT(showDetails()));
	m_details_action->setEnabled(false);
	menu->addAction(tr("&High Scores"), this, SLOT(showScores()));
	menu->addSeparator();
	menu->addAction(tr("&Quit"), this, SLOT(close()), tr("Ctrl+Q"));
	monitorVisibility(menu);

	// Create settings menu
	menu = menuBar()->addMenu(tr("&Settings"));
	QMenu* submenu = menu->addMenu(tr("Show &Maximum Score"));
	QAction* score_actions[3];
	score_actions[0] = submenu->addAction(tr("&Never"));
	score_actions[1] = submenu->addAction(tr("&End Of Game"));
	score_actions[2]  = submenu->addAction(tr("&Always"));
	QActionGroup* group = new QActionGroup(this);
	for (int i = 0; i < 3; ++i) {
		score_actions[i]->setData(i);
		score_actions[i]->setCheckable(true);
		group->addAction(score_actions[i]);
	}
	connect(group, SIGNAL(triggered(QAction*)), m_board, SLOT(setShowMaximumScore(QAction*)));
	QAction* missed_action = menu->addAction(tr("Show Missed &Words"));
	missed_action->setCheckable(true);
	connect(missed_action, SIGNAL(toggled(bool)), m_board, SLOT(setShowMissedWords(bool)));
	QAction* counts_action = menu->addAction(tr("Show Word &Counts"));
	counts_action->setCheckable(true);
	counts_action->setChecked(true);
	connect(counts_action, SIGNAL(toggled(bool)), m_board, SLOT(setShowWordCounts(bool)));
	menu->addAction(tr("&Board Language..."), this, SLOT(showLanguage()));
	menu->addSeparator();
	menu->addAction(tr("Application &Language..."), this, SLOT(showLocale()));
	monitorVisibility(menu);

	// Create help menu
	menu = menuBar()->addMenu(tr("&Help"));
	menu->addAction(tr("&Controls"), this, SLOT(showControls()));
	menu->addSeparator();
	menu->addAction(tr("&About"), this, SLOT(about()));
	menu->addAction(tr("About &Hspell"), this, SLOT(aboutHspell()));
	menu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
	menu->addAction(tr("About &SCOWL"), this, SLOT(aboutScowl()));
	monitorVisibility(menu);

	// Load settings
	QSettings settings;
	QAction* score_action = score_actions[qBound(0, settings.value("ShowMaximumScore", 1).toInt(), 2)];
	score_action->setChecked(true);
	m_board->setShowMaximumScore(score_action);
	missed_action->setChecked(settings.value("ShowMissed", true).toBool());
	counts_action->setChecked(settings.value("ShowWordCounts", true).toBool());
	restoreGeometry(settings.value("Geometry").toByteArray());

	// Start a new game
	m_state->finish();
	m_contents->setCurrentIndex(3);
	if (!settings.contains("Current/Version")) {
		newGame();
	} else {
		startGame(":saved:");
	}
}

//-----------------------------------------------------------------------------

bool Window::eventFilter(QObject* watched, QEvent* event) {
	if (watched == m_pause_screen) {
		if (event->type() == QEvent::MouseButtonPress) {
			m_state->resume();
			return true;
		} else {
			return false;
		}
	} else if (watched == m_contents->widget(3)) {
		if (event->type() == QEvent::MouseButtonPress) {
			newGame();
			return true;
		} else {
			return false;
		}
	} else {
		return QMainWindow::eventFilter(watched, event);
	}
}

//-----------------------------------------------------------------------------

void Window::closeEvent(QCloseEvent* event) {
	QSettings().setValue("Geometry", saveGeometry());
	QMainWindow::closeEvent(event);
}

//-----------------------------------------------------------------------------

bool Window::event(QEvent* event) {
	if (m_pause_action && m_pause_action->isEnabled()) {
		switch (event->type()) {
		case QEvent::WindowDeactivate:
			if (!QApplication::activeWindow() && !QApplication::activePopupWidget() && !QApplication::activeModalWidget()) {
				m_state->pause();
			}
			break;
		case QEvent::WindowBlocked:
			m_state->autoPause();
			break;
		case QEvent::WindowUnblocked:
			m_state->autoResume();
			break;
		default:
			break;
		}
	}
	return QMainWindow::event(event);
}

//-----------------------------------------------------------------------------

void Window::about() {
	QMessageBox::about(this, tr("About"),
		QString("<center><p><big><b>%1</b></big><br/>%2<br/><small>%3<br/>%4</small></p><p>%5</p><p>%6</p></center>")
		.arg(tr("Tanglet %1").arg(QCoreApplication::applicationVersion()))
		.arg(tr("A single player variant of <a href=\"http://en.wikipedia.org/wiki/Boggle\">Boggle</a>"))
		.arg(tr("Copyright &copy; 2009, 2010, 2011 Graeme Gott"))
		.arg(tr("Released under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GPL 3</a> license"))
		.arg(tr("English word list is based on <a href=\"http://wordlist.sourceforge.net/\">SCOWL</a> by Kevin Atkinson"))
		.arg(tr("Hebrew word list is based on <a href=\"http://hspell.ivrix.org.il/\">Hspell</a> by Nadav Har'El and Dan Kenigsberg")));
}

//-----------------------------------------------------------------------------

void Window::aboutHspell() {
	AboutDialog dialog(tr("About Hspell"), ":/hspell-readme", this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::aboutScowl() {
	AboutDialog dialog(tr("About SCOWL"), ":/scowl-readme", this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::newRoll() {
	if (endGame()) {
		startGame();
	}
}

//-----------------------------------------------------------------------------

void Window::newGame() {
	if (endGame()) {
		NewGameDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted) {
			startGame();
		}
	}
}


//-----------------------------------------------------------------------------

void Window::chooseGame() {
	if (endGame()) {
		QString filename = QFileDialog::getOpenFileName(window(), tr("Import Game"), QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation), tr("Tanglet Games (*.tanglet)"));
		if (!filename.isEmpty()) {
			startGame(filename);
		}
	}
}

//-----------------------------------------------------------------------------

void Window::shareGame() {
	QString filename = QFileDialog::getSaveFileName(window(), tr("Export Game"), QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation), tr("Tanglet Games (*.tanglet)"));
	if (!filename.isEmpty()) {
		QSettings settings;
		QSettings game(filename, QSettings::IniFormat);
		game.setValue("Game/Version", 1);
		game.setValue("Game/Size", settings.value("Current/Size"));
		game.setValue("Game/Density", settings.value("Current/Density"));
		game.setValue("Game/Minimum", settings.value("Current/Minimum"));
		game.setValue("Game/TimerMode", settings.value("Current/TimerMode"));
		game.setValue("Game/Letters", settings.value("Current/Letters"));
	}
}

//-----------------------------------------------------------------------------

bool Window::endGame() {
	if (!m_board->isFinished()) {
		if (QMessageBox::question(this, tr("Question"), tr("End the current game?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
			m_board->abort();
		} else {
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------

void Window::autoPause() {
	m_state->autoPause();
}

//-----------------------------------------------------------------------------

void Window::autoResume() {
	m_state->autoResume();
}

//-----------------------------------------------------------------------------

void Window::setPaused(bool paused) {
	if (paused) {
		m_state->pause();
	} else {
		m_state->resume();
	}
}

//-----------------------------------------------------------------------------

void Window::showDetails() {
	QSettings settings;
	int size = settings.value("Current/Size").toInt();
	int density = settings.value("Current/Density").toInt();
	int minimum = settings.value("Current/Minimum").toInt();
	int timer = settings.value("Current/TimerMode").toInt();
	QMessageBox::information(this, tr("Details"),
		QString("<p><b>%1</b> %2<br>"
			"<b>%3</b> %4<br>"
			"<b>%5</b> %6<br>"
			"<b>%7</b> %8<br>"
			"<b>%9</b> %10</p>")
		.arg(tr("Board Size:"), (size == 4) ? tr("Normal") : tr("Large"))
		.arg(tr("Word Density:"), NewGameDialog::densityString(density))
		.arg(tr("Word Length:"), tr("%1 or more letters").arg(minimum))
		.arg(tr("Game Type:"), Clock::timerToString(timer))
		.arg(tr("Description:"), Clock::timerDescription(timer)));
}

//-----------------------------------------------------------------------------

void Window::showScores() {
	ScoresDialog scores(this);
	scores.exec();
}

//-----------------------------------------------------------------------------

void Window::showLanguage() {
	LanguageDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		newGame();
	}
}

//-----------------------------------------------------------------------------

void Window::showLocale() {
	LocaleDialog dialog(this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::showControls() {
	QMessageBox::information(this, tr("Controls"), tr(
		"<p><b><big>Mouse Play:</big></b><br>"
		"<b>Select a word:</b> Click on the letters of a word.<br>"
		"<b>Make a guess:</b> Click on the last selected letter.<br>"
		"<b>Erase letters:</b> Click on an earlier selected letter.<br>"
		"<b>Clear the word:</b> Click twice on the first selected letter.</p>"
		"<p><b><big>Keyboard Play:</big></b><br>"
		"<b>Select a word:</b> Type the letters of a word.<br>"
		"<b>Make a guess:</b> Press Enter.<br>"
		"<b>Clear the word:</b> Press Ctrl+Backspace.</p>"
	));
}

//-----------------------------------------------------------------------------

void Window::gameStarted() {
	m_state->play();
	m_details_action->setEnabled(true);
}

//-----------------------------------------------------------------------------

void Window::gameFinished(int score) {
	m_state->finish();
	ScoresDialog scores(this);
	if (scores.addScore(score)) {
		scores.exec();
	}
}

//-----------------------------------------------------------------------------

void Window::monitorVisibility(QMenu* menu) {
#ifndef Q_WS_MAC
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(autoPause()));
	connect(menu, SIGNAL(aboutToHide()), this, SLOT(autoResume()));
#endif
}

//-----------------------------------------------------------------------------

void Window::startGame(const QString& filename) {
	QSettings* game = 0;

	if (filename.isEmpty()) {
		// Start a new game
		game = new QSettings;
		game->remove("Current");
		game->sync();
		game->beginGroup("Board");
		m_state->newGame();
	} else {
		if (filename == ":saved:") {
			// Continue previous game
			game = new QSettings;
			game->beginGroup("Current");
		} else {
			// Start requested game
			QSettings().remove("Current");
			game = new QSettings(filename, QSettings::IniFormat);
			game->beginGroup("Game");
		}

		bool loaded = false;
		if (game->value("Version").toInt() == 1) {
			loaded = !game->value("Letters").toStringList().isEmpty();
		}
		if (!loaded) {
			QMessageBox::warning(this, tr("Error"), tr("Unable to start requested game."));
			return;
		}

		m_state->openGame();
	}

	m_board->generate(*game);
	delete game;
}

//-----------------------------------------------------------------------------
