/*
 * Copyright 2014-2015 Shawn Bissell 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IMAGE_H
#define	IMAGE_H

#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/image/image_util.h"

extern "C" {
#include "gif_lib.h"    
}

#define PNG_DEBUG 3
#include <png.h>


using namespace pagespeed::image_compression;

enum Format {
  IMAGE_FORMAT_UNKNOWN,
  IMAGE_FORMAT_JPEG,
  IMAGE_FORMAT_PNG,
  IMAGE_FORMAT_GIF,
  IMAGE_FORMAT_WEBP,
  IMAGE_FORMAT_JP2K,
  IMAGE_FORMAT_JXR
};

class Image {
public:
    Image(bool verbose);
    virtual ~Image();
    
    bool readFile(const GoogleString& file_name);
    bool analyze(bool checkTransparency, bool checkAnimated, bool checkPhoto, bool checkExtended);
    bool isPhoto();
    bool isAnimated();
    bool hasTransparency();
    
    Format imageFormat();
    const char * imageFormatAsString();
    ImageFormat getGoogleImageFormat();
    int height();
    int width();
    int frames();
    int bitdepth();
    int colortype();
    bool hasGamma();
    double gamma();
 
    
private:
    bool verbose_;
    GoogleString filename_;
    GoogleString content_;
    Format imageFormat_;
    bool isPhoto_;
    bool isAnimated_;
    bool hasTransparency_;
    int height_;
    int width_;
    int frames_;
    int bitdepth_;
    int colortype_;
    bool hasGamma_;
    double gamma_;
    
    bool  CheckTranparentColorUsed(const GifFileType* gif, int transparentColor);
    
    void ComputeImageType();
    void FindJpegSize();
    void FindPngSize();
    bool FindPngDetails();
    void FindGifSize();
    bool FindGifDetails(bool checkTransparency);
    void FindWebpSize();
    
      

};

#endif	/* IMAGE_H */

