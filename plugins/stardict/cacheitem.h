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

#ifndef MULA_PLUGIN_STARDICT_CACHEITEM_H
#define MULA_PLUGIN_STARDICT_CACHEITEM_H

#include <QtCore/QByteArray>

namespace MulaPluginStarDict
{
    class CacheItem
    {
        public:
            CacheItem();
            virtual ~CacheItem();

            void setData(char *data);
            void setData(QByteArray data);
            QByteArray& data() const;

            void setOffset(quint32 offset);
            quint32 offset() const;

            void setDataSize(quint32 dataSize);
            quint32 dataSize() const;

        private:
            class Private;
            Private *const d;
    };
}

#endif // MULA_PLUGIN_STARDICT_CACHEITEM_H
