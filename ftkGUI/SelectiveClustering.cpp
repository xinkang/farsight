/*=========================================================================

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

#include "SelectiveClustering.h"

SelectiveClustering::SelectiveClustering()
{
	/*! 
	* Initalize selective clustering
	*/
	this->ClusterMap.clear();

	this->ObjectTableIDMap.clear();
	this->ObjectTable = vtkSmartPointer<vtkTable>::New();

	this->ClusterTable = vtkSmartPointer<vtkTable>::New();
	this->CreateClusterTableHeaders();

}

vtkIdType SelectiveClustering::AddCluster(std::set<vtkIdType> ClusterSelectionSet)
{
	/*! 
	* Create a new cluster from a selection
	*/
	vtkIdType newKey = this->ClusterMap.size();
	this->ClusterMap[newKey] = ClusterSelectionSet;

	vtkVariantArray * NewRow = vtkVariantArray::New();
	NewRow->InsertNextValue(newKey); 
	NewRow->InsertNextValue(vtkVariant(ClusterSelectionSet.size())); 
	NewRow->InsertNextValue(NULL);
	this->ClusterTable->InsertNextRow(NewRow);

	emit ClusterChanged();
	return newKey;
}

bool SelectiveClustering::AddCluster(vtkIdType key, std::set<vtkIdType> ClusterSelectionSet)
{
	/*! 
	* Adds a unique new cluster
	* replaces cluster if one with the key currently exists
	*/
	this->iter = this->ClusterMap.find(key);
	if ((this->ClusterMap.size() > 0) && (this->iter != this->ClusterMap.end()))
	{
		//Remove old item
		this->ClusterMap.erase(iter);
	}
	this->ClusterMap[key] = ClusterSelectionSet;
	emit ClusterChanged();
	return true;
}

bool SelectiveClustering::RemoveCluster(vtkIdType key)
{
	/*! 
	* Find and removes cluster and selections
	*/
	this->iter = this->ClusterMap.find(key);
	if (this->iter != this->ClusterMap.end())
	{
		//found and removed
		this->ClusterMap.erase(this->iter);
		emit ClusterChanged();
		return true;
	}
	//not found in map
	return false;
}

void SelectiveClustering::ClearClusters()
{
	/*! 
	* All Clusters must go
	*/
	this->ClusterMap.clear();
	this->CreateClusterTableHeaders();
	emit ClusterChanged();
}

void SelectiveClustering::AddSelectionToCluster(vtkIdType key, vtkIdType ID)
{
	/*!
	* Modify Cluster specifed by the key to add new selections
	*/
	this->iter = this->ClusterMap.find(key);
	(*this->iter).second.insert(ID);
}

void SelectiveClustering::AddSelectionToCluster(vtkIdType key, std::set<vtkIdType> ClusterSelectionSet)
{
	/*! 
	* Insert range of selections into existing cluster
	*/
	this->iter = this->ClusterMap.find(key);
	(*this->iter).second.insert(ClusterSelectionSet.begin(), ClusterSelectionSet.end());
}

void SelectiveClustering::RemoveSelectionFromCluster(vtkIdType key, vtkIdType ID)
{
	/*! 
	* Modifies selection in cluster fount by key
	*/
	this->iter = this->ClusterMap.find(key);
	(*this->iter).second.erase(ID);
}

void SelectiveClustering::RemoveSelectionFromCluster(vtkIdType key, std::set<vtkIdType> ClusterSelectionSet)
{
	/*! 
	* finds and removes selection from specified cluster
	*/
	this->iter = this->ClusterMap.find(key);
	std::set< vtkIdType > Selection = (*this->iter).second;
	std::set< vtkIdType >::iterator LocalIterr = ClusterSelectionSet.begin();
	for (; LocalIterr != ClusterSelectionSet.end(); LocalIterr++)
	{
		Selection.erase(*LocalIterr);
	}
	(*this->iter).second = Selection;
}

vtkIdType SelectiveClustering::ClusterSelectionSize(vtkIdType key)
{
	/*! 
	* How Many objects in the cluster
	*/
	this->iter = this->ClusterMap.find(key);
	std::set< vtkIdType > Selection = (*this->iter).second;
	return (vtkIdType) Selection.size();
}

vtkIdType SelectiveClustering::NumberOfClusters()
{
	/*! 
	* Count of clusters created
	*/
	return (vtkIdType) this->ClusterMap.size();
}

vtkIdType SelectiveClustering::GetNumberOfSelections()
{
	/*! 
	* Count of total number of selections
	*/
	vtkIdType totalCount = 0;
	this->iter = this->ClusterMap.begin();
	for (; this->iter != this->ClusterMap.end(); this->iter++)
	{
		std::set< vtkIdType > tempSet = (*this->iter).second;
		totalCount += tempSet.size();
	}
	return totalCount;
}

std::set< vtkIdType > SelectiveClustering::GetClusterIDs()
{
	/*! 
	* returns set containing the ID of each cluster
	*/
	std::set< vtkIdType > ClusterIDs;
	this->iter = this->ClusterMap.begin();
	for (; this->iter != this->ClusterMap.end(); this->iter++)
	{
		ClusterIDs.insert((*this->iter).first);
	}
	return ClusterIDs;
}
QStringList SelectiveClustering::GetClusterIDsList()
{
	/*! 
	* Returns QSTring List of Cluster Ids for display
	*/
	QStringList clusterList;
	this->iter = this->ClusterMap.begin();
	for (; this->iter != this->ClusterMap.end(); this->iter++)
	{
		clusterList << QString::number((int)(*this->iter).first);
	}
	return clusterList;
}
std::set< vtkIdType > SelectiveClustering::SelectionFromCluster(vtkIdType key)
{
	/*! 
	* returns the selection set from specified cluster
	*/
	this->iter = this->ClusterMap.find(key);
	return (*iter).second;
}

std::set< vtkIdType > SelectiveClustering::GetAllSelections()
{
	/*! 
	* returns one selection set for all clusters
	*/
	std::set< vtkIdType > selections;
	this->iter = this->ClusterMap.begin();
	for (; this->iter != this->ClusterMap.end(); this->iter++)
	{
		std::set< vtkIdType > tempSet = (*this->iter).second;
		selections.insert(tempSet.begin(), tempSet.end());
	}
	return selections;
}

vtkSmartPointer<vtkTable> SelectiveClustering::GetClusterTable()
{
	return this->ClusterTable;
}


bool SelectiveClustering::SetObjectTable(vtkSmartPointer<vtkTable>InputObjectTable)
{
	/*! 
	* Set table of objects to create selections and clusters from
	*/
	vtkIdType rows = InputObjectTable->GetNumberOfRows();
	if ( rows == 0)
	{
		return false;
	}
	this->ObjectTable->Initialize();
	this->ObjectTable = InputObjectTable;
	this->NumberOfObjects = rows;
	for ( vtkIdType currow = 0; currow <= this->NumberOfObjects; currow++ )
	{
		vtkIdType rowObjId = this->ObjectTable->GetValue( currow, 0 ).ToTypeInt64();
		this->ObjectTableIDMap[ rowObjId ] = currow;
	}
	emit DataChanged();
	return true;
}

vtkSmartPointer<vtkTable> SelectiveClustering::GetTableOfAllSelected()
{
	/*! 
	* Return a table containing all objects referenced in the clusters 
	*/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	selectedTable->Initialize();
	std::set< vtkIdType > selectedIDs = this->GetAllSelections();
	//std::cout<< " Total obj Sel " << selectedIDs.size() << std::endl;
	if ( selectedIDs.size() == 0)
	{
		return selectedTable; //should it return null?
	}
	this->CopySelectedIntoTable(selectedIDs, selectedTable);

	return selectedTable;
}

vtkSmartPointer<vtkTable> SelectiveClustering::GetTableOfSelectedFromCluster(vtkIdType key)
{
	/*! 
	* Return a table containing all objects from specified cluster
	*/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	selectedTable->Initialize();
	std::set< vtkIdType > selectedIDs = this->SelectionFromCluster(key);
	if ( selectedIDs.size() == 0)
	{
		return selectedTable; //should it return null?
	}
	this->CopySelectedIntoTable(selectedIDs, selectedTable);

	return selectedTable;
}

void SelectiveClustering::CopySelectedIntoTable(std::set<vtkIdType> selectedIDs, vtkSmartPointer<vtkTable> selectedTable)
{
	/*! 
	* Populate a table containing selected objects 
	*/
	//Add Header Columns
	for(vtkIdType NumberOfColumns = 0; NumberOfColumns < this->ObjectTable->GetNumberOfColumns(); NumberOfColumns++ )
	{
		vtkSmartPointer<vtkVariantArray> col = vtkSmartPointer<vtkVariantArray>::New();
		col->SetName(this->ObjectTable->GetColumnName(NumberOfColumns));
		selectedTable->AddColumn(col);
	}
	// 
	std::set< vtkIdType >::iterator selectionIter = selectedIDs.begin();
	for (; selectionIter != selectedIDs.end(); selectionIter++)
	{
		vtkIdType curSelection = *selectionIter;
		this->TableIDIter = this->ObjectTableIDMap.find(curSelection);
		if (this->TableIDIter != this->ObjectTableIDMap.end())
		{
			vtkVariantArray * RowCopy = this->ObjectTable->GetRow((*this->TableIDIter).second);
			selectedTable->InsertNextRow(RowCopy);
		}
	}
	////Code to compare timing map vs search

	//vtkIdType NumRows = this->ObjectTable->GetNumberOfRows();
	//for (vtkIdType row = 0; row <= this->NumberOfObjects; row++)
	//{
	//	vtkIdType rowObjId = this->ObjectTable->GetValue(row, 0).ToTypeInt64();
	//	//std::cout << "Searching for obj: " << rowObjId << std::endl;
	//	std::set< vtkIdType >::iterator FoundAt = selectedIDs.find(rowObjId);
	//	if (FoundAt != selectedIDs.end())
	//	{
	//		//std::cout << "found obj: " << rowObjId << std::endl;
	//		vtkVariantArray * RowCopy = this->ObjectTable->GetRow(row);
	//		selectedTable->InsertNextRow(RowCopy);
	//	}
	//}
}

void SelectiveClustering::CreateClusterTableHeaders()
{
	/*!
	* Creates an empty table then sets col headers to id size and name
	*/
	this->ClusterTable->Initialize();

	vtkSmartPointer<vtkVariantArray> IDcol = vtkSmartPointer<vtkVariantArray>::New();
	IDcol->SetName("Cluster ID");
	this->ClusterTable->AddColumn(IDcol);

	vtkSmartPointer<vtkVariantArray> Countcol = vtkSmartPointer<vtkVariantArray>::New();
	Countcol->SetName("Number Of Objects");
	this->ClusterTable->AddColumn(Countcol);

	vtkSmartPointer<vtkVariantArray> Namecol = vtkSmartPointer<vtkVariantArray>::New();
	Namecol->SetName("Cluster Name");
	this->ClusterTable->AddColumn(Namecol);
}

std::set< vtkIdType > SelectiveClustering::cluster_operator_ADD(vtkIdType key1, vtkIdType key2)
{
	/*!
	*  	adds two clusters - Union
	*   @param1 : vtkIdType key1 argument, 
	*	@param2	: vtkIdType argument
	*	@return the vtktable with the result
	**/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	std::set< vtkIdType > selectedIDs = this->SelectionFromCluster(key1);
	std::set< vtkIdType > tempSet = this->SelectionFromCluster(key2);
	selectedIDs.insert(tempSet.begin(), tempSet.end());
	return selectedIDs;
}


std::set< vtkIdType > SelectiveClustering::cluster_operator_SUBTRACT(vtkIdType key1, vtkIdType key2)
{
	/*!
	*  subtracts cluster with key2 from cluster with key1
	*   @param1 : vtkIdType key1 argument, 
	*	@param2	: vtkIdType argument
	*	@return the vtktable with the result
	**/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	std::set< vtkIdType > selectedIDs = this->SelectionFromCluster(key1);
	std::set< vtkIdType > tempSet = this->SelectionFromCluster(key2);
	std::set< vtkIdType > result ;
	std::set_difference(selectedIDs.begin(),selectedIDs.end(),tempSet.begin(),tempSet.end(),std::inserter(result, result.end()));
	return selectedIDs;
}

std::set< vtkIdType > SelectiveClustering::cluster_operator_AND(vtkIdType key1, vtkIdType key2)
{
	/*!
	*  	subtracts two clusters
	*   @param1 : vtkIdType key1 argument
	*	@param2	: vtkIdType argument
	*	@return the vtktable with the result
	**/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	std::set< vtkIdType > selectedIDs = this->SelectionFromCluster(key1);
	std::set< vtkIdType > tempSet = this->SelectionFromCluster(key2);
	std::set< vtkIdType > result ;
	std::set_intersection(selectedIDs.begin(),selectedIDs.end(),tempSet.begin(),tempSet.end(),std::inserter(result, result.end()));
	return selectedIDs;
}

std::set< vtkIdType > SelectiveClustering::cluster_operator_XOR(vtkIdType key1, vtkIdType key2)
{
	/*!
	*  	subtracts two clusters
	*   @param1 : vtkIdType key1 argument
	*	@param2	: vtkIdType argument
	*	@return the vtktable with the result
	**/
	vtkSmartPointer<vtkTable> selectedTable = vtkSmartPointer<vtkTable>::New();
	std::set< vtkIdType > selectedIDs = this->SelectionFromCluster(key1);
	std::set< vtkIdType > tempSet = this->SelectionFromCluster(key2);
	std::set< vtkIdType > result ;
	std::set_symmetric_difference(selectedIDs.begin(),selectedIDs.end(),tempSet.begin(),tempSet.end(),std::inserter(result, result.end()));
	return selectedIDs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClusterManager::ClusterManager()
{
	/*! 
	* Create Gui 
	*/
	this->ClusterModel = new SelectiveClustering();
	this->setModal(false);

	this->NumObjects = new QLabel(" None ");
	this->NumClusters = new QLabel(" None ");
	this->NumSelected = new QLabel(" None ");

	this->AddClusterButton = new QPushButton(" Add Cluster ");
	connect(this->AddClusterButton, SIGNAL(clicked()), this, SLOT(SelectionToClusterModification()));

	this->ClearClusterButton = new QPushButton(" Clear Cluster ");
	connect(this->ClearClusterButton, SIGNAL(clicked()), this, SLOT(ClearClusters()));
	
	//this->ClusterListView = new QListWidget(this);
	this->ClusterTableView = vtkSmartPointer<vtkQtTableView>::New();
	
	this->OperatorList = new QComboBox(this);
	OperatorList->addItem("ADD");
	OperatorList->addItem("SUBRACT");
	OperatorList->addItem("AND");
	OperatorList->addItem("XOR");

	QFormLayout *InfoLayout = new QFormLayout();
	InfoLayout->addRow("Number of Objects: ", this->NumObjects);
	InfoLayout->addRow("Number of Clusters: ", this->NumClusters);
	InfoLayout->addRow("Number Selected: ", this->NumSelected);
	InfoLayout->addRow("Operator: ", this->OperatorList);
	InfoLayout->addRow(this->AddClusterButton);
	InfoLayout->addRow(this->ClearClusterButton);

	this->MainLayout = new QHBoxLayout;
	//this->MainLayout->addWidget(this->ClusterListView);
	this->MainLayout->addWidget(this->ClusterTableView->GetWidget());
	this->MainLayout->addLayout(InfoLayout);

	this->setLayout(this->MainLayout);
	this->setWindowTitle(tr("Cluster Manager"));
}

void ClusterManager::setClusteringModel(SelectiveClustering * newClusterModel)
{
	/*! 
	* Create link to SelectiveClustering 
	*/
	this->ClusterModel = newClusterModel;
	connect(this->ClusterModel, SIGNAL(ClusterChanged()), this, SLOT(ChangeInClusters()));
}

void ClusterManager::setObjectSelection(ObjectSelection *ObjSelection)
{
	/*! 
	* allows for use of Object Selection
	*/
	this->LegacyObjectSelection = ObjSelection;
	connect(this->LegacyObjectSelection, SIGNAL(changed()), this, SLOT(ChangeInObjectSelection()));

}

void ClusterManager::SelectionToClusterModification()
{
	/*! 
	* Currently test of object Selection to Clusters
	*/
	std::set< vtkIdType > sel = this->ObjectSelectionToIDSet();
	this->ClusterModel->AddCluster(sel);
}

void ClusterManager::ClearClusters()
{
	this->ClusterModel->ClearClusters();
}

void ClusterManager::ChangeInClusters()
{
	/*! 
	* updates information when clusters change
	*/
	int numClust = (int) this->ClusterModel->NumberOfClusters();
	this->NumClusters->setNum(numClust);
	this->NumObjects->setNum((int) this->ClusterModel->GetNumberOfObjects());
	this->NumSelected->setNum((int) this->ClusterModel->GetNumberOfSelections());

	vtkQtTableModelAdapter adapt(this->ClusterModel->GetClusterTable());
	this->ClusterTableView->SetRepresentationFromInput(adapt.GetVTKDataObject());
	this->ClusterTableView->Update();

	//display of clusters in a list
	/*QStringList ClusterList = this->ClusterModel->GetClusterIDsList();
	this->ClusterListView->clear();
	this->ClusterListView->addItems( ClusterList);*/

	//
}

void ClusterManager::ChangeInObjectSelection()
{
	/*! 
	* Signal Slot interface for objectSelection
	*/
}

std::set< vtkIdType > ClusterManager::ObjectSelectionToIDSet()
{
	/*! 
	* convert objectSelection into form selective
	* clustering can use for operations
	*/
	std::set<long int> curSel = this->LegacyObjectSelection->getSelections();
	std::set<long int>::iterator iter = curSel.begin();
	std::set< vtkIdType > Selection;
	for (; iter != curSel.end(); iter++)
	{
		vtkIdType id = (vtkIdType) (*iter);
		Selection.insert(id);
	}
	return Selection;
}