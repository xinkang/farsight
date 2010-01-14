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
#include "ftkPreprocess.h"
#include "itkSimpleFilterWatcher.h"
#include "ftkUtils.h"

ftkPreprocess::ftkPreprocess()
{
	//std::cout<<"I was here"<<std::endl;
	channelNumber = 0;
	myImg = NULL;
}

void ftkPreprocess::MaskImage(std::vector< ftk::Object::Point > roiPoints)
{
	if(!myImg)
		return;

	std::cout << "Applying Mask Filter";

	typedef itk::Image<unsigned char, 2> ImageType2D;
	typedef itk::ExtractImageFilter< InpImageType, ImageType2D > ExtractFilterType;
	typedef itk::PolyLineParametricPath<2> PolylineType;
	typedef itk::PolylineMask2DImageFilter<ImageType2D,PolylineType,ImageType2D> MaskFilterType;
	typedef PolylineType::VertexType VertexType;
	
	//Create itk polyline:
	PolylineType::Pointer polyline = PolylineType::New();
	for(int i=0; i<(int)roiPoints.size()-1; i++)
	{
		VertexType v;
		v[0] = roiPoints.at(i).x;
		v[1] = roiPoints.at(i).y;
		polyline->AddVertex(v);
	}
	
	for(int ch=0; ch<(int)myImg->GetImageInfo()->numChannels; ++ch)
	{
		InpImageType::Pointer inImg3D = myImg->GetItkPtr<InpPixelType>(0,ch);
		InpImageType::RegionType dRegion = inImg3D->GetLargestPossibleRegion();

		ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
		//itk::SimpleFilterWatcher watcher1(extractFilter);
		extractFilter->SetInput(inImg3D);

		for(int z=0; z<(int)myImg->GetImageInfo()->numZSlices; ++z)
		{
			std::cout << ".";	//I am going to extract 2D Slices from the image and apply the polyline mask to each
			
			dRegion.SetIndex(2,z);
			dRegion.SetSize(2,0);
			extractFilter->SetExtractionRegion(dRegion);
			MaskFilterType::Pointer maskFilter = MaskFilterType::New();
			//itk::SimpleFilterWatcher watcher2(maskFilter);
			maskFilter->SetInput1( extractFilter->GetOutput() );
			maskFilter->SetInput2( polyline );
			try
			{
				maskFilter->Update();
			}
			catch( itk::ExceptionObject & err )
			{
				std::cerr << "\nException caught: " << err << std::endl;
			}
	
			//const ftk::Image::Info * info = myImg->GetImageInfo();
			//int bpChunk = info->numRows * info->numColumns * info->bytesPerPix;
			//memcpy( myImg->GetSlicePtr<unsigned char>(0,ch,z), maskFilter->GetOutput()->GetBufferPointer(), bpChunk );

			typedef itk::ImageFileWriter< ImageType2D > WriterType;
			WriterType::Pointer writer = WriterType::New();
			std::string fName = "masked"+ftk::NumToString(z)+".tif";
			writer->SetFileName(fName.c_str());
			writer->SetInput(extractFilter->GetOutput());
			writer->Update();
		}
	}
	std::cout << "done\n";
}

void ftkPreprocess::InvertIntensity(void)
{
	typedef itk::InvertIntensityImageFilter<InpImageType,InpImageType> FilterType;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	filter->InPlaceOn();

	std::cout << "Applying Invert Intensity Filter.........";

	try
	{
		filter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "\nException caught: " << err << std::endl;
	}
	
	std::cout << "done\n";
}

void ftkPreprocess::GADiffusion(void)
{
	typedef itk::GradientAnisotropicDiffusionImageFilter<InpImageType,FloatImageType> FilterType;
	typedef itk::CastImageFilter<FloatImageType,InpImageType> ReverseCastFilter;	
	FilterType::Pointer filter = FilterType::New();
    filter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	filter->SetTimeStep(filterParams[0]);
	filter->SetNumberOfIterations(filterParams[1]);
	filter->SetConductanceParameter(filterParams[2]);
		
	std::cout << "Applying Anisotropic Diffusion Filter.........: " << std::endl;
	//Run the filter:	
	try
	{
		filter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}
 
	//Iterate through output and truncate result:
	FloatImageType::Pointer imf = filter->GetOutput();
	typedef itk::ImageRegionIterator<FloatImageType> IRI;
	IRI iter(imf,imf->GetLargestPossibleRegion());					
	for(iter.GoToBegin();!iter.IsAtEnd(); ++iter)
	{
		float value = iter.Get();
		value = ((value < 0)?0:((value>255)?255:value));
		iter.Set(value);
	}

	//Cast the image back to uchar:
	ReverseCastFilter::Pointer rfilter = ReverseCastFilter::New();
	rfilter->SetInput(filter->GetOutput());
	rfilter->Update();
	
	//Replace myImg:
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rfilter->GetOutput()->GetBufferPointer(), bpChunk );									
																	
	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	WriterType::Pointer writer = WriterType::New();
	std::string fName = this->FN+"_median"+".tif";
	writer->SetFileName(fName.c_str());
	writer->SetInput(mfilter->GetOutput());
	writer->Update();
	 */	 			 
}

void ftkPreprocess::MedianFilter(void)
{
	typedef itk::MedianImageFilter<InpImageType,InpImageType> FilterType;
     
	FilterType::Pointer mFilter = FilterType::New();
	InpImageType::SizeType indexRadius; 
	indexRadius[0] = this->filterParams[0]; // radius along x 
	indexRadius[1] = filterParams[1]; // radius along y 
	indexRadius[2] = filterParams[2]; // radius along y 
	
	mFilter->SetRadius( indexRadius );  
	mFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber) );

    std::cout << "Applying Median Filter.........: " << std::endl;
	
	try
    {
		mFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), mFilter->GetOutput()->GetBufferPointer(), bpChunk );

	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	WriterType::Pointer writer = WriterType::New();
	std::string fName = this->FN+"_median"+".tif";
	writer->SetFileName(fName.c_str());
	writer->SetInput(mfilter->GetOutput());
	writer->Update();
	 */	 
}



void ftkPreprocess::SigmoidFilter(void)
{
	// Declare the anisotropic diffusion vesselness filter
	typedef itk::SigmoidImageFilter< InpImageType,InpImageType>  SigmoidImFilter;

	// Create a vesselness Filter
    SigmoidImFilter::Pointer SigmoidFilter = SigmoidImFilter::New();
    SigmoidFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	SigmoidFilter->SetAlpha(filterParams[0]); 
	SigmoidFilter->SetBeta( filterParams[1] ); 
    SigmoidFilter->SetOutputMinimum( filterParams[2] ); 
    SigmoidFilter->SetOutputMaximum( filterParams[3]); 

	std::cout << "Applying Sigmoid Filter.........: " << std::endl;

	try
    {
		SigmoidFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }
	
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), SigmoidFilter->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::GrayscaleErode(void)
{
	typedef itk::BinaryBallStructuringElement<InpPixelType,3> StructuringElementType;
	typedef itk::GrayscaleErodeImageFilter<InpImageType,InpImageType,StructuringElementType > ErodeFilterType;
	ErodeFilterType::Pointer  grayscaleErode  = ErodeFilterType::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	grayscaleErode->SetKernel(  structuringElement );	 
	grayscaleErode->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	std::cout << "Applying Grayscale Erosion Filter.........: " << std::endl;

	try
    {
		grayscaleErode->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), grayscaleErode->GetOutput()->GetBufferPointer(), bpChunk );
}




void ftkPreprocess::GrayscaleDilate(void)
{
	typedef itk::BinaryBallStructuringElement<InpPixelType,3> StructuringElementType;
	typedef itk::GrayscaleDilateImageFilter<InpImageType,InpImageType,StructuringElementType> DilateFilterType;	
	DilateFilterType::Pointer  grayscaleDilate  = DilateFilterType::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	grayscaleDilate->SetKernel(structuringElement);	 
	grayscaleDilate->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	std::cout << "Applying Grayscale Dilation Filter.........: " << std::endl;

	try
    {
		grayscaleDilate->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), grayscaleDilate->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::GrayscaleOpen(void)
{
	typedef itk::BinaryBallStructuringElement<InpPixelType,3> StructuringElementType;
	typedef itk::GrayscaleMorphologicalOpeningImageFilter<InpImageType,InpImageType,StructuringElementType >  OpenFilterType;
	OpenFilterType::Pointer  grayscaleOpen  = OpenFilterType::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	grayscaleOpen->SetKernel(  structuringElement );	 
	grayscaleOpen->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	std::cout << "Applying Grayscale Opening Filter.........: " << std::endl;

	try
	{
		grayscaleOpen->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), grayscaleOpen->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::GrayscaleClose(void)
{
	typedef itk::BinaryBallStructuringElement<InpPixelType,3> StructuringElementType;
	typedef itk::GrayscaleMorphologicalClosingImageFilter<InpImageType,InpImageType,StructuringElementType > CloseFilterType;

	CloseFilterType::Pointer  grayscaleClose  = CloseFilterType::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	grayscaleClose->SetKernel(structuringElement );	 
	grayscaleClose->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	std::cout << "Applying Grayscale Closing Filter.........: " << std::endl;

	try
	{
		grayscaleClose->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), grayscaleClose->GetOutput()->GetBufferPointer(), bpChunk );
}
 
void ftkPreprocess::Resample(void)
{
	typedef float InternalPixelType;
	typedef itk::Image< InternalPixelType, 3 > InternalImageType;	
	typedef itk::IntensityWindowingImageFilter< InpImageType,InternalImageType > IntensityFilterType;
	typedef itk::RecursiveGaussianImageFilter< InpImageType,InpImageType > GaussianFilterType;
		
	GaussianFilterType::Pointer smootherX = GaussianFilterType::New();
	GaussianFilterType::Pointer smootherY = GaussianFilterType::New();

	smootherX->SetInput( myImg->GetItkPtr<InpPixelType>(0,channelNumber) );
    smootherY->SetInput( smootherX->GetOutput() );
		
	InpImageType::Pointer inputImage = myImg->GetItkPtr<InpPixelType>(0,0);
	const InpImageType::SpacingType& inputSpacing = inputImage->GetSpacing();
	const double isoSpacing = vcl_sqrt( inputSpacing[2] * inputSpacing[0] );
    smootherX->SetSigma( isoSpacing );
	smootherY->SetSigma( isoSpacing );
	smootherX->SetDirection( 0 );
	smootherY->SetDirection( 1 );
	smootherX->SetNormalizeAcrossScale( true );
	smootherY->SetNormalizeAcrossScale( true );

	typedef unsigned char   OutputPixelType;
	typedef itk::Image< OutputPixelType,3 >   OutputImageType;
    typedef itk::ResampleImageFilter< InpImageType, OutputImageType >  ResampleFilterType;
    ResampleFilterType::Pointer resampler = ResampleFilterType::New();	
				
	typedef itk::IdentityTransform< double, 3 >  TransformType;
    TransformType::Pointer transform = TransformType::New();
    transform->SetIdentity();
	resampler->SetTransform( transform );
						
	typedef itk::LinearInterpolateImageFunction< InpImageType, double >  InterpolatorType;
	InterpolatorType::Pointer interpolator = InterpolatorType::New();
	resampler->SetInterpolator( interpolator );
	resampler->SetDefaultPixelValue( 255 ); // highlight regions without source				
	resampler->SetOutputOrigin( inputImage->GetOrigin() );
	resampler->SetOutputDirection(inputImage->GetDirection() );
									
	InpImageType::SizeType inputSize = inputImage->GetLargestPossibleRegion().GetSize();
  
	typedef InpImageType::SizeType::SizeValueType SizeValueType;

	const double dx = inputSize[0] * inputSpacing[0] / isoSpacing;
	const double dy = inputSize[1] * inputSpacing[1] / isoSpacing;
	const double dz = (inputSize[2] - 1 ) * inputSpacing[2] / isoSpacing;
	InpImageType::SizeType size;

	size[0] = static_cast<SizeValueType>( dx );
	size[1] = static_cast<SizeValueType>( dy );
	size[2] = static_cast<SizeValueType>( dz );	
																											
	resampler->SetSize( size );																																																																											
	resampler->SetInput( smootherY->GetOutput() );
		
	std::cout << "Resampling the image.........: " << std::endl;
		
	resampler->Update();
		
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), resampler->GetOutput()->GetBufferPointer(), bpChunk );		
}

void ftkPreprocess::CurvAnisotropicDiffusion(void)
{
	typedef itk::CurvatureAnisotropicDiffusionImageFilter<InpImageType,FloatImageType> FilterType;
	typedef itk::CastImageFilter<FloatImageType,InpImageType> ReverseCastFilter;	
	FilterType::Pointer cfilter = FilterType::New();
    cfilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	cfilter->SetTimeStep(filterParams[0]);
	cfilter->SetNumberOfIterations(filterParams[1]);
	cfilter->SetConductanceParameter(filterParams[2]);
		
	std::cout << "Applying Curvature Anisotropic Diffusion Filter.........: " << std::endl;
		  
	try
	{
		cfilter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}
 
	FloatImageType::Pointer imf = cfilter->GetOutput();
	typedef itk::ImageRegionIterator<FloatImageType> IRI;
	IRI iter(imf,imf->GetLargestPossibleRegion());				
	for(iter.GoToBegin();!iter.IsAtEnd(); ++iter)
	{
		float value = iter.Get();
		value = ((value < 0)?0:((value>255)?255:value));
		iter.Set(value);
	}

	ReverseCastFilter::Pointer rfilter = ReverseCastFilter::New();
	rfilter->SetInput(cfilter->GetOutput());
	rfilter->Update();

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rfilter->GetOutput()->GetBufferPointer(), bpChunk );		
											
	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	WriterType::Pointer writer = WriterType::New();
	std::string fName = this->FN+"_median"+".tif";
	writer->SetFileName(fName.c_str());
	writer->SetInput(mfilter->GetOutput());
	writer->Update();*/	 
}

void ftkPreprocess::OpeningbyReconstruction(void)
{
	typedef itk::BinaryBallStructuringElement< InpPixelType, 3 > StructuringElementType;
	typedef itk::OpeningByReconstructionImageFilter< InpImageType,InpImageType,StructuringElementType > ROpenFilter;

	ROpenFilter::Pointer  RgrayscaleOpen  = ROpenFilter::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	RgrayscaleOpen->SetKernel(structuringElement );	 
	RgrayscaleOpen->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	if(filterParams[1])
	{
		RgrayscaleOpen->PreserveIntensitiesOn();
	}
	else
	{
		RgrayscaleOpen->PreserveIntensitiesOff();				   
	}				   
					   
					   
	if(filterParams[2])
	{
		RgrayscaleOpen->FullyConnectedOn ();
	}
	else
	{
		RgrayscaleOpen->FullyConnectedOff ();	
	}
					   
					   
	std::cout << "Applying Grayscale Opening by Reconstruction Filter.........: " << std::endl;

	try
	{
		RgrayscaleOpen->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), RgrayscaleOpen->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::ClosingbyReconstruction(void)
{
	typedef itk::BinaryBallStructuringElement<InpPixelType,3> StructuringElementType;
	typedef itk::ClosingByReconstructionImageFilter<InpImageType,InpImageType,StructuringElementType > RCloseFilter;

	RCloseFilter::Pointer  RgrayscaleClose  = RCloseFilter::New();
	StructuringElementType  structuringElement;
	structuringElement.CreateStructuringElement();
	structuringElement.SetRadius(filterParams[0]);
	RgrayscaleClose->SetKernel(structuringElement );	 
	RgrayscaleClose->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

	if(filterParams[1])
	{
		RgrayscaleClose->PreserveIntensitiesOn();
	}
	else
	{
		RgrayscaleClose->PreserveIntensitiesOff();				   
	}				   
					   
					   
	if(filterParams[2])
	{
		RgrayscaleClose->FullyConnectedOn ();
	}
	else
	{
		RgrayscaleClose->FullyConnectedOff ();	
	}
					   
					   
	std::cout << "Applying Grayscale Closing by Reconstruction Filter.........: " << std::endl;

	try
	{
		RgrayscaleClose->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), RgrayscaleClose->GetOutput()->GetBufferPointer(), bpChunk );

}

void ftkPreprocess::MeanFilter(void)
{
	typedef itk::MeanImageFilter<InpImageType,InpImageType> FilterType;     
	FilterType::Pointer mFilter = FilterType::New();
	InpImageType::SizeType indexRadius; 
	indexRadius[0] = filterParams[0]; // radius along x 
	indexRadius[1] = filterParams[1]; // radius along y 
	indexRadius[2] = filterParams[2]; // radius along y 

	mFilter->SetRadius( indexRadius ); 
	mFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber) );
	
    std::cout << "Applying Mean Filter.........: " << std::endl;
	
	try
    {
		mFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), mFilter->GetOutput()->GetBufferPointer(), bpChunk );

	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	WriterType::Pointer writer = WriterType::New();
	std::string fName = this->FN+"_median"+".tif";
	writer->SetFileName(fName.c_str());
	writer->SetInput(mfilter->GetOutput());
	writer->Update();
	 */	 
}

void ftkPreprocess::LaplacianFilter(void)
{
	typedef unsigned char CharPixelType;  //IO
	typedef double RealPixelType;  //Operations

	const unsigned int Dimension = 2;

	typedef itk::Image<CharPixelType, Dimension> CharImageType;
	typedef itk::Image<RealPixelType, Dimension> RealImageType;
  
	typedef itk::ImageFileReader< CharImageType > ReaderType;
	typedef itk::ImageFileWriter< CharImageType > WriterType;

	typedef itk::CastImageFilter<InpImageType, DoubleImageType> CastToDoubleFilterType;
	typedef itk::CastImageFilter<DoubleImageType, InpImageType> CastToCharFilterType;

	typedef itk::RescaleIntensityImageFilter< DoubleImageType, DoubleImageType > RescaleFilter;
	typedef itk::LaplacianImageFilter< DoubleImageType, DoubleImageType > LaplacianFilter;
	typedef itk::ZeroCrossingImageFilter< DoubleImageType, DoubleImageType > ZeroCrossingFilter;

	CastToDoubleFilterType::Pointer toDouble = CastToDoubleFilterType::New();
	CastToCharFilterType::Pointer toChar = CastToCharFilterType::New();
	RescaleFilter::Pointer rescale = RescaleFilter::New();

	//Setting the ITK pipeline filter
	LaplacianFilter::Pointer lapFilter = LaplacianFilter::New();
	ZeroCrossingFilter::Pointer zeroFilter = ZeroCrossingFilter::New();  
  
	//The output of an edge filter is 0 or 1
	rescale->SetOutputMinimum(   0 );
	rescale->SetOutputMaximum( 255 );

	toDouble->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	toChar->SetInput( rescale->GetOutput() );
	//writer->SetInput( toChar->GetOutput() );

	//Edge Detection by Laplacian Image Filter:
	lapFilter->SetInput( toDouble->GetOutput() );
	zeroFilter->SetInput(lapFilter->GetOutput() );
	rescale->SetInput(zeroFilter->GetOutput() );

	try
    {
		rescale->Update();
    }
	catch( itk::ExceptionObject & err )
    { 
		std::cout << "ExceptionObject caught !" << std::endl; 
		std::cout << err << std::endl; 
	} 
	
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rescale->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::ThreeDSmoothingRGFilter(void)
{
	typedef itk::RecursiveGaussianImageFilter<FloatImageType,FloatImageType> RGFilterType;     
	RGFilterType::Pointer RGFilter = RGFilterType::New();
	 
	RGFilterType::Pointer filterX = RGFilterType::New(); 
	RGFilterType::Pointer filterY = RGFilterType::New();
	RGFilterType::Pointer filterZ = RGFilterType::New();
	 
	filterX->SetDirection( 0 ); // 0 --> X direction 
	filterY->SetDirection( 1 ); // 1 --> Y direction 
	filterZ->SetDirection( 2 ); // 2 --> Z direction 
	 
	filterX->SetOrder( RGFilterType::ZeroOrder ); 
	filterY->SetOrder( RGFilterType::ZeroOrder ); 
	filterZ->SetOrder( RGFilterType::ZeroOrder ); 
	  
	filterX->SetInput(myImg->GetItkPtr<FloatPixelType>(0,channelNumber)); 
	filterY->SetInput( filterX->GetOutput() ); 
	filterZ->SetInput( filterY->GetOutput() );	
	  
	filterX->SetSigma( filterParams[0] ); 
	filterY->SetSigma( filterParams[0] ); 
	filterZ->SetSigma( filterParams[0] ); 
 
	if(filterParams[2])
	{ 
		filterX->SetNormalizeAcrossScale( true ); 
		filterY->SetNormalizeAcrossScale( true ); 
		filterZ->SetNormalizeAcrossScale( true ); 
	}
	else
	{
		filterX->SetNormalizeAcrossScale( false ); 
		filterY->SetNormalizeAcrossScale( false ); 
		filterZ->SetNormalizeAcrossScale( false );	
	}

	filterX->SetNumberOfThreads ( filterParams[1] ); 
	filterY->SetNumberOfThreads ( filterParams[1] ); 
	filterZ->SetNumberOfThreads ( filterParams[1] );	
 	 
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), filterZ->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::NormalizeImage(void)
{
	typedef itk::NormalizeImageFilter<InpImageType,FloatImageType> FilterType;
	FilterType::Pointer nFilter = FilterType::New();
	nFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber) );
	 	
    std::cout << "Applying Normalizing Filter.........: " << std::endl;
	
	try
    {
		nFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), nFilter->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::ShiftScale(void)
{	 
	typedef itk::ShiftScaleImageFilter<InpImageType,InpImageType> FilterType;
	FilterType::Pointer SSFilter = FilterType::New();
	std::cout<<filterParams[0]<<std::endl;
	SSFilter->SetShift(filterParams[0]);
	SSFilter->SetScale(filterParams[1]);
	SSFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
				
    std::cout << "Applying the Shift and Scale Filter.........: " << std::endl;
	
	try
    {
		SSFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), SSFilter->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::SobelEdgeDetection(void)
{
	typedef itk::SobelEdgeDetectionImageFilter<InpImageType,InpImageType> FilterType;
     
	FilterType::Pointer SEFilter = FilterType::New();
	SEFilter->SetNumberOfThreads(filterParams[0]);
	SEFilter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));

    std::cout << "Applying the Sobel Edge Detetction Filter.........: " << std::endl;
	
	try
    {
		SEFilter->Update();
    }
	catch( itk::ExceptionObject & err )
    {
		std::cerr << "Exception caught: " << err << std::endl;
    }

	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), SEFilter->GetOutput()->GetBufferPointer(), bpChunk );
}

void ftkPreprocess::CurvatureFlow(void)
{
	typedef itk::CurvatureFlowImageFilter<InpImageType,FloatImageType> FilterType;
	typedef itk::CastImageFilter<FloatImageType,InpImageType> ReverseCastFilter;	
	FilterType::Pointer filter = FilterType::New();
    filter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	filter->SetTimeStep(filterParams[0]);
	filter->SetNumberOfIterations(filterParams[1]);
		 		
	std::cout << "Applying Curvature Flow Filter.........: " << std::endl;
		  
	try
	{
		filter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}
 
	FloatImageType::Pointer imf = filter->GetOutput();
	typedef itk::ImageRegionIterator<FloatImageType> IRI;
	IRI iter(imf,imf->GetLargestPossibleRegion());					
	for(iter.GoToBegin();!iter.IsAtEnd(); ++iter)
	{
		float value = iter.Get();
		value = ((value < 0)?0:((value>255)?255:value));
		iter.Set(value);
	}

	ReverseCastFilter::Pointer rfilter = ReverseCastFilter::New();
	rfilter->SetInput(filter->GetOutput());
	rfilter->Update();
		
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rfilter->GetOutput()->GetBufferPointer(), bpChunk );								
																	
	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	   WriterType::Pointer writer = WriterType::New();
	   std::string fName = this->FN+"_median"+".tif";
	   writer->SetFileName(fName.c_str());
	   writer->SetInput(mfilter->GetOutput());
	   writer->Update();
	 */	 			 
}


void ftkPreprocess::MinMaxCurvatureFlow(void)
{
	typedef itk::MinMaxCurvatureFlowImageFilter<InpImageType,FloatImageType> FilterType;
	typedef itk::CastImageFilter<FloatImageType,InpImageType> ReverseCastFilter;	
	FilterType::Pointer filter = FilterType::New();
    filter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	filter->SetTimeStep(filterParams[0]);
	filter->SetNumberOfIterations(filterParams[1]);
	filter->SetStencilRadius(filterParams[2]);
		 		
	std::cout << "Applying Curvature Flow Filter.........: " << std::endl;
		  
	try
	{
		filter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}
 
	FloatImageType::Pointer imf = filter->GetOutput();
	typedef itk::ImageRegionIterator<FloatImageType> IRI;
	IRI iter(imf,imf->GetLargestPossibleRegion());				
	for(iter.GoToBegin();!iter.IsAtEnd(); ++iter)
	{
		float value = iter.Get();
		value = ((value < 0)?0:((value>255)?255:value));
		iter.Set(value);
	}

	ReverseCastFilter::Pointer rfilter = ReverseCastFilter::New();
	rfilter->SetInput(filter->GetOutput());
	rfilter->Update();
		
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rfilter->GetOutput()->GetBufferPointer(), bpChunk );								
																	
	/* typedef itk::ImageFileWriter<imagetype > WriterType;
	   WriterType::Pointer writer = WriterType::New();
	   std::string fName = this->FN+"_median"+".tif";
	   writer->SetFileName(fName.c_str());
	   writer->SetInput(mfilter->GetOutput());
	   writer->Update();
	 */	 	 
}
	
void ftkPreprocess::VesselFilter(void)
{
	// typedef itk::AnisotropicDiffusionVesselEnhancementFilter<InpImageType,FloatImageType> FilterType;
	/* typedef itk::CastImageFilter<FloatImageType,InpImageType> ReverseCastFilter;	
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(myImg->GetItkPtr<InpPixelType>(0,channelNumber));
	filter->SetSigmaMin(filterParams[0]);
	filter->SetSigmaMax(filterParams[1]);
	filter->SetNumberOfSigmaSteps(filterParams[2]);
	filter->SetNumberOfIterations(filterParams[3]);
	 
	std::cout << "Applying Anisotropic Diffusion Vessel Enhancement Filter.........: " << std::endl;
				   
	try
	{
		filter->Update();
	}
	catch( itk::ExceptionObject & err )
	{
		std::cerr << "Exception caught: " << err << std::endl;
	}
		  
	FloatImageType::Pointer imf = filter->GetOutput();
	typedef itk::ImageRegionIterator<FloatImageType> IRI;
	IRI iter(imf,imf->GetLargestPossibleRegion());			 
	for(iter.GoToBegin();!iter.IsAtEnd(); ++iter)
	{
		float value = iter.Get();
		value = ((value < 0)?0:((value>255)?255:value));
		iter.Set(value);
	}

	ReverseCastFilter::Pointer rfilter = ReverseCastFilter::New();
	rfilter->SetInput(filter->GetOutput());
	rfilter->Update();
				 
	const ftk::Image::Info * info = myImg->GetImageInfo();
	int bpChunk = info->numZSlices * info->numRows * info->numColumns * info->bytesPerPix;
	memcpy( myImg->GetDataPtr(0,channelNumber), rFilter->GetOutput()->GetBufferPointer(), bpChunk );			
	*/
}
	
















