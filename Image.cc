/* 
 * File:   Image.cc
 * Author: shawn
 * 
 * Created on May 13, 2014, 11:04 PM
 */

#include "Image.h"

#include "pagespeed/kernel/image/image_util.h"
#include "pagespeed/kernel/image/image_analysis.h"
//#include "pagespeed/kernel/image/test_utils.h"
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
#include "gif_lib.h"
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


Image::Image() :  
        imageFormat_(IMAGE_UNKNOWN),
        analyzed_(false),
        isPhoto_(false),
        isAnimated_(false),
        frames_(1),
        hasTransparency_(false) {
    
}

Image::Image(const Image& orig) {
    
}

Image::~Image() {
}

bool Image::readFile(const GoogleString& file_name) {
  net_instaweb::StdioFileSystem file_system;
  net_instaweb::StringWriter writer(&content_);
  net_instaweb::MockMessageHandler messageHandler(new net_instaweb::NullMutex);
  return(file_system.ReadFile(file_name.c_str(), &writer, &messageHandler));
}


int ReadGifFromStream(GifFileType* gif_file, GifByteType* data, int length) {
  pagespeed::image_compression::ScanlineStreamInput* input =
    static_cast<pagespeed::image_compression::ScanlineStreamInput*>(
      gif_file->UserData);
  if (input->offset() + length <= input->length()) {
    memcpy(data, input->data() + input->offset(), length);
    input->set_offset(input->offset() + length);
    return length;
  } else {
    fprintf(stderr, "Unexpected EOF.");
    return 0;
  }
}

bool Image::analyze() {

    net_instaweb::MockMessageHandler messageHandler(new net_instaweb::NullMutex);
    
    ComputeImageType();
    
    if(imageFormat_ == pagespeed::image_compression::IMAGE_GIF) {
       
        ScanlineStreamInput input(NULL);
        input.Initialize(content_);
        GifFileType* gif = DGifOpen(&input, ReadGifFromStream, NULL);
        if (!gif) {
            fprintf(stderr, "Failed to get image descriptor.");
            analyzed_ = false;
            return false;
        }
        DGifSlurp(gif);
        frames_ = gif->ImageCount;
        if (frames_ > 1) {
            isAnimated_ = true;
        }
    }
        
    if(!isAnimated_) {
        if(AnalyzeImage(imageFormat_, content_.data(),
                            content_.length(), &messageHandler,
                            &hasTransparency_, &isPhoto_)) {

            analyzed_ = true;
        }
    }
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

ImageFormat Image::imageFormat() {
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

const char* Image::imageFormatAsString(){
    return ImageFormatToMimeTypeString(imageFormat_);
}
    

//code from ImageImpl

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
    height_ = net_instaweb::PngIntAtPosition(
        buf, ImageHeaders::kIHDRDataStart + ImageHeaders::kPngIntSize);
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
  if (buf.size() >= 8) {
    // Note that gcc rightly complains about constant ranges with the
    // negative char constants unless we cast.
    switch (net_instaweb::CharToInt(buf[0])) {
      case 0xff:
        // Either jpeg or jpeg2
        // (the latter we don't handle yet, and don't bother looking for).
        if (net_instaweb::CharToInt(buf[1]) == 0xd8) {
          imageFormat_ = pagespeed::image_compression::IMAGE_JPEG;
          FindJpegSize();
        }
        break;
      case 0x89:
        // Possible png.
        if (StringPiece(buf.data(), ImageHeaders::kPngHeaderLength) ==
            StringPiece(ImageHeaders::kPngHeader,
                        ImageHeaders::kPngHeaderLength)) {
          imageFormat_ = pagespeed::image_compression::IMAGE_PNG;
          FindPngSize();
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
          imageFormat_ = pagespeed::image_compression::IMAGE_GIF;
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
            imageFormat_ = pagespeed::image_compression::IMAGE_WEBP;
          } else {
            imageFormat_ = pagespeed::image_compression::IMAGE_WEBP;
          }
          FindWebpSize();
        }
        break;
      default:
        break;
    }
  }
}

