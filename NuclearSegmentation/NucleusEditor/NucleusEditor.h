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

#ifndef NUCLEUS_EDITOR_H
#define NUCLEUS_EDITOR_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//QT Includes:
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QToolBar>
#include <QtGui/QProgressBar>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>


//Farsight Includes:
//#include "SegmentationCommon/ftkSegmentationResult.h"
#include "NuclearSegmentation/ftkNuclearSegmentation.h"
#include "ftkImage/ftkImage.h"
#include "SegmentationModel.h"
#include "SegmentationWindow.h"
//#include "NuclearSegmentationWizard.h"
#include "ftkGUI/TableWindow.h"
#include "ftkGUI/PlotWindow.h"
#include "ftkGUI/ImageBrowser5D.h"
#include "ftkGUI/HistoWindow.h"
//#include "SegmentationView.h"

class Load : public QThread
{
public:
	Load(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};


class Binarize : public QThread
{
public:
	Binarize(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};

class SeedDetect : public QThread
{
public:
	SeedDetect(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};

class Cluster : public QThread
{
public:
	Cluster(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};

class Finalize : public QThread
{
public:
	Finalize(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};

class Features : public QThread
{
public:
	Features(ftk::NuclearSegmentation *seg = NULL);
	void run();

private:
	ftk::NuclearSegmentation *mySeg;
};

class NucleusEditor : public QMainWindow
{
    Q_OBJECT;

public:
	NucleusEditor(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	~NucleusEditor();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void loadImage(void);
	void loadResult(void);
	bool saveResult(void);
	void segmentImage(void);
	void abortSegment(void);
	void segment(void);
	void about(void);
	//void loadDatFile(void);
	//void closeWidget(QWidget *);
	void CreateNewPlotWindow();
	void CreateNewTableWindow();

	//DEMO - python is necessary for the demo
	void OpenPythonWindow();
	bool BrowseForPythonExecutable();

	//For Editing Menu
	void setEditsEnabled(bool val);
	void mergeCells(void);
	void deleteCells(void);
	void splitCells(void);

	void startSplitting(void);
	void endSplitting(void);

signals:
    
private:
	void createMenus();
	void createStatusBar();
	void createSegmentToolBar();
	void clearModel();
	void newModel();

	std::vector<PlotWindow *> pltWin;
	std::vector<TableWindow *> tblWin;
	HistoWindow * hisWin;
	SegmentationWindow *segWin;

	ftk::Image::Pointer NewFTKImage(std::vector<std::string> filenames);
	ftk::Image::Pointer NewFTKImage(std::string filename);

	QMenu *fileMenu;
	QAction *loadAction;
	QAction *saveAction;
	QAction *xmlAction;
	QAction *segmentAction;
	QAction *exitAction;
	//QAction *datAction;

	QMenu *viewMenu;
	QAction *newScatterAction;
	QMenu *helpMenu;
	QAction *aboutAction;
	QAction *pythonAction;

	//For Editing Menu
	QMenu *editMenu;
	QAction *mergeAction;
	QAction *deleteAction;
	QMenu *splitMenu;
	//QAction *splitAction;
	QAction *splitStartAction;
	QAction *splitEndAction;
	
	QLabel *statusLabel;

	ftk::NuclearSegmentation *seg;
	ftk::Image::Pointer myImg;
	SegmentationModel *currentModel;

	QString lastPath;
	QString myImgName;
	int segmentState;
	QAction * segmentAbort;
	QAction * segmentContinue;
	QLabel * segmentTaskLabel;
	QProgressBar * segmentProgress;
	QToolBar * segmentTool;
	Load * loadThread;
	Binarize * binaryThread;
	SeedDetect * seedThread;
	Cluster * clusterThread;
	Finalize * finalizeThread;
	Features * featuresThread;

	//DEMO variable
	bool ConfirmClosePython();
	QSettings *settings;
	QProcess *pythonProcess;
	QLabel *pythonLabel;
	QLabel *currentPythonLabel;
	QPushButton *browseForPythonButton;

	//bool editStatus; //false shows that we cannot apply editing to cells						 	
 };


#endif
