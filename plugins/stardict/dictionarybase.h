/******************************************************************************
 * This file is part of the MULA project
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

#ifndef MULA_PLUGIN_STARDICT_DICTIONARYBASE_H
#define MULA_PLUGIN_STARDICT_DICTIONARYBASE_H

#include <QtCore/QStringList>

namespace MULAPluginStardict
{
    class DictionaryBase
    {
        public:
            DictionaryBase();
            virtual ~DictionaryBase();

            QString wordData(quint32 indexItemOffset, quint32 indexItemSize);
            bool containSearchData();
            bool searchData(const QStringList &searchWords, quint32 indexItemOffset, quint32 indexItemSize, QString originalData);

        private:
            class Private;
            Private *const d;
    };
}

#endif // MULA_PLUGIN_STARDICT_DICTIONARYBASE_H