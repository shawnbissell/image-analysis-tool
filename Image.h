/*
 * Copyright 2014 Shawn Bissell 
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



using namespace pagespeed::image_compression;

class Image {
public:
    Image();
    Image(const Image& orig);
    virtual ~Image();
    
    bool readFile(const GoogleString& file_name);
    bool analyze(bool verbose, bool checkTransparency, bool checkAnimated, bool checkPhoto);
    bool isPhoto();
    bool isAnimated();
    bool hasTransparency();
    
    ImageFormat imageFormat();
    const char * imageFormatAsString();
    int height();
    int width();
    int frames();
 
    
private:
    GoogleString content_;
    pagespeed::image_compression::ImageFormat imageFormat_;
    bool analyzed_;
    bool isPhoto_;
    bool isAnimated_;
    bool hasTransparency_;
    int height_;
    int width_;
    int frames_;
    
    bool  CheckTranparentColorUsed(const GifFileType* gif, int transparentColor);
    
    void ComputeImageType();
    void FindJpegSize();
    void FindPngSize();
    void FindGifSize();
    void FindWebpSize();
    
   

};

#endif	/* IMAGE_H */

