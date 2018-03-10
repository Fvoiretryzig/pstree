#include <stdio.h>
#include <assert.h>
#include<dirent.h>

int main(int argc, char *argv[]) 
{
	int i;
	for (i = 0; i < argc; i++) 
	{
		assert(argv[i]); // specification
	    printf("argv[%d] = %s\n", i, argv[i]);
	}
	assert(!argv[argc]); // specification
	DIR *dirptr=NULL;
    i=1;
    struct dirent *entry;
    if((dirptr = opendir("/proc"))==NULL)
    {
        printf("opendir failed!");
        return 1;
    }
    else
    {
        while((entry=readdir(dirptr)))
        {
    	    printf("filename%d=%s\n",i,entry->d_name);
    	    i++;
        }
    	closedir(dirptr);
    }
  return 0;
}
