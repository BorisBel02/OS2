#include <malloc.h>
#include <string.h>

#include "arg_path.h"
#include "dirHandler.h"

int main(int argc, char* argv[]) {
    arg_path* argPath = (arg_path*) malloc(sizeof (arg_path));

    argPath->src_path_size = strlen(argv[1]) + 1;
    argPath->dst_path_size = strlen(argv[2]) + 1;

    argPath->src_path = (char*) malloc(sizeof(char) * argPath->src_path_size);
    argPath->dst_path = (char*) malloc(sizeof(char) * argPath->dst_path_size);
    strcpy(argPath->src_path, argv[1]);
    strcpy(argPath->dst_path, argv[2]);

    dir_handler(argPath);
}
