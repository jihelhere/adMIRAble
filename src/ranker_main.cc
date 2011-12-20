#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Predictor.hh"

#include <getopt.h>

#define NUM_THREADS 1

void  print_help_message(char *program_name)
{
fprintf(stderr, "%s usage: %s [options]\n", program_name, program_name);
 fprintf(stderr, "OPTIONS :\n");
 fprintf(stderr, "      --model,-m    <file>           : model file\n");
 fprintf(stderr, "      --threads,-j  <int>            : nb of threads (default is %d)\n", NUM_THREADS);
 fprintf(stderr, "      --limit,-l    <int>            : max nb of candidates (default is 0, no maximum)\n");
 fprintf(stderr, "      -help,-h                       : print this message\n");
}


int main(int argc, char** argv) {
  char * model = NULL;
  int threads = NUM_THREADS;
  int limit = 0;

  // read the commandline
  int c;
  while(1) {

    static struct option long_options[] =
      {
        /* These options don't set a flag.
           We distinguish them by their indices. */
        {"help",     no_argument,             0, 'h'},
        {"model",    required_argument,       0, 'm'},
        {"limit",    required_argument,       0, 'l'},
        {"threads",  required_argument,       0, 'j'},
        {0, 0, 0, 0}
      };
    // int to store arg position
    int option_index = 0;

    c = getopt_long (argc, argv, "m:l:j:h", long_options, &option_index);

    // Detect the end of the options
    if (c == -1)
      break;

    switch (c)
      {
      case 0:
        // If this option set a flag, do nothing else now.
        if (long_options[option_index].flag != 0)
          break;
        fprintf(stderr, "option %s", long_options[option_index].name);
        if (optarg)
          fprintf(stderr, " with arg %s", optarg);
        fprintf (stderr, "\n");
        break;


      case 'h':
        print_help_message(argv[0]);
        exit(0);

      case 'm':
        fprintf (stderr, "model filename: %s\n", optarg);
        model = optarg;
        break;

      case 'l':
        fprintf (stderr, "max number of candidates: %s\n", optarg);
        limit = atoi(optarg);
        break;

      case 'j':
        fprintf (stderr, "number of threads: %s\n", optarg);
        threads = atoi(optarg);
        break;

      case '?':
        // getopt_long already printed an error message.
        break;

      default:
        abort ();
      }

  }

  if(model == NULL || threads <= 0 || limit < 0) {
    print_help_message(argv[0]);
    return 1;
  }






  ranker::Predictor predictor(threads, model);

  char* buffer = NULL;
  size_t buffer_length = 0;
  ssize_t length = 0;

  std::vector<ranker::Example> examples;
  while(0 <= (length = read_line(&buffer, &buffer_length, stdin))) {
    if(*buffer == '\n') {
      fprintf(stdout, "%d\n", predictor.predict(examples));
      examples.clear();
    } else if(limit == 0 || (int) examples.size() < limit) {
      examples.emplace_back(buffer);
    }
  }
  return 0;
}
