/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
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
 */

#include "Application.h"

#include <gui/MainWindow.h>


Application::Application(int & argc, char ** argv)
    : QApplication(argc, argv)
    , m_mainWindow{ nullptr }
{
    auto f = font();
    f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);
}

Application::~Application() = default;

void Application::startup()
{
    m_mainWindow = std::make_unique<MainWindow>();
    m_mainWindow->show();

    QStringList fileNames = arguments();
    // skip the executable path
    fileNames.removeFirst();

    if (!fileNames.isEmpty())
    {
        m_mainWindow->openFiles(fileNames);
    }
}
