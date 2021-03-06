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

#include "Image.h"

#include "pagespeed/kernel/image/image_util.h"
#include "pagespeed/kernel/image/image_analysis.h"
#include "pagespeed/kernel/base/stdio_file_system.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/mock_message_handler.h"
#include "pagespeed/kernel/base/null_mutex.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/string_writer.h"
#include "net/instaweb/rewriter/public/image_data_lookup.h"
#include "webp/decode.h"
#include "pagespeed/kernel/image/scanline_utils.h"

extern "C" {
#include "gif_err.c"    
}

using namespace pagespeed::image_compression;

namespace ImageHeaders {

const char kPngHeader[] = "\x89PNG\r\n\x1a\n";
const size_t kPngHeaderLength = STATIC_STRLEN(kPngHeader);
const char kPngIHDR[] = "\0\0\0\x0dIHDR";
const size_t kPngIntSize = 4;
const size_t kPngSectionHeaderLength = 2 * kPngIntSize;
const size_t kIHDRDataStart = kPngHeaderLength + kPngSectionHeaderLength;
const size_t kPngSectionMinSize = kPngSectionHeaderLength + kPngIntSize;
const size_t kPngColourTypeOffset = kIHDRDataStart + 2 * kPngIntSize + 1;
const char kPngAlphaChannel = 0x4;  // bit of ColourType set for alpha channel
const char kPngIDAT[] = "IDAT";
const char kPngtRNS[] = "tRNS";

const char kGifHeader[] = "GIF8";
const size_t kGifHeaderLength = STATIC_STRLEN(kGifHeader);
const size_t kGifDimStart = kGifHeaderLength + 2;
const size_t kGifIntSize = 2;

const size_t kJpegIntSize = 2;
const int64 kMaxJpegQuality = 100;
const int64 kQualityForJpegWithUnkownQuality = 85;

}  // namespace ImageHeaders


Image::Image(bool verbose) :
        verbose_(false),
        imageFormat_(IMAGE_FORMAT_UNKNOWN),
        height_(0),
        width_(0),
        isPhoto_(false),
        isAnimated_(false),
        frames_(1),
        hasTransparency_(false), 
        bitdepth_(0),
        colortype_(0),
        hasGamma_(false),
        gamma_(.454545)
{
    verbose_ = verbose;
}


Image::~Image() {
}

bool Image::isPhoto() {
    return isPhoto_;
}
bool Image::hasTransparency() {
    return hasTransparency_;
}
bool Image::isAnimated() {
    return isAnimated_;
}

Format Image::imageFormat() {
    return imageFormat_;
}

int Image::height() { 
    return height_;
}
int Image::width() {
    return width_;
}
int Image::frames() {
    return frames_;
}
int Image::bitdepth() {
    return bitdepth_;
}
int Image::colortype() {
    return colortype_;
}
bool Image::hasGamma() {
    return hasGamma_;
}
double Image::gamma() {
    return gamma_;
}


const char* Image::imageFormatAsString(){
    
    switch(imageFormat_)
    {
        case IMAGE_FORMAT_JP2K: return "JP2K";
        case IMAGE_FORMAT_JXR: return "JXR";
        case IMAGE_FORMAT_JPEG: return "JPEG";
        case IMAGE_FORMAT_GIF: return "GIF";
        case IMAGE_FORMAT_PNG: return "PNG";
        case IMAGE_FORMAT_WEBP: return "WEBP";
        default: return "UNKNOWN";
    }
  
}

ImageFormat Image::getGoogleImageFormat()
{
    switch(imageFormat_)
    {
        case IMAGE_FORMAT_JPEG: return IMAGE_JPEG;
        case IMAGE_FORMAT_GIF: return IMAGE_GIF;
        case IMAGE_FORMAT_PNG: return IMAGE_PNG;
        case IMAGE_FORMAT_WEBP: return IMAGE_WEBP;
        default: return IMAGE_UNKNOWN;
    }
}


bool Image::readFile(const GoogleString& file_name) {
    filename_.append(file_name);
    net_instaweb::StdioFileSystem file_system;
    net_instaweb::StringWriter writer(&content_);
    net_instaweb::MockMessageHandler messageHandler(new net_instaweb::NullMutex);
    return(file_system.ReadFile(filename_.c_str(), &writer, &messageHandler));
}



bool Image::analyze(bool checkTransparency, bool checkAnimated, bool checkPhoto, bool checkExtended) {

    net_instaweb::MockMessageHandler messageHandler(new net_instaweb::NullMutex);
    
    ComputeImageType();
    if(imageFormat_ == IMAGE_FORMAT_UNKNOWN) {
        fprintf(stderr, "Unknown Image Format.\n");
        return false;   
    }
    
    if(imageFormat_ == IMAGE_FORMAT_GIF && (checkTransparency || checkAnimated)) {
        if(!FindGifDetails(checkTransparency))
        {
            fprintf(stderr, "Failed to find GIF details.\n");
            return false;
        }
    }
    if(imageFormat_ == IMAGE_FORMAT_PNG && checkExtended) {
        if(!FindPngDetails()){
            fprintf(stderr, "Failed to find PNG details.\n");
            return false;
        }
            
    }
        
    if(!isAnimated_ && (checkTransparency || checkPhoto)
            && imageFormat_ != IMAGE_FORMAT_JP2K
            && imageFormat_ != IMAGE_FORMAT_JXR) {
        if(verbose_) fprintf(stdout, "pagespeed: analyzing image\n"); 
        if(AnalyzeImage(getGoogleImageFormat(), content_.data(),
                            content_.length(), &messageHandler,
                            &hasTransparency_, &isPhoto_)) {
            if(verbose_) fprintf(stdout, "pagespeed: HasTransparency=%i\n", hasTransparency_); 
            if(verbose_) fprintf(stdout, "pagespeed: IsPhoto=%i\n", isPhoto_);  
        } else {
            return false;
        }
    }
}



// Gif handling with GifLib
int ReadGifFromStream(GifFileType* gif_file, GifByteType* data, int length) {
  pagespeed::image_compression::ScanlineStreamInput* input =
    static_cast<pagespeed::image_compression::ScanlineStreamInput*>(
      gif_file->UserData);
  if (input->offset() + length <= input->length()) {
    memcpy(data, input->data() + input->offset(), length);
    input->set_offset(input->offset() + length);
    return length;
  } else {
    fprintf(stderr, "Unexpected EOF.\n");
    return 0;
  }
}

bool Image::FindGifDetails(bool checkTransparency) {
    if(verbose_) fprintf(stdout, "libgif: analyzing gif image\n"); 
    ScanlineStreamInput input(NULL);
    input.Initialize(content_);
    GifFileType* gif = DGifOpen(&input, ReadGifFromStream, NULL);
    if (!gif) {
        fprintf(stderr, "Failed to get image descriptor.\n");
        return false;
    }
    if(DGifSlurp(gif)) {
        frames_ = gif->ImageCount;
        if(verbose_) fprintf(stdout, "libgif: Frames=%i\n", frames_);  
        if (frames_ <= 0) {
            fprintf(stderr, "No frames in gif file.\n");
            return false;
        }
        if (frames_ > 1) {
            if(verbose_) fprintf(stdout, "libgif: IsAnimated=1\n"); 
            isAnimated_ = true;
        }

        if(checkTransparency) {
            GraphicsControlBlock gcb;
            int result = DGifSavedExtensionToGCB(gif, 0, &gcb);
            if(verbose_ && result == GIF_ERROR) fprintf(stdout, "libgif: no GraphicsControlBlock found using default values\n"); 
            if(verbose_) fprintf(stdout, "libgif: TransparentColor=%i\n", gcb.TransparentColor);  
            bool isTransparent = CheckTranparentColorUsed(gif, gcb.TransparentColor); 
            hasTransparency_ = (int)isTransparent;
            if(verbose_) fprintf(stdout, "libgif: IsTransparent=%i\n", isTransparent);
            checkTransparency = false;
        }

    }
    else {
        const char* giflib_error_str = (const char*) GifErrorString(gif->Error);
        if (giflib_error_str == NULL) giflib_error_str = "Unknown error";
        if(verbose_) fprintf(stdout, "libgif: failed to slurp gif, %s\n", giflib_error_str ); 
        DGifCloseFile(gif, NULL);
        return false;
    }
    DGifCloseFile(gif, NULL);
    return true;
}

bool Image::CheckTranparentColorUsed(const GifFileType* gif, int transparentColor) {
    int width  = gif->Image.Width;
    int height = gif->Image.Height;
    for(int i=0; i < gif->ImageCount; i++) {
        unsigned char *ptr = gif->SavedImages[i].RasterBits;
        int color;
        for (int j=height-1; j>=0; j--) {
            int index = 3*j*width;
            for (int i=0; i < width; i++) {
                color = *ptr++;
                if(color == transparentColor) {
                    hasTransparency_ = true;
                    return true;
                }
            }
        }
    }
    return false;
}

//PNG handling with libpng
bool Image::FindPngDetails() {
    const StringPiece& buf = content_;
  
    png_structp png_ptr;
    png_infop info_ptr;
    

    if ((buf.size() > 0)) {// Not truncated
        FILE *fp = fopen(filename_.c_str(), "rb");
        if (!fp) {
           fprintf(stderr, "FindPngDetails File %s could not be opened for reading\n", filename_.c_str());
           return false;
        }
        
        char header[8];
        fread(header, 1, 8, fp);
       
        if (png_sig_cmp(reinterpret_cast<png_const_bytep>(header), 0, 8)) {
            fprintf(stderr, "FindPngDetails Did not recognize file has a PNG\n");
            fclose(fp);
            return false;
        }
       
       /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr) {
            fprintf(stderr, "FindPngDetails png_create_read_struct failed\n");
            png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
            fclose(fp);
            return false;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            fprintf(stderr, "FindPngDetails png_create_info_struct failed\n");
            png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
            fclose(fp);
            return false;
        }

        if(verbose_) fprintf(stdout, "libpng: init\n");
        
        if (setjmp(png_jmpbuf(png_ptr))) {
            
            fprintf(stderr, "FindPngDetails setjmp Error\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
            fclose(fp);
            return false;
        }
        
        if(verbose_) fprintf(stdout, "libpng: Reading in PNG\n");
        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);
        
        if(!png_get_gAMA(png_ptr, info_ptr, &gamma_)){
            if(verbose_) fprintf(stdout, "libpng: Gamma was not found in PNG, using default value.\n");
            gamma_ = .454545;
        } else {
            if(verbose_) fprintf(stdout, "libpng: Gamma was found in PNG.\n");
            hasGamma_ = true;
        }
            
        fclose(fp);
        return true;
    } else {
      fprintf(stderr, "Couldn't find png details (data truncated or IHDR missing).\n");
      return false;
    }
}

    

//From modpagespeed ImageImpl Class
//Code below this point is adapted from 
//https://code.google.com/p/modpagespeed/source/browse/trunk/src/net/instaweb/rewriter/image.cc

// Looks through blocks of jpeg stream to find SOFn block
// indicating encoding and dimensions of image.
// Loosely based on code and FAQs found here:
//    http://www.faqs.org/faqs/jpeg-faq/part1/
void Image::FindJpegSize() {
  const StringPiece& buf = content_;
  size_t pos = 2;  // Position of first data block after header.
  while (pos < buf.size()) {
    // Read block identifier
    int id = net_instaweb::CharToInt(buf[pos++]);
    if (id == 0xff) {  // Padding byte
      continue;
    }
    // At this point pos points to first data byte in block.  In any block,
    // first two data bytes are size (including these 2 bytes).  But first,
    // make sure block wasn't truncated on download.
    if (pos + ImageHeaders::kJpegIntSize > buf.size()) {
      break;
    }
    int length = net_instaweb::JpegIntAtPosition(buf, pos);
    // Now check for a SOFn header, which describes image dimensions.
    if (0xc0 <= id && id <= 0xcf &&  // SOFn header
        length >= 8 &&               // Valid SOFn block size
        pos + 1 + 3 * ImageHeaders::kJpegIntSize <= buf.size() &&
        // Above avoids case where dimension data was truncated
        id != 0xc4 && id != 0xc8 && id != 0xcc) {
      // 0xc4, 0xc8, 0xcc aren't actually valid SOFn headers.
      // NOTE: we don't care if we have the whole SOFn block,
      // just that we can fetch both dimensions without trouble.
      // Our image download could be truncated at this point for
      // all we care.
      // We're a bit sloppy about SOFn block size, as it's
      // actually 8 + 3 * buf[pos+2], but for our purposes this
      // will suffice as we don't parse subsequent metadata (which
      // describes the formatting of chunks of image data).
      height_ = net_instaweb::JpegIntAtPosition(buf, pos + 1 + ImageHeaders::kJpegIntSize);
      width_  = net_instaweb::JpegIntAtPosition(buf, pos + 1 + 2 * ImageHeaders::kJpegIntSize);
      break;
    }
    pos += length;
  }
  if ((height_ <= 0) || (width_ <= 0)) {
    height_ = 0;
    width_ = 0;
    fprintf(stderr, "Couldn't find jpeg dimensions (data truncated?).");
  }
}

// Looks at first (IHDR) block of png stream to find image dimensions.
// See also: http://www.w3.org/TR/PNG/
void Image::FindPngSize() {
  const StringPiece& buf = content_;
  // Here we make sure that buf contains at least enough data that we'll be able
  // to decipher the image dimensions first, before we actually check for the
  // headers and attempt to decode the dimensions (which are the first two ints
  // after the IHDR section label).
  if ((buf.size() >=  // Not truncated
       ImageHeaders::kIHDRDataStart + 2 * ImageHeaders::kPngIntSize) &&
      (StringPiece(buf.data() + ImageHeaders::kPngHeaderLength,
                   ImageHeaders::kPngSectionHeaderLength) ==
       StringPiece(ImageHeaders::kPngIHDR,
                   ImageHeaders::kPngSectionHeaderLength))) {
    width_ = net_instaweb::PngIntAtPosition(buf, ImageHeaders::kIHDRDataStart);
    height_ = net_instaweb::PngIntAtPosition(buf, ImageHeaders::kIHDRDataStart + ImageHeaders::kPngIntSize);
    bitdepth_ = net_instaweb::CharToInt(buf[ImageHeaders::kIHDRDataStart + (2 * ImageHeaders::kPngIntSize)]);
    colortype_ = net_instaweb::CharToInt(buf[ImageHeaders::kIHDRDataStart + (2 * ImageHeaders::kPngIntSize) + 1]);
  } else {
    fprintf(stderr, "Couldn't find png dimensions (data truncated or IHDR missing).");
  }
}
  


// Looks at header of GIF file to extract image dimensions.
// See also: http://en.wikipedia.org/wiki/Graphics_Interchange_Format
void Image::FindGifSize() {
  const StringPiece& buf = content_;
  // Make sure that buf contains enough data that we'll be able to
  // decipher the image dimensions before we attempt to do so.
  if (buf.size() >= ImageHeaders::kGifDimStart + 2 * ImageHeaders::kGifIntSize) {
    // Not truncated
    width_ = net_instaweb::GifIntAtPosition(buf, ImageHeaders::kGifDimStart);
    height_ = net_instaweb::GifIntAtPosition(
        buf, ImageHeaders::kGifDimStart + ImageHeaders::kGifIntSize);
  } else {
    fprintf(stderr, "Couldn't find gif dimensions (data truncated)");
  }
}

void Image::FindWebpSize() {
  const uint8* webp = reinterpret_cast<const uint8*>(content_.data());
  const int webp_size = content_.size();
  int width = 0, height = 0;
  if (WebPGetInfo(webp, webp_size, &width, &height) > 0) {
    width_ = width;
    height_ = height;
  } else {
    fprintf(stderr, "Couldn't find webp dimensions ");
  }
}

// Looks at image data in order to determine image type, and also fills in any
// dimension information it can (setting image_type_ and dims_).
void Image::ComputeImageType() {
  // Image classification based on buffer contents gakked from leptonica,
  // but based on well-documented headers (see Wikipedia etc.).
  // Note that we can be fooled if we're passed random binary data;
  // we make the call based on as few as two bytes (JPEG).
  const StringPiece& buf = content_;
  if (verbose_) fprintf(stdout, "buffer.size %ld\n", buf.size());
  if (buf.size() >= 8) {
    if (verbose_) fprintf(stdout, "buffer[0] %c %d\n", buf[0],  net_instaweb::CharToInt(buf[0]));
    if (verbose_) fprintf(stdout, "buffer[1] %c %d\n", buf[1],  net_instaweb::CharToInt(buf[1]));
    if (verbose_) fprintf(stdout, "buffer[2] %c %d\n", buf[2],  net_instaweb::CharToInt(buf[2]));
    if (verbose_) fprintf(stdout, "buffer[3] %c %d\n", buf[3],  net_instaweb::CharToInt(buf[3]));
    // Note that gcc rightly complains about constant ranges with the
    // negative char constants unless we cast.
    switch (net_instaweb::CharToInt(buf[0])) {
      case 0x00:
          //possible jp2k
          if (net_instaweb::CharToInt(buf[1]) == 0x00
                  && net_instaweb::CharToInt(buf[2]) == 0x00
                  && net_instaweb::CharToInt(buf[3]) == 0x0c 
                  && net_instaweb::CharToInt(buf[4]) == 0x6a 
                  && net_instaweb::CharToInt(buf[5]) == 0x50
                  && net_instaweb::CharToInt(buf[6]) == 0x20 
                  && net_instaweb::CharToInt(buf[7]) == 0x20) {
              imageFormat_ = IMAGE_FORMAT_JP2K;
          }
          break;
              
      case 0xff:
        // Either jpeg or jpeg2
        // (the latter we don't handle yet, and don't bother looking for).
        if (net_instaweb::CharToInt(buf[1]) == 0xd8) {
          imageFormat_ = IMAGE_FORMAT_JPEG;
          FindJpegSize();
        }
        break;
      case 0x89:
        // Possible png.
        if (StringPiece(buf.data(), ImageHeaders::kPngHeaderLength) ==
            StringPiece(ImageHeaders::kPngHeader,
                        ImageHeaders::kPngHeaderLength)) {
          imageFormat_ = IMAGE_FORMAT_PNG;
          FindPngSize();
        }
        break;
      case 0x49:
          //possible JXR
          if (net_instaweb::CharToInt(buf[1]) == 0x49
                  && net_instaweb::CharToInt(buf[2]) == 0xbc
                  && net_instaweb::CharToInt(buf[3]) == 0x01) {
              imageFormat_ = IMAGE_FORMAT_JXR;
          }
          break;
      case 'G':
        // Possible gif.
        if ((StringPiece(buf.data(), ImageHeaders::kGifHeaderLength) ==
             StringPiece(ImageHeaders::kGifHeader,
                         ImageHeaders::kGifHeaderLength)) &&
            (buf[ImageHeaders::kGifHeaderLength] == '7' ||
             buf[ImageHeaders::kGifHeaderLength] == '9') &&
            buf[ImageHeaders::kGifHeaderLength + 1] == 'a') {
          imageFormat_ = IMAGE_FORMAT_GIF;
          FindGifSize();
        }
        break;
      case 'R':
        // Possible Webp
        // Detailed explanation on parsing webp format is available at
        // http://code.google.com/speed/webp/docs/riff_container.html
        if (buf.size() >= 20 && buf.substr(1, 3) == "IFF" &&
            buf.substr(8, 4) == "WEBP") {
          if (buf.substr(12, 4) == "VP8L") {
            imageFormat_ = IMAGE_FORMAT_WEBP;
          } else {
            imageFormat_ = IMAGE_FORMAT_WEBP;
          }
          FindWebpSize();
        }
        break;
      default:
        break;
    }
  }
}

