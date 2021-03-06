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

#include "stardictdictionarymanager.h"

#include "distance.h"
#include "dictionary.h"
#include "file.h"

#include <QtCore/QtAlgorithms>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <zlib.h>

using namespace MulaPluginStarDict;

// Notice: read src/tools/DICTFILE_FORMAT for the dictionary
// file's format information!

static bool
isVowel(QChar inputChar)
{
    QChar ch = inputChar.toLower();
    return ( ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' );
}

static bool
isPureEnglish(const QByteArray& word)
{
    foreach (char c, word)
    {
        //if(str[i]<0)
        //if(str[i]<32 || str[i]>126) // tab equal 9,so this is not OK.
        // Better use isascii() but not str[i]<0 while char is default unsigned in arm
        if (!isascii(c))
            return false;
    }

    return true;
}

class StarDictDictionaryManager::Private
{
    public:
        Private()
           : found(false)
        {
        }

        ~Private()
        {
        }

        QList<Dictionary *> dictionaryList;
        progress_func_t progressFunction;

        QList<Dictionary *> previous;
        QList<Dictionary *> next;

        bool found;
        static const int maxMatchItemPerLib = 100;
        static const int maximumFuzzyDistance = 3; // at most MAX_FUZZY_DISTANCE-1 differences allowed when find similar words

};

StarDictDictionaryManager::StarDictDictionaryManager(progress_func_t progressFunction)
    : d(new Private)
{
    d->progressFunction = progressFunction;
}

StarDictDictionaryManager::~StarDictDictionaryManager()
{
    qDeleteAll(d->dictionaryList);
    delete d;
}

bool
StarDictDictionaryManager::loadDictionary(const QString& ifoFilePath)
{
    Dictionary *dictionary = new Dictionary;
    if (dictionary->load(ifoFilePath))
    {
        d->dictionaryList.append(dictionary);
    }
    else
    {
        qDebug() << "Could not load the dictionary according to the given ifo"
            "file:" << ifoFilePath;
        delete dictionary;
        return false;
    }

    return true;
}

long
StarDictDictionaryManager::articleCount(int index) const
{
    Q_ASSERT_X( index >= 0 && index < dictionaryCount(), Q_FUNC_INFO, "index out of range in list of dictionaries" );
    return d->dictionaryList.at(index)->articleCount();
}

QString
StarDictDictionaryManager::dictionaryName(int index) const
{
    Q_ASSERT_X( index >= 0 && index < dictionaryCount(), Q_FUNC_INFO, "index out of range in list of dictionaries" );
    return d->dictionaryList.at(index)->dictionaryName();
}

int
StarDictDictionaryManager::dictionaryCount() const
{
    return d->dictionaryList.size();
}

QByteArray
StarDictDictionaryManager::key(long keyIndex, int dictionaryIndex) const
{
    Q_ASSERT_X( keyIndex >= 0 && keyIndex < dictionaryCount(), Q_FUNC_INFO, "index out of range in list of dictionaries" );
    return d->dictionaryList.at(dictionaryIndex)->key(keyIndex).toUtf8();
}

QString
StarDictDictionaryManager::data(long dataIndex, int dictionaryIndex)
{
    Q_ASSERT_X( dataIndex >= 0 && dataIndex < dictionaryCount(), Q_FUNC_INFO, "index out of range in list of dictionaries" );
    return d->dictionaryList.at(dictionaryIndex)->data(dataIndex);
}

int
StarDictDictionaryManager::lookupWord(int dictionaryIndex, const QString& searchWord)
{
    Q_ASSERT_X( dictionaryIndex >= 0 && dictionaryIndex < dictionaryCount(), Q_FUNC_INFO, "index out of range in list of dictionaries" );
    return d->dictionaryList.at(dictionaryIndex)->lookup(searchWord);
}

template <typename Method>
void
StarDictDictionaryManager::recursiveTemplateHelper(const QString& directoryName, const QStringList& orderList, const QStringList& disableList,
                                                    Method method)
{
    QDir dir(directoryName);

    // Going through the files
    foreach (const QFileInfo& entryFileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot))
    {
        QString absolutePath = entryFileInfo.absoluteFilePath();

        if (entryFileInfo.isDir()) {
            recursiveTemplateHelper(absolutePath, orderList, disableList, method);
        } else {
            if (absolutePath.endsWith(QLatin1String(".ifo"))
                    && qFind(orderList.begin(), orderList.end(), absolutePath) == orderList.end()
                    && qFind(disableList.begin(), disableList.end(), absolutePath) == disableList.end())
            {
                (this->*method)(absolutePath);
            }
        }
    }
}

void
StarDictDictionaryManager::load(const QStringList& dictionaryDirectoryList,
                                const QStringList& orderList,
                                const QStringList& disableList)
{
    foreach (const QString& absoluteFilePath, orderList)
    {
        if (qFind(disableList.begin(), disableList.end(), absoluteFilePath) == disableList.end())
            loadDictionary(absoluteFilePath);
    }

    foreach (const QString& directoryName, dictionaryDirectoryList)
        recursiveTemplateHelper(directoryName, orderList, disableList, &StarDictDictionaryManager::loadDictionary);
}

Dictionary*
StarDictDictionaryManager::reloaderFind(const QString& completeFilePath)
{
    QList<Dictionary *>::iterator it;
    for (it = d->previous.begin(); it != d->previous.end(); ++it)
    {
        if ((*it)->ifoFilePath() == completeFilePath)
            break;
    }

    if (it != d->previous.end())
    {
        Dictionary *result = *it;
        d->previous.erase(it);
        return result;
    }

    return NULL;
}

void
StarDictDictionaryManager::reloaderHelper(const QString &absolutePath)
{
    Dictionary *dictionary = reloaderFind(absolutePath);
    if (dictionary)
        d->next.append(dictionary);
    else
        loadDictionary(absolutePath);
}

void
StarDictDictionaryManager::reload(const QStringList& dictionaryDirectoryList,
                                  const QStringList& orderList,
                                  const QStringList& disableList)
{
    d->previous = d->dictionaryList;
    d->dictionaryList.clear();

    foreach (const QString& absoluteFilePath, orderList)
    {
        if (qFind(disableList.begin(), disableList.end(), absoluteFilePath) == disableList.end())
            reloaderHelper(absoluteFilePath);
    }

    foreach (const QString& directoryName, dictionaryDirectoryList)
        recursiveTemplateHelper(directoryName, orderList, disableList, &StarDictDictionaryManager::reloaderHelper);

    qDeleteAll(d->previous);
}

QByteArray
StarDictDictionaryManager::poCurrentWord(int *iCurrent)
{
    const char *poCurrentWord = NULL;
    const char *word;

    for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
    {
        if (iCurrent[iLib] == invalidIndex)
            continue;

        if (iCurrent[iLib] >= articleCount(iLib) || iCurrent[iLib] < 0)
            continue;

        if (poCurrentWord == NULL )
        {
            poCurrentWord = key(iCurrent[iLib], iLib);
        }
        else
        {
            word = key(iCurrent[iLib], iLib);

            if (stardictStringCompare(poCurrentWord, word) > 0 )
                poCurrentWord = word;
        }
    }
    return poCurrentWord;
}

QByteArray
StarDictDictionaryManager::poNextWord(QByteArray searchWord, int* iCurrent)
{
    // the input can be:
    // (word,iCurrent),read word,write iNext to iCurrent,and return next word. used by TopWin::NextCallback();
    // (NULL,iCurrent),read iCurrent,write iNext to iCurrent,and return next word. used by AppCore::ListWords();
    QByteArray currentWord = NULL;
    QVector<Dictionary *>::size_type iCurrentLib = 0;
    const char *word;

    for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
    {
        if (!searchWord.isEmpty())
            iCurrent[iLib] = d->dictionaryList.at(iLib)->lookup(searchWord);

        if (iCurrent[iLib] == invalidIndex)
            continue;

        if (iCurrent[iLib] >= articleCount(iLib) || iCurrent[iLib] < 0)
            continue;

        if (currentWord.isNull())
        {
            currentWord = key(iCurrent[iLib], iLib);
            iCurrentLib = iLib;
        }
        else
        {
            word = key(iCurrent[iLib], iLib);

            if (stardictStringCompare(currentWord, word) > 0 )
            {
                currentWord = word;
                iCurrentLib = iLib;
            }
        }
    }

    if (!currentWord.isEmpty())
    {
        iCurrent[iCurrentLib]++;
        for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
        {
            if (iLib == iCurrentLib)
                continue;

            if (iCurrent[iLib] == invalidIndex)
                continue;

            if ( iCurrent[iLib] >= articleCount(iLib) || iCurrent[iLib] < 0)
                continue;

            if (strcmp(currentWord, key(iCurrent[iLib], iLib)) == 0 )
                iCurrent[iLib]++;
        }

        currentWord = poCurrentWord(iCurrent);
    }
    return currentWord;
}

QByteArray
StarDictDictionaryManager::poPreviousWord(long *iCurrent)
{
    // used by TopWin::PreviousCallback(); the iCurrent is cached by AppCore::TopWinWordChange();
    QByteArray poCurrentWord = NULL;
    QVector<Dictionary *>::size_type iCurrentLib = 0;
    const char *word;

    for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
    {
        if (iCurrent[iLib] == invalidIndex)
            iCurrent[iLib] = articleCount(iLib);
        else
        {
            if ( iCurrent[iLib] > articleCount(iLib) || iCurrent[iLib] <= 0)
                continue;
        }

        if ( poCurrentWord.isNull() )
        {
            poCurrentWord = key(iCurrent[iLib] - 1, iLib);
            iCurrentLib = iLib;
        }
        else
        {
            word = key(iCurrent[iLib] - 1, iLib);
            if (stardictStringCompare(poCurrentWord, word) < 0 )
            {
                poCurrentWord = word;
                iCurrentLib = iLib;
            }
        }
    }

    if (!poCurrentWord.isEmpty())
    {
        iCurrent[iCurrentLib]--;
        for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
        {
            if (iLib == iCurrentLib)
                continue;

            if (iCurrent[iLib] > articleCount(iLib) || iCurrent[iLib] <= 0)
                continue;

            if (poCurrentWord == key(iCurrent[iLib] - 1, iLib))
            {
                iCurrent[iLib]--;
            }
            else
            {
                if (iCurrent[iLib] == articleCount(iLib))
                    iCurrent[iLib] = invalidIndex;
            }
        }
    }
    return poCurrentWord;
}

bool
StarDictDictionaryManager::lookupPattern(QString searchWord, int dictionaryIndex,
                                         QString suffix, int wordLength, int truncateLength,
                                         QString addition, bool check)
{
        int searchWordLength = searchWord.length();
        if (!d->found && searchWordLength > wordLength)
        {
            QString caseString;
            int index;
            bool isUpperCase = searchWord.endsWith(suffix.toUpper());
            if (isUpperCase || searchWord.endsWith(suffix.toLower()))
            {
                QString searchNewWord = searchWord.left(searchWordLength - truncateLength);
                int searchNewWordLength = searchNewWord.length();
                if ( check && searchNewWordLength > 3 && (searchNewWord.at(searchNewWordLength - 1) == searchNewWord.at(searchNewWordLength - 2))
                        && !isVowel(searchNewWord.at(searchNewWordLength - 2)) && isVowel(searchNewWord.at(searchNewWordLength - 3)))
                {  //doubled
                    searchNewWord.remove(searchNewWordLength - 1, 1);
                    if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(searchNewWord)) != -1)
                    {
                        d->found = true;
                    }
                    else
                    {
                        if (isUpperCase || searchWord.at(0).isUpper())
                        {
                            caseString = searchNewWord.toLower();
                            if (caseString.compare(searchNewWord))
                            {
                                if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(caseString)) != -1)
                                    d->found = true;
                            }
                        }

                        if (!d->found)
                            searchNewWord.append(searchNewWord.at(searchNewWordLength - 1));  //restore
                    }
                }

                if (!d->found)
                {
                    if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(searchNewWord)) != -1)
                    {
                        d->found = true;
                    }
                    else if (isUpperCase || searchWord.at(0).isUpper())
                    {
                        caseString = searchNewWord.toLower();
                        if (caseString.compare(searchNewWord))
                        {
                            if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(caseString)) != -1)
                                d->found = true;
                        }
                    }
                }

                if (!d->found && !addition.isNull())
                {
                    if (isUpperCase)
                        searchNewWord.append(addition.toUpper());
                    else
                        searchNewWord.append(addition.toLower());

                    if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(searchNewWord)) != -1)
                    {
                        d->found = true;
                    }
                    else if (isUpperCase || searchWord.at(0).isUpper())
                    {
                        caseString = searchNewWord.toLower();
                        if (caseString.compare(searchNewWord))
                        {
                            if ((index = d->dictionaryList.at(dictionaryIndex)->lookup(caseString)) != -1)
                                d->found = true;
                        }
                    }
                }
            }
        }

        return d->found;
}

int
StarDictDictionaryManager::lookupSimilarWord(QByteArray searchWord, int iLib)
{
    int iIndex;

    // Upper case lookup
    iIndex = d->dictionaryList.at(iLib)->lookup(searchWord.toUpper());

    // Lower case lookup
    if (iIndex == -1)
        iIndex = d->dictionaryList.at(iLib)->lookup(searchWord.toLower());

    // Upper the first character and lower others
    if (!iIndex == -1)
        iIndex = d->dictionaryList.at(iLib)->lookup(QString(QString(searchWord)[0].toUpper()) + searchWord.mid(1).toLower());

    if (isPureEnglish(searchWord))
    {
        QString caseString;
        // If not Found, try other status of searchWord.
        int searchWordLength = searchWord.size();
        bool isUpperCase;
        QByteArray searchNewWord;

        lookupPattern(searchWord, iLib, "S", 1, 1, QString(), false);
        lookupPattern(searchWord, iLib, "ED", 1, 1, QString(), false);
        lookupPattern(searchWord, iLib, "LY", 2, 2, QString(), true);
        lookupPattern(searchWord, iLib, "ING", 3, 3, "E", true);


        if (iIndex == -1 && searchWordLength > 3)
        {
            isUpperCase = (searchWord.endsWith("ES") //krazy:exclude=strings
                        && (searchWord.at(searchWordLength - 3) == 'S'
                        || searchWord.at(searchWordLength - 3) == 'X'
                        || searchWord.at(searchWordLength - 3) == 'O'
                        || (searchWordLength > 4 && searchWord.at(searchWordLength - 3) == 'H'
                        && (searchWord.at(searchWordLength - 4) == 'C'
                        || searchWord.at(searchWordLength - 4) == 'S'))));

            if (isUpperCase ||
                    (searchWord.endsWith("es") //krazy:exclude=strings
                     && (searchWord.at(searchWordLength - 3) == 's'
                     || searchWord.at(searchWordLength - 3) == 'x'
                     || searchWord.at(searchWordLength - 3) == 'o'
                     || (searchWordLength > 4 && searchWord.at(searchWordLength - 3) == 'h'
                     && (searchWord.at(searchWordLength - 4) == 'c'
                     || searchWord.at(searchWordLength - 4) == 's')))))
            {
                searchNewWord = searchWord.left(searchWordLength - 2);
                if ((iIndex = d->dictionaryList.at(iLib)->lookup(searchNewWord)) == -1
                    && (isUpperCase || QString::fromUtf8(searchWord).at(0).isUpper()))
                {
                    caseString = searchNewWord.toLower();
                    if (caseString.compare(searchNewWord))
                    {
                        iIndex = d->dictionaryList.at(iLib)->lookup(caseString);
                    }
                }
            }
        }

        lookupPattern(searchWord, iLib, "ED", 3, 2, QString(), true);
        lookupPattern(searchWord, iLib, "IED", 3, 3, "Y", false);
        lookupPattern(searchWord, iLib, "IES", 3, 3, "Y", false);
        lookupPattern(searchWord, iLib, "ER", 2, 3, QString(), false);
        lookupPattern(searchWord, iLib, "EST", 3, 3, QString(), false);

    }

#if 0
    else
    {
        //don't change iWordIndex here.
        //when LookupSimilarWord all failed too, we want to use the old LookupWord index to list words.
        //iWordIndex = invalidIndex;
    }
#endif
    return iIndex;
}

int
StarDictDictionaryManager::simpleLookupWord(QByteArray searchWord, int iLib)
{
    int retval;

    if ((retval = d->dictionaryList.at(iLib)->lookup(searchWord)) == -1) {
        retval = lookupSimilarWord(searchWord, iLib);
    }

    return retval;
}

struct
Fuzzystruct
{
    QByteArray pMatchWord;
    int matchWordDistance;
};

inline bool
operator<(const Fuzzystruct & lh, const Fuzzystruct & rh)
{
    if (lh.matchWordDistance != rh.matchWordDistance)
        return lh.matchWordDistance < rh.matchWordDistance;

    if (!lh.pMatchWord.isNull() && !rh.pMatchWord.isNull())
        return stardictStringCompare(lh.pMatchWord, rh.pMatchWord) < 0;

    return false;
}

bool
StarDictDictionaryManager::lookupWithFuzzy(QByteArray searchWord, QStringList resultList, int resultListSize, int iLib)
{
    if (searchWord.isEmpty())
        return false;

    Fuzzystruct *oFuzzystruct = new Fuzzystruct[resultListSize];

    for (int i = 0; i < resultListSize; ++i)
    {
        oFuzzystruct[i].pMatchWord = NULL;
        oFuzzystruct[i].matchWordDistance = d->maximumFuzzyDistance;
    }

    int maximumDistance = d->maximumFuzzyDistance;
    int iDistance;
    bool found = false;
    EditDistance oEditDistance;

    long searchCheckWordLength;
    long searchWordLength;
    QString searchCheckWord;

    searchWord.toLower();

    if (d->progressFunction)
        d->progressFunction();


    //if (stardict_strcmp(searchWord, poGetWord(0,iLib))>=0 && stardict_strcmp(searchWord, poGetWord(articleCount(iLib)-1,iLib))<=0) {
    //there are Chinese dicts and English dicts...
    if (true)
    {
        int wordNumber = articleCount(iLib);
        for (int index = 0; index < wordNumber; ++index)
        {
            searchCheckWord = key(index, iLib);
            // tolower and skip too long or too short words
            searchCheckWordLength = searchCheckWord.length();
            searchWordLength = searchWord.length();
            if (searchCheckWordLength - searchWordLength >= maximumDistance
                    || searchWordLength - searchCheckWordLength >= maximumDistance)
                continue;

            searchCheckWord.toLower();

            iDistance = oEditDistance.calEditDistance(searchCheckWord, searchWord, maximumDistance);
            if (iDistance < maximumDistance && iDistance < searchWordLength)
            {
                // when searchWordLength=1,2 we need less fuzzy.
                found = true;
                bool isAlreadyInList = false;
                int maximumDistanceAt = 0;
                for (int j = 0; j < resultListSize; ++j)
                {
                    if (!oFuzzystruct[j].pMatchWord.isNull() && searchCheckWord.compare(oFuzzystruct[j].pMatchWord) == 0)
                    { //already in list
                        isAlreadyInList = true;
                        break;
                    }

                    //find the position,it will certainly be found (include the first time) as iMaxDistance is set by last time.
                    if (oFuzzystruct[j].matchWordDistance == maximumDistance )
                    {
                        maximumDistanceAt = j;
                    }
                }

                if (!isAlreadyInList)
                {
                    oFuzzystruct[maximumDistanceAt].pMatchWord = searchCheckWord.toUtf8();
                    oFuzzystruct[maximumDistanceAt].matchWordDistance = iDistance;
                    // calc new iMaxDistance
                    maximumDistance = iDistance;
                    int tmpVal = maximumDistance; //stupid workaround for gcc bug 44545
                    for (int j = 0; j < resultListSize; ++j)
                    {
                        if (oFuzzystruct[j].matchWordDistance > maximumDistance)
                            tmpVal = oFuzzystruct[j].matchWordDistance;
                    } // calc new iMaxDistance

                    maximumDistance = tmpVal; // end of stupid workaround
                }   // add to list
            }   // find one
        }   // each word
    }   // ok for search
//    }   // each lib

    if (found) // sort with distance
        qSort(oFuzzystruct, oFuzzystruct + resultListSize);

    for (int i = 0; i < resultListSize; ++i)
        resultList[i] = oFuzzystruct[i].pMatchWord;

    delete [] oFuzzystruct;

    return found;
}

inline bool
lessForCompare(QString lh, QString rh)
{
    return stardictStringCompare(lh, rh) < 0;
}

int
StarDictDictionaryManager::lookupPattern(QByteArray patternWord, QStringList patternMatchWords)
{
    QVector<int> indexList;
    indexList.reserve(d->maxMatchItemPerLib + 1);
    int matchCount = 0;

    for (QVector<Dictionary *>::size_type iLib = 0; iLib < d->dictionaryList.size(); ++iLib)
    {
        //if(oStarDictDictionaryManager.LookdupWordsWithRule(pspec,indexList,MAX_MATCH_ITEM_PER_LIB+1-iMatchCount,iLib))
        // -iMatchCount,so save time,but may got less result and the word may repeat.

        indexList = d->dictionaryList.at(iLib)->lookupPattern(patternWord, d->maxMatchItemPerLib + 1);
        if (!indexList.isEmpty())
        {
            if (d->progressFunction)
                d->progressFunction();

            int indexListSize = indexList.size();
            for (int i = 0; i < indexListSize; ++i)
            {
                QByteArray searchMatchWord = key(indexList.at(i), iLib);

                if (!patternMatchWords.contains(searchMatchWord))
                    patternMatchWords.append(searchMatchWord);
            }
        }
    }

    if (matchCount) // sort it.
        qSort(patternMatchWords.begin(), patternMatchWords.end(), lessForCompare);

    return matchCount;
}

bool
StarDictDictionaryManager::lookupData(QByteArray search_word, QStringList resultList)
{
    QStringList searchWords;
    QString searchWord;
    foreach (char ch, search_word)
    {
        if (ch == '\\')
        {
            switch (ch)
            {
            case ' ':
                searchWord.append(' ');
                break;
            case '\\':
                searchWord.append('\\');
                break;
            case 't':
                searchWord.append('\t');
                break;
            case 'n':
                searchWord.append('\n');
                break;
            default:
                searchWord.append(ch);
            }
        }
        else if (ch == ' ')
        {
            if (!searchWord.isEmpty())
            {
                searchWords.append(searchWord);
                searchWord.clear();
            }
        }
        else
        {
            searchWord.append(ch);
        }
    }

    if (!searchWord.isEmpty())
    {
        searchWords.append(searchWord);
        searchWord.clear();
    }

    if (searchWords.isEmpty())
        return false;

    quint32 maximumSize = 0;
    for (QVector<Dictionary *>::size_type i = 0; i < d->dictionaryList.size(); ++i)
    {
        if (!d->dictionaryList.at(i)->containFindData())
            continue;

        if (d->progressFunction)
            d->progressFunction();

        int wordSize = articleCount(i);
        for (int j = 0; j < wordSize; ++j)
        {
            WordEntry wordEntry = d->dictionaryList.at(i)->wordEntry(j);
            if (wordEntry.dataSize() > maximumSize)
            {
                maximumSize = wordEntry.dataSize();
            }

            if (d->dictionaryList.at(i)->findData(searchWords, wordEntry.dataOffset(), wordEntry.dataSize()))
                resultList[i].append(wordEntry.data());
        }
    }

    QVector<Dictionary *>::size_type i;
    for (i = 0; i < d->dictionaryList.size(); ++i)
        if (!resultList[i].isEmpty())
            break;

    return i != d->dictionaryList.size();
}

StarDictDictionaryManager::QueryType
StarDictDictionaryManager::analyzeQuery(QString string, QString& result)
{
    if (string.isNull() || result.isNull())
    {
        result = "";
        return SIMPLE;
    }

    if (string.startsWith('/'))
    {
        result = string.mid(1);
        return FUZZY;
    }
    else if (string.startsWith('|'))
    {
        result = string.mid(1);
        return DATA;
    }

    string.remove('\\');

    if (string.contains('*') || string.contains('?'))
        return REGEXP;

    return SIMPLE;
}
