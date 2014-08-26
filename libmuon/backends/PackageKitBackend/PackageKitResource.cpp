/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "PackageKitResource.h"
#include "PackageKitBackend.h"
#include "PackageKitUtils.h"
#include <MuonDataSources.h>
#include <KGlobal>
#include <KLocale>
#include <packagekitqt5/Details>
#include <packagekitqt5/Daemon>

PackageKitResource::PackageKitResource(const QString &packageId, PackageKit::Transaction::Info info, const QString &summary, PackageKitBackend* parent)
    : AbstractResource(parent)
    , m_backend(parent)
    , m_info(info)
    , m_summary(summary)
    , m_size(0)
    , m_gotDetails(false)
{
    addPackageId(info, packageId, summary);
    /*if (info == PackageKit::Transaction::InfoInstalled) {
        m_installedPackageId = packageId;
        m_installedVersion = PackageKit::Daemon::global()->packageVersion(m_availablePackageId);
    }
    if (!m_availablePackageId.isEmpty()) {
        m_name = PackageKit::Daemon::global()->packageName(m_availablePackageId);
        m_icon = PackageKit::Daemon::global()->packageIcon(m_availablePackageId);
        m_availableVersion = PackageKit::Daemon::global()->packageVersion(m_availablePackageId);
    }*/
    setObjectName(m_availablePackageId);
}

QString PackageKitResource::name()
{
    return m_name;
}

QString PackageKitResource::packageName() const
{
    return m_name;
}

QString PackageKitResource::availablePackageId() const
{
    return m_availablePackageId;
}

QString PackageKitResource::installedPackageId() const
{
    return m_installedPackageId;
}

QString PackageKitResource::comment()
{
    return m_summary;
}

QString PackageKitResource::longDescription()
{
    return m_detail;
}

QUrl PackageKitResource::homepage()
{
    return m_url;
}

QString PackageKitResource::icon() const
{
    return m_icon;
}

QString PackageKitResource::license()
{
    return m_license;
}

QList<PackageState> PackageKitResource::addonsInformation()
{
    return QList<PackageState>();
}

QString PackageKitResource::availableVersion() const
{
    return m_availableVersion;
}

QString PackageKitResource::installedVersion() const
{
    return m_installedVersion;
}

int PackageKitResource::downloadSize()
{
    return m_size;
}

QString PackageKitResource::origin() const
{
    //TODO
    return "PackageKit";
}

QString PackageKitResource::section()
{
    return QString();
}

QUrl PackageKitResource::screenshotUrl()
{
    return QUrl(MuonDataSources::screenshotsSource().toString() + "/screenshot/" + packageName());
}

QUrl PackageKitResource::thumbnailUrl()
{
    return QUrl(MuonDataSources::screenshotsSource().toString() + "/thumbnail/" + packageName());
}

AbstractResource::State PackageKitResource::state()
{
    if (availableVersion() != installedVersion() && !installedVersion().isEmpty() && !availableVersion().isEmpty())
        return Upgradeable;
    switch(m_info) {
        case PackageKit::Transaction::InfoInstalled:
            return Installed;
        case PackageKit::Transaction::InfoAvailable:
            return None;
        default:
            break;
    };
    return Broken;
}

void PackageKitResource::resetPackageIds()
{
    m_info = PackageKit::Transaction::InfoUnknown;
    m_availablePackageId = QString();
    m_installedPackageId = QString();
}

void PackageKitResource::addPackageId(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary)
{
    if (info == PackageKit::Transaction::InfoUnknown)
        qWarning() << "Received unknown PackageKit::Transaction::Info for " << name();

    bool changeState = (info != m_info);

//     TODO: only fetch the metadata from packagekit if it's not an appstream package
    if (m_availablePackageId.isEmpty()) {
        m_name = PackageKit::Daemon::global()->packageName(packageId);
        m_icon = PackageKit::Daemon::global()->packageIcon(packageId);
        m_availablePackageId = packageId;
        m_availableVersion = PackageKit::Daemon::global()->packageVersion(packageId);
        m_info = info;
    }
    if (info == PackageKit::Transaction::InfoInstalled) {
        m_installedPackageId = packageId;
        m_info = info;
        m_installedVersion = PackageKit::Daemon::global()->packageVersion(packageId);
        if (!PackageKit::Daemon::global()->filters().testFlag(PackageKit::Transaction::FilterNewest) &&
             PackageKitUtils::compare_versions(PackageKit::Daemon::global()->packageVersion(packageId), m_availableVersion) > 0) {
            m_availablePackageId = packageId;
            m_availableVersion = PackageKit::Daemon::global()->packageVersion(packageId);
            m_gotDetails = false;
        }
    } else if (PackageKit::Daemon::global()->filters().testFlag(PackageKit::Transaction::FilterNewest) ||
               PackageKitUtils::compare_versions(PackageKit::Daemon::global()->packageVersion(packageId), m_availableVersion) > 0) {
        m_availablePackageId = packageId;
        m_availableVersion = PackageKit::Daemon::global()->packageVersion(packageId);
        if (m_installedVersion == m_availableVersion && info != PackageKit::Transaction::InfoInstalled) { //This case will happen when we have a package installed and remove it
            qWarning() << "Caught the case of adding a package id which was previously installed and now is not anymore";
            m_info = info;
            m_installedPackageId = QString();
            m_installedVersion = QString();
        }
        m_gotDetails = false;
    }
    if (m_summary.isEmpty())
        m_summary = summary;
    
    if (changeState) {
        //kDebug() << "State changed" << m_info;
        emit stateChanged();
    }
}

QStringList PackageKitResource::categories()
{
    /*fetchDetails();
    QStringList categories;
    switch (m_group) {
        case PackageKit::Transaction::GroupUnknown:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupAccessibility:
            categories << "Accessibility";
            break;
        case PackageKit::Transaction::GroupAccessories:
            categories << "Utility";
            break;
        case PackageKit::Transaction::GroupAdminTools:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupCommunication:
            categories << "Chat";
            break;
        case PackageKit::Transaction::GroupDesktopGnome:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopKde:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopOther:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupDesktopXfce:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupEducation:
            categories << "Science";
            break;
        case PackageKit::Transaction::GroupFonts:
            categories << "Fonts";
            break;
        case PackageKit::Transaction::GroupGames:
            categories << "Games";
            break;
        case PackageKit::Transaction::GroupGraphics:
            categories << "Graphics";
            break;
        case PackageKit::Transaction::GroupInternet:
            categories << "Internet";
            break;
        case PackageKit::Transaction::GroupLegacy:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupLocalization:
            categories << "Localization";
            break;
        case PackageKit::Transaction::GroupMaps:
            categories << "Geography";
            break;
        case PackageKit::Transaction::GroupMultimedia:
            categories << "Multimedia";
            break;
        case PackageKit::Transaction::GroupNetwork:
            categories << "Network";
            break;
        case PackageKit::Transaction::GroupOffice:
            categories << "Office";
            break;
        case PackageKit::Transaction::GroupOther:
            categories << "Unknown";
            break;
        case PackageKit::Transaction::GroupPowerManagement:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupProgramming:
            categories << "Development";
            break;
        case PackageKit::Transaction::GroupPublishing:
            categories << "Publishing";
            break;
        case PackageKit::Transaction::GroupRepos:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupSecurity:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupServers:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupSystem:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupVirtualization:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupScience:
            categories << "Science";
            break;
        case PackageKit::Transaction::GroupDocumentation:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupElectronics:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupCollections:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupVendor:
            categories << "System";
            break;
        case PackageKit::Transaction::GroupNewest:
            categories << "System";
            break;
    };
    return categories;*/
    return QStringList() << "Unknown";
    //NOTE: I commented the category fetching code, as it seems to get called even for non-technical items
    //when selecting a category, and receiving details for all packages takes about 20 mins in my VirtualBox and probably not much less on real systems
}

bool PackageKitResource::isTechnical() const
{
    return true;//!m_availablePackageId.startsWith("flash");
}

void PackageKitResource::setDetails(const PackageKit::Details & details)
{
    if (details.packageId() != m_availablePackageId)
        return;
    qDebug() << "Got details for" << m_availablePackageId;
    bool newLicense = (details.license() != m_license);
    m_license = details.license();
    m_group = details.group();
    m_detail = details.description();
    m_url = details.url();
    m_size = details.size();
    if (newLicense)
        emit licenseChanged();
}

void PackageKitResource::fetchChangelog()
{
    //TODO: implement
    emit changelogFetched(QString());
}
