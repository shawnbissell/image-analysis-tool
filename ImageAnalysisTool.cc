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

#include "ImageAnalysisToolConfig.h"
#include "Image.h"
#include <getopt.h>

const char* program_name;

void print_usage (FILE* stream, int exit_code)
{
    fprintf (stream, "Version: Image Analysis Tool %i.%i https://github.com/shawnbissell/image-analysis-tool\n", 
                    ImageAnalysisTool_VERSION_MAJOR, ImageAnalysisTool_VERSION_MINOR);
    fprintf (stream, "Copyright: Shawn Bissell (C) 2015\n");
    fprintf (stream, "Usage:  %s options [ inputfile ... ]\n", program_name);
    fprintf (stream,
            "  -h  --help             Display this usage information.\n"
            "  -p  --photo            Check if the image is a photo.\n"
            "  -t  --transparency     Check if the image usage transparency.\n"
            "  -a  --animated         Check if the image is animated.\n"
            "  -e  --extended         Output extended PNG information (bit-depth, color-type, gamma).\n"
            "  -A  --All              Check all available options.\n"
            "  -v  --verbose          Print verbose messages.\n");
    exit (exit_code);
}


int main(int argc, char *argv[]) {

    int next_option;

    /* A string listing valid short options letters.  */
    const char* const short_options = "hvptaeA";
    /* An array describing valid long options.  */
    const struct option long_options[] = {
        { "help",       0, NULL, 'h' },
        { "verbose",    0, NULL, 'v' },
        { "photo",      0, NULL, 'p' },
        { "transparency",0,NULL, 't' },
        { "animated",   0, NULL, 'a' },
        { "extended",   0, NULL, 'e' },
        { "all",        0, NULL, 'A' },
        { NULL,         0, NULL, 0   }   /* Required at end of array.  */
    };

    int verbose = 0;
    int checkPhoto = 0;
    int checkTransparency = 0;
    int checkAnimated = 0;
    int checkExtended = 0;

    /* Remember the name of the program, to incorporate in messages.
       The name is stored in argv[0].  */
    program_name = argv[0];

    do {
        next_option = getopt_long (argc, argv, short_options,
                                   long_options, NULL);
        switch (next_option)
        {
        case 'h':  
          print_usage (stdout, 0);
          break;  
        case 'v':   
          verbose = 1;
          break;
        case 'p':   
          checkPhoto = 1;
          break;
        case 't':   
          checkTransparency = 1;
          break;
        case 'a':   
          checkAnimated = 1;
          break;
        case 'e':
            checkExtended = 1;
            break;
        case 'A':
          checkPhoto = 1;
          checkTransparency = 1;
          checkAnimated = 1;
          checkExtended = 1;
          break;
        case '?':   /* The user specified an invalid option.  */
          /* Print usage information to standard error, and exit with exit
             code one (indicating abnormal termination).  */
          print_usage (stderr, 69);
          return 1;
        case -1:    /* Done with options.  */
          break;

        default:    /* Something else: unexpected.  */
            return 1;
        }
    }
    while (next_option != -1);
    
    if (verbose) {
        int i;
        for (i = optind; i < argc; ++i) 
          printf ("input file: %s\n", argv[i]);
    }
    
    if(optind < argc) {
        
        GoogleString imageString;
        imageString.clear();
        GoogleString fileName;
        fileName.append(argv[optind]);

        Image image(verbose);
        if(!image.readFile(fileName)){
            fprintf(stderr, "Could not read image\n");
            return 66;
        }

        if(image.analyze(checkTransparency, checkAnimated, checkPhoto, checkExtended)) {
            fprintf(stdout, "format=%s\n",  image.imageFormatAsString());
            fprintf(stdout, "width=%i\nheight=%i\n", image.width(), image.height());
            if(checkPhoto) fprintf(stdout, "photo=%i\n", image.isPhoto()); 
            if(checkTransparency) fprintf(stdout, "transparent=%i\n", image.hasTransparency());
            if(checkAnimated) {
                fprintf(stdout, "animated=%i\n", image.isAnimated());  
                fprintf(stdout, "frames=%i\n", image.frames()); 
            }
            if(checkExtended && image.imageFormat() == IMAGE_FORMAT_PNG) {
                fprintf(stdout, "bitdepth=%i\n", image.bitdepth());
                fprintf(stdout, "colortype=%i\n", image.colortype());
                fprintf(stdout, "hasGamma=%i\n", image.hasGamma());
                fprintf(stdout, "gamma=%f\n", image.gamma());
            }
        } else {
            fprintf(stderr, "Could not analyze image.\n");
            return 65;
        }
        return 0;
    } 
    print_usage (stderr, 64);
}
