/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#ifndef SCORES_DIALOG
#define SCORES_DIALOG

#include <QDateTime>
#include <QDialog>
class QGridLayout;
class QLabel;
class QLineEdit;

class ScoresDialog : public QDialog {
	Q_OBJECT

	public:
		ScoresDialog(QWidget* parent = 0);

		bool addScore(int score);

	private slots:
		void editingFinished();

	private:
		int addScore(const QString& name, int score, const QDateTime& date);
		void load();
		void updateItems();

	private:
		struct Score {
			QString name;
			int score;
			QDateTime date;

			bool operator<(const Score& s) const {
				return score < s.score;
			}
		};
		QList<Score> m_scores;
		QString m_default_name;

		QLabel* m_score_labels[10][4];
		QGridLayout* m_scores_layout;
		QLineEdit* m_username;
		int m_row;
};

#endif