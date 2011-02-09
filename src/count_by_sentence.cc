#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

struct sort_func
{ bool operator()(const std::pair<std::string, int>& p1, 
		  const std::pair<std::string, int>& p2)
  {
    return p1.second > p2.second;
  };
};


int main(int, char**) 
{
  std::unordered_map<std::string, int> global_counts;
  //  std::unordered_map<std::string, int> local_counts;
  std::unordered_set<std::string> local_counts;

  int num_sentence = 0;

  int buffer_size = 1024;
  char* buffer = (char*) malloc(buffer_size);
  while(NULL != fgets(buffer, buffer_size, stdin)) {
    while(buffer[strlen(buffer) - 1] != '\n') {
      buffer_size *= 2;
      buffer = (char*) realloc(buffer, buffer_size);
      if(fgets(buffer + strlen(buffer), buffer_size - strlen(buffer), stdin) == NULL) break;
    }
    if(*buffer == '\n') {

      for (auto i(local_counts.begin()); i != local_counts.end(); ++i)  {
	//++global_counts[i->first];
	++global_counts[*i];
      }

      local_counts.clear();

      fprintf(stderr, "processed %d sentences\r", ++num_sentence);

    } else {
      char* token = strtok(buffer, " \t");

      for(token = strtok(NULL, " \t\n"); token != NULL; token = strtok(NULL, " \t\n")) {
	char* name = strrchr(token, ':');
	if(name != NULL) {
	  *name = '\0';
            
	  local_counts.insert(std::string(token));
	    //	  ++local_counts[std::string(token)];
	}
      }
    }
  }
  free(buffer);

  // for (std::unordered_map<std::string,int>::const_iterator i(global_counts.begin()); i != global_counts.end(); ++i) 
  //   if(i->second)
  //     fprintf(stdout, "%s %d\n", i->first.c_str(), i->second);

  std::vector<std::pair<std::string,int>> output_vector;
  output_vector.insert(output_vector.end(), global_counts.begin(), global_counts.end());

  std::cerr << "Nb features: " << output_vector.size() << std::endl;

  std::sort(output_vector.begin(), output_vector.end(), sort_func());
  
  for(auto i(output_vector.begin()); i != output_vector.end(); ++i)
    if(i->second)
      fprintf(stdout, "%s %d\n", i->first.c_str(), i->second);
  
  return 0;
}
