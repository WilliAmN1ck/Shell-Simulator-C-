//
//  Hello_Name.c
//  
//
//  Created by Nick W on 2/27/18.
//

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
    char * name = (char*)malloc(sizeof(char)*25);
    printf("Hello, World!\n");
    printf("What is your name?\n");
    scanf("%s", name);
    printf("Hello, %s!\n",name);
    if(name != NULL) {
        free(name);
        name = NULL;
    }
    
    return 0;
}
