// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include <third_party/apr/src/include/apr_time.h>
//#include <pagespeed/kernel/base/string.h>
#include "pagespeed/kernel/image/image_util.h"
#include "pagespeed/kernel/image/image_analysis.h"
//#include "pagespeed/kernel/image/test_utils.h"
#include "pagespeed/kernel/base/stdio_file_system.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/mock_message_handler.h"
#include "pagespeed/kernel/base/null_mutex.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/string_writer.h"

#include "Image.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, "Usage: %s image\n", argv[0]);
        return 1;
    }

    GoogleString imageString;
    imageString.clear();
    GoogleString fileName;
    fileName.append(argv[1]);
    
    Image image;
    image.readFile(fileName);
    
    if(image.analyze()) {
        fprintf(stdout, "AnalyzeImage Complete\n");  
        fprintf(stdout, "\tformat=%s\n",  image.imageFormatAsString());
        fprintf(stdout, "\twidth=%i\n\theight=%i\n", image.width(), image.height());
        fprintf(stdout, "\tphoto=%i\n\ttransparent=%i\n", image.isImage(), image.hasTransparency());  
    } else {
        fprintf(stdout, "AnalyzeImage failed");
    }
    



    
    return 0;
}