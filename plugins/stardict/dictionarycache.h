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

#ifndef MULA_PLUGIN_STARDICT_DICTIONARYCACHE_H
#define MULA_PLUGIN_STARDICT_DICTIONARYCACHE_H

#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>

namespace MulaPluginStarDict
{
    class DictionaryCache
    {
        public:
            DictionaryCache();
            DictionaryCache(int chunk, QByteArray byteArray, int stamp, int count);
            DictionaryCache(const DictionaryCache &other);
            virtual ~DictionaryCache();

            DictionaryCache& operator=(const DictionaryCache &other);

            void setChunk(int chunk);
            int chunk() const;

            void setByteArray(QByteArray inByteArray);
            QByteArray byteArray() const;

            void setStamp(int stamp);
            int stamp() const;

            void setCount(int count);
            int count() const;

        private:
            class Private;
            QSharedDataPointer<Private> d;
    };
}

#endif // MULA_PLUGIN_STARDICT_DICTIONARYCACHE_H
