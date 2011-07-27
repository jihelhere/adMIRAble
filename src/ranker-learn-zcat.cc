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
#define NUM_THREADS 1

using namespace ranker;

static int verbose_flag = 0;

void  print_help_message(char *program_name)
{
  fprintf(stderr, "%s usage: %s [options]\n", program_name, program_name);
  fprintf(stderr, "OPTIONS :\n");
  fprintf(stderr, "      --train,-s   <file>           : training set file\n");
  fprintf(stderr, "      --dev,-d     <file>           : dev set file\n");
  fprintf(stderr, "      --test,-t    <file>           : test set file\n");
  fprintf(stderr, "      --clip,-c    <double>         : clip value (default is %f)\n", CLIP);
  fprintf(stderr, "      --iter,-i    <int>            : nb of iterations (default is %d)\n", LOOP);
  fprintf(stderr, "      --threads,-j <int>            : nb of threads (default is %d)\n", NUM_THREADS);
  fprintf(stderr, "      --mode,-m    <train|predict>  : running mode\n");
    
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


double process(char* filename, std::vector<double> &weights, bool alter_model, int num_threads, MiraOperator& mira) 
{
  int num = 0;
  int errors = 0;
  double avg_loss = 0;
  double one_best_loss = 0;
  
  FILE* fp = openpipe(filename);
  if(!fp) {
    fprintf(stderr, "ERROR: cannot open \"%s\"\n", filename);
    return 1;
  }

  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);

  std::vector<char*> lines;

  //read examples
  while(read_line(&buffer, &buffer_size, fp))  {

    // store example lines
    if(buffer[0] != '\n') {
      lines.push_back(strdup(buffer));
    }
    

    // empty line -> end of examples for this instance
    else {
      std::vector<ExampleMaker*> exampleMakers(num_threads, NULL);
      
      for(int i = 0; i < num_threads; ++i) {
      	exampleMakers[i] = new ExampleMaker(lines, weights);
      	exampleMakers[i]->start(i*lines.size()/num_threads, (i+1)*lines.size()/num_threads);
      }

      
      // fprintf(stderr, "converting exampleMakers to examples\n");
      for(auto i = exampleMakers.begin(); i != exampleMakers.end(); ++i) {
	(*i)->join();
      }
    
      std::vector<Example*> examples;
    
      //fprintf(stderr, "creating vectors of examples\n");
      for(auto maker = exampleMakers.begin(); maker != exampleMakers.end(); ++maker) {
	examples.insert(examples.end(), (*maker)->examples.begin(), (*maker)->examples.end());
      }
      
      // fprintf(stderr, "%d %d %d\n", exampleMakers.size(), examples.size(), lines.size());
      assert(examples.size() == lines.size());
    
 
 
      // compute scores and metrics
    
      one_best_loss += examples[0]->loss;

      Example* oracle = examples[0];
      for(unsigned i = 0; i < examples.size(); ++i) {
	if(examples[i]->loss < oracle->loss) 
	  oracle = examples[i];
      }
    
      //count the number of errors
      //fprintf(stdout, "num examples = %d\n", examples.size());
    
      // sort the examples by score
      sort(examples.begin(), examples.end(), Example::example_ptr_desc_score_order());
      avg_loss += examples[0]->loss;
    
      for(unsigned int i = 0; i < examples.size(); ++i) {
	if(examples[i]->score > oracle->score || (examples[i]->score == oracle->score && examples[i]->loss > oracle->loss)) {
	  ++errors;
	  break;
	}
      }
      
      ++num;
      if(num % 10 == 0) fprintf(stderr, "\r%d %d %f %f/%f", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);

      // training -> update
      if(alter_model) {
	sort(oracle->features.begin(), oracle->features.end());
      
	mira.update(oracle, num);

	// std::for_each(examples.begin(),examples.end(), mira);
	std::for_each(examples.begin(),examples.begin()+1, mira);
      }
    
      // reset data structures for next sentence
    
      for(unsigned i = 0; i < exampleMakers.size(); ++i) {
	delete exampleMakers[i];
      }
      for(unsigned i = 0; i < lines.size(); ++i) {
	free(lines[i]);
      }
      lines.clear();
      exampleMakers.clear();
    }
  }
  
  fprintf(stderr, "\r%d %d %f %f/%f\n", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);
  
  fclose(fp);
  int status;
  wait(&status);
  free(buffer);

  return avg_loss/num;
}

int main(int argc, char** argv) {

  char * trainset = NULL;
  char * devset = NULL;
  char * testset = NULL;
  double clip = CLIP;
  int loop = LOOP;
  int num_threads = NUM_THREADS;

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
    std::vector<double> saveweights;

    double dev_loss = -1;


    fprintf(stderr, "examples: %d\n", num_examples);
    for(unsigned iteration = 0; iteration < unsigned(loop); ++iteration) {
      fprintf(stderr, "iteration %d\n", iteration);

      MiraOperator mira(loop, iteration, num_examples, clip, weights, avgWeights);
      (void) process(trainset, weights, true, num_threads, mira);
      // averaging for next iteration
      for(unsigned int i = 0; i < avgWeights.size(); ++i) {
	if(avgWeights[i] != 0.0)
	  weights[i] = avgWeights[i] / (num_examples * loop);
      }

      if(devset) {
        double d = process(devset, weights, false, num_threads, mira);
	if(dev_loss < 0 || d < dev_loss) {
	  dev_loss = d;
	  saveweights = weights;
	}
      }

      if(testset) 
        (void) process(testset, weights, false, num_threads, mira);
    }


    if(dev_loss < 0) { saveweights = weights;}
    
    for(unsigned int i = 0; i < saveweights.size(); ++i) {
      if(saveweights[i] != 0) {
        fprintf(stdout, "%d 0 %32.31g\n", i, saveweights[i] );
      }
    }
    return 0;
}
