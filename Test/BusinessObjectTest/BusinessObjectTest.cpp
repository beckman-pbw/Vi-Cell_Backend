#include "stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <conio.h>
#include <memory>
#include <map>
#include <iostream>
#include <sstream>

#include "BusinessObjectTest.hpp"
#include "ImageProcessing.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "BusinessObjectTest";
static const char VERSION[] = "0.41";

//*****************************************************************************
static uint32_t numImagesSinceLastMaximum_ = 0;
static BusinessObject::SharpnessData_t maxFocusData = { 0, 0, 0 };
static std::string focusCsvPath;
static std::ofstream focusCsvStream;
static bool positionError = false;
static std::map<uint16_t, uint16_t> reagentRemainingVarOffset_;



typedef struct {
	uint32_t start;
	uint32_t  end;
} FocusRange_t;

typedef std::vector<unsigned char> t_txrxbuf;
typedef std::shared_ptr < t_txrxbuf > t_ptxrxbuf;

//*****************************************************************************
BusinessObject::BusinessObject() {

	pLocalIosvc_.reset (new boost::asio::io_service());
	pLocalWork_.reset (new boost::asio::io_service::work (*pLocalIosvc_));

	// Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
	auto THREAD = std::bind (static_cast <std::size_t (boost::asio::io_service::*)(void)> (&boost::asio::io_service::run), pLocalIosvc_.get());
	pThread_.reset (new std::thread (THREAD));

	pSignals_.reset (new boost::asio::signal_set(*pLocalIosvc_, SIGINT, SIGTERM));

	pCBI_.reset (new ControllerBoardInterface (*pLocalIosvc_, CNTLR_SN_A_STR, CNTLR_SN_B_STR));
    pLed_.reset (new Led (*pLocalIosvc_, pCBI_));
    pCamera_.reset (new Camera (*pLocalIosvc_, pCBI_));
	pSyringePump.reset (new SyringePump (pCBI_));
	pRfid_.reset (new Rfid (pCBI_));

	inited_ = false;
	imageCnt_ = 0;

#ifdef REV1_HW
	Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("BusinessObjectTest v%s using Rev1HW") % VERSION));
#else
 #ifdef REV2_HW
	Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("BusinessObjectTest v%s using Rev2HW") % VERSION));
 #else
	Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("BusinessObjectTest v%s using Rev3HW") % VERSION));
#endif
#endif
}

//*****************************************************************************
BusinessObject::~BusinessObject() {	

	if (!pLocalIosvc_ || !pThread_)
		return;

//    pMOG_.release();

	pFocusController_.reset();
	pCarouselController_.reset();
	pPlateController_.reset();
    pReagentController_.reset();

	pSyringePump.reset();
//	pCamera_.reset();
	pCamera_.reset();
	pLed_.reset();
//	pLCBI_.reset();
	pCBI_.reset();
	pRfid_.reset();

	pLocalWork_.reset();   // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	pLocalIosvc_->stop();  // instruct io_service to stop processing (should exit ::run() and end thread.
	if (pThread_->joinable()) {
		pThread_->join();
	}
	pThread_.reset();
	pLocalIosvc_.reset();  // Destroys the queue
}

//*****************************************************************************
void BusinessObject::quit() {

	pCBI_->CloseSerial();
}

//*****************************************************************************
void BusinessObject::signalHandler (const boost::system::error_code& ec, int signal_number)
{
	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"");
	}

	Logger::L().Log (MODULENAME, severity_level::critical, boost::str (boost::format ("Received signal no. %d") % signal_number).c_str());

	// All done listening.
	pSignals_->cancel();

	// Try to get out of here.
	pLocalIosvc_->post (std::bind(&BusinessObject::quit, this));
}

//*****************************************************************************
bool BusinessObject::init() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "init: <enter>");

	if (inited_) {
		return true;
	}

#if 0
	pRfid_->testReadingTagFile ("./C06019_ViCell_BLU_Reagent_Pak.bin");
#endif

#if 0
//TODO: *** for algorithm testing only ***
	{
		imageProcessing_.init (2048, 2048);

		imageProcessing_.setRootDirectory ("./algorithm_test");
		imageProcessing_.setSampleDirectory ("Z-1");

		imageProcessing_.processImages();

		ImageProcessing::ImageCollection_t imageCollection = imageProcessing_.readImages ("./algorithm_test_data/Z-1");

		for (auto imgSet : imageCollection) {
			imageProcessing_.addImageSet (imgSet.first, imgSet.second);
		}

// 		imageProcessing_.addImageCollection (imageCollection);

//		imageProcessing_.processImages();

		imageProcessing_.waitForImageProcessingCompletion();
	}
#endif

//TODO: for debugging initialization of CellCounting DLL.
//imageProcessing_.init (2048, 2048);

	if (!pCBI_->Initialize()) {
		return false;
	}

	if (!pCamera_->initialize()) {
		return false;
	}
	pCamera_->setOnTriggerCallback (std::bind (&BusinessObject::onCameraTrigger, this));

	if (!pLed_->initialize()) {
		return false;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	pCamera_->getImageSize (width, height);

	imageProcessing_.init (width, height);

	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "init: initing CarouselController");
		pCarouselController_.reset (new CarouselController (*pLocalIosvc_, pCBI_));
		t_pPTree cfgTree;
		pCarouselController_->Init ("", cfgTree, true, "MotorControl.info");
	}
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "init: initing PlateController");
		pPlateController_.reset (new PlateController (*pLocalIosvc_, pCBI_));
		t_pPTree cfgTree;
		pPlateController_->Init ("", cfgTree, true, "MotorControl.info");
	}
    {
        Logger::L().Log( MODULENAME, severity_level::debug1, "init: initing ReagentController" );
        pReagentController_.reset( new ReagentController( *pLocalIosvc_, pCBI_) );
        t_pPTree cfgTree;
        pReagentController_->Init( "", cfgTree, true, "MotorControl.info" );
    }
    {
		Logger::L().Log (MODULENAME, severity_level::debug1, "init: initing FocusController");
		pFocusController_.reset (new FocusController (*pLocalIosvc_, pCBI_));
		t_pPTree cfgTree;
		pFocusController_->Init ("", cfgTree, true, "MotorControl.info");
	}

//TODO: hack for the slow focus homing in the ControllerBoard.
	//if (pFocusController_->IsHome()) {
	//	pFocusController_->SetPosition (pFocusController_->GetFocusMax() / 5);
	//}

	pMOG_ = new BackgroundSubtractorMOG();


	std::cout << "***************************************************************************" << std::endl;
	std::cout << "*** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***" << std::endl;
	std::cout << "The instrument is going to initialize the sample carrier." << std::endl;
	std::cout << "Ensure an empty carousel is inserted if using one." << std::endl;	// using a carouselRemove the carousel or plate." << std::endl;
	std::cout << "A plate may be inserted later." << std::endl;
	std::cout << "When the instrument has completed its initialization," << std::endl;
	std::cout << "you will be prompted to populate the carousel or insert a plate." << std::endl;
	std::cout << "*** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***" << std::endl;
	std::cout << "***************************************************************************" << std::endl;
	std::cout << "Press <enter> to continue..." << std::endl;
	_getch();

	std::vector<std::string> strParams;
	std::string response;

READ_CARRIER_TYPE:
	std::cout << "Select the type of sample carrier to be used ([C]arousel or [P]late): ";
	std::cin >> response;
	boost::to_upper (response);
	if (response.compare(SampleRunList::CarrierSelectParamCarousel) == 0) {
		strParams.push_back (SampleRunList::CarrierSelectParamCarousel);
		doCommand ("CS", strParams);
		isCarrierCarousel_ = true;
		isCarrierPlate_ = false;
		pCarouselController_->EjectCarousel();
		std::cout << "It is now safe to load the carousel.  Ensure that the carousel is positioned to Tube #1..." << std::endl;

	} else if (response.compare(SampleRunList::CarrierSelectParam96WellPlate) == 0) {
		strParams.push_back (SampleRunList::CarrierSelectParam96WellPlate);
		doCommand ("CS", strParams);
		isCarrierPlate_ = true;
		isCarrierCarousel_ = false;
		pPlateController_->EjectPlate();
		std::cout << "It is now safe to insert a plate..." << std::endl;

	} else {
		std::cout << "Invalid carrier type..." << std::endl;
		goto READ_CARRIER_TYPE;
	}

	std::cout << "Press <enter> when ready..." << std::endl;
	_getch();


	if ( isCarrierCarousel_ )
	{
		pCarouselController_->LoadCarousel();
	}
	else if ( isCarrierPlate_ )
	{
		pPlateController_->LoadPlate();
	}

	inited_ = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "init: <exit>");

	return true;
}

//*****************************************************************************=
// Verify that all of the required sections are in the *info* file.
//*****************************************************************************
bool BusinessObject::readConfiguration (const std::string& configuration_file, boost::system::error_code& ec) {

    HawkeyeConfig::setWithHardware();

	if (!HawkeyeConfig::load (configuration_file)) {
		return false;
	}

	config_ = HawkeyeConfig::get();

	return true;
}

//*****************************************************************************
void BusinessObject::onCameraTrigger() {

	Logger::L().Log (MODULENAME, severity_level::debug1, "onCameraTrigger <enter>");

	Mat image;
	pCamera_->getImage (image);

	if (config_->cellCounting.analyzeImages) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "onCameraTrigger adding image");

		ImageProcessing::FLImages_t flImages;
		imageProcessing_.addImageSet (image, flImages);
	}

	createDataDirectories (imageCnt_, "");

	std::string dataPath;
	getDataPath (0, dataPath);

	std::string dataType;
	getDataType (0, dataType);

    if (config_->displayImage) {
    	ImageProcessing::showImage ("Raw Image", &IplImage(image));
    }
	if (config_->saveImage) {
        std::string imagePath = boost::str (boost::format ("%s/%s_%d.png") % dataPath % dataType % imageCnt_);

        // Must use the C language API as the C++ API causes the application to crash
		// when building for debug.
		// Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/

		Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraTrigger: writing " + imagePath);
		cvSaveImage (imagePath.c_str(), &IplImage(image));

		// This C++ API crashes in debug mode.
		//	imwrite ("testimage.png", image);
	}

	if (config_->generateHistogram) {
		std::string histogramPath = boost::str (boost::format ("%s/%s_%d_hist.csv") % dataPath % dataType % imageCnt_);
		ImageProcessing::generateHistogram (image, histogramPath);
	}

	if (config_->generateColorizedImage) {
		Mat colorizedImage;
		ImageProcessing::colorizeImage (image, colorizedImage);

		if (config_->displayImage) {
			ImageProcessing::showImage ("Colorized Image", &IplImage(colorizedImage));
		}
		if (config_->saveImage) {
			std::string imagePath = boost::str (boost::format ("%s/%s_%d_c.png") % dataPath % dataType % imageCnt_);
			Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraTrigger: writing " + imagePath);
			cvSaveImage (imagePath.c_str(), &IplImage(colorizedImage));
		}
	}

	imageCnt_++;

	Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraTrigger <exit>");
}

//*****************************************************************************
void BusinessObject::onAutoFocusCameraTrigger() {

	Logger::L().Log (MODULENAME, severity_level::debug1, "onAutoFocusCameraTrigger <enter>");

	Mat image;
	pCamera_->getImage (image);


	if (resetBackgroundImageSubtraction_) {
		pMOG_->initialize (image.size(), image.type());
		resetBackgroundImageSubtraction_ = false;
	}

	createDataDirectories (imageCnt_, "AF");

	std::string dataPath;
	getDataPath (0, dataPath);

	std::string dataType;
	getDataType (0, dataType);

	if (config_->displayImage) {
		ImageProcessing::showImage ("Raw Image", &IplImage(image));
	}
	if (config_->saveImage) {
		std::string imagePath = boost::str (boost::format ("%s/%s_%d.png") % dataPath % dataType % imageCnt_);

		// Must use the C language API as the C++ API causes the application to crash
		// when building for debug.
		// Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/

		Logger::L().Log (MODULENAME, severity_level::debug2, "onAutoFocusCameraTrigger: writing " + imagePath);
		cvSaveImage (imagePath.c_str(), &IplImage(image));

		// This C++ API crashes in debug mode.
		//	imwrite ("testimage.png", image);
	}

	if (config_->generateHistogram) {
		std::string histogramPath = boost::str (boost::format ("%s/%s_%d_hist.csv") % dataPath % dataType % imageCnt_);
		ImageProcessing::generateHistogram (image, histogramPath);
	}

	pMOG_->operator() (image, fgMaskMOG_);
	if (config_->displayImage) {
		ImageProcessing::showImage ("FG Mask MOG", &IplImage(fgMaskMOG_));
	}
	if (config_->saveImage) {
		std::string imagePath = boost::str (boost::format ("%s/%s_%d_mog.png") % dataPath % dataType % imageCnt_);
		cvSaveImage (imagePath.c_str(), &IplImage (fgMaskMOG_));
		Logger::L().Log (MODULENAME, severity_level::debug2, "onAutoFocusCameraTrigger: writing " + imagePath);

		std::string histogramPath = boost::str (boost::format ("%s/%s_%d_hist_mog.txt") % dataPath % dataType % imageCnt_);
		ImageProcessing::generateHistogram (fgMaskMOG_, histogramPath);
	}

	if (config_->generateColorizedImage) {
		Mat colorizedImage;
		ImageProcessing::colorizeImage (image, colorizedImage);

		if (config_->displayImage) {
			ImageProcessing::showImage ("Colorized Image", &IplImage(colorizedImage));
		}
		if (config_->saveImage) {
			std::string imagePath = boost::str (boost::format ("%s/%s_%d_c.png") % dataPath % dataType % imageCnt_);
			Logger::L().Log (MODULENAME, severity_level::debug2, "onAutoFocusCameraTrigger: writing " + imagePath);
			cvSaveImage (imagePath.c_str(), &IplImage(colorizedImage));
		}
	}

	imageCnt_++;

	Logger::L().Log (MODULENAME, severity_level::debug2, "onAutoFocusCameraTrigger <exit>");
}

//*****************************************************************************
void BusinessObject::onAdjustBackgroundIntensityCameraTrigger() {

    Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraAdjustBackgroundIntensityTrigger <enter>");

    Mat image;
    pCamera_->getImage (image);

    if (config_->displayImage) {
		ImageProcessing::showImage ("Raw Image", &IplImage (image));
    }

    if (resetBackgroundImageSubtraction_) {
        pMOG_->initialize (image.size(), image.type());
        resetBackgroundImageSubtraction_ = false;
    }

    createDataDirectories (imageCnt_, "ABI");

    std::string dataPath;
    getDataPath (0, dataPath);

    std::string dataType;
    getDataType (0, dataType);

    if (config_->saveImage) {
        std::string imagePath = boost::str (boost::format ("%s/%s_%d.png") % dataPath % dataType % imageCnt_);

        // Must use the C language API as the C++ API causes the application to crash
        // when building for debug.
        // Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/

        cvSaveImage (imagePath.c_str(), &IplImage (image));
        Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraAdjustBackgroundIntensityTrigger: writing " + imagePath);

        // This C++ API crashes in debug mode.
        //	imwrite ("testimage.png", image);
    }

	if (config_->generateHistogram) {
		std::string histogramPath = boost::str (boost::format ("%s/%s_%d_hist.txt") % dataPath % dataType % imageCnt_);
		ImageProcessing::generateHistogram (image, histogramPath);
	}

    pMOG_->operator() (image, fgMaskMOG_);
	if (config_->displayImage) {
		ImageProcessing::showImage ("FG Mask MOG", &IplImage(fgMaskMOG_));
	}
    if (config_->saveImage) {
        std::string imagePath = boost::str (boost::format ("%s/%s_%d_mog.png") % dataPath % dataType % imageCnt_);
        cvSaveImage (imagePath.c_str(), &IplImage (fgMaskMOG_));
        Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraAdjustBackgroundIntensityTrigger: writing " + imagePath);

        std::string histogramPath = boost::str (boost::format ("%s/%s_%d_hist_mog.txt") % dataPath % dataType % imageCnt_);
        ImageProcessing::generateHistogram (fgMaskMOG_, histogramPath);
    }

    uint32_t totalPixelIntensity = 0;
    uint32_t totalPixels = 0;
    for (int row = 0; row < image.rows; row++) {
        for (int col = 0; col < image.cols; col++) {

            // Check for all black pixel and accumulate the pixel values of the *image* when foreground image is black.
            if (fgMaskMOG_.at<uint8_t> (row, col) == 0) {
                totalPixelIntensity += (uint32_t)image.at<uint8_t> (row, col);
                totalPixels++;
            }
        }
    }

    if (totalPixels > 0) {
        averageIntensity_ = totalPixelIntensity / totalPixels;
        Logger::L().Log (MODULENAME, severity_level::debug2,
                         boost::str (boost::format ("BackgroundSubtraction::avg background intensity: %d: ") % averageIntensity_));
    } else {
        Logger::L().Log (MODULENAME, severity_level::warning, "BackgroundSubtraction: Total pixel count is zero.");
    }

    imageCnt_++;

    Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraAdjustBackgroundIntensityTrigger <exit>");
}

//*****************************************************************************
void BusinessObject::onCameraTriggerTimeout() {

	Logger::L().Log (MODULENAME, severity_level::normal, "onCameraTriggerTimeout called...");
}

//*****************************************************************************
bool BusinessObject::coarseFocus (BusinessObject::SharpnessData_t& sharpnessData, uint32_t startingPosition) {

	numImagesSinceLastMaximum_ = 0;

	SharpnessData_t maxFocusData = { 0, 0, 0 };

	pFocusController_->SetPosition (startingPosition);

	Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Coarse Focus ##########");

	pFocusController_->AdjustCoarseStep (300);

	while (true) {
		pFocusController_->FocusStepUpCoarse();
		if (pFocusController_->IsMax()) {
			Logger::L().Log (MODULENAME, severity_level::notification, "Failed to find coarse focus position.");
			return false;
		}

		pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);

		numImagesSinceLastMaximum_++;

		ImageProcessing::getHistogramWhiteCount (fgMaskMOG_, &sharpness_);

		Logger::L().Log(MODULENAME, severity_level::debug1,
			boost::str (boost::format ("coarseFocus:: sharpness: %f, position: %d, filenum: %d, numImagesSinceLastMaximum: %d")
				% sharpness_
				% pFocusController_->Position()
				% imageCnt_
				% numImagesSinceLastMaximum_));

		focusCsvStream << (imageCnt_ - 1) << ", " << pFocusController_->Position() << ", " << sharpness_ << std::endl;

		if (sharpness_ > maxFocusData.sharpness) {
			Logger::L().Log (MODULENAME, severity_level::debug1, "*** sharpness > max sharpness ***");

			maxFocusData.sharpness = sharpness_;
			maxFocusData.position = pFocusController_->Position();
			maxFocusData.filenum = imageCnt_;

			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("coarseFocus:: MAX sharpness: %f, position: %d, FileNum: %d, numImagesSinceLastMaximum: %d")
					% maxFocusData.sharpness
					% maxFocusData.position
					% maxFocusData.filenum
					% numImagesSinceLastMaximum_));

			numImagesSinceLastMaximum_ = 0;
		}

		if (sharpness_ > 0 && numImagesSinceLastMaximum_ >= 10) {
			Logger::L().Log (MODULENAME, severity_level::debug1, "######### Coarse Focus Complete ##########");
			Logger::L().Log(MODULENAME, severity_level::debug1,
				boost::str (boost::format ("focus::fileNum: %d, sharpness: %f, poosition: %d")
					% maxFocusData.filenum
					% maxFocusData.sharpness
					% maxFocusData.position
				));
			break;
		}
	}

	return true;
}

//*****************************************************************************
BusinessObject::SharpnessData_t BusinessObject::fineFocus (BusinessObject::SharpnessData_t& maxFocusData, uint32_t range, uint32_t stepSize) {
    
    Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Fine Focus ##########");

    std::vector<SharpnessData_t> sharpnessData_;

    FocusRange_t focusRange_;
    focusRange_.start = maxFocusData.position - range;
    focusRange_.end = maxFocusData.position + range;
    pFocusController_->SetPosition (focusRange_.end);
    pFocusController_->AdjustFineStep (stepSize);

    Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("using +/- %d counts and step size of %d") % range % stepSize));

    // Scan through the fine focus range collecting sharpness values.
    uint32_t curPos = pFocusController_->Position();

    while (curPos > focusRange_.start) {
        pFocusController_->FocusStepDnFine();
        Logger::L().Log  (MODULENAME, severity_level::debug1, boost::str  (boost::format  ("focus::fine_focus1 curPos: %d") % curPos));
        pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);

        ImageProcessing::getHistogramWhiteCount (fgMaskMOG_, &sharpness_);

        BusinessObject::SharpnessData_t sharpnessData;
        sharpnessData.filenum = imageCnt_;
        sharpnessData.sharpness = sharpness_;
        sharpnessData.position = pFocusController_->Position();
        sharpnessData_.push_back  (sharpnessData);

        Logger::L().Log (MODULENAME, severity_level::debug1,
                            boost::str (boost::format ("fineFocus:: filenum: %d, sharpness: %d, position: %d")
                                        % sharpnessData.filenum
                                        % sharpnessData.sharpness
                                        % sharpnessData.position));

		focusCsvStream << sharpnessData.filenum << ", " << sharpnessData.position << ", " << sharpnessData.sharpness << std::endl;

        curPos = pFocusController_->Position();

    } // End "while (curPos > focusRange_.start)"

    // Find the max sharpness value for the fine focus.
    maxFocusData = { 0, 0, 0 };
    uint32_t maxIdx = 0;
    uint32_t idx = 0;
    for (auto it : sharpnessData_) {
        Logger::L().Log  (MODULENAME, severity_level::debug1,
            boost::str  (boost::format  ("sharpnessData:: filenum: %d, sharpness: %d, position: %d")
                % it.filenum
                % it.sharpness
                % it.position));
        if (it.sharpness > maxFocusData.sharpness) {
            maxFocusData = it;
            maxIdx = idx;
        }
        idx++;
    }

    Logger::L().Log  (MODULENAME, severity_level::debug1, "######### Fine Focus Complete ##########");
    Logger::L().Log  (MODULENAME, severity_level::debug1,
        boost::str  (boost::format  ("focus::fine_focus: filenum: %d, sharpness: %d, position: %d")
            % maxFocusData.filenum
            % maxFocusData.sharpness
            % maxFocusData.position));

    Logger::L().Log (MODULENAME, severity_level::debug1, "######### Calculating Optimal Focus Position ##########");

    std::vector<SharpnessData_t> M;
    M.resize (sharpnessData_.size());
    
    uint32_t W0_half = maxFocusData.sharpness / 2;

    uint32_t i = 0;
    for  (auto it : sharpnessData_) {
        M[i].sharpness = it.sharpness - W0_half;
        M[i].position = it.position;
        M[i].filenum = it.filenum;
        Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]: %d, %d") % i % M[i].sharpness % M[i].position));
        i++;
    }

    // Starting from the beginning of M, find the position where the sharpness is 1/2 of W0.
    uint32_t idx1 = 0;
    while (M[idx1].sharpness < 0 && idx1 < (M.size() / 2)) {
        idx1++;
    }
    Logger::L().Log  (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % (uint32_t)(idx1-1) % M[idx1-1].position % M[idx1-1].sharpness));
    Logger::L().Log  (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % idx1 % M[idx1].position % M[idx1].sharpness));


    // Interpolate to find Cmin.
    uint32_t Cmin = (sharpnessData_[idx1].position * M[idx1+1].sharpness - sharpnessData_[idx1+1].position * M[idx1].sharpness)
        / (M[idx1+1].sharpness - M[idx1].sharpness);


    // Starting from the maximum of M, find the position where the sharpness is 1/2 of W0.
    uint32_t idx2 = maxIdx;
    while (M[idx2].sharpness > 0 && idx2 < M.size()) {
        idx2++;
    }

    Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % (uint32_t)(idx2-1) % M[idx2-1].position % M[idx2-1].sharpness));
    Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % idx2 % M[idx2].position % M[idx2].sharpness));


    // Interpolate to find Cmax.
    uint32_t Cmax = (sharpnessData_[idx2].position * M[idx2+1].sharpness - sharpnessData_[idx2+1].position * M[idx2].sharpness)
        /  (M[idx2+1].sharpness - M[idx2].sharpness);

    uint32_t Cref = (uint32_t) round ((Cmin + Cmax) / 2);
    Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Cref: %d") % Cref));

    maxFocusData.position = Cref;

    return maxFocusData;
}

//*****************************************************************************
bool BusinessObject::autofocus (uint32_t& focusPosition) {

    Logger::L().Log (MODULENAME, severity_level::normal, "*************************************");
    Logger::L().Log (MODULENAME, severity_level::normal, "************ autofocus() ************");
    Logger::L().Log (MODULENAME, severity_level::normal, "*************************************");

    pCamera_->setOnTriggerCallback (std::bind (&BusinessObject::onAutoFocusCameraTrigger, this));

	focusCsvPath = boost::str (boost::format ("%sresults.csv") % outputDirectory_);
	focusCsvStream.open (focusCsvPath);
	if (!focusCsvStream.is_open()) {
		Logger::L().Log(MODULENAME, severity_level::normal, "failed to create results file: " + focusCsvPath);
		return false;
	}

	focusCsvStream << "File #, Position, Pixel Count" << std::endl;

	// Goto focus home.
	pFocusController_->FocusHome();

    std::vector<SharpnessData_t> sharpnessData_;
    
    resetBackgroundImageSubtraction_ = true;

	pFocusController_->SetPosition (config_->autoFocusCoarseStartingPosition);

	numImagesSinceLastMaximum_ = 0;

    SharpnessData_t maxFocusData = { 0, 0, 0 };

    Logger::L().Log (MODULENAME, severity_level::normal, "######### Starting Coarse Focus ##########");

    pFocusController_->AdjustCoarseStep (300);

	while (pFocusController_->Position() <= (int32_t)(config_->autoFocusCoarseStartingPosition + config_->autoFocusCoarseRange)) {
		pFocusController_->FocusStepUpCoarse();
        pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);

        numImagesSinceLastMaximum_++;

        ImageProcessing::getHistogramWhiteCount (fgMaskMOG_, &sharpness_);

        Logger::L().Log (MODULENAME, severity_level::normal,
            boost::str (boost::format ("autofocus:: sharpness: %f, position: %d, filenum: %d, numImagesSinceLastMaximum: %d")
                % sharpness_
                % pFocusController_->Position()
                % imageCnt_
                % numImagesSinceLastMaximum_));

		focusCsvStream << (imageCnt_ - 1) << ", " << pFocusController_->Position() << ", " << sharpness_ << std::endl;

        if (sharpness_ > maxFocusData.sharpness) {
            Logger::L().Log (MODULENAME, severity_level::normal, "*** sharpness > max sharpness ***");

            maxFocusData.sharpness = sharpness_;
            maxFocusData.position = pFocusController_->Position();
            maxFocusData.filenum = imageCnt_;

			Logger::L().Log  (MODULENAME, severity_level::normal,
				boost::str (boost::format ("autofocus:: MAX sharpness: %f, position: %d, FileNum: %d, numImagesSinceLastMaximum: %d")
					% maxFocusData.sharpness
					% maxFocusData.position
					% maxFocusData.filenum
					% numImagesSinceLastMaximum_));

            numImagesSinceLastMaximum_ = 0;
        }

        if (pFocusController_->IsMax()) {
            Logger::L().Log (MODULENAME, severity_level::notification, "Failed to find coarse focus position.");
            return false;
        }
	}

	Logger::L().Log (MODULENAME, severity_level::normal, "######### Coarse Focus Complete ##########");
	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("focus::fileNum: %d, sharpness: %f, poosition: %d")
			% maxFocusData.filenum
			% maxFocusData.sharpness
			% maxFocusData.position));

	uint32_t fineFocusRange = 2000;
	uint32_t fineFocusStepSize = 20;
    maxFocusData = fineFocus (maxFocusData, fineFocusRange, fineFocusStepSize);

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("autofocus:: moving to optimal focus position: %d") % maxFocusData.position));
	pFocusController_->SetPosition (maxFocusData.position);

	auto offSet = static_cast<int32_t>(std::ceil(config_->autoFocusOffset));
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("autofocus:: moving to adjusted optimal focus position: %d") % (maxFocusData.position - offSet)));
    pFocusController_->SetPosition (maxFocusData.position - offSet);

	uint32_t curPos = pFocusController_->Position();
	Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("autofocus:: controller says we are at focus position: %d") % curPos));

    focusPosition = maxFocusData.position;

	focusCsvStream.close();

	return true;
}

//*****************************************************************************
BusinessObject::SharpnessData_t BusinessObject::fineFocusForAnalysis (BusinessObject::SharpnessData_t& maxFocusData, uint32_t range, uint32_t stepSize) {

	Logger::L().Log(MODULENAME, severity_level::debug1, "######### Starting Fine Focus ##########");

	std::vector<SharpnessData_t> sharpnessData_;

	FocusRange_t focusRange_;
	focusRange_.start = maxFocusData.position - range;
	focusRange_.end = maxFocusData.position + range;
	pFocusController_->SetPosition (focusRange_.start);
	pFocusController_->AdjustFineStep (stepSize);

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("using +/- %d counts and step size of %d") % range % stepSize));

	// Scan through the fine focus range collecting sharpness values.
	uint32_t curPos = pFocusController_->Position();

	while (curPos > focusRange_.start) {
		pFocusController_->FocusStepUpFine();
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("focus::fine_focus1 curPos: %d") % curPos));
		pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);

		ImageProcessing::getHistogramWhiteCount (fgMaskMOG_, &sharpness_);

		BusinessObject::SharpnessData_t sharpnessData;
		sharpnessData.filenum = imageCnt_;
		sharpnessData.sharpness = sharpness_;
		sharpnessData.position = pFocusController_->Position();
		sharpnessData_.push_back (sharpnessData);

		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("fineFocusForAnalysis:: filenum: %d, sharpness: %d, position: %d")
				% sharpnessData.filenum
				% sharpnessData.sharpness
				% sharpnessData.position));

		focusCsvStream << sharpnessData.filenum << ", " << sharpnessData.position << ", " << sharpnessData.sharpness << std::endl;

		curPos = pFocusController_->Position();

	} // End "while (curPos > focusRange_.start)"

	  // Find the max sharpness value for the fine focus.
	maxFocusData = { 0, 0, 0 };
	uint32_t maxIdx = 0;
	uint32_t idx = 0;
	for (auto it : sharpnessData_) {
		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("sharpnessData:: filenum: %d, sharpness: %d, position: %d")
				% it.filenum
				% it.sharpness
				% it.position
			));
		if (it.sharpness > maxFocusData.sharpness) {
			maxFocusData = it;
			maxIdx = idx;
		}
		idx++;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "######### Fine Focus Complete ##########");
	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("focus::fine_focus: filenum: %d, sharpness: %d, position: %d")
			% maxFocusData.filenum
			% maxFocusData.sharpness
			% maxFocusData.position
		));

	focusCsvStream.close();

	return maxFocusData;
}

//*****************************************************************************
// Collect data for analyzing the precision of the auto-focus algorithm by
// comparing this data to what the L&T algorithm determines as the focal point.
//*****************************************************************************
bool BusinessObject::autofocusForAnalysis (uint32_t startingPosition, uint32_t range, uint32_t fineFocusStepSize, uint32_t& focusPosition)
{
	Logger::L().Log(MODULENAME, severity_level::normal, "************************************************");
	Logger::L().Log(MODULENAME, severity_level::normal, "************ autofocusForAnalysis() ************");
	Logger::L().Log(MODULENAME, severity_level::normal, "************************************************");

	pCamera_->setOnTriggerCallback (std::bind(&BusinessObject::onCameraTrigger, this));

	focusCsvPath = boost::str (boost::format ("%sresults.csv") % outputDirectory_);
	focusCsvStream.open (focusCsvPath);
    if (!focusCsvStream.is_open())
    {
        Logger::L().Log (MODULENAME, severity_level::normal, "failed to create results file: " + focusCsvPath);
        return false;
    }

	focusCsvStream << "File #, Position, Pixel Count" << std::endl;

	// Goto focus home.
    pFocusController_->FocusHome();

	SharpnessData_t coarseFocusData;
	bool retStat = coarseFocus (coarseFocusData, startingPosition);

	maxFocusData = fineFocusForAnalysis (coarseFocusData, 2000, 20);

	focusCsvStream.close();


////    focusMode_ = CollectBackgroundImages;
//
//	std::vector<SharpnessData_t> sharpnessData_;
//
//    resetBackgroundImageSubtraction_ = true;
//
////TODO: define a starting position based on the geometry of the hardware.
//	pFocusController_->SetPosition (startingPosition);
//
////    focusMode_ = FindCoarseFocus;
//
//    // Coarse Focus.
//    // Take images (write to temporary *focus* directory) until either we 
//    // find an image that meets the coarse focus requirements (?) or we run
//    // out of focus range.
//    // Keep track of the current focus position.
//    numImagesSinceLastMaximum_ = 0;
//
//	SharpnessData_t maxFocusData = { 0, 0, 0 };
//
//    Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Coarse Focus ##########");
//
//    pFocusController_->AdjustCoarseStep (300);
//
//    while (true) {
//        pFocusController_->FocusStepUpCoarse();
//        pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);
//
//		numImagesSinceLastMaximum_++;
//
//		focusCsvStream << (imageCnt_ - 1) << ", " << pFocusController_->Position() << ", " << sharpness_ << std::endl;
//
//        if (pFocusController_->IsMax()) {
//            Logger::L().Log (MODULENAME, severity_level::notification, "Failed to find coarse focus position.");
//            return false;
//        }
//
//#define MIN_SHARPNESS 0     //1000
//        if (maxSharpness_ > MIN_SHARPNESS && numImagesSinceLastMaximum_ >= 10) {
//            Logger::L().Log (MODULENAME, severity_level::debug1, "######### Coarse Focus Complete ##########");
//            Logger::L().Log (MODULENAME, severity_level::debug1,
//                             boost::str (boost::format ("focus:: maxSharpnessFileNum: %d, maxSharpness: %f, maxCoarseSharpnessPosition: %d")
//                                         % maxSharpnessFileNum_
//                                         % maxSharpness_
//                                         % maxSharpnessPosition_
//                            ));
//            break;
//        }
//    }
//
//    // Fine Focus.
//    max.filenum = maxSharpnessFileNum_;
//    max.sharpness = maxSharpness_;
//    max.position = maxSharpnessPosition_;
//
//    Logger::L().Log (MODULENAME, severity_level::debug1, "######### Starting Fine Focus ##########");
//    {
//        focusMode_ = FindFineFocus;
//
//        Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("using +/- %d counts and step size of 5") % range));
//
//        focusRange_.start = max.position - range;
//        focusRange_.end = max.position + range;
//
//        Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Fine focus range: %d -> %d <- %d") % focusRange_.start % max.position % focusRange_.end));
//
//        pFocusController_->SetPosition (focusRange_.end);
//        pFocusController_->AdjustFineStep (fineFocusStepSize);
//
//        // Scan through the fine focus range collecting sharpness values.
//        sharpnessData_.clear();
//        uint32_t curPos = pFocusController_->Position();
//
//        while (curPos > focusRange_.start) {
//            pFocusController_->FocusStepUpFine();
//            Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("focus::fine_focus1 curPos: %d") % curPos));
//            pCamera_->takePicture (exposureTime_usec_, Led::LED_BrightField);
//            curPos = pFocusController_->Position();
//
//            resultsFile <<  (imageCnt_ - 1) << ", " << curPos << ", " << sharpness_ << std::endl;
//        }
//
//        // Find the max sharpness value for the fine focus.
//        max = {0.0, 0, 0};
//        for  (auto it : sharpnessData_)
//        {
//            if (it.sharpness > max.sharpness)
//            {
//                max = it;
//            }
//        }
//
//        sharpnessData_.clear();
//
//        Logger::L().Log (MODULENAME, severity_level::debug1, "######### Fine Focus 1 Complete ##########");
//        Logger::L().Log (MODULENAME, severity_level::debug1,
//                         boost::str (boost::format ("focus::fine_focus: filenum: %d, sharpness: %f, position: %d")
//                                     % max.filenum
//                                     % max.sharpness
//                                     % max.position
//                        ));
//    }
//
//    focusPosition = max.position;
//
//    focusMode_ = NoFocus;
//
//	focusCsvStream.close();

    return true;
}



#define NUM_BACKGROUND_IMAGES 3
//*****************************************************************************
bool BusinessObject::adjustBackgroundIntensity (Led::Type ledType, uint32_t targetIntensity, uint32_t offset) {

    Logger::L().Log (MODULENAME, severity_level::normal, "***************************************************");
    Logger::L().Log (MODULENAME, severity_level::normal, "************ adjustBackgroundIntensity ************");
    Logger::L().Log (MODULENAME, severity_level::normal, "***************************************************");

    float currentPercentPower = pLed_->getPower (ledType);
	//double adjustByPercentPower = 10.0;
	float adjustByPercentPower = 5.0;
    bool isGoingNegative = false;
    bool isGoingPositive = false;


    pCamera_->setOnTriggerCallback (std::bind (&BusinessObject::onAdjustBackgroundIntensityCameraTrigger, this));

    // Take ten throwaway images to warm up the Brightfield LED.
	Logger::L().Log (MODULENAME, severity_level::normal, "Warming up the BrightField LED by taking " + std::to_string(NUM_BACKGROUND_IMAGES) + " images.");
	for (int i = 0; i < NUM_BACKGROUND_IMAGES; i++) {
		resetBackgroundImageSubtraction_ = true;
		pCamera_->takePicture (exposureTime_usec_, ledType);
    }

	resetBackgroundImageSubtraction_ = true;

    while (true) {
		pCamera_->takePicture (exposureTime_usec_, ledType);

        // Check if we are done.
        if (averageIntensity_ >= (targetIntensity - offset) && averageIntensity_ <=  (targetIntensity + offset)) {
            Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("Background Intensity is now %d (%d +/- %d) using %f percent power") 
                % averageIntensity_ 
                % targetIntensity 
                % offset 
                % currentPercentPower));
            break;
        }

        // Adjust the BrightField LED power.
        if (averageIntensity_ >= (targetIntensity + offset)) {
            if (isGoingPositive) {
                adjustByPercentPower /= 2.0;
                isGoingPositive = false;
            }
            currentPercentPower -= adjustByPercentPower;  // Reduce current power.
            isGoingNegative = true;

        } else if (averageIntensity_ <= (targetIntensity - offset)) {
            if (isGoingNegative) {
                adjustByPercentPower /= 2.0;
                isGoingNegative = false;
            }
            currentPercentPower += adjustByPercentPower;  // INcrease current power.
            isGoingPositive = true;
        }
        Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("adjustBackgroundIntensity:: currentPercentPower: %f") % currentPercentPower));

        if (currentPercentPower > 0) {
			pLed_->setPower (ledType, currentPercentPower);

        } else {
            Logger::L().Log (MODULENAME, severity_level::error, "LED power at zero, unable to adjust background image intensity.");
            break;
        }

    } // End "while (true)"

	imageCnt_ = 0;

    return true;
}

//*****************************************************************************
bool BusinessObject::doWorkflow (std::string workflowFile) {





	return doSampleRunList (workflowFile);
}

//*****************************************************************************
bool BusinessObject::doSampleRunList (std::string runFile) {

	if (!runList_.open (runFile)) {
		Logger::L().Log (MODULENAME, severity_level::error, "Unable to open Sample RunFile: " + runFile);
		return false;
	}

	boost::filesystem::path p (runFile);
	runFilename_ = p.stem().string();

	std::string cmd;
    std::vector<std::string> strParams;
    while (runList_.read (cmd, strParams)) {

        if (!doCommand (cmd, strParams)) {
            break;
        }

	} // End "while (true)"

	if ( isCarrierCarousel_ )
	{
		pCarouselController_->ProbeUp();
	}
	else if ( isCarrierPlate_ )
	{
		pPlateController_->ProbeUp();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("RunFile complete: %d images taken.") % getNumImages()));

	return true;
}

//*****************************************************************************
bool BusinessObject::doCommandLine (std::string cmdline) {

	//CommandParser cmdParser;

 //   boost::to_upper (cmdline);

 //   cmdParser.parse (",", cmdline);

	//std::string cmd = cmdParser.getByIndex(0);

 //   std::string strParam1 = cmdParser.getByIndex(1);
 //   std::string strParam2 = cmdParser.getByIndex(2);
 //   std::string strParam3 = cmdParser.getByIndex(3);
 //   std::string strParams[3] = cmdParser.getByIndex(4);

	//return doCommand (cmd, strParam1, strParam2, strParam3, strParams[3]);

    return true;
}

//*****************************************************************************
bool BusinessObject::doCommand (std::string cmd, std::vector<std::string> strParams) {      //std::string strParam1, std::string strParam2, std::string strParam3, std::string strParams[3]) {

	if (cmd.compare(SampleRunList::CarrierSelectCmd) == 0) {

		if (strParams[0].compare(SampleRunList::CarrierSelectParamCarousel) == 0) {

			//TODO: monitor carousel presence...
			Logger::L().Log (MODULENAME, severity_level::debug1, "Selecting Carousel...");
			pCarouselController_->SelectCarousel();

			samplePosition.setRowColumn ('Z', pCarouselController_->GetCurrentTubeNum());
			isTubePresent_ = pCarouselController_->IsTubePresent();

		} else if (strParams[0].compare(SampleRunList::CarrierSelectParam96WellPlate) == 0) {
			Logger::L().Log (MODULENAME, severity_level::debug1, "Selecting 96 Well Plate...");
			pPlateController_->SelectPlate();

		} else {
			Logger::L().Log (MODULENAME, severity_level::error, "Invalid carrier selection: " + strParams[0]);
		}

	} else if (cmd.compare(SampleRunList::SyringeCmd) == 0) {

		if (strParams[0].compare(SampleRunList::SyringeParamInitialize) == 0) {
			pSyringePump->initialize();

		} else if (strParams[0].compare(SampleRunList::SyringeParamMove) == 0) {
			pSyringePump->setPosition (std::stoul (strParams[1]), std::stoul (strParams[2]));

		} else if (strParams[0].compare(SampleRunList::SyringeParamValve) == 0) {
			pSyringePump->setValve (SyringePump::paramToPort (std::stoul (strParams[1])), SyringePump::paramToDirection(std::stoul (strParams[2])));

		} else {
			Logger::L().Log (MODULENAME, severity_level::error, "Invalid syringe command: " + strParams[0]);
		}

    } else if (cmd.compare (SampleRunList::CameraCmd) == 0) {

        if (strParams[0].compare (SampleRunList::CameraParamSetGain) == 0) {
            pCamera_->setGain (std::stod (strParams[1]));

        } else if (strParams[0].compare (SampleRunList::CameraParamSetExposure) == 0) {
            exposureTime_usec_ = std::stoul (strParams[1]);

        } else if (strParams[0].compare (SampleRunList::CameraParamRemoveGainLimits) == 0) {
            pCamera_->setGainLimitsState ( (std::stoul (strParams[1]) > 0 ? 1 : 0));

        } else if (strParams[0].compare (SampleRunList::CameraParamCapture) == 0) {

            pCamera_->setOnTriggerCallback (std::bind (&BusinessObject::onCameraTrigger, this));

            // Only move the syringe if the rate value (param2) is greater than zero.
            // This supports testing and debugging.
            if (std::stoul (strParams[2]) != 0) {
                pSyringePump->setPosition (std::stoul (strParams[1]), std::stoul (strParams[2]), false);
            }

            // This supports testing and debugging.
            uint32_t frameCnt = 0;
            bool loopForever = false;

            uint32_t numFrames = std::stoul (strParams[4]);
            if (numFrames == 0) {
                loopForever = true;
            }

			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Starting to take %d images...") % numFrames));
			try
            {
                while (frameCnt < numFrames || loopForever) {
					Led::Type ledType = (Led::Type) std::stoul (strParams[3]);
					pCamera_->takePicture (exposureTime_usec_, ledType);
                    frameCnt++;
                }

//TODO: needs timeout...
//TODO: put in HawkeyeDLL.info.
                pSyringePump->waitUntilIdle();

            } catch (Exception e) {
                Logger::L().Log (MODULENAME, severity_level::debug1, "Exception: " + std::string (e.what()));
            }
			Logger::L().Log (MODULENAME, severity_level::debug1, "Done taking images.");

        } else if (strParams[0].compare (SampleRunList::CameraParamTakePicture) == 0) {
			Led::Type ledType = (Led::Type) std::stoul (strParams[1]);
            pCamera_->takePicture (exposureTime_usec_, ledType);

        } else if (strParams[0].compare(SampleRunList::CameraAdjustBackgroundIntensity) == 0) {
            adjustBackgroundIntensity ((Led::Type)std::stoul(strParams[1]), std::stoul(strParams[2]), std::stoul(strParams[3]));
        }

	} else if (cmd.compare(SampleRunList::FocusCmd) == 0) {

		if (strParams[0].compare(SampleRunList::FocusParamMoveHome) == 0) {
			pFocusController_->FocusHome();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveCenter) == 0) {
			pFocusController_->FocusCenter();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveMax) == 0) {
			pFocusController_->FocusMax();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveUpFast) == 0) {
			pFocusController_->FocusStepUpFast();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveDownFast) == 0) {
			pFocusController_->FocusStepDnFast();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveUpCoarse) == 0) {
			pFocusController_->FocusStepUpCoarse();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveDownCoarse) == 0) {
			pFocusController_->FocusStepDnCoarse();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveUpFine) == 0) {
			pFocusController_->FocusStepUpFine();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamMoveDownFine) == 0) {
			pFocusController_->FocusStepDnFine();
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamSetCoarseStep) == 0) {
			pFocusController_->AdjustCoarseStep (std::stol (strParams[1]));
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

		} else if (strParams[0].compare(SampleRunList::FocusParamSetFineStep) == 0) {
			pFocusController_->AdjustFineStep (std::stol (strParams[1]));
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string (pFocusController_->Position()));

        } else if (strParams[0].compare (SampleRunList::FocusParamSetPosition) == 0) {
            pFocusController_->SetPosition (std::stol (strParams[1]));            
            Logger::L().Log (MODULENAME, severity_level::debug1, "Focus position: " + std::to_string(pFocusController_->Position()));

        } else if (strParams[0].compare(SampleRunList::FocusParamRun) == 0) {
            
            uint32_t focusPosition = 0;
			if (autofocus (focusPosition)) {
				Logger::L().Log (MODULENAME, severity_level::normal, "Foundautofocus position at: " + std::to_string (focusPosition));

			} else {
				Logger::L().Log (MODULENAME, severity_level::warning, "Unable to find a valid focus position.");
			}

        } else if (strParams[0].compare(SampleRunList::FocusParamRunAnalysis) == 0) {


//            uint32_t startingPosition, uint32_t range, uint32_t fineFocusStepSize,

            uint32_t focusPosition = 0;
            if (autofocusForAnalysis (std::stoul(strParams[1]), std::stoul(strParams[2]), std::stoul(strParams[3]), focusPosition)) {
                Logger::L().Log (MODULENAME, severity_level::normal, "Found autofocusForAnalysis position at: " + std::to_string(focusPosition));

            } else {
                Logger::L().Log (MODULENAME, severity_level::warning, "Unable to find a valid focus position.");
            }

        } else {
            Logger::L().Log (MODULENAME, severity_level::error, "Currently unsupported Focus command: " + strParams[1]);
        }

	} else if (cmd.compare(SampleRunList::LedCmd) == 0) {

		Led::Type ledNum = (Led::Type) std::stoul (strParams[0]);
        {
			if (strParams[1].compare(SampleRunList::LedParamSetPower) == 0) {
				pLed_->setPower (ledNum, std::stof (strParams[2]));
                //ledToUse_ = ledNum;

            } else if (strParams[0].compare(SampleRunList::LedParamSetOnTime) == 0) {

                //TODO: this is only used in REV1_HW to enable the Omicron LED enough to allow pictures to be captured.
                //			pLedDriver_->setOnTime (RegisterIds::LED1Regs, std::stoul (strParams[0]));
                //					setOnTime (RegisterIds::LED1Regs, std::stoul (strParams[0]));
                //ledToUse_ = ledNum;

			} else {
				Logger::L().Log (MODULENAME, severity_level::error, "Unknown LED command: " + strParams[0]);
			}
		}

	// Focus, Reagent, Probe, Tube#...
	} else if (cmd.compare(SampleRunList::TubeCmd) == 0) {

		if (samplePosition.setRowColumn (strParams[0].data()[0], (uint8_t)std::stoul (strParams[1]))) {

			if (config_->cellCounting.analyzeImages) {
				std::string tmp = strParams[0] + "-" + strParams[1];
				imageProcessing_.setSampleDirectory (tmp);
				imageProcessing_.waitForImageProcessingCompletion();
			}

			if (isCarrierCarousel_) {
				if (samplePosition.getColumn() == pCarouselController_->GetCurrentTubeNum()) {
					Logger::L().Log  (MODULENAME, severity_level::debug1, "Already at tube " + strParams[1]);

				} else {
					Logger::L().Log  (MODULENAME, severity_level::debug1, "Moving carousel to tube " + strParams[1]);
					samplePosition.setColumn ((uint8_t) pCarouselController_->GotoTubeNum  (std::stoul  (strParams[1])));
				}

				isTubePresent_ = pCarouselController_->IsTubePresent();
				Logger::L().Log (MODULENAME, severity_level::debug1, "Tube is " + std::string((isTubePresent_ ? "present" : "not present")));

				Logger::L().Log(MODULENAME, severity_level::debug1, "Carousel is at tube " + std::to_string(pCarouselController_->GetCurrentTubeNum()));

			} else {	// Carrier is Plate.
				uint32_t curRow;
				uint32_t curCol;
				uint32_t row=0;
				uint32_t col=0;

				pPlateController_->GetCurrentRowCol (curRow, curCol);
				samplePosition.getRowColumn (row, col);
				if (row == curRow && col == curCol) {
					Logger::L().Log  (MODULENAME, severity_level::debug1, "Already at tube " + strParams[1]);

				} else {
					Logger::L().Log  (MODULENAME, severity_level::debug1, "Moving plate to tube " + samplePosition.getAsStr());
					if ( !pPlateController_->SetRowCol( (const TCHAR)strParams[0].data()[0], strParams[1])) {
						positionError = true;
						Logger::L().Log( MODULENAME, severity_level::error, "failed to move plate to tube " + samplePosition.getAsStr() );
					}
					else
					{
						positionError = false;
					}
				}

//TODO: get the current position in (char, int) format...   pPlateController_->GetCurrentRowCol ();
				Logger::L().Log(MODULENAME, severity_level::debug1, "Plate is at tube " + samplePosition.getAsStr());
			}

			imageCnt_ = 0;

		} else {
			Logger::L().Log  (MODULENAME, severity_level::debug1, "Invalid position: " + strParams[0] + "-" + strParams[1]);
		}

	} else if (cmd.compare(SampleRunList::ProbeCmd) == 0) {

		if (positionError) {
			Logger::L().Log (MODULENAME, severity_level::error, "Due to positioning error, skipping probe down");
		} else {

			if ( strParams[0].compare( SampleRunList::ProbeParamUp ) == 0 )
			{
				if ( isCarrierCarousel_ )
				{
					pCarouselController_->ProbeUp();
				}
				else if ( isCarrierPlate_ )
				{
					pPlateController_->ProbeUp();
				}
			}
			else if ( strParams[0].compare( SampleRunList::ProbeParamDown ) == 0 )
			{
				if ( isCarrierCarousel_ )
				{
					pCarouselController_->ProbeDown();
				}
				else if ( isCarrierPlate_ )
				{
					pPlateController_->ProbeDown();
				}
			}
			else
			{
				Logger::L().Log( MODULENAME, severity_level::warning, "Probe motor operation is NOT available." );
			}
		}

	} else if (cmd.compare(SampleRunList::ReagentCmd) == 0) {
//		Logger::L().Log (MODULENAME, severity_level::warning, "Reagent motor operation is NOT available.");

		if (strParams[0].compare(SampleRunList::ReagentParamUp) == 0) {
			pReagentController_->ArmUp ();
		} else if (strParams[0].compare(SampleRunList::ReagentParamDown) == 0) {
			pReagentController_->ArmDown ();
		}
		else if ( strParams[0].compare( SampleRunList::ReagentParamPurge ) == 0 ) {
			pReagentController_->ArmPurge();
		}
        else if ( strParams[0].compare( SampleRunList::ReagentParamDoor ) == 0 ) {
			pReagentController_->ReleaseDoor();
		}
		else if (strParams[0].compare("4") == 0) {
//			pReagentController_->ArmMove(std::stoul (strParams[1]));
			Logger::L().Log (MODULENAME, severity_level::warning, "Reagent motor move operation is NOT available.");
		}

	} else if (cmd.compare(SampleRunList::WaitForKeyPressCmd) == 0) {
		cin.get();

	} else if (cmd.compare(SampleRunList::WaitCmd) == 0) {
		Sleep (std::stoul (strParams[0]));

	} else if (cmd.compare(SampleRunList::StopCmd) == 0) {
		if (config_->cellCounting.analyzeImages) {
			Logger::L().Log (MODULENAME, severity_level::debug1, "analyzing images after stop command");
//			imageProcessing_.addImages();
			imageProcessing_.waitForImageProcessingCompletion();
		}
		return false;

	} else if (cmd.compare(SampleRunList::DirectoryCmd) == 0) {
		imageCnt_ = 0;

		outputDirectory_ = strParams[0];
		boost::algorithm::trim (outputDirectory_);

		// Make sure that there is a "/" at the end of the string.
		if (outputDirectory_[outputDirectory_.length() - 1] != '/') {
			outputDirectory_ += '/';
		}

		imageProcessing_.setRootDirectory (outputDirectory_);

		// Create the directory path.
		boost::filesystem::path p(outputDirectory_);
		boost::system::error_code ec;
		boost::filesystem::create_directories (p, ec);
		if (ec == boost::system::errc::success) {
			Logger::L().Log (MODULENAME, severity_level::debug2, "created directory: " + p.generic_string());

		} else {
			Logger::L().Log (MODULENAME, severity_level::error, "failed to create directory: " + p.generic_string() + " : " + ec.message());
			return false;
		}

	} else if (cmd.compare(SampleRunList::ImageAnalysisCmd) == 0) {
		
		if (strParams[0].compare(SampleRunList::ImageAnalysisParamEnable) == 0) {
			config_->cellCounting.analyzeImages = true;

		} else if (strParams[0].compare(SampleRunList::ImageAnalysisParamInstrumentConfigFile) == 0) {
			config_->cellCounting.imageAnalysisInstrumentConfigFile = strParams[1];

		} else if (strParams[0].compare(SampleRunList::ImageAnalysisParamCellAnalysisConfigFile) == 0) {
			config_->cellCounting.imageAnalysisCellAnalysisConfigFile = strParams[1];

		} else if (strParams[0].compare(SampleRunList::ImageAnalysisParamRun) == 0) {
			if (config_->cellCounting.analyzeImages) {
				imageProcessing_.processImages();
			}
		}

	} else if (cmd.compare(SampleRunList::RfidCmd) == 0) {

		if (strParams[0].compare(SampleRunList::RfidParamScan) == 0) {
			pRfid_->scan();

		} else if (strParams[0].compare(SampleRunList::RfidParamRead) == 0) {
			std::vector<RfidTag_t> rfidTags;
			pRfid_->read (rfidTags, reagentRemainingVarOffset_);
		}

	} else if (cmd.compare(SampleRunList::RunScriptCmd) == 0) {
		// Actual directory creation is done in SampleRunList.cpp.
		Logger::L().Log (MODULENAME, severity_level::debug1, "Running included script...");

	} else if (cmd.compare(SampleRunList::RunScriptReturnCmd) == 0) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "Return from running included script...");

	} else {
		Logger::L().Log (MODULENAME, severity_level::warning, "*** UNSUPPORTED SCRIPT COMMAND: " + cmd + " ***");
	}

	return true;
}


// Indices into *dataPaths*                  0         1         2         3         4
// Only do the BF for now.
static std::vector<std::string> dataTypes {"CamBF", "CamFL1", "CamFL2", "CamFL3", "CamFL4"};
static std::vector<std::string> dataPaths;

//*****************************************************************************
bool BusinessObject::getDataType (size_t index, std::string& dataType) {

	if (dataPaths.size() == 0 || index > dataPaths.size()) {
		return false;
	}

	dataType = dataTypes[index];

	return true;
}

//*****************************************************************************
bool BusinessObject::createDataDirectories (uint32_t imageCnt, std::string subdir) {

	dataPaths.clear();

	for (size_t i=0; i < dataTypes.size(); i++) {
		char row;
		uint32_t col;
		samplePosition.getRowColumn (row, col);

		std::string dirPath;
		if (subdir.length() == 0) {
			dirPath = boost::str (boost::format ("%s%c-%d/%d/%s")
								  % outputDirectory_
								  % row
								  % col
								  % imageCnt
								  % dataTypes[i]);
		} else {
			dirPath = boost::str (boost::format ("%s%s/%c-%d/%d/%s")
								  % outputDirectory_
								  % subdir
								  % row
								  % col
								  % imageCnt
								  % dataTypes[i]);
		}

		boost::filesystem::path p(dirPath);
		boost::system::error_code ec;
		boost::filesystem::create_directories (p, ec);
		if (ec == boost::system::errc::success) {
			Logger::L().Log (MODULENAME, severity_level::debug3, "created directory: " + p.generic_string());
			dataPaths.push_back (dirPath);

		} else {
			Logger::L().Log (MODULENAME, severity_level::error, "failed to create directory: " + p.generic_string() + " : " + ec.message());
			return false;
		}
	}

	return true;
}

//*****************************************************************************
bool BusinessObject::getDataPath (size_t index, std::string& dataPath) {

	if (dataPaths.size() == 0 || index > dataPaths.size()) {
		return false;
	}

    dataPath = dataPaths[index];

	return true;
}

//*****************************************************************************
int main (int argc, char *argv[]) {

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "BusinessObjectTest.info");
	
    Logger::L().Log (MODULENAME, severity_level::normal, "Starting BusinessObject");

	BusinessObject bizobjTest;
	if (!bizobjTest.readConfiguration ("HawkeyeDLL.info", ec)) {
		Logger::L().Log (MODULENAME, severity_level::normal, "error loading application or DLL configuration, exiting...");
		Logger::L().Flush();
		exit (0);
	}

	po::options_description desc("Allowed options");
	desc.add_options ()
		("help,h", "Display options")
		("interactive,i", "Read script from the command line")	//TODO: this is incomplete...
		("file,f", po::value<std::string>(), "Read script from the specified file")
		("analyze,a", "Run L&T analysis on data")
		;

	po::variables_map vm;
	try {
		po::store (po::parse_command_line (argc, argv, desc), vm);
	}
	catch (po::error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << "Exception: " << desc << std::endl;
		exit (-1);
	}

	po::notify (vm);

	if (vm.count ("help")) {
		std::cout << desc << std::endl;
		exit (0);
	}

	if (bizobjTest.init()) {
	}

	//TODO: the *interactive* part of this is incomplete...
	if (vm.count ("interactive")) {
		std::string line;

		while (true) {
			std::cout << "Command: ";
			std::cin >> line;
			boost::algorithm::trim (line);
			if (line.length() > 0) {
				if (!bizobjTest.doCommandLine (line)) {
					Logger::L().Log (MODULENAME, severity_level::normal, "error processing: " + line);
					Logger::L().Flush();
				}
			}
		}
		exit (0);
	
	} else if (vm.count ("file")) {
		Logger::L().Log (MODULENAME, severity_level::normal, "Running: " + vm["file"].as<std::string>());
		if (!bizobjTest.doSampleRunList (vm["file"].as<std::string>())) {
			Logger::L().Log (MODULENAME, severity_level::normal, "error processing Sample RunFile, exiting...");
			Logger::L().Flush();
			exit (0);
		}

	//TODO: incomplete...
	} else if (vm.count ("analyze")) {	


	} else {
		Logger::L().Log (MODULENAME, severity_level::normal, "No Sample RunFile specified.");
	}

	Logger::L().Flush();

    return 0;
}
