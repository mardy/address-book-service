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

#include "base-client-test.h"
#include "lib/source.h"
#include "common/dbus-service-defs.h"
#include "common/vcard-parser.h"

#include <QObject>
#include <QtDBus>
#include <QtTest>
#include <QDebug>
#include <QtVersit>

class AddressBookTest : public BaseClientTest
{
    Q_OBJECT
private:
    QString m_basicVcard;
    QString m_resultBasicVcard;

    QtContacts::QContact basicContactWithId(const QString &id)
    {
        QString newVcard = m_resultBasicVcard.arg(id);
        QList<QtContacts::QContact> contacts = galera::VCardParser::vcardToContact(QStringList() << newVcard);
        return contacts[0];
    }

    void compareContact(const QtContacts::QContact &contact, const QtContacts::QContact &other)
    {
        // id
        QCOMPARE(contact.id(), other.id());

        // name
        QCOMPARE(contact.detail(QtContacts::QContactDetail::TypeName),
                 other.detail(QtContacts::QContactDetail::TypeName));

        // phone - this is necessary because:
        //    1 ) the QContactDetail::FieldDetailUri can change based on the detail order
        //    2 ) the phone number can be returned in different order
        QList<QtContacts::QContactDetail> phones = contact.details(QtContacts::QContactDetail::TypePhoneNumber);
        QList<QtContacts::QContactDetail> otherPhones = other.details(QtContacts::QContactDetail::TypePhoneNumber);
        QCOMPARE(phones.size(), otherPhones.size());
        for(int i=0; i < phones.size(); i++) {
            QtContacts::QContactDetail phone = phones[i];
            bool found = false;
            for(int x=0; x < otherPhones.size(); x++) {
                QtContacts::QContactDetail otherPhone = otherPhones[x];
                if (phone.value(QtContacts::QContactPhoneNumber::FieldNumber) ==
                    otherPhone.value(QtContacts::QContactPhoneNumber::FieldNumber)) {
                    found = true;
                    QList<int> phoneTypes = phone.value(QtContacts::QContactPhoneNumber::FieldSubTypes).value< QList<int> >();
                    QList<int> otherPhoneTypes = otherPhone.value(QtContacts::QContactPhoneNumber::FieldSubTypes).value< QList<int> >();
                    QCOMPARE(phoneTypes, otherPhoneTypes);
                    QCOMPARE(phone.value(QtContacts::QContactPhoneNumber::FieldContext),
                         otherPhone.value(QtContacts::QContactPhoneNumber::FieldContext));
                    break;
                }
            }
            QVERIFY2(found, "Phone number is not equal");
        }

        // email same as phone number
        QList<QtContacts::QContactDetail> emails = contact.details(QtContacts::QContactDetail::TypeEmailAddress);
        QList<QtContacts::QContactDetail> otherEmails = other.details(QtContacts::QContactDetail::TypeEmailAddress);
        QCOMPARE(emails.size(), otherEmails.size());
        for(int i=0; i < emails.size(); i++) {
            QtContacts::QContactDetail email = emails[i];
            bool found = false;
            for(int x=0; x < otherEmails.size(); x++) {
                QtContacts::QContactDetail otherEmail = otherEmails[x];
                if (email.value(QtContacts::QContactEmailAddress::FieldEmailAddress) ==
                    otherEmail.value(QtContacts::QContactEmailAddress::FieldEmailAddress)) {
                    found = true;
                    QCOMPARE(email.value(QtContacts::QContactEmailAddress::FieldContext),
                             otherEmail.value(QtContacts::QContactEmailAddress::FieldContext));
                    break;
                }
            }
            QVERIFY2(found, "Email is not equal");
        }
    }

private Q_SLOTS:
    void initTestCase()
    {
        BaseClientTest::initTestCase();
        m_basicVcard = QStringLiteral("BEGIN:VCARD\n"
                                      "VERSION:3.0\n"
                                      "N:Tal;Fulano_;de;;\n"
                                      "EMAIL:fulano_@ubuntu.com\n"
                                      "TEL;PID=1.1;TYPE=ISDN:33331410\n"
                                      "TEL;PID=1.2;TYPE=CELL:8888888\n"
                                      "END:VCARD");

        m_resultBasicVcard = QStringLiteral("BEGIN:VCARD\r\n"
                                       "VERSION:3.0\r\n"
                                       "UID:%1\r\n"
                                       "CLIENTPIDMAP:1;dummy:dummy-store:0\r\n"
                                       "N;PID=1.1:Tal;Fulano_;de;;\r\n"
                                       "FN;PID=1.1:Fulano_ Tal\r\n"
                                       "X-QTPROJECT-FAVORITE;PID=1.1:false;0\r\n"
                                       "EMAIL;PID=1.1:fulano_@ubuntu.com\r\n"
                                       "TEL;PID=1.1;TYPE=ISDN:33331410\r\n"
                                       "TEL;PID=1.2;TYPE=CELL:8888888\r\n"
                                       "END:VCARD\r\n");
    }

    void testSortFields()
    {
        QStringList defaultSortFields;
        defaultSortFields << "ADDR_COUNTRY"
                          << "ADDR_LOCALITY"
                          << "ADDR_POSTCODE"
                          << "ADDR_POST_OFFICE_BOX"
                          << "ADDR_REGION"
                          << "ADDR_STREET"
                          << "BIRTHDAY"
                          << "EMAIL"
                          << "FIRST_NAME"
                          << "FULL_NAME"
                          << "IM_PROTOCOL"
                          << "IM_URI"
                          << "LAST_NAME"
                          << "MIDLE_NAME"
                          << "NAME_PREFIX"
                          << "NAME_SUFFIX"
                          << "NICKNAME"
                          << "ORG_DEPARTMENT"
                          << "ORG_LOCATION"
                          << "ORG_NAME"
                          << "ORG_ROLE"
                          << "ORG_TITLE"
                          << "PHONE"
                          << "PHOTO"
                          << "URL";
        QDBusReply<QStringList> reply = m_serverIface->call("sortFields");
        QCOMPARE(reply.value(), defaultSortFields);
    }

    void testSource()
    {
        QDBusReply<galera::Source> reply = m_serverIface->call("source");
        QVERIFY(reply.isValid());
        QCOMPARE(reply.value().id(), QStringLiteral("dummy-store"));
    }

    void testAvailableSources()
    {
        QDBusReply<QList<galera::Source> > reply = m_serverIface->call("availableSources");
        galera::SourceList list = reply.value();
        QCOMPARE(list.count(), 1);
        galera::Source src = list[0];
        QCOMPARE(src.id(), QStringLiteral("dummy-store"));
    }

    void testCreateContact()
    {
        // spy 'contactsAdded' signal
        QSignalSpy addedContactSpy(m_serverIface, SIGNAL(contactsAdded(const QStringList &)));

        // call create contact
        QDBusReply<QString> reply = m_serverIface->call("createContact", m_basicVcard, "dummy-store");

        // check if the returned id is valid
        QString newContactId = reply.value();
        QVERIFY(!newContactId.isEmpty());

        // check if the cotact was created with the correct fields
        QtContacts::QContact newContact = basicContactWithId(newContactId);
        QDBusReply<QStringList> reply2 = m_dummyIface->call("listContacts");
        QCOMPARE(reply2.value().count(), 1);
        QList<QtContacts::QContact> contactsCreated = galera::VCardParser::vcardToContact(reply2.value());
        QCOMPARE(contactsCreated.count(), 1);
        compareContact(contactsCreated[0], newContact);

        // wait for folks to emit the signal
        QTest::qWait(500);

        // check if the signal "contactAdded" was fired
        QCOMPARE(addedContactSpy.count(), 1);
        QList<QVariant> args = addedContactSpy.takeFirst();
        QCOMPARE(args.count(), 1);
        QStringList ids = args[0].toStringList();
        QCOMPARE(ids[0], newContactId);
    }

    void testDuplicateContact()
    {
        // spy 'contactsAdded' signal
        QSignalSpy addedContactSpy(m_serverIface, SIGNAL(contactsAdded(const QStringList &)));

        // call create contact first
        QDBusReply<QString> reply = m_serverIface->call("createContact", m_basicVcard, "dummy-store");

        // wait for folks to emit the signal
        QTest::qWait(500);

        // user returned id to fill the new vcard
        QString newContactId = reply.value();
        QString newVcard = m_resultBasicVcard.arg(newContactId);

        // try create a contact with the same id
        QDBusReply<QString> reply2 = m_serverIface->call("createContact", newVcard, "dummy-store");

        // wait for folks to emit the signal
        QTest::qWait(500);

        // result should be null
        QVERIFY(reply2.value().isEmpty());

        // contactsAdded should be fired only once
        QCOMPARE(addedContactSpy.count(), 1);
    }

    void testCreateInvalidContact()
    {
        // spy 'contactsAdded' signal
        QSignalSpy addedContactSpy(m_serverIface, SIGNAL(contactsAdded(const QStringList &)));

        // call create contact with a invalid vcard string
        QDBusReply<QString> reply = m_serverIface->call("createContact", "INVALID VCARD", "dummy-store");

        // wait for folks to emit the signal
        QTest::qWait(500);

        QVERIFY(reply.value().isEmpty());
        QCOMPARE(addedContactSpy.count(), 0);
    }

    void testRemoveContact()
    {
        // create a basic contact
        QDBusReply<QString> replyAdd = m_serverIface->call("createContact", m_basicVcard, "dummy-store");
        QString newContactId = replyAdd.value();

        // spy 'contactsRemoved' signal
        QSignalSpy removedContactSpy(m_serverIface, SIGNAL(contactsRemoved(const QStringList &)));

        // try remove the contact created
        QDBusReply<int> replyRemove = m_serverIface->call("removeContacts", QStringList() << newContactId);
        QCOMPARE(replyRemove.value(), 1);

        // check if the 'contactsRemoved' signal was fired with the correct args
        QTRY_COMPARE(removedContactSpy.count(), 1);
        QList<QVariant> args = removedContactSpy.takeFirst();
        QCOMPARE(args.count(), 1);
        QStringList ids = args[0].toStringList();
        QCOMPARE(ids[0], newContactId);

        // check if the contact was removed from the backend
        QDBusReply<QStringList> replyList = m_dummyIface->call("listContacts");
        QCOMPARE(replyList.value().count(), 0);
    }
};

QTEST_MAIN(AddressBookTest)

#include "addressbook-test.moc"