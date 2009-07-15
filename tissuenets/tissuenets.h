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

#include "vtkGeometryFilter.h"
#include "vtkGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkStringArray.h"
#include "vtkGraphLayoutView.h"
#include "vtkGraphLayoutFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkViewTheme.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkDataSetAttributes.h"
#include <vtkSmartPointer.h>
#include "vtkActor.h"
#include "vtkGraphToPolyData.h"
#include "vtkPoints.h"

#include "vtkGraphMapper.h"
#include "vtkLabeledDataMapper.h"
#include "vtkActor2D.h"
#include "vtkTextProperty.h"

#include "vtkPolyDataMapper.h"
#include "vtkAssignCoordinates.h"
#include "vtkGraphMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkXMLReader.h"
#include "vtkProperty.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkVertexListIterator.h"
#include "vtkTubeFilter.h"
#include "vtkSphereSource.h"
#include "vtkTestingColors.h"
#include "vtkGraphLayout.h"
#include "vtkPassThroughLayoutStrategy.h"

// include libxml2
//#include "libxml/parser.h"
//#include <libxml/tree.h>
//#include <libxml/xmlreader.h>

// include tinyxml
#include "tinyxml.h"

#include <stdio.h>
#include <map>

//Silinecek
#include "vtkActor.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphToPolyData.h"
#include "vtkGraphWriter.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkVertexDegree.h"
#include <vtkStdString.h>
#include <vtkStringArray.h>
#include <vector>

//Required for Graph operations
//#include <vtkBoostKruskalMinimumSpanningTree.h>
//#include <vtkBoostPrimMinimumSpanningTree.h>
#include <vtkTree.h>
#include "vtkExtractSelectedGraph.h"

//Temp
#include "vtkMutableDirectedGraph.h"
#include "vtkBoostBreadthFirstSearchTree.h"
#include "vtkBoostBreadthFirstSearch.h"

// These are required for implementing Remove vertex/edge
#include "vtkSelectionSource.h"
#include "vtkSelectionNode.h"
#include <vtkAdjacentVertexIterator.h>
#include "vtkInEdgeIterator.h"
#include "vtkOutEdgeIterator.h"

#include "vtkTextureMapToSphere.h"
#include "vtkTexture.h"
#include "vtkBMPReader.h"
#include <vtkDoubleArray.h>
#include "vtkLookupTable.h"

#include <vtkEdgeListIterator.h>
#include <set> //for detecting cycles in the graph
#include <vtkWindowToImageFilter.h>
#include <vtkCamera.h>
#include <vtkTIFFWriter.h>

#include <vtkRenderView.h> //Mouse drag and selections
#include <vtkCommand.h>

#include "vtkInteractorStyleJoystickActor.h"
#include <vtkInteractorStyleJoystickCamera.h>
#include "vtkInteractorStyleTrackballCamera.h"
//#include "vtkRenderInteractor.h"
#include "vtkCellPicker.h"
#include "vtkCallBackCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkSelectionLink.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkVariant.h"

#include <sstream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#define myVTKInt vtkSmartPointer<vtkIntArray>

using namespace std;
using namespace boost;

// Description:
// Will be used for storing selected vertices and edges which are originally stored in a 
// vtkSelectionLink structure when selections are done using mouse-dragging.

class SelectedVerticesAndEdges {
public:
	set<double> vertices;
	set<string> edges;	
	SelectedVerticesAndEdges(){};
};

// Description:
class BioNet
{
public:
	vtkCallbackCommand* edgePicked;
	vtkCallbackCommand* vertexPicked;
	vtkSurfaceRepresentation* vertexRep;
	vtkGlyph3D* vertexGlyph;

	//vtkCallbackCommand* delPicked;
	//vtkSelectionLink* edgeLink;
	vtkSurfaceRepresentation* selections;
	//vtkSurfaceRepresentation* vertexRep;
	
	vtkRenderView* view;
	vtkRenderWindowInteractor* iren;
	vtkRenderWindow* win;
	vtkMutableUndirectedGraph* g;

	//vtkBoostKruskalMinimumSpanningTree* mst;
	//vtkExtractSelectedGraph* mstGraph;

	SelectedVerticesAndEdges* sve; // Temporary location for vertex and edge selections
	SelectedVerticesAndEdges* finalSelections; //Selected edges and vertices will be stored here
	bool visited;
	vtkVertexDegree* degree; //Required for displaying graphs

	int numOfPasses; //Required for proper event handling
	float cutoff;

	void Haha() {cout<<"Hahaha"<<endl;};
	void RenderWin();
	void Interact();
	BioNet();
	~BioNet(); //{g->Delete();} //Will be filled later
	map<char,int> SetColorCode();
	bool CycleDetected(set<int>* vertices, vtkIdType v);
	void Averages(); //Use this If pyramidial Region Computation is required
	bool ReadXGMML(char* graphFileName, float n); // Construct a vtkGraph from an XGMML file 	
	void Display(vtkRenderView* ren, vtkAlgorithm* alg, 
		double xoffset, double yoffset, 
		const char* vertColorArray, double vertMin, double vertMax, 
		const char* edgeColorArray, double edgeMin, double edgeMax);
	template<class EdgeType>
		void RemoveEdge(EdgeType id_of_edge_to_remove);
	void RemoveVertex(vtkIdType id_of_vertex_to_remove);
	void RecordImage (vtkRenderer* ren, vtkRenderWindow* win);
	static void ProcessVertexSelection(vtkObject* caller, unsigned long event, 
							void* clientdata, void* callerdata);
	static void ProcessEvents(vtkObject* caller, 
				   unsigned long event,
				   void* clientdata, 
				   void* callerdata); 
	vtkMutableUndirectedGraph* GetNetwork();
	vtkRenderView* GetRenderView();
	vector<double> GetSelectedVertices(){};
	vector<double> GetSelectedEdges(){};
	void AddLabels();
	vtkGraphToPolyData* ToPoly();
	void setNetworkStatusOn(){emptyNetwork=false;};	//Do this if the network has nodes and edges
	void setNetworkStatusOff(){ emptyNetwork=true; // Do this if the network is empty 
								g=vtkMutableUndirectedGraph::New();}; /*Start with a new network*/
	bool NetworkEmpty(){return emptyNetwork;};	//Do this if the network has nodes and edges
	SelectedVerticesAndEdges* GetSelections(vtkSelectionLink* p);
	void InsertIntoMap(set<double>* vertices, double d);
	void UpdateView();
	//void Kruskalmst();

private:
	//vtkMutableUndirectedGraph* g;
	bool emptyNetwork; //true if the network is empty
	/**************************************************************************************/
	//We need an associative sorted container to be able to relate vertex ids to           /
	//node ids given in the xml file. vtk forces us to do it this way. We cannot use       /
	//node ids as vertex ids in the graphs. Vertex ids have to start from 0                /
	/**************************************************************************************/
	map<int,int> vertexNodeLabelToID;	
	/************************************************************************************/
	// Store Edge Ids with source and target of the edge                                   /
	// Will be used for edge deletion                                                      /
	/**************************************************************************************/
	map<pair<int,int>, vtkEdgeType> edgeSourgeTargetToID;
	double avg; //average edge length
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/

};

// Description:
// We want to pass both the selections and an instance of BioNet class to Event Handler.
// But we have only one clientdata to be set for this. Therefore we need to define a new class.

class BioNetAndSelectionLink {
public:
	BioNet* bioObj;
	vtkSelectionLink* edgeLink;
	BioNetAndSelectionLink();
	~BioNetAndSelectionLink(){delete bioObj;};
};
