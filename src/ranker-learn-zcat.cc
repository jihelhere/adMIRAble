#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

#include <cassert>

#include <getopt.h>

#include <thread>

#include "utils.h"
#include "Example.hh"
#include "ExampleMaker.hh"
#include "MiraOperator.hh"

#define CLIP 0.05
#define LOOP 10

static int verbose_flag = 0;

void  print_help_message(char *program_name)
{
  fprintf(stderr, "%s usage: %s [options]\n", program_name, program_name);
  fprintf(stderr, "OPTIONS :\n");
  fprintf(stderr, "      --train,-s <file>           : training set file\n");
  fprintf(stderr, "      --dev,-d   <file>           : dev set file\n");
  fprintf(stderr, "      --test,-t  <file>           : test set file\n");
  fprintf(stderr, "      --clip,-c  <double>         : clip value (default is %f)\n", CLIP);
  fprintf(stderr, "      --iter,-i  <int>            : nb of iterations (default is %d)\n", LOOP);
  fprintf(stderr, "      --mode,-m  <train|predict>  : running mode\n");
    
  fprintf(stderr, "      -help,-h                    : print this message\n");
}

FILE* openpipe(const char* filename) {
  int fd[2];
  pid_t childpid;
  if(pipe(fd) == -1) {
    perror("openpipe/pipe");
    exit(1);
  }
  if((childpid = fork()) == -1) {
    perror("openpipe/fork");
    exit(1);
  }
  if(childpid == 0) {
    close(fd[0]);
    dup2(fd[1], STDOUT_FILENO);
    //	execlp("zcat", "zcat", "-c", filename, (char*) NULL);
    execlp("pigz", "pigz", "-f", "-d", "-c", filename, (char*) NULL);
    perror("openpipe/execl");
    exit(1);
  }
  close(fd[1]);
  FILE* output = fdopen(fd[0], "r");
  return output;
}

int compute_num_examples(const char* filename)
{
  FILE* fp = openpipe(filename);
  if(!fp) return 0;
  
  int num = 0;
  
  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);
  while(read_line(&buffer,&buffer_size,fp)) {
    
    //if line is empty -> we've read all the examples
    if(buffer[0] == '\n') {
      ++num;
      if(num % 100 ==0) fprintf(stderr, "%d\r", num);
    }
    if(feof(fp)) break;
  }
  int status;
  wait(&status);
  free(buffer);
  fclose(fp);
    
  return num;
}


int process(char* filename, int loop, std::vector<double> &weights, std::vector<double> &avgWeights, int iteration, 
	    int num_examples, double clip, bool alter_model, int num_threads) 
{
  int num = 0;
  int errors = 0;
  double avg_loss = 0;
  double one_best_loss = 0;
  
  std::vector<char*> lines;

  FILE* fp = openpipe(filename);
  if(!fp) {
    fprintf(stderr, "ERROR: cannot open \"%s\"\n", filename);
    return 1;
  }

  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);
  while(read_line(&buffer, &buffer_size, fp)) {
    
    //if line is empty -> we've read all the examples
    if(buffer[0] == '\n') {
      std::vector<example_maker*> examplemakers;
      int num_per_thread = lines.size() / num_threads;
      int start = 0;
      for(int i = 0; i < num_threads; i++) {
          example_maker* maker = new example_maker(lines, weights);
          examplemakers.push_back(maker);
          int end = start + num_per_thread;
          // give one remaining examples to each thread
          if(i < lines.size() % num_threads) end ++;
          maker->start(start, end);
          start = end;
      }

      //      fprintf(stderr, "converting examplemakers to examples\n");
      for(auto i = examplemakers.begin(); i != examplemakers.end(); ++i) {
        (*i)->join();
      }

      std::vector<example*> examples;
      example* oracle = NULL;

      //      fprintf(stderr, "creating vectors of examples\n");
      //      fprintf(stderr, "retrieveing oracle \n");
      for(auto maker = examplemakers.begin(); maker != examplemakers.end(); maker++) {
          for(auto current = (*maker)->examples.begin(); current != (*maker)->examples.end(); current++) {
              examples.push_back(*current);
          }
      }
      assert(examples.size() == lines.size());

      for(unsigned i = 0; i < examples.size(); ++i) {
          if(oracle == NULL || examples[i]->loss < oracle->loss) 
              oracle = examples[i];
      }

      //count the number of errors
      //fprintf(stdout, "num examples = %d\n", examples.size());

      // sort the examples by score
      one_best_loss += examples[0]->loss;
      sort(examples.begin(), examples.end(), example::example_ptr_desc_order());
      avg_loss += examples[0]->loss;

      for(unsigned int i = 0; i < examples.size(); ++i) {
          if(examples[i]->score > oracle->score || (examples[i]->score == oracle->score && examples[i]->loss > oracle->loss)) {
              ++errors;
              break;
          }
      }

      // training -> update
      if(alter_model) {
          // std::for_each(examples.begin(),examples.end(), mira_operator(loop, num_examples, iteration, num, clip, 
          //                                                              weights, avgWeights, oracle));
          sort(oracle->features.begin(), oracle->features.end());
          std::for_each(examples.begin(),examples.begin()+1, mira_operator(loop, num_examples, iteration, num, clip,
                      weights, avgWeights, oracle));
      }

      ++num;
      if(num % 10 == 0) fprintf(stderr, "\r%d %d %f %f/%f", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);
            
      // reset data structures for next sentence
      oracle = NULL;

      for(unsigned i = 0; i < examplemakers.size(); ++i) {
        delete examplemakers[i];
      }
      for(unsigned i = 0; i < lines.size(); ++i) {
          free(lines[i]);
        }
      lines.clear();

    }
    
    //read examples
    else{
      char * copy = strdup(buffer);
      lines.push_back(copy);
    }
  }
  
  fprintf(stderr, "\r%d %d %f %f/%f\n", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);
  
  if(alter_model)
    // averaging for next iteration
    for(unsigned int i = 0; i < avgWeights.size(); ++i) {
      if(avgWeights[i] != 0.0)
	weights[i] = avgWeights[i] / (num_examples * loop);
    }
  
  fclose(fp);
  int status;
  wait(&status);
  free(buffer);
  return 0;
}

int main(int argc, char** argv) {

  char * trainset = NULL;
  char * devset = NULL;
  char * testset = NULL;
  double clip = CLIP;
  int loop = LOOP;
  int num_threads = 1;

  char mode_def[] = "train";
  char * mode = mode_def;


  // read the commandline
  int c;
  
  while(1) {
    
    static struct option long_options[] =
      {
	/* These options set a flag. */
	{"verbose", no_argument,       &verbose_flag, 1},
	/* These options don't set a flag.
	   We distinguish them by their indices. */
	{"help",        no_argument,             0, 'h'},
	{"train",       required_argument,       0, 's'},
	{"dev",         required_argument,       0, 'd'},
	{"test",        required_argument,       0, 't'},
	{"clip",        required_argument,       0, 'c'},
	{"iterations",  required_argument,       0, 'i'},
	{"threads",  required_argument,       0, 'j'},
        //	{"mode",        required_argument,       0, 'm'},
	{0, 0, 0, 0}
      };
    // int to store arg position
    int option_index = 0;
    
    c = getopt_long (argc, argv, "s:d:t:c:i:j:m:hv", long_options, &option_index);

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

      case 's':
	if(trainset) fprintf (stderr, "redefining ");
	fprintf (stderr, "training set filename: %s\n", optarg);
	trainset = optarg;
	break;
     
      case 'd':
	if(devset) fprintf (stderr, "redefining ");
	fprintf (stderr, "dev set filename: %s\n", optarg);
	devset = optarg;
	break;

      case 't':
	if(testset) fprintf (stderr, "redefining ");
	fprintf (stderr, "test set filename: %s\n", optarg);
	testset = optarg;
	break;


      case 'c':
	fprintf (stderr, "clip value: %s\n", optarg);
	clip = strtod(optarg, NULL);
	break;

      case 'i':
	fprintf (stderr, "number of iterations: %s\n", optarg);
	loop = atoi(optarg);
	break;

      case 'j':
	fprintf (stderr, "number of threads: %s\n", optarg);
	num_threads = atoi(optarg);
	break;

      // case 'm':
      //   fprintf (stderr, "mode is: %s (but I don't care ;)\n", optarg);
      //   mode = optarg;
      //   break;

      case '?':
	// getopt_long already printed an error message.
	break;
     
      default:
	abort ();
      }

  }


  if( trainset == NULL /* && !strcmp(mode, "train") */) {
    fprintf(stderr, "training mode and no trainset ? Aborting\n");
    abort();
  } 
 

    int num_examples = compute_num_examples(trainset);

    std::vector<double> weights;
    std::vector<double> avgWeights;


    fprintf(stderr, "examples: %d\n", num_examples);
    for(unsigned iteration = 0; iteration < unsigned(loop); ++iteration) {
      fprintf(stderr, "iteration %d\n", iteration);
      process(trainset, loop, weights, avgWeights, iteration, num_examples, clip, true, num_threads);
      if(devset)
        process(devset, loop, weights, avgWeights, iteration, num_examples, clip, false, num_threads);
      if(trainset) 
        process(testset, loop, weights, avgWeights, iteration, num_examples, clip, false, num_threads);
    }

    for(unsigned int i = 0; i < weights.size(); i++) {
      if(weights[i] != 0) {
        fprintf(stdout, "%d 0 %32.31g\n", i, weights[i] );
      }
    }
    return 0;
}
