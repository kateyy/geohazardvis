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

#include "TestEnvironment.h"

#include <cassert>
#include <memory>

#include <QApplication>
#include <QDir>


namespace
{

std::unique_ptr<QApplication> l_app;

const QString & testDataDirName()
{
    static const QString d = "test_data";
    return d;
}

}


void TestEnvironment::init(int & argc, char ** argv)
{
    assert(!l_app);
    l_app = std::make_unique<QApplication>(argc, argv);
}

void TestEnvironment::release()
{
    assert(l_app);
    l_app.reset();
}

const QString & TestEnvironment::applicationFilePath()
{
    static const auto p = l_app->applicationFilePath();
    return p;
}

const QString & TestEnvironment::applicationDirPath()
{
    static const auto p = l_app->applicationDirPath();
    return p;
}

const QString & TestEnvironment::testDirPath()
{
    static const auto p = QDir(applicationDirPath()).filePath(testDataDirName());
    return p;
}

void TestEnvironment::createTestDir()
{
    QDir(applicationDirPath()).mkpath(testDataDirName());
}

void TestEnvironment::clearTestDir()
{
    QDir(testDirPath()).removeRecursively();
}
