//
//  dng_converter.cpp
//  dng_converter
//
//  Created by mgharbi on 5/14/16.
//  Copyright © 2016 mgharbi. All rights reserved.
//

#include "dng_converter.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

#include "dng_types.h"
#include "dng_orientation.h"
#include "dng_xy_coord.h"
#include "dng_orientation.h"
#include "dng_host.h"
#include "dng_file_stream.h"
#include "dng_tag_values.h"
#include "dng_rect.h"
#include "dng_pixel_buffer.h"
#include "dng_negative.h"
#include "dng_exif.h"
#include "dng_camera_profile.h"
#include "dng_image.h"
#include "dng_image_writer.h"
#include "dng_preview.h"
#include "dng_render.h"
#include "dng_color_space.h"

#include "png_load.hpp"

using namespace std;

// --------------------------------------------------------------------------------
//
// MakeDNGSample
//

void make_bayer(vector<unsigned char> image, unsigned width, unsigned height, uint16 *buffer) {
    int idx = 0;
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) 
    {
        double r = image[idx++];
        double g = image[idx++];
        double b = image[idx++];
        idx++;

        // mosaick
        double val = 0.0;
        if(y%2 == 0 && x%2 == 1) {
            val = r;
        } else if (y%2 == 1 && x%2 == 0) {
            val = b;
        } else {
            val = g;
        }

        val /= 255.0;
        assert(val<=1);


        // srgb2linear
        if ( val > 0.04045 ) {
            val = pow(( val + 0.055 ) / 1.055 , 2.4);
        } else {
            val = val / 12.92;
        }  

        // map to x-bits
        val *= (1 <<8)-1;

        buffer[x+width*y] = val;
    }
}

dng_error_code make_dng(std::string m_szInputFile, std::string output_file)
{
    // Sample BAYER image at ISO100 and tEXP 1/10 on f/4.0 and focal length 10mm
    uint16 m_unISO            = 100;
    double m_dExposureTime    = 0.1;
    double m_dLensAperture    = 4.0;
    double m_dFocalLength     = 10.0;
    
    // SETTINGS: Whitebalance D65, Orientation "normal"
    dng_orientation m_oOrientation = dng_orientation::Normal();
    
    // SETTINGS: Names
    std::string m_szMake = "MGH limited";
    std::string m_szCameraModel = "MGH Canon 1Dx mark III";
    std::string szProfileName = "MGH profile";
    std::string szProfileCopyright = m_szMake;
    
    
    // Form output filenames
    std::string szBaseFilename         = "";
    std::string m_szOutputFile         = "";
    std::string m_szRenderFile         = "";
    // std::string m_szPathPrefixInput    = "";
    // std::string m_szPathPrefixOutput   = "";
    // std::string m_szPathPrefixProfiles = "";
    size_t unIndex = output_file.find_last_of(".");
    if (unIndex == std::string::npos)
    {
        szBaseFilename = output_file;
    }
    else
    {
        szBaseFilename = output_file.substr(0, unIndex);
    }
    // m_szInputFile = m_szInputFile;
    m_szOutputFile = output_file;
    m_szRenderFile = szBaseFilename + ".tiff";
    

    dng_host oDNGHost;
    printf("Converting %s\n", m_szInputFile.c_str());

    // SETTINGS: 12-Bit RGGB BAYER PATTERN
    uint8 m_unColorPlanes = 3;
    uint8 m_unBitDepth    = 8;
    uint32 m_ulBlackLevel = 0;
    uint32 m_ulWidth;
    uint32 m_ulHeight;

    // Calculate bit limit
    uint16 m_unBitLimit = 0x01 << m_unBitDepth;

    // Load input
    unsigned width, height;
    vector<unsigned char> image = png_load(m_szInputFile.c_str(), width, height);
    m_ulWidth = width;
    m_ulHeight = height;
    printf("loaded png %dx%d\n", width, height);

    // Make Bayer
    uint32 ullBayerSize = m_ulWidth*m_ulHeight*sizeof(uint16);
    AutoPtr<dng_memory_block> oBayerData(oDNGHost.Allocate(ullBayerSize));
    uint16* buffer = (uint16*) oBayerData->Buffer();
    memset(buffer,0,ullBayerSize);
    make_bayer(image, width, height, buffer);

    // -------------------------------------------------------------
    // DNG Host Settings
    // -------------------------------------------------------------

    // Set DNG version
    oDNGHost.SetSaveDNGVersion(dngVersion_SaveDefault);

    // Set DNG type to RAW DNG
    oDNGHost.SetSaveLinearDNG(false);

    // -------------------------------------------------------------
    // DNG Image Settings
    // -------------------------------------------------------------

    dng_rect vImageBounds(m_ulHeight, m_ulWidth);

    AutoPtr<dng_image> oImage(oDNGHost.Make_dng_image(vImageBounds, m_unColorPlanes, ttShort));

    dng_pixel_buffer oBuffer;

    oBuffer.fArea      = vImageBounds;
    oBuffer.fPlane     = 0;
    oBuffer.fPlanes    = 1;
    oBuffer.fRowStep   = oBuffer.fPlanes * m_ulWidth;
    oBuffer.fColStep   = oBuffer.fPlanes;
    oBuffer.fPlaneStep = 1;
    oBuffer.fPixelType = ttShort;
    oBuffer.fPixelSize = TagTypeSize(ttShort);
    oBuffer.fData      = oBayerData->Buffer();

    oImage->Put(oBuffer);


    // -------------------------------------------------------------
    // DNG Negative Settings
    // -------------------------------------------------------------

    AutoPtr<dng_negative> oNegative(oDNGHost.Make_dng_negative());

    // Set camera model
    oNegative->SetModelName(m_szCameraModel.c_str());
    oNegative->SetLocalName(m_szCameraModel.c_str());

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetColorKeys(colorKeyRed, colorKeyGreen, colorKeyBlue);
    uint16 m_unBayerType  = 0;    // RGGB
    oNegative->SetBayerMosaic(m_unBayerType);
    oNegative->SetColorChannels(m_unColorPlanes);

    // Set linearization table
    AutoPtr<dng_memory_block> oCurve(oDNGHost.Allocate(sizeof(uint16)*m_unBitLimit));
    for ( int32 i=0; i<m_unBitLimit; i++ )
    {
        uint16 *pulItem = oCurve->Buffer_uint16() + i;
        *pulItem = (uint16)(i);
    }
    oNegative->SetLinearization(oCurve);

    oNegative->SetBlackLevel(m_ulBlackLevel);
    oNegative->SetWhiteLevel(m_unBitLimit-1);
    oNegative->SetDefaultScale(dng_urational(1,1), dng_urational(1,1));
    oNegative->SetBestQualityScale(dng_urational(1,1));
    oNegative->SetDefaultCropOrigin(0, 0);
    oNegative->SetDefaultCropSize(m_ulWidth, m_ulHeight);
    oNegative->SetBaseOrientation(m_oOrientation);
    oNegative->SetBaselineExposure(0);
    oNegative->SetNoiseReductionApplied(dng_urational(1,1)); // no noise reduction
    oNegative->SetBaselineSharpness(1);
    oNegative->SetChromaBlurRadius(dng_urational(0,1));
    oNegative->SetAntiAliasStrength(dng_urational(0,1));

    // -------------------------------------------------------------
    // DNG EXIF Settings
    // -------------------------------------------------------------

    dng_exif *poExif = oNegative->GetExif();
    poExif->fMake.Set_ASCII(m_szMake.c_str());
    poExif->fModel.Set_ASCII(m_szCameraModel.c_str());
    poExif->fISOSpeedRatings[0] = m_unISO;
    poExif->fISOSpeedRatings[1] = 0;
    poExif->fISOSpeedRatings[2] = 0;
    poExif->fWhiteBalance = 0;
    poExif->fMeteringMode = 2;
    poExif->fExposureBiasValue = dng_srational(0, 0);
    poExif->SetFNumber(m_dLensAperture);
    poExif->SetExposureTime(m_dExposureTime);
    poExif->fFocalLength.Set_real64(m_dFocalLength, 1000);
    poExif->fLensInfo[0].Set_real64(m_dFocalLength, 10);
    poExif->fLensInfo[1].Set_real64(m_dFocalLength, 10);
    poExif->fLensInfo[2].Set_real64(m_dLensAperture, 10);
    poExif->fLensInfo[3].Set_real64(m_dLensAperture, 10);

    // -------------------------------------------------------------
    // DNG Profile Settings: Simple color calibration
    // -------------------------------------------------------------

    // White balance
    uint32 ulCalibrationIlluminant1 = lsD65;
    dng_xy_coord m_oWhitebalanceDetectedXY = D65_xy_coord();
    oNegative->SetCameraWhiteXY(m_oWhitebalanceDetectedXY);

    dng_matrix_3by3 XYZ_to_RGB = dng_matrix_3by3( 
      3.2406, -1.5372 ,-0.4986,
     -0.9689,  1.8758 , 0.0415,
      0.0557, -0.2040 , 1.0570);

    AutoPtr<dng_camera_profile> oProfile(new dng_camera_profile);

    printf("set calib illuminant 1\n");
    oProfile->SetCalibrationIlluminant1(ulCalibrationIlluminant1);
    oProfile->SetColorMatrix1(XYZ_to_RGB);

    oProfile->SetName(szProfileName.c_str());
    oProfile->SetCopyright(szProfileCopyright.c_str());
    oProfile->SetWasReadFromDNG(true);
    oProfile->SetEmbedPolicy(pepAllowCopying);
    dng_tone_curve ptc;
    printf("valid curve %d\n", ptc.IsValid());
    oProfile->SetToneCurve(ptc);

    oNegative->AddProfile(oProfile);


    // -------------------------------------------------------------
    // Write DNG file
    // -------------------------------------------------------------

    // Assign Raw image data.
    oNegative->SetStage1Image(oImage);
    // Compute linearized and range mapped image
    oNegative->BuildStage2Image(oDNGHost);
    // Compute demosaiced image (used by preview and thumbnail)
    oNegative->BuildStage3Image(oDNGHost);

    // Update XMP / EXIF
    oNegative->SynchronizeMetadata();
    // Update IPTC
    oNegative->RebuildIPTC(true,true);

    // Create stream writer for output file
    dng_file_stream oDNGStream(m_szOutputFile.c_str(), true);
    // Write DNG file to disk
    AutoPtr<dng_image_writer> oWriter(new dng_image_writer());
    dng_image_preview oPreview;
    oWriter->WriteDNG(oDNGHost, oDNGStream, *oNegative.Get(), oPreview, ccUncompressed);

    // -------------------------------------------------------------
    // Write TIFF file
    // -------------------------------------------------------------

    // Create stream writer for output file
    dng_file_stream oTIFFStream(m_szRenderFile.c_str(), true);

    // Create render object
    dng_render oRender (oDNGHost, *oNegative);

    // Set exposure compensation
    oRender.SetExposure(0.0);
    oRender.SetShadows(0);
    oRender.SetFinalSpace(dng_space_sRGB::Get());

    // Create final image
    AutoPtr<dng_image> oFinalImage;

    oRender.SetWhiteXY(m_oWhitebalanceDetectedXY);

    // Render image
    oFinalImage.Reset (oRender.Render ());
    oFinalImage->Rotate(oNegative->Orientation());

    // Write TIFF file to disk  
    oWriter->WriteTIFF(oDNGHost, oTIFFStream, *oFinalImage.Get(), piRGB, ccUncompressed, oNegative.Get(), &oRender.FinalSpace ());  

    printf ("Conversion complete\n");  

    return dng_error_none;  
}
