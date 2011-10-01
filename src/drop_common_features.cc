#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unordered_map>
#include <vector>
#include <string>


#include "utils.h"

int always_keep(char * token)
{
    return !strncmp(token, "prob", 4) && !strncmp(token, "score",5) ;
}


int main()
{
    std::unordered_map<std::string, int> hashtable(100000000);
    hashtable.max_load_factor(0.7f);

    std::vector<char*> lines;

    size_t buffer_size = 0;
    char* buffer = NULL;
    long int skipped = 0, total = 0;

    while(0 < read_line(&buffer, &buffer_size, stdin))  {

        if(buffer[0] == '\n') {

            for(unsigned i = 0; i < lines.size(); ++i) {
                char* token;

                //read label
                token = strtok(lines[i], " \t\n");
                fprintf(stdout, "%s", token);

                for(token = strtok(NULL, " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {
                    char* end = strrchr(token, ':');
                    if(end != NULL) {
                        *end = '\0';
                        if(always_keep(token)) {
                            *end = ':';
                            fprintf(stdout," %s", token);
                        }
                        else {
                            *end = ':';
                            if(hashtable.at(token) != (int) lines.size()) {
                                fprintf(stdout," %s", token);
                            }
                            else
                                ++skipped;
                        }
                    }
                    ++total;
                }
                fprintf(stdout, "\n");
                free(lines[i]);
            }
            lines.clear();

            hashtable.clear();

            fprintf(stdout, "\n");



        }
        // read line
        else {
            lines.push_back(strdup(buffer));
            char* token;

            token = strtok(buffer, " \t");

            for(token = strtok(NULL, " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {



                ++hashtable[token];
            }
        }
    }
    free(buffer);

    for(auto i = hashtable.begin(); i != hashtable.end(); ++i) {
        fprintf(stdout, "%s %d\n", i->first.c_str(), i->second);
    }

    fprintf(stderr, "skipped %ld/%ld features (%.2f%%)\n", skipped, total, 100.0 * skipped / (double) total);
    return 0;
}
