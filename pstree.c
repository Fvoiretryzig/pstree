#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>



struct pstree_node
{
	char name[128];
	pid_t pid;
	pid_t ppid;
	int children_cnt;
	struct pstree_node *parent;
	struct pstree_node *children[128];
	struct pstree_node *next;
};
struct pstree_node *list_head;
/*----------删除空格---------*/
void remove_space(char *s)
{
	char *temp = s;
	printf("s before dealing:%s\n", s);
	while(*temp != '\0')
	{
		if(*temp != ' ')
			*s ++= *temp;
		temp++;
	}
	char print_s[256];
	strcpy(print_s, s);
	*s += '\0';
	printf("print_s:%s %s\n", print_s, s);
	return;
}
/*----------建链表为了以后建树----------*/
void insert_list(char *proc_name, pid_t proc_pid, pid_t proc_ppid)	
{
	struct pstree_node *node = (struct pstree_node*)malloc(sizeof(struct pstree_node));	
	if (node == NULL) 	
	{		
		printf("malloc failed\n");
		return;
	}	
	node->children_cnt = 0;
	strcpy(node->name, proc_name);
	node->pid = proc_pid; node->ppid = proc_ppid;
	node->children[0] = NULL; node->parent = NULL;
	
	node->next = list_head;
	list_head = node;
	
	return;
}
/*---------对dirname中的status中的信息进行保存---------*/
void save_info(char* dirname)
{
	char filename[256];
	char proc_name[256];
	char proc_pid[32];
	char proc_ppid[32];
	char buffer[256];
	char* header;
	char* content;
	int flag = 0;
	
	FILE* pstree_file;
	/*--------打开每个进程里面的status--------*/
	strcpy(filename, dirname);
	//printf("filename:%s\n", filename);
	strcat(filename, "/status");
	pstree_file = fopen(filename, "r");
	if(pstree_file == NULL)
	{
		printf("this file does not exist\n");
		return;
	}	
	/*--------从status文件中读取ppid和pid以及name--------*/
	while(fgets(buffer, sizeof(buffer), pstree_file) != NULL)
	{
		header = strtok(buffer, ":");	//以冒号为标志分割
		content = strtok(NULL, ":");
		if(header != NULL && content != NULL)
		{
			remove_space(header); remove_space(content);
			//printf("header:%s content:%s\n", header, content);
			if(!strcmp(header, "Name"))
				strcpy(proc_name, content);
			else if(!strcmp(header, "Pid"))
				strcpy(proc_pid, content);
			else if(!strcmp(header, "PPid"))
				strcpy(proc_ppid, content);
			else if(!strcmp(header, "VmPeak"))
				flag = 1;
		}
	}
	if(flag) 
		insert_list(&proc_name[0], atoi(proc_pid), atoi(proc_ppid));
	printf("this is %s\n", proc_name);
	return;
}
/*---------找节点---------*/
struct pstree_node* find_node(pid_t pid)
{
	struct pstree_node *temp;
	for(temp = list_head; temp != NULL; temp = temp->next)
	{
		if(temp->pid == pid)
			return temp;
	}
	return NULL;
}
/*---------建树---------*/
void create_tree()
{
	struct pstree_node *cur_node;
	struct pstree_node *parent_node = NULL;
	for(cur_node = list_head; cur_node != NULL; cur_node = cur_node->next)	//对每个节点进行遍历找到父亲和孩子
	{
		if(cur_node->ppid)
			parent_node = find_node(cur_node->pid);
		if(parent_node != NULL)
		{
			cur_node->parent = parent_node;
			parent_node->children[parent_node->children_cnt++] = cur_node;
			parent_node->children[parent_node->children_cnt] = NULL;
		}
	}
	return;
}
/*---------打印树---------*/
void print_tree(int option, struct pstree_node *root, int layer)
{
	struct pstree_node *temp;
	for(int i = 1; i<=layer; i++)
		printf("	");
	switch(option)
	{
		default:
			printf("name: %s (pid:%d)\n", root->name, root->pid);
			for(int i = 1; i<=root->children_cnt; i++)
			{
				temp = root->children[i];
				print_tree(0, temp, ++layer);
			}
			break;
	}
	return;
}
/*---------main函数---------*/
int main(int argc, char *argv[]) 
{
	/*--------读取命令行参数--------*/
	for (int i = 0; i < argc; i++) 
	{
		assert(argv[i]); // specification
	    printf("argv[%d] = %s\n", i, argv[i]);
	}
	assert(!argv[argc]); // specification
	/*--------打开文件--------*/
	DIR *dirptr=NULL;
    struct dirent *entry;
    char dirname[256];
    if((dirptr = opendir("/proc"))==NULL)
    {
        printf("opendir failed!");
        return 1;
    }
    else
    {
    	//int i = 1 ;
        while((entry=readdir(dirptr)))
        {
    	    //printf("filename%d=%s\n",i,entry->d_name);
    	    //i++;
    	    if(entry->d_type == DT_DIR)
    	    {
    	    	strcpy(&dirname[0], "/proc/");
    	    	strcat(&dirname[0], entry->d_name);
    	    	if(entry->d_name[0]>48 && entry->d_name[0]<=57)
    	    		save_info(dirname);
    	    }
        }
    	closedir(dirptr);
    }
    
	create_tree();
    if(!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version"))
    	printf("my_pstree 0.0.1\n");
    else if(argc == 1)
    {
    	for(struct pstree_node *node = list_head; node!=NULL; node = node->next)
    	{
    		printf("name: %s (pid:%d)\n", node->name, node->pid);
    		if(node->parent == NULL)
    			print_tree(0, node, 0);
    	}
    }
  return 0;
}

