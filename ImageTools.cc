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
#include <getopt.h>

/* The name of this program.  */
const char* program_name;

void print_usage (FILE* stream, int exit_code)
{
  fprintf (stream, "Usage:  %s options [ inputfile ... ]\n", program_name);
  fprintf (stream,
           "  -h  --help             Display this usage information.\n"
           "  -v  --verbose          Print verbose messages.\n");
  exit (exit_code);
}


int main(int argc, char *argv[]) {

    int next_option;

    /* A string listing valid short options letters.  */
    const char* const short_options = "ho:vf";
    /* An array describing valid long options.  */
    const struct option long_options[] = {
        { "help",       0, NULL, 'h' },
        //{ "output",       1, NULL, 'o' },
        { "verbose",    0, NULL, 'v' },
        { "format",     0, NULL, 'f' },
        { NULL,         0, NULL, 0   }   /* Required at end of array.  */
    };

    int verbose = 0;

    /* Remember the name of the program, to incorporate in messages.
       The name is stored in argv[0].  */
    program_name = argv[0];

    do {
        next_option = getopt_long (argc, argv, short_options,
                                   long_options, NULL);
        switch (next_option)
        {
        case 'h':   /* -h or --help */
          /* User has requested usage information.  Print it to standard
             output, and exit with exit code zero (normal termination).  */
          print_usage (stdout, 0);

        case 'v':   /* -v or --verbose */
          verbose = 1;
          break;

        case '?':   /* The user specified an invalid option.  */
          /* Print usage information to standard error, and exit with exit
             code one (indicating abnormal termination).  */
          print_usage (stderr, 1);

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
    
    
    

    GoogleString imageString;
    imageString.clear();
    GoogleString fileName;
    fileName.append(argv[optind]);
    
    Image image;
    image.readFile(fileName);
    
    if(image.analyze()) {
        fprintf(stdout, "format=%s\n",  image.imageFormatAsString());
        fprintf(stdout, "width=%i\nheight=%i\n", image.width(), image.height());
        fprintf(stdout, "photo=%i\ntransparent=%i\n", image.isPhoto(), image.hasTransparency());  
        fprintf(stdout, "animated=%i\n", image.isAnimated());  
        fprintf(stdout, "frames=%i\n", image.frames()); 
    } else {
        fprintf(stderr, "Could not analyze image");
    }
    



    
    return 0;
}