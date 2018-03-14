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
	int thread_cnt;
	int if_thread;
	struct pstree_node *parent;
	struct pstree_node *thread[128];
	struct pstree_node *children[128];
	struct pstree_node *next;
};
struct pstree_node *list_head;
/*----------删除空格---------*/
void remove_space(char* s)
{
	//printf("s:%s\n", s);
	char *pos1 = s;
	char *pos2 = s;

	while(*pos1 != '\0')
	{
		if(*pos1 != ' '&& *pos1 != '	')
			*pos2++ = *pos1;
		pos1++;
	}
	*pos2 = '\0';
	
	//printf("s:%s\n", s);
	return;
}
/*----------建链表为了以后建树----------*/
void insert_list(char *proc_name, pid_t proc_pid, pid_t proc_ppid, int if_thread)	
{
	struct pstree_node *node = (struct pstree_node*)malloc(sizeof(struct pstree_node));	
	if (node == NULL) 	
	{		
		printf("malloc failed\n");
		return;
	}	
	node->children_cnt = 0; node->thread_cnt = 0;
	strcpy(node->name, proc_name);
	node->pid = proc_pid; node->ppid = proc_ppid;
	node->children[0] = NULL; node->parent = NULL; node->thread[0] = NULL;
	node->if_thread = if_thread;
	//printf("pid:%d if_thread:%d\n", node->pid, node->if_thread);
	//printf("pid:%d ppid:%d\n", proc_pid, proc_ppid);
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
	char proc_tgid[32];
	char buffer[256];
	char* header;
	char* content;
	char* content_after_colon;
	int flag = 0;
	int if_thread = 0;
	
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
		content_after_colon = strtok(NULL, ":");
		if(header != NULL && content != NULL)
		{
			if(content_after_colon != NULL)
			{
				strcat(content, ":");
				strcat(content, content_after_colon);
			}
			remove_space(header); remove_space(content);
			strcpy(&content[strlen(content)-1], "\0");
			//printf("header:%s content:%s\n", header, content);
			if(!strcmp(header, "Name"))
				strcpy(proc_name, content);
			else if(!strcmp(header, "Pid"))
				strcpy(proc_pid, content);
			else if(!strcmp(header, "PPid"))
				strcpy(proc_ppid, content);
			else if(!strcmp(header, "Tgid"))
				strcpy(proc_tgid, content);
			else if(!strcmp(header, "VmPeak"))
				flag = 1;
		}
	}
	if(strcmp(&proc_pid[0], &proc_tgid[0]))
	{
		strcpy(&proc_ppid[0], &proc_tgid[0]);
		if_thread = 1;
	}
	if(flag) 
		insert_list(&proc_name[0], atoi(proc_pid), atoi(proc_ppid), if_thread);			
	//printf("flag is true, filename:%s, ppid:%d\n", dirname, atoi(proc_ppid));
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
	struct pstree_node *parent_node;
	
	for(cur_node = list_head; cur_node != NULL; cur_node = cur_node->next)	//对每个节点进行遍历找到父亲和孩子
	{
		parent_node = NULL;	//！！！！！要初始化！！！！！
		if(cur_node->ppid)
			parent_node = find_node(cur_node->ppid);
		if(parent_node != NULL)
		{
			cur_node->parent = parent_node;
			if(cur_node->if_thread)
			{
				//printf("pid:%d ppid:%d thread_cnt:%d\n", cur_node->pid, parent_node->pid, parent_node->thread_cnt);
				parent_node->thread[parent_node->thread_cnt] = cur_node;
				parent_node->thread_cnt++;
				parent_node->thread[parent_node->thread_cnt] = NULL;
				//printf("pid:%d ppid:%d thread_cnt:%d\n\n", cur_node->pid, parent_node->pid, parent_node->thread_cnt);
			}
			else
			{
				parent_node->children[parent_node->children_cnt] = cur_node;
				parent_node->children_cnt++;
				parent_node->children[parent_node->children_cnt] = NULL;
			}
			//printf("parent_node:%d children:%d parent_children:%d\n\n", parent_node->pid, cur_node->pid, parent_node->children_cnt);
		}
	}
	return;
}
/*---------一个简单的排序---------*/
void pstree_node_sort(struct pstree_node *node)
{
	int n = node->children_cnt;
	for(int i = 0; i<n; i++)
	{
		for(int j = i+1; j<n; j++)
		{
			//printf("children[i]:%d children[j]:%d\n",node->children[i]->pid, node->children[j]->pid );
			if(node->children[i]->pid > node->children[j]->pid)
			{
				struct pstree_node *temp = node->children[i];
				node->children[i] = node->children[j];
				node->children[j] = temp;
			}
			//printf("children[i]:%d children[j]:%d\n\n",node->children[i]->pid, node->children[j]->pid );
		}
	}
}
/*---------打印树---------*/
void print_tree(int option, struct pstree_node *root, int layer)
{
	struct pstree_node *temp;
	struct pstree_node *print_temp = NULL;
	if(root->parent != NULL)
		print_temp = root->parent;
	while(print_temp)
	{
		int len = strlen(print_temp->name);
		if(print_temp->pid == 1)
			len -= 3;
		for(int i = 0; i<len+1; i++)
			printf(" ");
		print_temp = print_temp->parent;		
	}
	if(layer)
		printf("|--");
	switch(option)
	{
		case 1:		//打印pid的选项
			printf("(pid:%d)%s", root->pid, root->name);
			for(int i = 0; i<root->children_cnt; i++)
			{
				temp = root->children[i];
				print_tree(1, temp, ++layer);
				layer--;
			}
			break;
		case 2:
			printf("%s", root->name);
			if(root->children[1]!=NULL)
				pstree_node_sort(root);
			for(int i = 0; i<root->children_cnt; i++)
			{
				temp = root->children[i];
				print_tree(2, temp, ++layer);
				layer--;
			}
			break;
		case 3:
			printf("(pid:%d)%s",root->pid, root->name);
			if(root->children[1]!=NULL)
				pstree_node_sort(root);
			for(int i = 0; i<root->children_cnt; i++)
			{
				temp = root->children[i];
				print_tree(3, temp, ++layer);
				layer--;
			}
			break;		
		default:	//默认
			if(root->if_thread)
				printf("{%s}(pid:%d)\n", root->name, root->pid);
			else
				printf("%s(pid:%d)\n", root->name, root->pid);
			for(int i = 0; i<root->thread_cnt; i++)
			{
				temp = root->thread[i];
				print_tree(0, temp, ++layer);
				layer--;
			}
			for(int i = 0; i<root->children_cnt; i++)
			{
				temp = root->children[i];
				print_tree(0, temp, ++layer);
				layer--;
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
	DIR *proc_dirptr = NULL;
	DIR *task_dirptr = NULL;
    struct dirent *proc_entry;
    struct dirent *task_entry;
    char dirname[256];
    if((proc_dirptr = opendir("/proc"))==NULL)
    {
        printf("opendir failed!");
        return 1;
    }
    else
    {
        while((proc_entry=readdir(proc_dirptr)))
        {
    	    if(proc_entry->d_type == DT_DIR)
    	    {
    	    	if(proc_entry->d_name[0]>48 && proc_entry->d_name[0]<=57)
    	    	{
    	    		char temp_dirname[256];
	    	    	strcpy(&dirname[0], "/proc/");
	    	    	strcat(&dirname[0], proc_entry->d_name);
	    	    	strcat(&dirname[0], "/task/");
	    	    	strcpy(&temp_dirname[0], &dirname[0]);
	    	    	if((task_dirptr = opendir(&dirname[0]))==NULL)
	    	    	{
	    	    		printf("opendir failed while reading task\n");
	    	    		return 1;
	    	    	}
	    	    	else
	    	    	{
	    	    		while((task_entry=readdir(task_dirptr)))	//读取task里面的东西
	    	    		{
	    	    			if(task_entry->d_name[0]>48 && task_entry->d_name[0]<=57)
	    	    			{
	    	    				strcpy(&dirname[0], &temp_dirname[0]);	//相当于初始化一下不然会持续接在上一个名字后面
		    	    			strcat(&dirname[0], task_entry->d_name);
		    	    			save_info(dirname);
	    	    			}
	    	    			
	    	    		}
	    	    		closedir(task_dirptr);
	    	    	}
    	    	}
    	    }
        }
    	closedir(proc_dirptr);
    }
	create_tree();
	/*---------对于不同参数的处理--------*/
	if(argc > 1)
	{
    	if(!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version"))
    		printf("my_pstree 0.0.1\n");
    	if(argc==2 && (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--show-pids")))
    	{
    		for(struct pstree_node *node = list_head; node!=NULL; node = node->next)
	    	{
	    		if(node->parent == NULL)
	    		{
	    			print_tree(1, node, 0);
	    		}
	    	}    		
    	}
    	if(argc==2 && (!strcmp(argv[1], "-n") || !strcmp(argv[1], "--ns-sort")))
    	{
    		for(struct pstree_node *node = list_head; node!=NULL; node = node->next)
	    	{
	    		if(node->parent == NULL)
	    		{
	    			print_tree(2, node, 0);
	    		}
	    	}       		
    	}
    	if(argc==3 && ((!strcmp(argv[1], "-n") || !strcmp(argv[1], "--ns-sort")) || 
    		(!strcmp(argv[2], "-n") || !strcmp(argv[2], "--ns-sort"))))
    		if(((!strcmp(argv[1], "-p") || !strcmp(argv[1], "--show-pids")) || 
    		(!strcmp(argv[2], "-p") || !strcmp(argv[2], "--show-pids"))))
		{
    		for(struct pstree_node *node = list_head; node!=NULL; node = node->next)
	    	{
	    		if(node->parent == NULL)
	    		{
	    			print_tree(3, node, 0);
	    		}
	    	}  
		}
    }
    
    if(argc == 1)
    {
    	for(struct pstree_node *node = list_head; node!=NULL; node = node->next)
    	{
    		if(node->parent == NULL)
    		{
    			print_tree(0, node, 0);
    		}
    	}
    }
  return 0;
}

