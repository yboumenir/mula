/******************************************************************************
 * This file is part of the Mula project
 * Copyright (c) 2011 Laszlo Papp <lpapp@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MULA_CORE_STARDICTDICTIONARYINFOTEST_H
#define MULA_CORE_STARDICTDICTIONARYINFOTEST_H

#include <QtCore/QObject>

class StarDictDictionaryInfoTest : public QObject
{
        Q_OBJECT

    public:
        StarDictDictionaryInfoTest();
        virtual ~StarDictDictionaryInfoTest();

    private Q_SLOTS:
        void testIfoFilePath();
        void testWordCount();
        void testBookName();
        void testAuthor();
        void testEmail();
        void testWebsite();
        void testDateTime();
        void testDescription();
        void testIndexFileSize();
        void testIndexOffsetBits();
        void testSameTypeSequence();
};

#endif // MULA_CORE_STARDICTDICTIONARYINFOTEST_H

