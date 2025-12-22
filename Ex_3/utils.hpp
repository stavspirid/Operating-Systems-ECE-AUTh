#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <cstring>

/**
 * Convert vector of strings to C-style argv array
 * 
 * @param args Vector of argument strings
 * @return Dynamically allocated argv array (must be freed with freeArgv)
 */
char** vectorToArgv(const std::vector<std::string>& args) {
    char** argv = new char*[args.size() + 1];
    
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    
    argv[args.size()] = nullptr;
    return argv;
}

/**
 * Free memory allocated for argv array
 * 
 * @param argv Array to free
 * @param size Number of elements in array
 */
void freeArgv(char** argv, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}


#endif // UTILS_HPP