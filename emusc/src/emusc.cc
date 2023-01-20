/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2023  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#include "main_window.h"

#include <iostream>
#include <string>

#include <QApplication>
#include <QSettings>
#include <QString>


void show_arguments(std::string program)
{
  std::cerr << "Usage: " << program << " [OPTION...]\n\n"
	    << "Options:\n"
	    << "  -c, --print-config      \tPrint configuration to stdout\n"
	    << "  -p, --power-on          \tStart with synth powered on\n"
	    << "  -h, --help              \tShow this help message\n"
	    << std::endl;
}


void print_config()
{
  QSettings settings;

  std::cout << "-- EmuSC configuration --" << std::endl << std::endl;

  foreach (const QString &group, settings.childGroups()) {
    std::cout << "[" << group.toStdString() << "]" << std::endl;
    settings.beginGroup(group);

    foreach (const QString &key, settings.childKeys())
      std::cout << key.toStdString() << " = "
		<< settings.value(key).toString().toStdString() << std::endl;

    settings.endGroup();
    std::cout << std::endl;
  }

  std::cout << "-------------------------" << std::endl;
}


int parse_arguments(int argc, char *argv[])
{
  if (QCoreApplication::arguments().contains("-h") ||
      QCoreApplication::arguments().contains("--help")) {
    show_arguments(argv[0]);
    return 1;

  } else if (QCoreApplication::arguments().contains("-c") ||
	     QCoreApplication::arguments().contains("--print-config")) {
    print_config();
    return 1;
  }

  return 0;
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("emusc");
    QCoreApplication::setApplicationName("EmuSC");

    if (argc > 1) {
      int ret = parse_arguments(argc, argv);
      if (ret)
	return ret;
    }

    MainWindow window;
    window.show();

    QApplication::connect(&app, SIGNAL(aboutToQuit()),
			  &window, SLOT(cleanUp()));

    return app.exec();
}
