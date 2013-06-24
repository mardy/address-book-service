/*
 * Copyright 2013 Canonical Ltd.
 *
 * This file is part of contact-service-app.
 *
 * contact-service-app is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * contact-service-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "view.h"
#include "view-adaptor.h"
#include "contacts-map.h"
#include "qindividual.h"

#include "common/vcard-parser.h"
#include "common/filter.h"
#include "common/dbus-service-defs.h"

#include <QtContacts/QContact>

#include <QtVersit/QVersitDocument>

using namespace QtContacts;
using namespace QtVersit;

namespace galera
{
class ContactLessThan
{
public:
    ContactLessThan(const SortClause &sortClause)
        : m_sortClause(sortClause)
    {

    }

    bool operator()(galera::ContactEntry *entryA, galera::ContactEntry *entryB)
    {
        return QContactManagerEngine::compareContact(entryA->individual()->contact(),
                                                     entryB->individual()->contact(),
                                                     m_sortClause.toContactSortOrder()) < 0;
    }
private:
    SortClause m_sortClause;
};

class FilterThread: public QThread
{
public:
    FilterThread(QString filter, QString sort, ContactsMap *allContacts)
        : m_filter(filter),
          m_sortClause(sort),
          m_allContacts(allContacts)
    {
    }

    QList<ContactEntry*> result() const
    {
        if (isRunning()) {
            return QList<ContactEntry*>();
        } else {
            return m_contacts;
        }
    }

    bool appendContact(ContactEntry *entry)
    {
        if (checkContact(entry)) {
            m_contacts << entry;
            return true;
        }
        return false;
    }

    bool removeContact(ContactEntry *entry)
    {
        return m_contacts.removeAll(entry);
    }

    void chageSort(SortClause clause)
    {
        m_sortClause = clause;
        ContactLessThan lessThan(m_sortClause);
        qSort(m_contacts.begin(), m_contacts.end(), lessThan);
    }

protected:
    void run()
    {
        Q_FOREACH(ContactEntry *entry, m_allContacts->values())
        {
            if (checkContact(entry)) {
                m_contacts << entry;
            }
        }

        chageSort(m_sortClause);
    }

private:
    Filter m_filter;
    SortClause m_sortClause;
    ContactsMap *m_allContacts;
    QList<ContactEntry*> m_contacts;

    bool checkContact(ContactEntry *entry)
    {
        return m_filter.test(entry->individual()->contact());
    }
};

View::View(QString clause, QString sort, QStringList sources, ContactsMap *allContacts, QObject *parent)
    : QObject(parent),
      m_sources(sources),
      m_filterThread(new FilterThread(clause, sort, allContacts)),
      m_adaptor(0)
{
    m_filterThread->start();
}

View::~View()
{
    close();
}

void View::close()
{
    if (m_adaptor) {
        Q_EMIT m_adaptor->contactsRemoved(0, m_filterThread->result().count());
        Q_EMIT closed();

        QDBusConnection conn = QDBusConnection::sessionBus();
        unregisterObject(conn);
        m_adaptor->deleteLater();
        m_adaptor = 0;
    }

    delete m_filterThread;
    m_filterThread = 0;
}

QString View::contactDetails(const QStringList &fields, const QString &id)
{
    return QString();
}

QStringList View::contactsDetails(const QStringList &fields, int startIndex, int pageSize, const QDBusMessage &message)
{
    m_filterThread->wait();
    QList<ContactEntry*> entries = m_filterThread->result();

    if (startIndex < 0) {
        startIndex = 0;
    }

    if ((pageSize < 0) || ((startIndex + pageSize) >= entries.count())) {
        pageSize = entries.count() - startIndex;
    }

    QList<QContact> contacts;
    for(int i = startIndex, iMax = (startIndex + pageSize); i < iMax; i++) {
        // TODO: filter fields
        contacts << entries[i]->individual()->contact();
    }

    QStringList ret =  VCardParser::contactToVcard(contacts);
    qDebug() << "fetch" << ret;
    QList<QContact> cs = VCardParser::vcardToContact(ret);
    QStringList ret2 =  VCardParser::contactToVcard(cs);
    qDebug() << "fetch2" << ret2;

    QDBusMessage reply = message.createReply(ret);
    QDBusConnection::sessionBus().send(reply);
    return ret;
}

int View::count()
{
    m_filterThread->wait();

    return m_filterThread->result().count();
}

void View::sort(const QString &field)
{
    m_filterThread->chageSort(SortClause(field));
}

QString View::objectPath()
{
    return CPIM_ADDRESSBOOK_VIEW_OBJECT_PATH;
}

QString View::dynamicObjectPath() const
{
    return objectPath() + "/" + QString::number((long)this);
}

bool View::registerObject(QDBusConnection &connection)
{
    if (!m_adaptor) {
        m_adaptor = new ViewAdaptor(connection, this);
        if (!connection.registerObject(dynamicObjectPath(), this))
        {
            qWarning() << "Could not register object!" << objectPath();
            delete m_adaptor;
            m_adaptor = 0;
        } else {
            qDebug() << "Object registered:" << objectPath();
        }
    }

    return (m_adaptor != 0);
}

void View::unregisterObject(QDBusConnection &connection)
{
    if (m_adaptor) {
        connection.unregisterObject(dynamicObjectPath());
    }
}

bool View::appendContact(ContactEntry *entry)
{
    return m_filterThread->appendContact(entry);
}

QObject *View::adaptor() const
{
    return m_adaptor;
}

} //namespace
