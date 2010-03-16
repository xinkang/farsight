#include "ColorSegmentation.h"

//Constructor
ColorSegmentation::ColorSegmentation(RGBImageType::Pointer input)
{
	rgb_input = input;

	rli_image = NULL;
	bin_image = NULL;

	hist = NULL;

	red_weights = NULL;
	blue_weights = NULL;

	IGNORE_BACKGROUND = false;
	LIGHT_BACKGROUND = false; // normally background is black
	TESTING = false;
	GEN_PROJ = false;
}

void ColorSegmentation::TransformToRLI()
{
	if(!rgb_input)
		return;

	std::cerr << "Transforming to RLI...";

	int size1 = rgb_input->GetLargestPossibleRegion().GetSize()[0];
	int size2 = rgb_input->GetLargestPossibleRegion().GetSize()[1];
	int size3 = rgb_input->GetLargestPossibleRegion().GetSize()[2];

	rli_image = RLIImageType::New();

	RLIImageType::PointType origin;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;
	rli_image->SetOrigin( origin );

	RLIImageType::IndexType start = { 0,0,0 };
	RLIImageType::SizeType  size = { size1, size2, size3 };
	RLIImageType::RegionType region;
	region.SetSize( size );
	region.SetIndex( start );

	rli_image->SetRegions( region ); 
	rli_image->Allocate();                       

	typedef itk::ImageRegionIterator< RLIImageType > IteratorType;
	IteratorType iteratorRLI ( rli_image, rli_image->GetRequestedRegion() );

	typedef itk::ImageRegionConstIterator< RGBImageType > RGBIteratorType;
	RGBIteratorType iteratorRGB( rgb_input, rgb_input->GetRequestedRegion() );

	for( iteratorRLI.GoToBegin(), iteratorRGB.GoToBegin();
	      !iteratorRLI.IsAtEnd() && !iteratorRGB.IsAtEnd();
	      ++iteratorRLI, ++iteratorRGB
		  )
	{ 
		RGBPixelType p_rgb = iteratorRGB.Get();

		dh::_RGB rgb(p_rgb[0], p_rgb[1], p_rgb[2]);
		dh::RLI rli = (dh::RLI)rgb;

		RLIPixelType p_rli;
		p_rli[0] = rli.R;
		p_rli[1] = rli.L;
		p_rli[2] = rli.I;
		iteratorRLI.Set( p_rli );
	}

	if(TESTING)
	{
		typedef itk::ImageFileWriter< RLIImageType > WriterType;
		WriterType::Pointer writer = WriterType::New();

		writer->SetFileName( "rli.tif" );
		writer->SetInput( rli_image );
		writer->Update();
	}

	std::cerr << "...Done" << std::endl;
}

void ColorSegmentation::SetArchetypalColors(dh::RLI r, dh::RLI b, dh::RLI w)
{
	archTypRED = r;
	archTypBLUE = b;
	archTypBACK = w;
}

void ColorSegmentation::SetArchetypalColors(dh::_RGB r, dh::_RGB b, dh::_RGB w)
{
	archTypRED = (dh::RLI)r;
	archTypBLUE = (dh::RLI)b;
	archTypBACK = (dh::RLI)w;

	//this->GenerateColors(w, r, b, "inATColors.tif");
}

//Run Pixel classification
void ColorSegmentation::FindArchetypalColors()
{
	if(!rli_image)
		return;

	std::cerr << "Finding Archetypal Colors..." << std::endl;

	// ========== Create 3D Histogram =========
	if(hist)
		delete hist;
	hist = new dh::Histogram(2);

	typedef itk::ImageRegionConstIterator< RLIImageType > IteratorType;
	IteratorType it( rli_image, rli_image->GetRequestedRegion() );
	for ( it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		RLIPixelType p_rli = it.Get();
		dh::RLI P = dh::RLI( p_rli[0], p_rli[1], p_rli[2] );	//Creating a 3D "pixel"
		hist->inc( P );
	}

	hist->find_bounding_box();
	hist->smooth();
	hist->delete_secondary_blobs();

	std::cout << "  Mode " << hist->modeAsRLI() << " has max frequency " << hist->max_freq << std::endl;

	//===================================================================
	// =========== Find Color Archetypes ================================

	dh::SeedGrid seed_grid( hist, LIGHT_BACKGROUND );

	dh::_RGB a1, a2, bkgrnd; //REMEMBER That a1 and a2 are actually RLI values

	seed_grid.find_seeds( a1, a2, bkgrnd );

	std::cout << "  Seed1 = " << a1.mapRGBtoRLI() << std::endl;
	std::cout << "  Seed2 = " << a2.mapRGBtoRLI() << std::endl;
	std::cout << "  Bgrnd = " << bkgrnd.mapRGBtoRLI() << std::endl;

	seed_grid.find_most_distinct_colors( a1, a2, bkgrnd );

	if( a1.B > a2.B )
	{
		archTypRED = a1.mapRGBtoRLI(); 
		archTypBLUE = a2.mapRGBtoRLI();
	}
	else
	{
		archTypRED = a2.mapRGBtoRLI(); 
		archTypBLUE = a1.mapRGBtoRLI();
	}
 
	archTypBACK = bkgrnd.mapRGBtoRLI();

	std::cout << "  FOUND ARCHETYPES: " << std::endl;
	std::cout << "   RED =        " << archTypRED << std::endl;
	std::cout << "   BLUE =       " << archTypBLUE << std::endl;
	std::cout << "   BACKGROUND = " << archTypBACK << std::endl;

	std::cerr << "...Done" << std::endl;
}

void ColorSegmentation::ComputeClassWeights()
{ 
	std::cerr << "Computing Weight Images..." << std::endl;

	std::cout << "  USING ARCHETYPES: " << std::endl;
	std::cout << "   RED =        " << archTypRED << std::endl;
	std::cout << "   BLUE =       " << archTypBLUE << std::endl;
	std::cout << "   BACKGROUND = " << archTypBACK << std::endl;

	if(TESTING)
	{
		this->GenerateColors(archTypBACK.mapRLItoRGB(), archTypRED.mapRLItoRGB(),archTypBLUE.mapRLItoRGB(),"atRLIColors.tif");
		this->GenerateATColors("atColors.tif");
	}
	
	if(GEN_PROJ)
	{
		std::cout << "  GENERATING PROJECTION IMAGES" << std::endl;
		this->GenerateProjection(1, "projLIs.tif");
		this->GenerateProjection(2, "projRIs.tif");
		this->GenerateProjection(3, "projRLs.tif");
	}

	//Preliminary computations:
	float R_B_axis_len = dh::Classifier::euclidean_dist (archTypRED, archTypBLUE );
	float Y_B_axis_len = dh::Classifier::euclidean_dist (archTypBACK, archTypBLUE );
	float Y_R_axis_len = dh::Classifier::euclidean_dist (archTypBACK, archTypRED );
	
	float slide_wt = 0.7;//0.7; //Set to magic number
	//Set slide_wt to deal with differing spread
	//slide_wt = Y_R_axis_len / ( Y_B_axis_len + Y_R_axis_len );

	//----------- Find decision planes ---------------
	// 1. Find D (split point)
	dh::XYZ r = archTypRED;
	dh::XYZ b = archTypBLUE;
	dh::XYZ w = archTypBACK;
	const double spw = 0.50;
	dh::XYZ d =  (b *(1-spw)+ r*spw);
	dh::XYZ split_point = d;
	
	// 2. Find p (decision plan for 2 colors - represented by normal vector)
	dh::XYZ wb = b - w;
	dh::XYZ wr = r - w;
	dh::XYZ wd = d - w;
	dh::XYZ wbwr = dh::cross( wb, wr );
	dh::XYZ p = dh::cross ( wbwr, wd );
	dh::XYZ decision_plane = p / magnitude ( p );	//decision plane normal vector

	p = dh::cross ( wbwr, wr );
	dh::XYZ red_plane = p / magnitude ( p );	//red plane normal vector
	double red_sp_dist = dh::dot ( (d - r), red_plane ); //scalar projection of RD onto red plane unit normal vector
	//This is the distance between d and the red plane (negative = inside triangle)

	p = dh::cross ( wbwr, wb );
	dh::XYZ blue_plane = p / magnitude ( p );	//blue plane normal vector
	double blue_sp_dist = dh::dot ( (d - b), blue_plane );//scalar projection of BD onto blue plane unit normal vector
	//Distance between d and the blue plane (positive = inside triangle)

	if(TESTING)
	{ 
		std::cerr << "  Distance between red & blue: "
				  << R_B_axis_len << std::endl
			      << "  Distance between bkgrnd & blue: "
				  << Y_B_axis_len << std::endl
			      << "  Distance between bkgrnd & red: "
				  << Y_R_axis_len << std::endl;

		std::cerr //<< "split_point: " << split_point << std::endl
				//<< "decision_plane: " << decision_plane << std::endl
				<< "  red_sp_dist: " << red_sp_dist << std::endl
				<< "  blue_sp_dist: " << blue_sp_dist << std::endl;
  
		std::cerr << "  Using slide_wt: " << slide_wt << std::endl;
	}

	//------------------------------------------------
	// FIND WEIGHTS FOR EACH PIXEL:
	//**************************************
	int size1 = rgb_input->GetLargestPossibleRegion().GetSize()[0];
	int size2 = rgb_input->GetLargestPossibleRegion().GetSize()[1];
	int size3 = rgb_input->GetLargestPossibleRegion().GetSize()[2];

	typedef itk::Image< float, 3> FloatImageType;
	FloatImageType::Pointer red_weights_temp = FloatImageType::New();
	FloatImageType::Pointer blue_weights_temp = FloatImageType::New();

	FloatImageType::PointType origin;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;
	red_weights_temp->SetOrigin( origin ); 
	blue_weights_temp->SetOrigin( origin );

	FloatImageType::IndexType start = { 0,0,0 };
	FloatImageType::SizeType  size = { size1, size2, size3 };
	FloatImageType::RegionType region;
	region.SetSize( size );
	region.SetIndex( start );

	red_weights_temp->SetRegions( region ); 
	blue_weights_temp->SetRegions( region );
	red_weights_temp->Allocate();            
	blue_weights_temp->Allocate();
	red_weights_temp->FillBuffer(0.0);         
	blue_weights_temp->FillBuffer(0.0);
	red_weights_temp->Update();              
	blue_weights_temp->Update();

	typedef itk::ImageRegionIteratorWithIndex< FloatImageType > FloatIteratorType;
	FloatIteratorType iterator1 ( red_weights_temp, red_weights_temp->GetRequestedRegion() );
	FloatIteratorType iterator2 ( blue_weights_temp, blue_weights_temp->GetRequestedRegion() );
	
	typedef itk::ImageRegionConstIterator< RLIImageType > rliIteratorTypeConst;
	rliIteratorTypeConst rli_it( rli_image, rli_image->GetRequestedRegion() );
	
	for ( rli_it.GoToBegin(), iterator1.GoToBegin(), iterator2.GoToBegin();
	      !rli_it.IsAtEnd() && !iterator1.IsAtEnd() && !iterator2.IsAtEnd();
	      ++rli_it, ++iterator1, ++iterator2 )
	{
		RLIPixelType p_rli = rli_it.Get();
		dh::RLI pixel = dh::RLI( p_rli[0], p_rli[1], p_rli[2] );	//Creating a 3D "pixel"

		float red_dist = 2.0 * ( 1.0 - slide_wt ) *
			dh::Classifier::euclidean_dist( pixel, archTypRED );	//Distance to red archtype
		
		float blue_dist = 2.0 * slide_wt *
			dh::Classifier::euclidean_dist( pixel, archTypBLUE );//Distance to blue archtype
		
		float bkgrnd_dist = dh::Classifier::euclidean_dist( pixel, archTypBACK );//Distance to background archtype

		Pixel_Class pixel_class;

		//certainty is a measure of how close to the decision plane the point is
		//does not take into account how close it may be to the background color
		float certainty;  	// 1 = certain; 0 = unknown

		//Distance from pixel to decision plane (pos=red, neg=blue):
		double s_plane_dist = dh::dot ( ((dh::XYZ)pixel - split_point), decision_plane );

		if ( s_plane_dist >= 0 ) 
		{ 
			// Pixel is "red"
			pixel_class = RED_CELL;
			certainty = bkgrnd_dist;
			/*
			//Distance from pixel to red plane (pos=outside, neg=inside)
			double r_plane_dist = dh::dot ( ((dh::XYZ)pixel - (dh::XYZ)archTypRED), red_plane );
			// This could be done with signum functions to make it faster...
			if ( r_plane_dist * red_sp_dist < 0 ) //If signs are different the point is outside the triangle
			{
				certainty = 1.0;
			}
			else
			{
				certainty = s_plane_dist / ( s_plane_dist + fabs(r_plane_dist) );
				//certainty = 0.25;
			}
			*/
		}
		else
		{ 
			// Pixel is "blue"
			pixel_class = BLUE_CELL;
			certainty = bkgrnd_dist;
			/*
			double b_plane_dist = dh::dot ( ((dh::XYZ)pixel - (dh::XYZ)archTypBLUE), blue_plane );
			// This could be done with signum functions to make it faster...
			if ( b_plane_dist * blue_sp_dist < 0 )
			{
				certainty = 1.0;
			}
			else
			{
				s_plane_dist = - s_plane_dist;
				certainty = s_plane_dist / ( s_plane_dist + fabs(b_plane_dist) );
				//certainty = 0.0;
			}
			*/
		}

		float bkgrnd_wt = 1 - certainty;

		if(IGNORE_BACKGROUND && bin_image)
		{
			UcharImageType::IndexType index = iterator1.GetIndex();
			UcharImageType::PixelType pix = bin_image->GetPixel(index);
			if(pix == 0)
			{
				pixel_class = BKGD_FIELD;
			}
		}
		/*
		if(IGNORE_BACKGROUND)
		{
			// If pixel is near background color, ignore it
			if(pixel_class == RED_CELL)
			{ 
				pixel_class = (red_dist * (1 - bkgrnd_wt))
			                < (bkgrnd_dist * bkgrnd_wt)
							? RED_CELL : BKGD_FIELD; 
			}
			else
			{ 
				pixel_class = (blue_dist * (1 - bkgrnd_wt))
			                < (bkgrnd_dist * bkgrnd_wt)
			   				? BLUE_CELL : BKGD_FIELD; 
			}

		}
		else
		{
			// If pixel is near background color, discount it by cubing certainty factor (which is < 1.0)
			if( ( pixel_class == RED_CELL && (red_dist * (1 - bkgrnd_wt) < bkgrnd_dist * bkgrnd_wt))
				||(pixel_class == BLUE_CELL && (blue_dist * (1 - bkgrnd_wt) < bkgrnd_dist * bkgrnd_wt))
				) 
			{
				//certainty = pow(certainty, 3);
			}
		}
		*/
		

		/*
		if( pixel_class != RED_CELL && pixel_class != BLUE_CELL )
		{
			iterator1.Set( blue_dist/(blue_dist+red_dist)*certainty );  //Function1
			iterator2.Set( red_dist/(blue_dist+red_dist) *certainty );  //Function2
		}
		else
		{
			iterator1.Set( (blue_dist+bkgrnd_dist)/(blue_dist+red_dist+bkgrnd_dist)*certainty );  //Function3
			iterator2.Set( (red_dist+bkgrnd_dist) /(blue_dist+red_dist+bkgrnd_dist)*certainty );  //Function4
		}
		*/
		
		switch(pixel_class)
		{
		case BKGD_FIELD:
			break;
		case RED_CELL:
			iterator1.Set(certainty);
			//iterator1.Set(255);
			break;
		case BLUE_CELL:
			iterator2.Set(certainty);
			//iterator2.Set(255);
			break;
		default:
			std::cerr << "  INVALID PIXEL CLASS" << std::endl;
		}
	}

	//Rescale weights:
	typedef itk::RescaleIntensityImageFilter< FloatImageType, UcharImageType > RescaleFlUcType;
	RescaleFlUcType::Pointer rescaleRed = RescaleFlUcType::New();
	RescaleFlUcType::Pointer rescaleBlue = RescaleFlUcType::New();

	rescaleRed->SetOutputMaximum( 255 );
	rescaleRed->SetOutputMinimum( 0 );
	rescaleBlue->SetOutputMaximum( 255 );
	rescaleBlue->SetOutputMinimum( 0 );

	rescaleRed->SetInput( red_weights_temp );
	rescaleRed->Update();
	red_weights = rescaleRed->GetOutput();

	rescaleBlue->SetInput( blue_weights_temp );
	rescaleBlue->Update();
	blue_weights = rescaleBlue->GetOutput();

	if(TESTING)
	{
		typedef itk::ImageFileWriter< UcharImageType > WriterType;
		WriterType::Pointer writer = WriterType::New();
		writer->SetFileName( "red_wts.tif" );
		writer->SetInput( red_weights );
		writer->Update();
		WriterType::Pointer writer1 = WriterType::New();
		writer1->SetFileName( "blue_wts.tif" );
		writer1->SetInput( blue_weights );
		writer1->Update();
	}

	std::cerr << "...Done" << std::endl;
	
}

void ColorSegmentation::GenerateATColors(std::string outFilename)
{
	//Convert RLI archetypes back to RGB:
	dh::_RGB aRed = (dh::_RGB)archTypRED;
	dh::_RGB aBlu = (dh::_RGB)archTypBLUE;
	dh::_RGB aBac = (dh::_RGB)archTypBACK;

	this->GenerateColors(aBac, aRed, aBlu, outFilename);
}

void ColorSegmentation::GenerateColors(dh::_RGB c1, dh::_RGB c2, dh::_RGB c3, std::string outFilename)
{
	int imgSize = 256;

	//I need to create a projection RGB image of the histogram
	typedef itk::Image< RGBPixelType, 2 > RGBImageType2D;
	RGBImageType2D::Pointer colorImage = RGBImageType2D::New();

	RGBImageType2D::PointType origin;
	origin[0] = 0;
	origin[1] = 0;
	colorImage->SetOrigin( origin ); 
	RGBImageType2D::IndexType start = { 0,0 };
	RGBImageType2D::SizeType size = { imgSize, imgSize };
	RGBImageType2D::RegionType region;
	region.SetSize( size );
	region.SetIndex( start );
	colorImage->SetRegions( region );
	colorImage->Allocate();
	RGBPixelType p_rgb;
	p_rgb[0] = 0;
	p_rgb[1] = 0;
	p_rgb[2] = 0;
	colorImage->FillBuffer(p_rgb);         
	colorImage->Update();

	std::cout << "  GENERATING COLORS IMAGE: " << std::endl;
	std::cout << "   RED =        " << c2 << std::endl;
	std::cout << "   BLUE =       " << c3 << std::endl;
	std::cout << "   BACKGROUND = " << c1 << std::endl;

	RGBPixelType red_rgb;
	red_rgb[0] = c2.R;
	red_rgb[1] = c2.G;
	red_rgb[2] = c2.B;

	RGBPixelType blue_rgb;
	blue_rgb[0] = c3.R;
	blue_rgb[1] = c3.G;
	blue_rgb[2] = c3.B;

	RGBPixelType back_rgb;
	back_rgb[0] = c1.R;
	back_rgb[1] = c1.G;
	back_rgb[2] = c1.B;

	typedef itk::ImageRegionIteratorWithIndex< RGBImageType2D > IteratorType;
	IteratorType iteratorRGB( colorImage, colorImage->GetRequestedRegion() );
	for( iteratorRGB.GoToBegin(); !iteratorRGB.IsAtEnd(); ++iteratorRGB )
	{
		RGBImageType2D::IndexType index = iteratorRGB.GetIndex();
		int x = index[0];
		int y = index[1];

		if(x < imgSize / 4)
		{
			iteratorRGB.Set(back_rgb);
		}
		else
		{
			if(y < imgSize / 2)
			{
				iteratorRGB.Set(red_rgb);
			}
			else
			{
				iteratorRGB.Set(blue_rgb);
			}
		}
	}

	typedef itk::ImageFileWriter< RGBImageType2D > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( outFilename );
	writer->SetInput( colorImage );
	writer->Update();

}

void ColorSegmentation::GenerateProjection(int dir, std::string outFilename)
{
	if(!hist)
		return;

	//I need to create a projection RGB image of the histogram
	typedef itk::Image< RGBPixelType, 2 > RGBImageType2D;
	RGBImageType2D::Pointer histImage = RGBImageType2D::New();

	RGBImageType2D::PointType origin;
	origin[0] = 0;
	origin[1] = 0;
	histImage->SetOrigin( origin ); 
	RGBImageType2D::IndexType start = { 0,0 };
	RGBImageType2D::SizeType  size = { dh::histSize, dh::histSize };
	RGBImageType2D::RegionType region;
	region.SetSize( size );
	region.SetIndex( start );
	histImage->SetRegions( region );
	histImage->Allocate();
	RGBPixelType p_rgb;
	p_rgb[0] = 0;
	p_rgb[1] = 0;
	p_rgb[2] = 0;
	histImage->FillBuffer(p_rgb);         
	histImage->Update();

	long int max, min;
	hist->projection_extrema(dir, max, min);

	typedef itk::ImageRegionIteratorWithIndex< RGBImageType2D > IteratorType;
	IteratorType iteratorRGB( histImage, histImage->GetRequestedRegion() );
	for( iteratorRGB.GoToBegin(); !iteratorRGB.IsAtEnd(); ++iteratorRGB )
	{
		RGBImageType2D::IndexType index = iteratorRGB.GetIndex();
		int x = index[0];
		int y = index[1];

		long int count = hist->proj_at(dir, x, y);

		if(count > 0)
		{
			double diff = (double)(max-min);
			double factor = (double)(count-min) / diff;
			
			RGBPixelType p_rgb;
			p_rgb[0] = 127 + (unsigned char)(factor * 128);
			p_rgb[1] = 127 + (unsigned char)(factor * 128);
			p_rgb[2] = 127 + (unsigned char)(factor * 128);
			//p_rgb[0] = 255 - (unsigned char)(factor * 255);
			//p_rgb[1] = 0;// + (unsigned char)(factor * 127);
			//p_rgb[2] = (unsigned char)(factor * 255);

			iteratorRGB.Set(p_rgb);
		}
	}

	//Now put a DOT on each archetype color:
	RGBPixelType red_rgb;
	red_rgb[0] = 255;
	red_rgb[1] = 0;
	red_rgb[2] = 0;

	RGBPixelType blue_rgb;
	blue_rgb[0] = 0;
	blue_rgb[1] = 0;
	blue_rgb[2] = 255;

	RGBPixelType back_rgb;
	back_rgb[0] = 0;
	back_rgb[1] = 255;
	back_rgb[2] = 0;

	if(dir==3)
	{
		RGBImageType2D::IndexType indexR = { archTypRED.R, archTypRED.L };
		RGBImageType2D::IndexType indexB = { archTypBLUE.R, archTypBLUE.L };
		RGBImageType2D::IndexType indexW = { archTypBACK.R, archTypBACK.L };
		histImage->SetPixel(indexR, red_rgb);
		histImage->SetPixel(indexB, blue_rgb);
		histImage->SetPixel(indexW, back_rgb);
	}
	else if(dir==2)
	{
		RGBImageType2D::IndexType indexR = { archTypRED.R, archTypRED.I };
		RGBImageType2D::IndexType indexB = { archTypBLUE.R, archTypBLUE.I };
		RGBImageType2D::IndexType indexW = { archTypBACK.R, archTypBACK.I };
		histImage->SetPixel(indexR, red_rgb);
		histImage->SetPixel(indexB, blue_rgb);
		histImage->SetPixel(indexW, back_rgb);
	}
	else if(dir==1)
	{
		RGBImageType2D::IndexType indexR = { archTypRED.L, archTypRED.I };
		RGBImageType2D::IndexType indexB = { archTypBLUE.L, archTypBLUE.I };
		RGBImageType2D::IndexType indexW = { archTypBACK.L, archTypBACK.I };
		histImage->SetPixel(indexR, red_rgb);
		histImage->SetPixel(indexB, blue_rgb);
		histImage->SetPixel(indexW, back_rgb);
	}


	typedef itk::ImageFileWriter< RGBImageType2D > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( outFilename );
	writer->SetInput( histImage );
	writer->Update();

}

void ColorSegmentation::ComputeBinary(int num_bins, int num_in_fg, bool fgrnd_dark)
{
	if(!rgb_input)
		return;

	std::cerr << "Creating Binary Image...";

	typedef itk::RGBToLuminanceImageFilter< RGBImageType, UcharImageType > ConvertFilterType;
	ConvertFilterType::Pointer convert = ConvertFilterType::New();
	convert->SetInput( rgb_input );
	convert->Update();

	UcharImageType::Pointer intensity_image = convert->GetOutput();

	//Create histogram:
	typedef itk::Statistics::ScalarImageToHistogramGenerator< UcharImageType > HistogramGeneratorType;
	typedef HistogramGeneratorType::HistogramType HistogramType;
	HistogramGeneratorType::Pointer histoGenerator = HistogramGeneratorType::New();
	histoGenerator->SetNumberOfBins( 256 );
	histoGenerator->SetInput( intensity_image );
	try
	{
		histoGenerator->Compute();
	}
	catch( itk::ExceptionObject & excep )
    {
		std::cerr << "    Histogram Computation: exception caught !" << std::endl;
		std::cerr << excep << std::endl;
		return;
    }
	

	//Estimate thresholds:
	typedef itk::OtsuMultipleThresholdsCalculator< HistogramType > CalculatorType;
	CalculatorType::Pointer calculator = CalculatorType::New();
	calculator->SetNumberOfThresholds( num_bins );
	calculator->SetInputHistogram( histoGenerator->GetOutput() );
	try
	{
		calculator->Update();
	}
	catch( itk::ExceptionObject & excep )
    {
		std::cerr << "    Otsu Computation: exception caught !" << std::endl;
		std::cerr << excep << std::endl;
		return;
    }

	const CalculatorType::OutputType &thresholdVector = calculator->GetOutput();

	//Estimate Binarization thresholds:
	int lowerThreshold,upperThreshold;
	if( fgrnd_dark )	//Do I want to make the foregound of the binary dark:
	{
		CalculatorType::OutputType::const_iterator itNum = thresholdVector.end();
 		for( int i=0; i<(num_bins-num_in_fg+1); ++i ) 
			--itNum;
		upperThreshold = static_cast<unsigned char>(*itNum);
		lowerThreshold = itk::NumericTraits<unsigned char>::min();
	} 
	else
	{
		CalculatorType::OutputType::const_iterator itNum = thresholdVector.begin();
		for( int i=0; i<(num_bins-num_in_fg); ++i ) 
			++itNum;
		lowerThreshold = static_cast<unsigned char>(*itNum);
		upperThreshold = itk::NumericTraits<unsigned char>::max();
	}

	std::cerr << "Binarization Thresholds: " << lowerThreshold << "  " << upperThreshold << std::endl;

	typedef itk::BinaryThresholdImageFilter< UcharImageType, UcharImageType >  ThreshFilterType;
	ThreshFilterType::Pointer threshfilter = ThreshFilterType::New();
	threshfilter->SetOutsideValue( 0 );
	threshfilter->SetInsideValue( (int)itk::NumericTraits<unsigned char>::max() );
	threshfilter->SetInput( intensity_image );
	threshfilter->SetLowerThreshold( lowerThreshold );
	threshfilter->SetUpperThreshold( upperThreshold );
	try
	{
		threshfilter->Update();
	}
	catch( itk::ExceptionObject & excep )
    {
		std::cerr << "    Threshold: exception caught !" << std::endl;
		std::cerr << excep << std::endl;
		return;
    }

	bin_image = threshfilter->GetOutput();

	if(TESTING)
	{
		typedef itk::ImageFileWriter< UcharImageType > WriterType;
		WriterType::Pointer writer = WriterType::New();
		writer->SetFileName( "bin.tif" );
		writer->SetInput( bin_image );
		writer->Update();
	}

	std::cerr << "...Done" << std::endl;
}