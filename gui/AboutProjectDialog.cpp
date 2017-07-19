/*
 * GeohazardVis plug-in: pCDM Modeling
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

#include "AboutProjectDialog.h"
#include "ui_AboutProjectDialog.h"

#include <core/config.h>
#include <core/VersionInfo.h>


AboutProjectDialog::AboutProjectDialog(QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui{ std::make_unique<Ui_AboutProjectDialog>() }
{
    m_ui->setupUi(this);

    const QString header = R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">
<html><head><meta name="qrichtext" content="1" /><style type="text/css">
p, li { white-space: pre-wrap; }
</style></head><body style=" font-family:'Cantarell'; font-size:11pt; font-weight:400; font-style:normal;">
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-weight:600;">)";
    const QString preVersion = R"(</span></p>
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">)";
    const QString preAbout = R"(</p>
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">)";
    const QString preUrl = R"(</p>
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><a href=")";
    const QString preUrlTitle = R"("><span style=" text-decoration: underline; color:#0000ff;">)";
    const QString preCopyright = R"(</span></a></p>
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">)";
    const QString preGitRev = R"(</p>
<p align="center" style="-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><br /></p>
<p align="center" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">Git Revision: )";
    const QString footer = R"(</p></body></html>)";


    const auto & projectInfo = VersionInfo::projectInfo();

    const QString versionString = QString("%1.%2.%3")
        .arg(projectInfo.major)
        .arg(projectInfo.minor)
        .arg(projectInfo.patch);
    const QString aboutString = "A visualization and modeling tool for analysis of volcanic hazards";
    const QString copyright = QString(R"(Copyright %1 %2 %3 (<a href="%4">%4</a>))")
        .arg(QChar(0x24B8)) // (C)
        .arg(2017)
        .arg(config::metaProjectMaintainer)
        .arg(config::metaProjectMaintainerEmail);
    const QString revDate = QString("%1 (%2)")
        .arg(projectInfo.gitRevision)
        .arg(projectInfo.gitCommitDate.date().toString(Qt::DefaultLocaleLongDate));

    m_ui->aboutTextEdit->setHtml(
        header + config::metaProjectName
        + preVersion + versionString
        + preAbout + aboutString
        + preUrl + config::metaProjectDomain
        + preUrlTitle + config::metaProjectDomain
        + preCopyright + copyright
        + preGitRev + revDate
        + footer);


    QString thirdPartyString;
    for (auto && thirdParty : VersionInfo::thirdPartiesWithVersionInfo())
    {
        auto && info = VersionInfo::versionInfoFor(thirdParty);
        thirdPartyString += QString(
            " - %1 (Version %2.%3.%4)\n"
            "\tRevision: %5\n"
            "\tDate: %6\n")
            .arg(thirdParty)
            .arg(info.major)
            .arg(info.minor)
            .arg(info.patch)
            .arg(info.gitRevision)
            .arg(info.gitCommitDate.date().toString());
    }
    for (auto && thirdParty : VersionInfo::otherThirdParties())
    {
        thirdPartyString += " - " + thirdParty + "\n";
    }
    thirdPartyString += "\n";
    thirdPartyString += QString(R"(Plese refer to the ")") + config::installThirdPartyLicensePath +
        R"(" folder in the appplication folder for further information and licenses.)";

    m_ui->thirdPartyEdit->setText(thirdPartyString);
}

AboutProjectDialog::~AboutProjectDialog() = default;
