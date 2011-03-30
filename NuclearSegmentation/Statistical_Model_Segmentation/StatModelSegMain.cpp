

#include "model_nucleus_seg.h"


template <typename T>
typename T::Pointer readImage(const char *filename)
{
	printf("Reading %s ... ",filename);
	typedef typename itk::ImageFileReader<T> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	
	ReaderType::GlobalWarningDisplayOff();
	reader->SetFileName(filename);
	try
	{
		reader->Update();
	}
	catch(itk::ExceptionObject &err)
	{
		std::cout << "ExceptionObject caught!" <<std::endl;
		std::cout << err << std::endl;
		//return EXIT_FAILURE;
	}
	printf("Done.\n");
	return reader->GetOutput();
}



template <typename T>
int writeImage(typename T::Pointer im, const char* filename)
{
	printf("Writing %s ... ",filename);
	typedef typename itk::ImageFileWriter<T> WriterType;
	
	typename WriterType::Pointer writer = WriterType::New();
	writer->SetFileName(filename);
	writer->SetInput(im);
	try
	{
		writer->Update();
	}
	catch(itk::ExceptionObject &err)
	{
		std::cout << "ExceptionObject caught!" <<std::endl;
		std::cout << err << std::endl;
		return EXIT_FAILURE;
	}
	printf("Done.\n");
	return EXIT_SUCCESS;
}




int main ( int argc ,  char** argv)
{	
	argc = 6;

	if(argc <3)
	{
		std::cout<<"Usage1: segment_nuclei <InputImageFileName> <OutputImageFileName>\n";
		std::cout<<"Usage2: segment_nuclei <InputImageFileName> <OutputImageFileName> <ParametersFileName>\n";
		return 0;
	}

	clock_t startTimer = clock();
	std::cout<<"reading input image...";
	

	argv[1] = "C:/Data/S_06_3423_1C_pERK_F04/S 06-3423 1C_pERK_CD34 AF488 CA9 AF647 SMA Cy3_IP8_F04_40XQB_Nuclear(H).tif";
	argv[2] = "C:/Data/Phantom/SegParams.ini";
	argv[3] = "C:/Data/S_06_3423_1C_pERK_F04/training3.txt";
	argv[4] = "C:/Data/S_06_3423_1C_pERK_F04/Histo_Input_Image.xml";
	argv[5] = "C:/Data/S_06_3423_1C_pERK_F04/HistoProjectDef3.xml";

	

	typedef itk::Image< unsigned char,3>InputImageType;
	typedef itk::Image< unsigned short,3> OutputImageType;
	
	model_nucleus_seg *MNS = new model_nucleus_seg();
	MNS->SetRawImage(argv[1]);
	MNS->GetYousefSeg();
	MNS->LoadSegParams(argv[2]);
	MNS->SetTrainingFile(argv[3]);
	
	////Compute the associations if associative features provided
	if(MNS->getasscofeatnumb()>0)
		MNS->Associations(argv[4],argv[5]);
	
	std::vector<unsigned short> US_Cell_ids = MNS->Detect_undersegmented_cells();
	
	if(US_Cell_ids.size()>0)
	{
		MNS->bImage = MNS->SplitImage(US_Cell_ids,MNS->inputImage,MNS->bImage);
		MNS->splitflag = 1;
	}

	
	typedef ftk::LabelImageToFeatures< unsigned char,  unsigned short, 3 > FeatureCalcType;
	typedef ftk::IntrinsicFeatures FeaturesType;


	//Calculates all the features and stores the individual 
	//labeled and raw images
	MNS->GetFeatsnImages();


	////Compute the associations after the split for split Ids only
	if(MNS->splitflag==1 && MNS->getasscofeatnumb()>0 )
		MNS->splitAssociations();

	
	//Iniscores contains the inital scores of all the fragments
	std::vector<double> IniScores = MNS->GetOriginalScores();

//	 idlist will contain the list of all the ids for which the 
//	 MergeTrees have to be built and analyzed.
	
	std::vector<unsigned short> idList = MNS->getListofIds(); 	
	std::vector< unsigned short > labelIndex = MNS->getLabelIndex();
	std::vector<FeaturesType> allFeat = MNS->getFeats();
	
	
	//Loop through the list of ids and build graphs.
	ftkgnt *sg1 = new ftkgnt();
	sg1->runLabFilter(MNS->inputImage,MNS->bImage);
	sg1->setFeats(allFeat,labelIndex);


	// 1e5 is the default volume I choose. With this
	// default value, the size of the tree is controlled 
	// only by the depth value and volume has no effect
	// If this limit is specified in the parameters file 
	// that value takes precedence

	if(MNS->MAX_VOL!=1e5)
		sg1->setmaxVol(MNS->MAX_VOL);
	
	// 6 is the default depth I choose. If specidied differently 
	// in the parameters file, then use that value
	if(MNS->MAX_DEPTH!=6)
		sg1->setmaxDepth(MNS->MAX_DEPTH);
		
	
	for (unsigned short r = 0;r < idList.size();r++)
	{		
		FeatureCalcType::LabelPixelType id = idList[r]; 
		cout<<"\rEvaluating Hypothesis for id "<<id;
		sg1->RAG = sg1->BuildRAG(id); 
		//Build the Merge Tree from the RAG and root information
		ftkgnt::MTreeType mTree =  sg1->BuildMergeTreeDcon(sg1->RAG,id,sg1->hypotheses);	
		MNS->GetScoresfromKPLS(mTree);
		//std::cout<<"\rr"<<r;
	}

	 std::cout<<""<<std::endl;
	 MNS->SelectHypothesis();
	
/////////////////////////////////////////////////////////////////////////////////////////////////////
 //PERFORM MERGES 
/////////////////////////////////////////////////////////////////////////////////////////////////////
	MNS->PerformMerges(argv[6]);
}	
