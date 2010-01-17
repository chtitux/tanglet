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

#include "scores_dialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

#if defined(Q_OS_UNIX)
  #include <pwd.h>
  #include <unistd.h>
#elif defined(Q_OS_WIN32)
  #include <lmcons.h>
  #include <windows.h>
#endif

//-----------------------------------------------------------------------------

ScoresDialog::ScoresDialog(QWidget* parent)
: QDialog(parent), m_row(-1) {
	setWindowTitle(tr("High Scores"));

	// Load default name
	m_default_name = QSettings().value("Scores/DefaultName").toString();
	if (m_default_name.isEmpty()) {
#if defined(Q_OS_UNIX)
		passwd* pws = getpwuid(geteuid());
		if (pws) {
			m_default_name = pws->pw_gecos;
			if (m_default_name.isEmpty()) {
				m_default_name = pws->pw_name;
			}
		}
#elif defined(Q_OS_WIN32)
		WCHAR buffer[UNLEN + 1];
		DWORD count = sizeof(buffer);
		if (GetUserName(buffer, &count)) {
			m_default_name = QString::fromStdWString(buffer);
		}
#endif
	}

	// Create score widgets
	m_scores_layout = new QGridLayout;
	m_scores_layout->setSpacing(12);
	m_scores_layout->setColumnStretch(1, 1);
	m_scores_layout->addWidget(new QLabel(tr("<b>Name</b>"), this), 0, 1, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Score</b>"), this), 0, 2, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Date</b>"), this), 0, 3, Qt::AlignCenter);

	Qt::Alignment alignments[3] = { Qt::AlignLeft, Qt::AlignRight, Qt::AlignRight };
	for (int r = 0; r < 10; ++r) {
		m_score_labels[r][0] = new QLabel(tr("#%1").arg(r + 1), this);
		m_scores_layout->addWidget(m_score_labels[r][0], r + 1, 0, Qt::AlignRight | Qt::AlignVCenter);
		for (int c = 1; c < 4; ++c) {
			m_score_labels[r][c] = new QLabel("-", this);
			m_scores_layout->addWidget(m_score_labels[r][c], r + 1, c, alignments[c - 1] | Qt::AlignVCenter);
		}
	}

	load();

	m_username = new QLineEdit(this);
	m_username->hide();
	connect(m_username, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

	// Lay out dialog
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addLayout(m_scores_layout);
	layout->addWidget(buttons);
}

//-----------------------------------------------------------------------------

bool ScoresDialog::addScore(int score) {
	// Add score
	m_row = addScore(m_default_name, score, QDateTime::currentDateTime());
	if (m_row == -1) {
		return false;
	}
	for (int c = 0; c < 4; ++c) {
		QFont f = m_score_labels[m_row][c]->font();
		f.setWeight(QFont::Bold);
		m_score_labels[m_row][c]->setFont(f);
	}
	updateItems();

	// Show lineedit
	m_scores_layout->addWidget(m_username, m_row + 1, 1);
	m_score_labels[m_row][1]->hide();
	m_username->setText(m_default_name);
	m_username->show();

	return true;
}

//-----------------------------------------------------------------------------

void ScoresDialog::editingFinished() {
	// Hide lineedit
	m_username->hide();
	m_scores_layout->removeWidget(m_username);
	m_scores[m_row].name = m_username->text();
	m_score_labels[m_row][1]->show();
	updateItems();

	// Save scores
	QStringList values;
	foreach (const Score& s, m_scores) {
		values += QString("%1:%2:%3") .arg(s.name) .arg(s.score) .arg(s.date.toString("yyyy.MM.dd-hh.mm.ss"));
	}
	QSettings settings;
	settings.setValue("Scores/DefaultName", m_username->text());
	settings.setValue("Scores/Values", values);
}

//-----------------------------------------------------------------------------

int ScoresDialog::addScore(const QString& name, int score, const QDateTime& date) {
	if (score == 0) {
		return -1;
	}

	int row = 0;
	foreach (const Score& s, m_scores) {
		if (score >= s.score && date >= s.date) {
			break;
		}
		row++;
	}
	if (row == 10) {
		return -1;
	}

	Score s = { name, score, date };
	m_scores.insert(row, s);
	if (m_scores.count() == 11) {
		m_scores.removeLast();
	}

	return row;
}

//-----------------------------------------------------------------------------

void ScoresDialog::load() {
	QStringList data = QSettings().value("Scores/Values").toStringList();
	foreach (const QString& s, data) {
		QStringList values = s.split(':');
		if (values.size() == 3) {
			addScore(values[0], values[1].toInt(), QDateTime::fromString(values[2], "yyyy.MM.dd-hh.mm.ss"));
		}
	}
	updateItems();
}

//-----------------------------------------------------------------------------

void ScoresDialog::updateItems() {
	int count = m_scores.count();
	for (int r = 0; r < count; ++r) {
		const Score& score = m_scores.at(r);
		m_score_labels[r][1]->setText(score.name);
		m_score_labels[r][2]->setNum(score.score);
		m_score_labels[r][3]->setText(score.date.toString(Qt::DefaultLocaleShortDate));
	}
	for (int r = count; r < 10; ++r) {
		for (int c = 1; c < 4; ++c) {
			m_score_labels[r][c]->setText("-");
		}
	}
}

//-----------------------------------------------------------------------------