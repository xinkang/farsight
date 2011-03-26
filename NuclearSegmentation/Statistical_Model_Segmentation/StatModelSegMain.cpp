#include "model_nucleus_seg.h"
#include "seg_graphs.h"
#include "ftkLabelImageToFeatures.h"
#include "ftkIntrinsicFeatures.h"

#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkMedianImageFilter.h>


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
	
	argv[1] = "C:/Data/S 99 5276 pAKT F02/S 99 5276_pAKT_CD34 AF488 CA9 AF647 SMA Cy3_IP8_F02_40XQB_Nuclear(H).tif";
	argv[2] = "C:/Data/S 99 5276 pAKT F02/S 99 5276_pAKT_CD34 AF488 CA9 AF647 SMA Cy3_IP8_F02_40XQB_Nuclear(H)_label_nuc.tif";
	argv[3] = "4";
	argv[4] = "C:/Data/S 99 5276 pAKT F02/training3.txt";
	argv[5] = "C:/Data/S 99 5276 pAKT F02/Histo_Input_Image.xml";
	argv[6] = "C:/Data/S 99 5276 pAKT F02/HistoProjectDef3.xml";
	argv[7] = "C:/Data/S 99 5276 pAKT F02/finalseg.tif";	

	typedef itk::Image< unsigned char,3>InputImageType;
	typedef itk::Image< unsigned short,3> OutputImageType;
	
	model_nucleus_seg *MNS = new model_nucleus_seg();
	MNS->SetRawImage(argv[1]);
	MNS->SetLabelImage((argv[2]));
	MNS->SetAssocFeatNumber(atoi(argv[3]));

	int numfeat = MNS->getNumFeat();
	MNS->SetTrainingFile(argv[4],numfeat+atoi(argv[3]));
	MNS->getFeatureNames(argv[4]);
	

	////Compute the associations
	MNS->Associations(argv[5],argv[6]);
	std::vector<unsigned short> US_Cell_ids = MNS->Detect_undersegmented_cells();

	//if(US_Cell_ids.size()>0)
	//{
	//	typedef itk::Image< unsigned short, 3 > OutputImageType;
	//	OutputImageType::Pointer  newLabelImage;

	//	newLabelImage = MNS->SplitImage(US_Cell_ids,MNS->inputImage,MNS->bImage);
	//	//Reset the Label Image after splitting
	//	MNS->SetSplitImage(newLabelImage);
	//}


	typedef ftk::LabelImageToFeatures< unsigned char,  unsigned short, 3 > FeatureCalcType;
	typedef ftk::IntrinsicFeatures FeaturesType;


//	//Calculates all the features and stores the individual 
//	//labeled and raw images
	MNS->GetFeatsnImages();


//	//Compute the associations after the split 
	//MNS->Associations(argv[5],argv[6]);


	std::cout<<"Computing Scores"<<std::endl;

//	//Iniscores contains the inital scores of all the fragments
	std::vector<double> IniScores = MNS->GetOriginalScores();

////	 idlist will contain the list of all the ids for which the 
////	 MergeTrees have to be built and analyzed.
	std::vector<unsigned short> idList = MNS->getListofIds(); 	
	std::vector< unsigned short > labelIndex = MNS->getLabelIndex();
	std::vector<FeaturesType> allFeat = MNS->getFeats();
	
  //Loop through the list of ids and build graphs.
	seg_graphs *sg1 = new seg_graphs();
	InputImageType::Pointer im = MNS->getRawImage();
	OutputImageType::Pointer om = MNS->getLabImage();
	sg1->runLabFilter(im,om);
	sg1->setFeats(allFeat,labelIndex);

	

	for (unsigned short r = 0;r < idList.size();r++)
	{		
		FeatureCalcType::LabelPixelType id = idList[r]; 
		sg1->RAG = sg1->BuildRAG(id); 
		//std::vector< std::set<int> > hypothesis = sg1->getHypothesis();
		//Build the Merge Tree from the RAG and root information
		seg_graphs::MTreeType mTree =  sg1->BuildMergeTreeDcon(sg1->RAG,id,sg1->hypotheses);	
		MNS->GetScoresfromKPLS(mTree);	
	}

	MNS->SelectHypothesis();

/////////////////////////////////////////////////////////////////////////////////////////////////////
 //PERFORM MERGES 
/////////////////////////////////////////////////////////////////////////////////////////////////////
MNS->PerformMerges(argv[7]);
}	



//  typedef itk::Image< unsigned char, 3 >InputImageType;
//    typedef itk::Image< unsigned short, 3 > OutputImageType;
//	typedef OutputImageType::RegionType RegionType;
//	typedef itk::ImageRegionIterator< InputImageType > IIteratorType;
//	typedef itk::ImageRegionIterator< OutputImageType> IteratorType;
//	typedef itk::BinaryBallStructuringElement<unsigned char,3 > StructuringElementType;	
//	typedef itk::GrayscaleErodeImageFilter<OutputImageType,OutputImageType,StructuringElementType > ErodeFilterType;
//	typedef itk::GrayscaleDilateImageFilter<OutputImageType,OutputImageType,StructuringElementType > DilateFilterType;
//	typedef itk::MedianImageFilter<InputImageType,InputImageType> MedianFilterType;
//
//	InputImageType::Pointer im;
//	MedianFilterType::Pointer filt = MedianFilterType::New();
//	InputImageType::SizeType radius;
//	radius[0] = 5;
//	radius[1] = 5;
//	radius[1] = 3;
//	filt->SetRadius(radius);
//	
//	InputImageType::Pointer om;
//	OutputImageType::Pointer bImagetemp = OutputImageType::New();
//	
//	im = readImage<InputImageType>("C:/R2080_6wk_site.tif");
//	filt->SetInput(im);
//	InputImageType::SizeType size = im->GetLargestPossibleRegion().GetSize();
//	filt->Update();
//	im = filt->GetOutput();
//
//	RegionType region1;	
//	//Allocate Memory for Images
//	OutputImageType::IndexType start;
//	start.Fill(0);
//	region1.SetSize(size);
//	region1.SetIndex( start );
//	bImagetemp->SetRegions( region1 );
//	bImagetemp->Allocate();
//
//	unsigned char *in_Image;
//	in_Image = (unsigned char *) malloc (size[0]*size[1]*size[2]);
//	//in_Image = (unsigned char *) malloc (size[0]*size[1]*size[2]);
//	memset(in_Image/*destination*/,0/*value*/,size[0]*size[1]*size[2]*sizeof(unsigned char)/*num bytes to move*/);
//	
//	IIteratorType pix_buf(im,im->GetRequestedRegion());
//	int ind=0;
//	for ( pix_buf.GoToBegin(); !pix_buf.IsAtEnd(); ++pix_buf, ++ind )
//		in_Image[ind]=(pix_buf.Get());
//
//
//	yousef_nucleus_seg *NucleusSeg = new yousef_nucleus_seg();
//	NucleusSeg->readParametersFromFile("");
//	NucleusSeg->setDataImage(in_Image,size[0],size[1],size[2],"");
//	
//
//	NucleusSeg->runBinarization();
//
//	unsigned short *output_img;
//	output_img = NucleusSeg->getBinImage();
//
//	IteratorType biterator(bImagetemp,bImagetemp->GetRequestedRegion());
//
//	for(unsigned int i=0; i<size[0]*size[1]*size[2]; i++)
//	{		
//		biterator.Set(output_img[i]);
//		++biterator;	
//	}
//
//	ErodeFilterType::Pointer erode_filter = ErodeFilterType::New();
//	DilateFilterType::Pointer dilate_filter = DilateFilterType::New();
//	StructuringElementType structuringElement;
//	structuringElement.SetRadius( 5 );
//	structuringElement.CreateStructuringElement();
//	erode_filter->SetKernel( structuringElement );
//	dilate_filter->SetKernel( structuringElement );
//	//Morphological opening of the resultant image
//	erode_filter->SetInput(bImagetemp);
//	dilate_filter->SetInput(erode_filter->GetOutput());
//	writeImage<OutputImageType>(erode_filter->GetOutput(),"C:/R2080_6wk_site_openedbinary.tif");
//	
//
//}
