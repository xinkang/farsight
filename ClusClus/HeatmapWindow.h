#ifndef HEATMAPWINDOW_H
#define HEATMAPWINDOW_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
 
#include <QVTKWidget.h>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QItemSelection>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QTableView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QCloseEvent>
#include <QtGui/QDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtGui/QDoubleSpinBox>
#include <QtCore/QMap>
#include <QtCore/QSignalMapper>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>

#include "vtkTable.h"
#include <vtkSelection.h>
#include <vtkLookupTable.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkRenderedGraphRepresentation.h>
#include <vtkTableToGraph.h>
#include <vtkViewTheme.h>
#include <vtkStringToCategory.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkGraphToGlyphs.h>
#include <vtkRenderer.h>
#include <vtkFast2DLayoutStrategy.h>
#include <vtkArcParallelEdgeStrategy.h>
#include <vtkPolyDataMapper.h>
#include <vtkEdgeLayout.h>
#include <vtkGraphToPolyData.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkLineWidget2.h>
#include <vtkLineSource.h>
#include <vtkSelectionNode.h>
#include <vtkLineRepresentation.h>
#include <vtkIdTypeArray.h>
#include <vtkAbstractArray.h>
#include <vtkAnnotationLink.h>
#include "vtkSmartPointer.h"
#include "vtkDoubleArray.h"
#include "vtkAbstractArray.h"
#include "vtkVariantArray.h"
#include <vtkIdTypeArray.h>
#include <vtkPoints.h>
#include <vtkCallbackCommand.h>
#include <vtkFloatArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkCellData.h>
#include <vtkPolyData.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>

#include <ftkCommon/ftkUtils.h>
#include "ObjectSelection.h"

using namespace std;

class Heatmap : public QMainWindow
{
    Q_OBJECT;

public:
	Heatmap(QWidget * parent = 0);
	~Heatmap();
	void setDataForHeatmap(double** features, int* optimalleaforder1, int* optimalleaforder2,int num_samples, int num_features);
	void creatDataForHeatmap();
	void setModels(vtkSmartPointer<vtkTable> table, ObjectSelection * sels, ObjectSelection * sels2);
	void showGraph();

	double** mapdata;
	double** connect_Data_Tree1;
	int*    Optimal_Leaf_Order1;
	vector<vector<double > > Processed_Coordinate_Data_Tree1;
	int num_samples;
	double** connect_Data_Tree2;
	int*    Optimal_Leaf_Order2;
	vector<vector<double > > Processed_Coordinate_Data_Tree2;	
	int num_features;

protected slots:
	//static void SelectionCallbackFunction(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData );	

private:
	QVTKWidget mainQTRenderWidget;
	vtkSmartPointer<vtkPlaneSource> aPlane;
	vtkSmartPointer<vtkFloatArray> cellData;
	vtkSmartPointer<vtkLookupTable> lookuptable;
	vtkSmartPointer<vtkPolyDataMapper> mapper;
	vtkSmartPointer<vtkActor> actor;
	vtkSmartPointer<vtkIntArray> cellColors;
	vtkSmartPointer<vtkRenderView> view;
	vtkSmartPointer<vtkViewTheme> theme;
};

#endif