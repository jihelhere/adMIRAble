#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

#include <cassert>

#include <getopt.h>

#define CLIP 0.2
#define LOOP 10

//TODO: remove
using namespace std;

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
	execlp("zcat", "zcat", "-c", filename, (char*) NULL);
	//execlp("pigz", "pigz", "-f", "-d", "-c", filename, (char*) NULL);
        perror("openpipe/execl");
        exit(1);
    }
    close(fd[1]);
    FILE* output = fdopen(fd[0], "r");
    return output;
}

static int verbose_flag = 0;


struct Example {
  double loss;
  double score;
  int label;
  unordered_map<int, double> features;

  Example() : loss(0.0), score(0.0), label(0), features() {};
  Example(double lo, double sc, int la) : loss(lo), score(sc), label(la), features() {};



  // for sorting examples
  struct example_ptr_desc_order 
  {
    bool operator()(const Example* i, const Example* j) {return (i->score > j->score);}
  };

};

int compute_num_examples(char* filename) {
  int num = 0;
  FILE* fp = openpipe(filename);
  if(!fp) return 0;

  char previous = '\0';
  while(previous != EOF) {
    char current = getc(fp);
    if(previous == '\n' && current == '\n') {
      ++num;
      fprintf(stderr, "%d\r", num);
      }
    previous = current;
    if(feof(fp)) break;
  }
  fclose(fp);
  int status;
  wait(&status);
  return num;
}

void read_example_line(char*& buffer, int& buffer_size, FILE*& fp)
{
  while(buffer[strlen(buffer) - 1] != '\n') {
    buffer_size *= 2;
    buffer = (char*) realloc(buffer, buffer_size);
    if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), fp) == NULL) break;
  }
}



struct mira_operator
{
  double& alpha;
  double avgUpdate;
  std::vector<double> &weights;
  std::vector<double> &avgWeights;
  Example* oracle;

  mira_operator(double& alpha_, double avgUpdate_, std::vector<double> &weights_, std::vector<double> &avgWeights_, Example* oracle_)
    : alpha(alpha_), avgUpdate(avgUpdate_), weights(weights_), avgWeights(avgWeights_), oracle(oracle_) {};

  inline
  bool incorrect_rank(const Example * example) 
  {
    return example->score > oracle->score || 
      (example->score == oracle->score && example->loss > oracle->loss);
  }


  void operator()(Example * example)
  {
    //fprintf(stdout, "%g %g\n", example->score, example->loss);
    
    // skip the oracle -> useless update
    if(example == oracle) return;
    
    
    if(incorrect_rank(example)) {

      double delta = example->loss - oracle->loss - (oracle->score - example->score);
    
      //copy
      unordered_map<int, double> difference = oracle->features;
      double norm = 0;
      
      unordered_map<int, double>::iterator end = example->features.end();
      
      for(unordered_map<int, double>::iterator j = example->features.begin(); j != end; ++j) {
	double&ref = difference[j->first];
	ref -= j->second;
	norm += ref*ref;
      }

      delta /= norm;
      alpha += delta;
      if(alpha < 0) alpha = 0;
      if(alpha > CLIP) alpha = CLIP;
      double avgalpha = alpha * avgUpdate;

      end = difference.end();
      for(unordered_map<int, double>::iterator j = difference.begin(); j != end; ++j) {
	weights[j->first] += alpha * j->second;
	avgWeights[j->first] += avgalpha * j->second;
      }
    }
    
  }
 
};



int process(char* filename, int loop, vector<double> &weights, vector<double> &avgWeights, unordered_map<string, int> &features, int &next_id, int iteration, int num_examples, bool alter_model) 
{
  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);

  FILE* fp = openpipe(filename);

  if(!fp) {
    fprintf(stderr, "ERROR: cannot open \"%s\"\n", filename);
    return 1;
  }

  int num = 0;
  int errors = 0;
  double avg_loss = 0;
  double one_best_loss = 0;
  bool is_one_best = true;
  
  vector<Example*> examples;
  Example* oracle = NULL;
  Example* official_oracle = NULL;
  
  while(NULL != fgets(buffer, buffer_size, fp)) {
    
    //get a line
    read_example_line(buffer,buffer_size,fp);
    
    //if line is empty -> we've read all the examples
    if(buffer[0] == '\n') {
      
      is_one_best = true;

      // First count the number of errors
      //fprintf(stdout, "num examples = %d\n", examples.size());

      // sort the examples by score
      sort(examples.begin(), examples.end(), Example::example_ptr_desc_order());
      avg_loss += examples[0]->loss;

      for(unsigned int i = 0; i < examples.size(); ++i) {
	if(examples[i]->score > oracle->score || (examples[i]->score == oracle->score && examples[i]->loss > oracle->loss)) {
	  ++errors;
	  break;
	}
      }
      
      // training -> update
      if(alter_model) {

	if(official_oracle == NULL)
	  fprintf(stderr,"sentence %d doesn't have an oracle. Using the computed one.\n", num);
	
	if(official_oracle != oracle) {
	  fprintf(stderr,"sentence %d has a strange oracle. Using the official one.\n", num);
	  oracle = official_oracle;
	}


	double alpha = 0;
	double avgUpdate = (double)(loop * num_examples - (num_examples * ((iteration + 1) - 1) + (num + 1)) + 1);


	// std::for_each(examples.begin(),examples.end(), mira_operator(alpha, avgUpdate, weights, avgWeights, oracle));
	std::for_each(examples.begin(),examples.begin()+1, mira_operator(alpha, avgUpdate, weights, avgWeights, oracle));
      }

      ++num;
      if(num % 10 == 0) fprintf(stderr, "\r%d %d %f %f/%f", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);

      official_oracle = NULL;
      oracle = NULL;

      //fprintf(stdout, "\n");
      //if(num > 1000) break;

      for(unsigned i = 0; i < examples.size(); ++i)
	delete examples[i];

      examples.clear();
    }

    
    //read examples
    else{
      char* token;
      bool first = true;
      
      Example* example = new Example();
      examples.push_back(example);
      

      string token_as_string;
      
      // read a line and fill label/features
      for(token = strtok(buffer, " \t"); token != NULL; token = strtok(NULL, " \t\n")) {
	if(first) {
	  if(!strcmp(token, "1")) {
	    example->label = 1;
	    official_oracle = example;
	  }
	  first = false;
	} 
	else {
	  char* value = strrchr(token, ':');
	  if(value != NULL) {
	    *value = '\0';
	    if(!strcmp(token, "las")) {
	      // skip las!!!
	    } 
	    if(!strcmp(token, "nbe")) {
	      example->loss = strtod(value + 1, NULL);
	      if(is_one_best) one_best_loss += example->loss;
	    } 
	    else {
	      token_as_string = token;
	      unordered_map<string, int>::iterator id = features.find(token_as_string);
	      unsigned int location = 0;
	      if(id == features.end()) {
		if(!alter_model) continue; // skip unseen features
		location = next_id;
		features[token_as_string] = next_id;
		++next_id;
	      } else {
		location = id->second;
	      }
	      
	      if(location+1 > weights.size()) {
		weights.resize(location+1,0.0);
		avgWeights.resize(location+1,0.0);
	      }
	      
	      double value_as_double = strtod(value + 1, NULL);
	      if(!isinf(value_as_double) && !isnan(value_as_double)) {
		example->features[location] = value_as_double;
		//fprintf(stdout, "%s %d %g\n", token, location, value_as_double);
		//if(iteration == 1) fprintf(stdout, "%s %g %g %g\n", token, vector_last(values), weights[location], weights[location + 1]);
		example->score += value_as_double * weights[location];
	      }
	    }
	  }
	}
      }

      // see label = 1
      if(oracle == NULL || example->loss < oracle->loss) 
       	oracle = example;
      
      is_one_best = false;
      //fprintf(stdout, "%d %g %g\n", label, score[0], score[1]);
      //if(iteration == 1) fprintf(stdout, "%d %g %g\n", label, score[0], score[1]);
    }
  }

  fprintf(stderr, "\r%d %d %f %f/%f\n", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);
  
  for(unsigned int i = 0; i < weights.size(); ++i) {
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
	{"mode",        required_argument,       0, 'm'},
	{0, 0, 0, 0}
      };
    // int to store arg position
    int option_index = 0;
    
    c = getopt_long (argc, argv, "s:d:t:c:i:m:h", long_options, &option_index);

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

      case 'm':
	fprintf (stderr, "mode is: %s (but I don't care ;)\n", optarg);
	mode = optarg;
	break;

      case '?':
	// getopt_long already printed an error message.
	break;
     
      default:
	abort ();
      }

  }


  if(trainset == NULL && !strcmp(mode, "train")) {
    fprintf(stderr, "training mode and no trainset ? Aborting\n");
    abort();
  }  

  
    vector<double> weights;
    vector<double> avgWeights;
    unordered_map<string, int> features;
    int next_id = 0;

    int num_examples = compute_num_examples(trainset);

    fprintf(stderr, "examples: %d\n", num_examples);
    for(unsigned iteration = 0; iteration < unsigned(loop); ++iteration) {
        fprintf(stderr, "iteration %d\n", iteration);
        process(trainset, loop, weights, avgWeights, features, next_id, iteration, num_examples, true);
        if(devset) process(devset, loop, weights, avgWeights, features, next_id, iteration, num_examples, false);
        if(trainset) process(testset, loop, weights, avgWeights, features, next_id, iteration, num_examples, false);
    }

    unordered_map<string, int>::iterator end = features.end();
    for( unordered_map<string, int>::iterator i = features.begin(); i != end; ++i) {
      if(weights[i->second] != 0) {
	fprintf(stdout, "%s 0 %32.31g\n", i->first.c_str(), weights[i->second]);
      }
    }
    return 0;
}

