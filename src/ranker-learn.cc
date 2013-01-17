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
#include <fcntl.h>

#include "utils.h"
#include "Example.hh"
#include "ExampleMaker.hh"
#include "MiraOperator.hh"

#define CLIP 0.05
#define LOOP 10
#define NUM_THREADS 1

static int verbose_flag = 0;


int processed_lines = 0;
int finished = 0;
threadns::mutex mutex_processed_lines;
threadns::mutex mutex_examples;
threadns::condition_variable cond_process;


void  print_help_message(char *program_name)
{
    fprintf(stderr, "%s usage: %s [options]\n", program_name, program_name);
    fprintf(stderr, "OPTIONS :\n");
    fprintf(stderr, "      --train,-s    <file>           : training set file\n");
    fprintf(stderr, "      --dev,-d      <file>           : dev set file\n");
    fprintf(stderr, "      --test,-t     <file>           : test set file\n");
    fprintf(stderr, "      --clip,-c     <double>         : clip value (default is %f)\n", CLIP);
    fprintf(stderr, "      --iter,-i     <int>            : nb of iterations (default is %d)\n", LOOP);
    fprintf(stderr, "      --threads,-j  <int>            : nb of threads (default is %d)\n", NUM_THREADS);
    fprintf(stderr, "      --filter,-f   <command>        : filter input through command (\"%%s\" is replaced by the filename)\n");
    fprintf(stderr, "      --examples,-e <int>            : set number of examples (if known in advance)\n");

    fprintf(stderr, "      -help,-h                       : print this message\n");
}

/* Open a file through an optional filter. If filter contains %s, filename is
 * used in place, otherwise it is piped through stdin.
 */

FILE* openpipe(const char* filename, const char* filter) {
    int fd[2];
    pid_t childpid;
    if(filter == NULL) {
        return fopen(filename, "r");
    }
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
        char* command = NULL;
        if(NULL != strstr(filter, "%s")) {
	  asprintf(&command, filter, filename);
        } else {
            int input = open(filename, O_RDONLY);
            if(input < 0) {
                perror("openpipe/open");
                exit(1);
            }
            dup2(input, STDIN_FILENO);
            command = strdup(filter);
        }
        execlp("sh", "sh", "-c", command, (char*) NULL);
        perror("openpipe/execl");
        free(command);
        exit(1);
    }
    close(fd[1]);
    FILE* output = fdopen(fd[0], "r");
    return output;
}

int compute_num_examples(const char* filename, const char* filter)
{
    FILE* fp = openpipe(filename, filter);
    if(!fp) return 0;

    int num = 0;

    size_t buffer_size = 0;
    char* buffer = NULL;
    while(0 < read_line(&buffer,&buffer_size,fp)) {

        //if line is empty -> we've read one instance
        if(buffer[0] == '\n') {
            ++num;
            if(num % 100 ==0) fprintf(stderr, "%d\r", num);
        }
        if(feof(fp)) break;
    }
    if(filter != NULL) {
        int status;
        wait(&status);
    }
    free(buffer);
    fclose(fp);

    return num;
}



struct file_reader
{
  char * buffer;
  size_t buffer_size;

  file_reader() : buffer(NULL), buffer_size(0) {}

  ~file_reader() {free(buffer);}

  void
  process_file(FILE** fp, std::vector<char*>* lines)
  {
    while(0 < read_line(&buffer, &buffer_size, *fp))  {
      if(buffer[0] != '\n') {

        mutex_processed_lines.lock();
	lines->push_back(strdup(buffer));
        cond_process.notify_all();
        //        fprintf(stderr, "after notify, size lines: %ld\n", lines->size());
        mutex_processed_lines.unlock();
      }
      else break;

    }
    finished = 1;
    cond_process.notify_all();
  }
};


struct oracle_checker
{
  const ranker::Example * oracle;

  oracle_checker(const ranker::Example* o) : oracle(o) {};

  inline bool operator()(const ranker::Example* e)
  {
    //    fprintf(stderr, "e.score %g oracle.score %g\n", e->score, oracle->score);
    //    fprintf(stderr, "e.loss %g oracle.loss %g\n", e->score, oracle->score);
    return ( (e->score > oracle->score) || (e->score == oracle->score && e->loss > oracle->loss));
  }

};




double process(const char* filename, const char* filter, std::vector<double> &weights, bool alter_model, int num_threads, ranker::MiraOperator& mira)
{
  int num = 0;
  int errors = 0;
  double avg_loss = 0;
  double one_best_loss = 0;

  FILE* fp = openpipe(filename, filter);
  if(!fp) {
    fprintf(stderr, "ERROR: cannot open \"%s\"\n", filename);
    return -1;
  }

  std::vector<char*> lines;
  std::vector<ranker::Example*> examples;
  std::vector<ranker::ExampleMaker*> exampleMakers(num_threads, NULL);

  for(int i = 0; i < num_threads; ++i) {
    exampleMakers[i] = new ranker::ExampleMaker(lines, weights, examples);
  }

  file_reader fr;

  while(!feof(fp)) {

    processed_lines = 0;
    finished = 0;

    threadns::thread thread_read(&file_reader::process_file, &fr, &fp, &lines);

    for(int i = 0; i < num_threads; ++i) {
      exampleMakers[i]->start(&mutex_processed_lines, &processed_lines, &finished, &cond_process, &mutex_examples);
    }

    thread_read.join();

    for(auto i = exampleMakers.begin(); i != exampleMakers.end(); ++i)
      (*i)->join();


    if(examples.empty())
      continue;

    one_best_loss += examples[0]->loss;

    ranker::Example* oracle = examples[0];
    for(unsigned i = 1; i < examples.size(); ++i) {
      if(examples[i]->loss < oracle->loss)
	oracle = examples[i];
    }

    // sort the examples by score
    auto pos = std::min_element(examples.begin(), examples.end(), ranker::Example::example_ptr_desc_score_order());
    avg_loss += (*pos)->loss ;

    if (std::find_if(examples.begin(), examples.end(), oracle_checker(oracle)) != examples.end())
      ++errors;

    ++num;
    if(num % 10 == 0)
      fprintf(stderr, "\r%d %d %f %f/%f", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);

    // training -> update
    if(alter_model) {
      mira.update(oracle, num);

      mira(*pos);
    }

    // reset data structures for next sentence
    for(unsigned i = 0; i < examples.size(); ++i) {
      delete examples[i];
    }

    lines.clear();
    examples.clear();
  }

  fprintf(stderr, "\r%d %d %f %f/%f\n", num, errors, (double)errors/num, avg_loss / num, one_best_loss / num);

  fclose(fp);
  if(filter != NULL) {
    int status;
    wait(&status);
  }

  return avg_loss/num;
}

int main(int argc, char** argv) {

    char * trainset = NULL;
    char * devset = NULL;
    char * testset = NULL;
    double clip = CLIP;
    int loop = LOOP;
    int num_threads = NUM_THREADS;
    char* filter = NULL;
    int num_examples = -1;


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
            {"threads",     required_argument,       0, 'j'},
            {"examples",    required_argument,       0, 'e'},
            {0, 0, 0, 0}
        };
        // int to store arg position
        int option_index = 0;

        c = getopt_long (argc, argv, "s:d:t:c:i:j:e:f:hv", long_options, &option_index);

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

            case 'f':
                fprintf(stderr, "filter: %s\n", optarg);
                filter = optarg;
                break;

	case 'e':
	  fprintf (stderr, "number of examples is assumed to be: %s\n", optarg);
	  num_examples = atoi(optarg);
	  break;

            case '?':
                // getopt_long already printed an error message.
                break;

            default:
                abort ();
        }

    }


    if( trainset == NULL) {
        fprintf(stderr, "training and no trainset ? Aborting\n");
        abort();
    }


    if(num_examples <= 0)
      num_examples = compute_num_examples(trainset, filter);

    std::vector<double> weights;
    std::vector<double> avgWeights;
    std::vector<double> saveweights;
    std::vector<double> predWeights;
    double dev_loss = -1;


    fprintf(stderr, "examples: %d\n", num_examples);
    for(unsigned iteration = 0; iteration < unsigned(loop); ++iteration) {
        fprintf(stderr, "iteration %d\n", iteration);

        ranker::MiraOperator mira(loop, iteration, num_examples, clip, weights, avgWeights);
        (void) process(trainset, filter, weights, true, num_threads, mira);
        // averaging for next iteration

        predWeights.resize(avgWeights.size());
        for(unsigned int i = 0; i < avgWeights.size(); ++i) {
            if(avgWeights[i] != 0.0)
                predWeights[i] = avgWeights[i] / (num_examples * loop);
        }

        if(devset) {
            double d = process(devset, filter, predWeights, false, num_threads, mira);
            if(dev_loss < 0 || d < dev_loss) {
                dev_loss = d;
                saveweights = predWeights;
            }
        }

        if(testset)
            (void) process(testset, filter, predWeights, false, num_threads, mira);
    }


    if(dev_loss < 0) { saveweights = predWeights;}

    for(unsigned int i = 0; i < saveweights.size(); ++i) {
        if(saveweights[i] != 0) {
            fprintf(stdout, "%d %32.31g\n", i, saveweights[i] );
        }
    }
    return 0;
}
