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

#include "language_dialog.h"
#include "window.h"

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("Tanglet");
	app.setApplicationVersion("1.0.1");
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");

	QString path = app.applicationDirPath();
	QStringList paths;
	paths.append(path + "/data/");
	paths.append(path + "/../share/tanglet/data/");
	paths.append(path + "/../Resources/");
	QDir::setSearchPaths("tanglet", paths);

	QString language = QLocale::system().name().left(2);

	QTranslator qt_translator;
	qt_translator.load("qt_" + language, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qt_translator);

	QTranslator translator_en;
	translator_en.load(":/tanglet_en");
	app.installTranslator(&translator_en);

	QTranslator translator;
	translator.load(":/tanglet_" + language);
	app.installTranslator(&translator);

	QSettings settings;
	if (settings.value("Language", -1).toInt() == -1) {
		int default_count = 0;
		if (settings.value("Dice", ":/en_US/dice").toString() == ":/en_US/dice") {
			settings.setValue("Dice", "tanglet:en/dice");
			default_count++;
		}
		if (settings.value("Words", ":/en_US/words").toString() == ":/en_US/words") {
			settings.setValue("Words", "tanglet:en/words");
			default_count++;
		}
		if (settings.value("Dictionary").toString().isEmpty()) {
			settings.setValue("Dictionary", "http://www.google.com/dictionary?langpair=en|en&q=%s");
			default_count++;
		}
		if (default_count == 3) {
			LanguageDialog::restoreDefaults();
		} else {
			settings.setValue("Language", 0);
		}
	}

	Window window;
	window.show();

	return app.exec();
}
