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
#include "utils.h"

#define CLIP 0.2
#define LOOP 10

//TODO: remove
using namespace std;

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
    FILE* fp = open_pipe(filename, "r");
    if(!fp) return 0;

    char previous = '\0';
    while(previous != EOF) {
        char current = getc(fp);
        if(feof(fp)) break;
        if(previous == '\n' && current == '\n')
            ++num;
        previous = current;
    }
    close_pipe(fp);
    return num;
}

struct mira_operator
{
    double& alpha;
    double avgUpdate;
    std::vector<double> &weights;
    std::vector<double> &avgWeights;
    Example* oracle;
    double clip;


  mira_operator(double& alpha_, double avgUpdate_, std::vector<double> &weights_, std::vector<double> &avgWeights_, Example* oracle_, double clip_)
    : alpha(alpha_), avgUpdate(avgUpdate_), weights(weights_), avgWeights(avgWeights_), oracle(oracle_), clip(clip_){};

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
            if(alpha > clip) alpha = clip;
            double avgalpha = alpha * avgUpdate;

            end = difference.end();
            for(unordered_map<int, double>::iterator j = difference.begin(); j != end; ++j) {
                weights[j->first] += alpha * j->second;
                avgWeights[j->first] += avgalpha * j->second;
            }
        }

    }

};



int process(char* filename, int num_iterations, vector<double> &weights, vector<double> &avgWeights, unordered_map<string, int> &features, int &next_id, int iteration, int num_examples, bool alter_model, double clip)
{
    size_t buffer_size = 0;
    char* buffer = NULL;

    FILE* fp = open_pipe(filename, "r");

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

    while(0 < read_line(&buffer, &buffer_size,fp)) {

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
                double avgUpdate = (double)(num_iterations * num_examples - (num_examples * ((iteration + 1) - 1) + (num + 1)) + 1);


                // std::for_each(examples.begin(),examples.end(), mira_operator(alpha, avgUpdate, weights, avgWeights, oracle));
                std::for_each(examples.begin(),examples.begin()+1, mira_operator(alpha, avgUpdate, weights, avgWeights, oracle, clip));
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
                            if(!std::isinf(value_as_double) && !std::isnan(value_as_double)) {
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
        weights[i] = avgWeights[i] / (num_examples * num_iterations);
    }
    fclose(fp);
    int status;
    wait(&status);
    free(buffer);
    return 0;
}

void  print_help_message(char *program_name)
{
    fprintf(stderr, "%s usage: %s [options]\n", program_name, program_name);
    fprintf(stderr, "OPTIONS :\n");
    fprintf(stderr, "      --training-data,-t <file>   : training set file\n");
    fprintf(stderr, "      --clip,-c  <double>         : clip value (default is %g)\n", CLIP);
    fprintf(stderr, "      --iteration,-i  <int>       : current iteration (default is %d)\n", 0);
    fprintf(stderr, "      --num-iterations,-n  <int>  : total number of iterations (default is %d)\n", LOOP);
    fprintf(stderr, "      --num-examples,-e  <int>    : number of examples (computed if not specified)\n");
    fprintf(stderr, "      --model-in,-m               : load optional model\n");

    fprintf(stderr, "      --help,-h                   : print this message\n");
    fprintf(stderr, "      --verbose,-v                : print extra messages\n");
}

int main(int argc, char** argv) {

    char * trainset = NULL;
    char* model = NULL;
    double clip = CLIP;
    int num_iterations = LOOP;
    int num_examples = -1;
    int iteration = 0;

    // read the commandline
    int c;

    // USAGE: current-iteration num clip data input-model
    while(1) {

        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"help",        no_argument,             0, 'h'},
            {"iteration",   required_argument,       0, 'i'},
            {"num-iterations",  required_argument,       0, 'n'},
            {"num-examples",    required_argument,       0, 'e'},
            {"clip",        required_argument,       0, 'c'},
            {"training-data",   required_argument,       0, 't'},
            {"model-in",    required_argument,       0, 'm'},
            {0, 0, 0, 0}
        };
        // int to store arg position
        int option_index = 0;

        c = getopt_long (argc, argv, "vhi:n:e:c:m:t:", long_options, &option_index);

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

            case 't':
                if(trainset) fprintf (stderr, "redefining ");
                fprintf (stderr, "training set filename: %s\n", optarg);
                trainset = optarg;
                break;

            case 'i':
                iteration = atoi(optarg);
                break;

            case 'n':
                num_iterations = atoi(optarg);
                break;

            case 'c':
                fprintf (stderr, "clip value: %s\n", optarg);
                clip = strtod(optarg, NULL);
                break;

            case 'm':
                model = optarg;
                break;

            case 'e':
                num_examples = atoi(optarg);
                break;

            case '?':
                // getopt_long already printed an error message.
                break;

            default:
                abort ();
        }

    }
    if(trainset == NULL) {
        print_help_message(argv[0]);
        exit(1);
    }


    vector<double> weights;
    vector<double> avgWeights;
    unordered_map<string, int> features;
    int next_id = 0;

    if(num_examples == -1)
        num_examples = compute_num_examples(trainset);

    if(model != NULL) {
        fprintf(stderr, "loading model %s\n", model);
        FILE* fp = fopen(model, "r");
        if(!fp) {
            fprintf(stderr, "ERROR: cannot open \"%s\"\n", argv[1]);
            return 1;
        }
        int next_id = 0;
        size_t buffer_size = 0;
        char* buffer = NULL;
        while(0 < read_line(&buffer, &buffer_size, fp)) {
            buffer[strlen(buffer) - 1] = '\0'; // chop
            char* weight = strchr(buffer, ' ');
            *weight = '\0';
            double value = strtod(weight + 1, NULL);
            features[string(buffer)] = next_id;
            weights.push_back(value);
            avgWeights.push_back(value * (num_examples * num_iterations));
            next_id++;
        }
        fprintf(stderr, "loaded %d feature weights\n", next_id);
        fclose(fp);
        free(buffer);
    }

    fprintf(stderr, "examples: %d\n", num_examples);
    fprintf(stderr, "iteration %d\n", iteration);
    fprintf(stderr, "num iterations %d\n", num_iterations);
    process(trainset, num_iterations, weights, avgWeights, features, next_id, iteration, num_examples, true, clip);

    unordered_map<string, int>::iterator end = features.end();
    for( unordered_map<string, int>::iterator i = features.begin(); i != end; ++i) {
        if(weights[i->second] != 0) {
            fprintf(stdout, "%s %32.31g\n", i->first.c_str(), weights[i->second]);
        }
    }
    return 0;
}

