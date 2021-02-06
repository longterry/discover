/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QCommandLineParser>
#include <KLocalizedString>
#include <KAboutData>
#include <QIcon>
#include "MuonExporter.h"
#include <DiscoverBackendsFactory.h>
#include "../DiscoverVersion.h"

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("plasma-discover-exporter");
    KAboutData about(QStringLiteral("muonexporter"), i18n("Muon Exporter"), version, QString(),
                     KAboutLicense::GPL, i18n("©2013 Aleix Pol Gonzalez"), QString());
    about.addAuthor(i18n("Jonathan Thomas"), QString(), QStringLiteral("echidnaman@kubuntu.org"));
    about.addAuthor(i18n("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@blue-systems.com"));
    about.setProductName("discover/exporter");

    MuonExporter exp;
    {
        QCommandLineParser parser;
        parser.addPositionalArgument(QStringLiteral("file"), i18n("File to which we'll export"));
        DiscoverBackendsFactory::setupCommandLine(&parser);
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        DiscoverBackendsFactory::processCommandLine(&parser, false);

        if(parser.positionalArguments().count() != 1) {
            parser.showHelp(1);
        }
        exp.setExportPath(QUrl::fromUserInput(parser.positionalArguments().at(0), QString(), QUrl::AssumeLocalFile));
    }

    QObject::connect(&exp, &MuonExporter::exportDone, &app, &QCoreApplication::quit);

    return app.exec();
}
