/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitSourcesBackend.h"
#include <QStandardItemModel>
#include <KLocalizedString>
#include <KDesktopFile>
#include <PackageKit/Daemon>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <resources/DiscoverAction.h>
#include <resources/SourcesModel.h>
#include <resources/AbstractResourcesBackend.h>
#include "PackageKitBackend.h"
#include "config-paths.h"

class PKSourcesModel : public QStandardItemModel
{
public:
    PKSourcesModel(PackageKitSourcesBackend* backend)
        : QStandardItemModel(backend)
        , m_backend(backend) {}

    bool setData(const QModelIndex & index, const QVariant & value, int role) override {
        auto item = itemFromIndex(index);
        if (!item)
            return false;

        switch(role) {
            case Qt::CheckStateRole: {
                auto transaction = PackageKit::Daemon::global()->repoEnable(item->data(AbstractSourcesBackend::IdRole).toString(), value.toInt() == Qt::Checked);
                connect(transaction, &PackageKit::Transaction::errorCode, m_backend, &PackageKitSourcesBackend::transactionError);
                return true;
            }
        }
        item->setData(value, role);
        return true;
    }

private:
    PackageKitSourcesBackend* m_backend;
};

static DiscoverAction* createActionForService(const QString &servicePath, QObject* parent)
{
    DiscoverAction* action = new DiscoverAction(parent);
    KDesktopFile parser(servicePath);
    action->setIcon(QIcon::fromTheme(parser.readIcon()));
    action->setText(parser.readName());
    action->setToolTip(parser.readComment());
    QObject::connect(action, &DiscoverAction::triggered, action, [servicePath](){
        bool b = QProcess::startDetached(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/discover/runservice"), {servicePath});
        if (!b)
            qWarning() << "Could not start" << servicePath;
    });
    return action;
}

PackageKitSourcesBackend::PackageKitSourcesBackend(AbstractResourcesBackend* parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new PKSourcesModel(this))
{
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::repoListChanged, this, &PackageKitSourcesBackend::resetSources);
    connect(SourcesModel::global(), &SourcesModel::showingNow, this, &PackageKitSourcesBackend::resetSources);

    // Kubuntu-based
    auto addNativeSourcesManager = [this](const QString &file){
        auto service = PackageKitBackend::locateService(file);
        if (!service.isEmpty())
            m_actions += QVariant::fromValue<QObject*>(createActionForService(service, this));
        };

    //New Ubuntu
    addNativeSourcesManager(QStringLiteral("software-properties-qt.desktop"));

    //Old Ubuntu
    addNativeSourcesManager(QStringLiteral("software-properties-kde.desktop"));

    //OpenSuse
    addNativeSourcesManager(QStringLiteral("YaST2/sw_source.desktop"));
}

QString PackageKitSourcesBackend::idDescription()
{
    return i18n("Repository URL:");
}

QStandardItem* PackageKitSourcesBackend::findItemForId(const QString &id) const
{
    for(int i=0, c=m_sources->rowCount(); i<c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(AbstractSourcesBackend::IdRole).toString() == id)
            return it;
    }
    return nullptr;
}

void PackageKitSourcesBackend::addRepositoryDetails(const QString &id, const QString &description, bool enabled)
{
    bool add = false;
    QStandardItem* item = findItemForId(id);

    if (!item) {
        item = new QStandardItem(description);
        if (PackageKit::Daemon::backendName() == QLatin1String("aptcc")) {
            QRegularExpression exp(QStringLiteral("^/etc/apt/sources.list.d/(.+?).list:.*"));

            auto matchIt = exp.globalMatch(id);
            if (matchIt.hasNext()) {
                auto match = matchIt.next();
                item->setData(match.captured(1), Qt::ToolTipRole);
            }
        }
        add = true;
    }
    item->setData(id, IdRole);
    item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);

    if (add)
        m_sources->appendRow(item);
}

QAbstractItemModel * PackageKitSourcesBackend::sources()
{
    return m_sources;
}

bool PackageKitSourcesBackend::addSource(const QString& /*id*/)
{
    return false;
}

bool PackageKitSourcesBackend::removeSource(const QString& id)
{
    auto transaction = PackageKit::Daemon::global()->repoRemove(id, false);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    return false;
}

QVariantList PackageKitSourcesBackend::actions() const
{
    return m_actions;
}

void PackageKitSourcesBackend::resetSources()
{
    disconnect(SourcesModel::global(), &SourcesModel::showingNow, this, &PackageKitSourcesBackend::resetSources);
    m_sources->clear();
    auto transaction = PackageKit::Daemon::global()->getRepoList();
    connect(transaction, &PackageKit::Transaction::repoDetail, this, &PackageKitSourcesBackend::addRepositoryDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
}

void PackageKitSourcesBackend::transactionError(PackageKit::Transaction::Error error, const QString& message)
{
    Q_EMIT passiveMessage(message);
    qWarning() << "Transaction error: " << error << message << sender();
}
