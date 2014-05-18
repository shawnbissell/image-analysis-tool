/* 
 * File:   Image.h
 * Author: shawn
 *
 * Created on May 13, 2014, 11:04 PM
 */

#ifndef IMAGE_H
#define	IMAGE_H

#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/image/image_util.h"

using namespace pagespeed::image_compression;

class Image {
public:
    Image();
    Image(const Image& orig);
    virtual ~Image();
    
    bool readFile(const GoogleString& file_name);
    bool analyze();
    bool isImage();
    bool hasTransparency();
    
    ImageFormat imageFormat();
    const char * imageFormatAsString();
    int height();
    int width();
 
    
private:
    GoogleString content_;
    pagespeed::image_compression::ImageFormat imageFormat_;
    bool analyzed_;
    bool isImage_;
    bool hasTransparency_;
    int height_;
    int width_;
    
    void ComputeImageType();
    void FindJpegSize();
    void FindPngSize();
    void FindGifSize();
    void FindWebpSize();
    

};

#endif	/* IMAGE_H */

