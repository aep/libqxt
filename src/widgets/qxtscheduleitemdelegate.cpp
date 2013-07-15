#include "qxtscheduleitemdelegate.h"
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#include <QModelIndex>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QSize>
#include <QFontMetrics>
#include <QIcon>
#include <QVariant>
#include <QDateTime>
#include "qxtnamespace.h"

QxtScheduleItemDelegate::QxtScheduleItemDelegate(QObject *parent)
        : QAbstractItemDelegate(parent)
{
}


QxtScheduleItemDelegate::~QxtScheduleItemDelegate()
{
}


/*!
 *  reimplemented for item painting
 * You should not reimplement this to change the item painting, use paintItemBody, paintItemHeader and paintSubItem instead
 * because this function uses caches to speed up painting. If you want to change the item shape only you could also reimplement
 * the createPainterPath function.
 * \note the parameter option hast to be of type QxtStyleOptionScheduleViewItem or the delegate will not paint something
 * \sa paintItemBody(), paintItemHeader(), paintSubItem()
 */
void QxtScheduleItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    const QxtStyleOptionScheduleViewItem *agendaOption = qstyleoption_cast<const QxtStyleOptionScheduleViewItem *>(&option);
    if (!agendaOption)
        return;

    QStringList rowsData = index.data(Qt::EditRole).toStringList();

    QRect currRect;

    painter->save();

    if (agendaOption->itemPaintCache->size() !=  agendaOption->itemGeometries.size())
        (*agendaOption->itemPaintCache) = QVector<QPixmap>(agendaOption->itemGeometries.size(), QPixmap());

    int lastPart = agendaOption->itemGeometries.size() - 1;
    int paintedSubItems = 0;

    for (int iLoop = 0; iLoop < agendaOption->itemGeometries.size();iLoop++)
    {
        if ((*agendaOption->itemPaintCache)[iLoop].width() != agendaOption->itemGeometries[iLoop].width()
                || (*agendaOption->itemPaintCache)[iLoop].height() != agendaOption->itemGeometries[iLoop].height())
        {
            //If we enter this codepath we have to rebuild the pixmap cache
            //so first we create a empty pixmap
            (*agendaOption->itemPaintCache)[iLoop] = QPixmap(agendaOption->itemGeometries[iLoop].size());
            (*agendaOption->itemPaintCache)[iLoop].fill(Qt::transparent);

            QPainter cachePainter(&(*agendaOption->itemPaintCache)[iLoop]);
            QRect rect = QRect(QPoint(0, 0), agendaOption->itemGeometries[iLoop].size());

            //what kind of itempart do we need to paint?
            ItemPart part = iLoop == 0 ? Top : (iLoop == lastPart ? Bottom : Middle);

            //if the item has only one part
            if (lastPart == 0)
                part = Single;

            //paint the item body
            cachePainter.save();
            paintItemBody(&cachePainter, rect, *agendaOption, part, index);
            cachePainter.restore();

            int remainingHeight = rect.height();

            //paint item header
            if (iLoop == 0 && agendaOption->itemHeaderHeight > 0 && agendaOption->itemHeaderHeight < remainingHeight)
            {
                QRect headerRect(0, 0, rect.width(), agendaOption->itemHeaderHeight);
                paintItemHeader(&cachePainter, headerRect, *agendaOption, index);
                remainingHeight -= agendaOption->itemHeaderHeight;
            }

            //paint subitems if there are any
            int subItems       = index.model()->rowCount(index);
            for (int items = paintedSubItems; items < subItems; items++)
            {
                QModelIndex currSubItem = index.model()->index(items, 0, index);
                QSize size = sizeHint(option, currSubItem);

                if (currSubItem.isValid()){
                    paintSubItem(&cachePainter, QRect(), *agendaOption, currSubItem);
                }

                paintedSubItems++;
            }

            cachePainter.end();

        }
        currRect = agendaOption->itemGeometries[iLoop];
        currRect.translate(agendaOption->translate);
        painter->drawPixmap(currRect, (*agendaOption->itemPaintCache)[iLoop]);
    }
    painter->restore();
}

/*!
 * \brief paints the items body reimplement this to paint a custom body
 * \a QPainter *painter the initialized painter
 * \a const QRect rect the ItemPart rect
 * \a const QxtStyleOptionScheduleViewItem & option
 * \a const ItemPart part this indicates what part of the item gets painted, remember items can be splitted in parts
 * \a const QModelIndex &index the items model index
 */
void QxtScheduleItemDelegate::paintItemBody(QPainter *painter, const QRect rect , const QxtStyleOptionScheduleViewItem & option , const ItemPart part, const QModelIndex & index) const
{
    int iCurrRoundTop, iCurrRoundBottom;
    iCurrRoundTop = iCurrRoundBottom = 0;

    QColor fillColor = index.data(Qt::BackgroundRole).value<QColor>();
    fillColor.setAlpha(120);
    QColor outLineColor = index.data(Qt::ForegroundRole).value<QColor>();

    painter->setFont(option.font);
    painter->setRenderHint(QPainter::Antialiasing);

    if (part == Top || part == Single)
        iCurrRoundTop = option.roundCornersRadius;
    if (part == Bottom || part == Single)
        iCurrRoundBottom = option.roundCornersRadius;

    QPainterPath cachePath;
    QRect cacheRect = QRect(QPoint(1, 1), rect.size() - QSize(1, 1));

    painter->setBrush(fillColor);
    painter->setPen(outLineColor);

    createPainterPath(cachePath, cacheRect, iCurrRoundTop, iCurrRoundBottom);
    painter->drawPath(cachePath);
}

/*!
 * \brief paints the items header reimplement this to paint a custom header
 * \a QPainter *painter the initialized painter
 * \a const QRect rect the header rect
 * \a const QxtStyleOptionScheduleViewItem & option
 * \a const QModelIndex &index the items model index
 */
void QxtScheduleItemDelegate::paintItemHeader(QPainter *painter, const QRect rect , const QxtStyleOptionScheduleViewItem & option, const QModelIndex &index) const
{
    bool converted = false;
    int startUnixTime =  index.data(Qxt::ItemStartTimeRole).toInt(&converted);
    if (!converted)
        return;

    int duration = index.data(Qxt::ItemDurationRole).toInt(&converted);
    if (!converted)
        return;

    QDateTime startTime = QDateTime::fromTime_t(startUnixTime);
    QDateTime endTime = QDateTime::fromTime_t(startUnixTime + duration);

    if (!startTime.isValid() || !endTime.isValid())
        return;

    QFont font;
    QVariant vfont = index.data(Qt::FontRole);

    if (vfont.isValid())
        font = vfont.value<QFont>();
    else
        font = option.font;

    QString text = startTime.toString("hh:mm") + ' ' + endTime.toString("hh:mm");
    QFontMetrics metr(font);
    text = metr.elidedText(text, Qt::ElideRight, rect.width());
    painter->drawText(rect, Qt::AlignCenter, text);
}

/*!
 * \brief paints a subitem, if you want custom subitem painting reimplement this member function
 * \a QPainter *painter the initialized painter
 * \a const QRect rect the subitem rect
 * \a const QxtStyleOptionScheduleViewItem & option
 * \a const QModelIndex &index the items model index
 */
void QxtScheduleItemDelegate::paintSubItem(QPainter * /*painter*/, const QRect /*rect*/, const QxtStyleOptionScheduleViewItem & /*option*/, const QModelIndex & /*index*/) const
{

}

/*!
 * \brief returns the sizeHint for subitems.
 */
QSize QxtScheduleItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    //we return the size only for subitems and only the height

    if (index.parent().isValid())
    {
        QSize size = index.data(Qt::SizeHintRole).toSize();

        if (!size.isValid())
        {
            QFont font;
            QVariant vfont = index.data(Qt::FontRole);

            if (vfont.isValid())
                font = vfont.value<QFont>();
            else
                font = option.font;

            int height = 0;
            QFontMetrics metr(font);
            height = metr.height() + 2;

            return QSize(0, height);
        }
    }
    return QSize();
}

void QxtScheduleItemDelegate::createPainterPath(QPainterPath & emptyPath, const QRect & fullItemRect, const int iRoundTop, const int iRoundBottom) const
{
    emptyPath = QPainterPath();
    bool bRoundTop = iRoundTop > 0;
    bool bRountBottom = iRoundBottom > 0;

    if (bRoundTop)
    {
        emptyPath.moveTo(fullItemRect.topLeft() + QPoint(0, iRoundTop));
        emptyPath.quadTo(fullItemRect.topLeft(), fullItemRect.topLeft() + QPoint(iRoundTop, 0));
    }
    else
        emptyPath.moveTo(fullItemRect.topLeft());

    emptyPath.lineTo(fullItemRect.topRight() - QPoint(iRoundTop, 0));

    if (bRoundTop)
        emptyPath.quadTo(fullItemRect.topRight(), fullItemRect.topRight() + QPoint(0, iRoundTop));

    emptyPath.lineTo(fullItemRect.bottomRight() - QPoint(0, iRoundBottom));

    if (bRountBottom)
        emptyPath.quadTo(fullItemRect.bottomRight(), fullItemRect.bottomRight() - QPoint(iRoundBottom, 0));

    emptyPath.lineTo(fullItemRect.bottomLeft() + QPoint(iRoundBottom, 0));

    if (bRountBottom)
        emptyPath.quadTo(fullItemRect.bottomLeft(), fullItemRect.bottomLeft() - QPoint(0, iRoundBottom));

    emptyPath.closeSubpath();
}

