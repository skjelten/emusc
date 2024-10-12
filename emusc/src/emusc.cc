/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
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
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QSettings>
#include <QString>
#include <QStringList>

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif

#include "config.h"


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


int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/icon-256.png"));

  QCoreApplication::setOrganizationName("emusc");
  QCoreApplication::setApplicationName("EmuSC");
  QCoreApplication::setApplicationVersion(VERSION);

  QCommandLineParser parser;
  parser.setApplicationDescription("Roland SC-55 emulator");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption printConfig(QStringList() << "c" << "show-config",
                                 "Print configuration to stdout");
  parser.addOption(printConfig);

  QCommandLineOption power(QStringList() << "p" << "power",
                           "Override configuration with synth power ON or OFF",
                           "state", "");
  parser.addOption(power);

#ifdef __ALSA_MIDI__
  QCommandLineOption midiPort(QStringList() << "m" << "midi-port",
                              "Connect to MIDI port (ALSA only)",
                              "address", "");
  parser.addOption(midiPort);
#endif

  parser.process(app);

  if (parser.isSet(power) &&
      !(!parser.value(power).compare("ON", Qt::CaseInsensitive) ||
        !parser.value(power).compare("OFF", Qt::CaseInsensitive))) {
    std::cerr << "Error: Invalid power state. Must be ON or OFF." << std::endl;
    parser.showHelp(1);
  }

  if (parser.isSet(printConfig)) {
    print_config();
    return 1;
  }

  // Make Windows send stdout & stderr to console if started from one
#ifdef Q_OS_WINDOWS
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }
#endif

  MainWindow window;
  window.show();

  QApplication::connect(&app, SIGNAL(aboutToQuit()),
			&window, SLOT(cleanUp()));

  return app.exec();
}
