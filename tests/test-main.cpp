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

#include <QCommandLineParser>
#include <QCoreApplication>

#include <gtest/gtest.h>

#if defined(TEST_WITH_OPENGL_SUPPORT)
#include <gui/data_view/t_QVTKWidget.h>
#endif

#include <TestEnvironment.h>


int main(int argc, char* argv[])
{
#if defined(TEST_WITH_OPENGL_SUPPORT)
    t_QVTKWidget::initializeDefaultSurfaceFormat();
#endif

    TestEnvironment::init(argc, argv);

    QCommandLineParser cmdParser;
    const QCommandLineOption waitAfterFinishedOption("waitAfterFinished");
    cmdParser.addOption(waitAfterFinishedOption);
    cmdParser.parse(QCoreApplication::instance()->arguments());

    ::testing::InitGoogleTest(&argc, argv);

    auto result = RUN_ALL_TESTS();

    TestEnvironment::release();

    if (cmdParser.isSet(waitAfterFinishedOption))
    {
        getchar();
    }

    return result;
}
