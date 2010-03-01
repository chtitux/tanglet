/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef BOARD_H
#define BOARD_H

#include "trie.h"

#include <QHash>
#include <QList>
#include <QPoint>
#include <QWidget>
class QLabel;
class QLineEdit;
class QMainWindow;
class QTabWidget;
class QToolButton;
class QTreeWidgetItem;
class Clock;
class Letter;
class LanguageSettings;
class View;
class WordTree;

class Board : public QWidget {
	Q_OBJECT

	public:
		Board(QWidget* parent = 0);

		bool isFinished() const;
		void abort();
		void generate(int seed);
		void loadSettings(const LanguageSettings& settings);
		void setPaused(bool pause);

		static QString sizeToString(int size);

	public slots:
		void setHigherScoringBoards(bool higher);
		void setShowMissedWords(bool show);
		void setShowMaximumScore(QAction* show);

	signals:
		void started();
		void finished(int score);
		void pauseAvailable(bool available);

	private slots:
		void gameStarted();
		void clearGuess();
		void guess();
		void guessChanged();
		void finish();
		void wordSelected();
		void letterClicked(Letter* letter);

	private:
		void highlightWord(const QList<QPoint>& positions, const QColor& color);
		void highlightWord();
		void clearHighlight();
		void selectGuess();
		int updateScore();
		void updateClickableStatus();

	private:
		Clock* m_clock;
		View* m_view;
		QLabel* m_score;
		QVector<QVector<Letter*> > m_cells;
		QLineEdit* m_guess;
		QTabWidget* m_tabs;
		WordTree* m_found;
		WordTree* m_missed;
		QToolButton* m_clear_button;
		QToolButton* m_guess_button;

		bool m_paused;
		bool m_wrong;
		bool m_valid;
		int m_score_type;

		QList<QStringList> m_dice;
		QList<QStringList> m_dice_larger;
		int m_size;
		int m_minimum;
		int m_maximum;
		int m_max_score;
		Trie m_words;
		QStringList m_letters;
		QHash<QString, QList<QList<QPoint> > > m_solutions;
		QList<QPoint> m_positions;

		class Generator;
		Generator* m_generator;
};

#endif
