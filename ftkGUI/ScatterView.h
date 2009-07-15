/*=========================================================================
Copyright 2009 Rensselaer Polytechnic Institute
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
=========================================================================*/

/****************************************************************************
 **
 ** Adapted from the Qt Examples pieview.h
 **
 ****************************************************************************/

#ifndef SCATTERVIEW_H
#define SCATTERVIEW_H

#include <QtGui/QAbstractItemView>
#include <QtGui/QFont>
#include <QtGui/QItemSelection>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QStandardItemModel>
#include <QtGui/QWidget>
#include <QtGui/QBitmap>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QPainterPath>
#include <QtGui/QStylePainter>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QMouseEvent>
#include <QtCore/QModelIndex>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QPoint>
#include <QtCore/QMap>

#include <iostream>

//#include "SegmentationModel.h"

class QRubberBand;
class PlotSettings;

class ScatterView : public QAbstractItemView
{
	Q_OBJECT

public:
	ScatterView(QWidget *parent = 0);

	void setModel(QAbstractItemModel *model);
	
    QRect visualRect(const QModelIndex &index) const;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    QModelIndex indexAt(const QPoint &point) const;

	int ColForX(){ return columnNumForX; };
	int ColForY(){ return columnNumForY; };
	int ColForColor(){ return columnNumForColoring; };

public slots:
	void SetColForX(int x, std::string name);
	void SetColForY(int y, std::string name);
	void SetColForColor(int c);
	void SetColForColor(int c, QMap<int, QColor>  newMap);
	void SetColorMap(QMap<int, QColor> map){ colorMap = map; };
	void selModeChanged(int s);
	void selectClicked(void);
	void clearClicked(void);

protected slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
	void dataChanged();
    //void rowsInserted(const QModelIndex &parent, int start, int end);
    //void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);
    QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers);

    int horizontalOffset() const;
    int verticalOffset() const;

    bool isIndexHidden(const QModelIndex &index) const;

    void setSelection(const QRect&, QItemSelectionModel::SelectionFlags command);
	void setSelection(const QRegion &region, QItemSelectionModel::SelectionFlags command);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent * event );

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    //void scrollContentsBy(int dx, int dy);

    QRegion visualRegionForSelection(const QItemSelection &selection) const;

private:
    QRect itemRect(const QModelIndex &item) const;
    QRegion itemRegion(const QModelIndex &index) const;
    //int rows(const QModelIndex &index = QModelIndex()) const;

	void refreshPixmap();
	void drawGrid(QPainter *painter);
	void drawCurves(QPainter *painter);
	QMap<int, QColor> GetDefaultColors();
	void drawSelection(QPainter *painter);
	bool itemInRowIsSelected(int row);
	void initColumns();
	void updateAxis();
	enum {LMargin = 60, BMargin = 35, RMargin = 20, TMargin = 20};

	int columnNumForX;
	std::string columnNameForX;
	int columnNumForY;
	std::string columnNameForY;
	int columnNumForColoring;
	QMap<int, QColor> colorMap;

	int selMode;

    QPoint origin;
    QRubberBand *rubberBand;
	QPixmap pixmap;
	PlotSettings *mySettings;

	QVector<QPoint> selectionRegion;			//current region
	//Removed don't need to repaint all the time
	//QVector<QVector<QPoint>> selectionRegions; //past regions

	int x;
	int y;
};

class PlotSettings
{
public:
	PlotSettings();
	void setRange(double x1, double x2, double y1, double y2);
	void adjust();

	double spanX() const { return maxX - minX; }
	double spanY() const { return maxY - minY; }
	double minX;
	double maxX;
	int numXTicks;
	double minY;
	double maxY;
	int numYTicks;
	
private:
	static void adjustAxis(double &min, double &max, int &numTicks);
};

#endif 
