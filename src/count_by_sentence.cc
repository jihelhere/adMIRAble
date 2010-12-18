#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unordered_map>
#include <string>


int main(int, char**) 
{
  std::unordered_map<std::string, int> global_counts;
  std::unordered_map<std::string, int> local_counts;

  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);
  while(NULL != fgets(buffer, buffer_size, stdin)) {
    while(buffer[strlen(buffer) - 1] != '\n') {
      buffer_size *= 2;
      buffer = (char*) realloc(buffer, buffer_size);
      if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
    }
    if(*buffer == '\n') {

      for (std::unordered_map<std::string,int>::const_iterator i(local_counts.begin()); i != local_counts.end(); ++i)  {
        if(i->second)
          ++global_counts[i->first];
      }

      local_counts.clear();

    } else {
      char* token;
      int first = 1;
      for(token = strtok(buffer, " \t"); token != NULL; token = strtok(NULL, " \t\n")) {
        if(first == 1) {
          first = 0;
        } else {
          char* name = strrchr(token, ':');
          if(name != NULL) {
            *name = '\0';
            
            ++local_counts[std::string(token)];
          }
        }
      }
    }
  }
  free(buffer);

  for (std::unordered_map<std::string,int>::const_iterator i(global_counts.begin()); i != global_counts.end(); ++i) 
    if(i->second)
      fprintf(stdout, "%s %d\n", i->first.c_str(), i->second);
  
  return 0;
}
