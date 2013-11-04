#include <dirent.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

struct node {
	char *name;
	long time;
	struct node *child;
};

//Given a filename, find the find finename + number which is unique.
//i.e. testdir_2, testdir_3 etc.
char* unique_dir(const char *name) {

	//Set up our name.
	char *new_name = malloc(strlen(name + 3));
	strncpy(new_name, name, strlen(name));
	new_name[strlen(name)] = '\0';

	DIR *d;

	//Was the original input unqiue in the first place?
	if (!(d = opendir(new_name))) {

		closedir(d);

		//The caller is expecting a malloced string, 
		//we cannot alias the input pointer.
		return new_name;
	}

	int count = 1;
	int place = 1;
	float res = 10;

	int pos = strlen(name) + 1;
	new_name[pos-1] = '_';
      //new_name[pos] = '1', '2', '3' etc...
	new_name[pos+1] = '\0';

	do  {

		if (d)
			closedir(d);

		//Have we crossed a power boundery? i.e. 9-->10 or 999 --> 1000
		if(++count == res) {
			
			//Increase this by 1.
			new_name = realloc(new_name, strlen(new_name) + 2);
			new_name[strlen(new_name)] = '\0';
			
			//Calculate the new power boundery.
			res = pow(10, ++place);
		}

		//Convert integer to string.
		char *num = alloca(place + 1);
		sprintf(num, "%d", count);
		num[place] = '\0';

		//Overwrite the old number with the new one.
		strncpy(new_name + pos, num, place + 1);

	} while (d = opendir(new_name));

	return new_name;
}

int main(int argc, char **argv) {

	if (argc != 4) {
		printf("Usage %s <num> <seg> <directory>\n"
		"\t<num> = Number of slices to create, 0 = as many as possible\n"
		"\t<seg> = The size of each slice.\n", argv[0]);
		return 1;
	}

	int num = atoi(argv[1]),
	    seg = atoi(argv[2]);

	if (seg < 1) {
		printf("Seg cannot be zero or negative.\n");
		return 1;
	}

	const char *input = argv[3];
	int base_len = 0;
	int num_files = 0;

	struct node *head = NULL;

	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir(input)) != NULL) {

		//md5sum + period + 3 letter extension + NUL = 37
		int current_size = 37;
	
		char *path = malloc((current_size) * sizeof *path);

		//cd to this directory so we don't need to worry about paths anymore.
		chdir(input);
		
		//For each file ent.
		while ((ent = readdir(dir)) != NULL) {

			int len = strlen(ent->d_name);

			//If the filename length is bigger than path then resize path...
			if (len > current_size) {
				current_size = len;
				path = realloc(path, current_size);
			}

			//Copy our filename into path.
			strncpy(path, ent->d_name, len);
			path[len] = '\0';
			
			//Stat
			struct stat buf;
			stat(path, &buf);

			//Skip if this is not a regular file.
			if(!S_ISREG(buf.st_mode)) {
				continue;
			}

			//Prepare a new node for our linked list.
			struct node *m_node = malloc(sizeof *m_node);
			char *name = malloc(len+1);
			strncpy(name, ent->d_name, len);
			name[len] = '\0';

			m_node->name = name;
			m_node->time = buf.st_mtime;
			m_node->child = NULL;

			//If this is the first element...
			if (!head) {
				head = m_node;
				num_files++;
				continue;
			}

			//For each node in the linked list.
			for (struct node *cur = head; cur != NULL; cur = cur->child) {

				//Is this the last node?
				if (!cur->child) {
					//Add to the end of the list.
					cur->child = m_node;
					num_files++;
					break;
				}
				
				//Compare timestamps to keep items in chorological order.
				if (m_node->time > cur->child->time) {

					//Insert new node here.
					m_node->child = cur->child;
					cur->child = m_node;
					num_files++;
					break;
				}
				/*
				//Alphabetical.
				if (strcmp(m_node->name, cur->child->name) < 0) {
					m_node->child = cur->child;
					cur->child = m_node;
					break;

				}
				*/
			}
		}

		closedir(dir);
		dir = NULL;
	} else {
		printf("Could not open directory.\n");
		return 1;
	}

	int count = 0;

	//Get the current directory.
	char *pwd = getcwd(NULL, 0);
	char *m_pwd = strrchr(pwd, '/');
	m_pwd++;

	if (!*m_pwd) { 
		//Current directory was root TODO
		return 1;
	}

	//Until we have split the folder into num subfolders or 
	//If num is zero keep going until we run out of files.
	for (int num_count = 0; num_count < num || num == 0; num_count++) {
	
		//Generate a unique directory name based on the current one.
		char *new_dir = unique_dir(m_pwd);

		//My umask is 022, this might be different for other systems.
		mkdir(new_dir, 0777);
		int len = strlen(new_dir);
		int size = len + 2 + 37;

		//Create a path template we can copy filenames into later on.
		char *path = malloc(size);
		strncpy(path, new_dir, len);
		path[len++] = '/';

		int seg_count = 0;
		struct node *current = head;
		for (;;) {

			if (!current) {
				//End of the list.
				return 0;
			}

			if (seg_count++ == seg) {
				//Reached the number of files the user wanted in each directory.
				break;
			}
		
			//Do we need to resize path?
			if (strlen(current->name) > size) {

				size = len + 1 + strlen(current->name);
				//Memory leak if realloc fails TODO
				path = realloc(path, size);
			}

			strncpy(path + len, current->name, strlen(current->name));
			path[len + strlen(current->name)] = '\0';

			printf("Renaming %s to %s\n", current->name, path);
			rename(current->name, path);
			
			//Free uneeded nodes from the linked list.
			struct node *tmp = current;
			free(tmp->name);
			free(tmp);

			current = current->child;

		}

		head = current;
		free(new_dir);
		free(path);
	}


	free(pwd);
	pwd = NULL;
	m_pwd = NULL;

	return 0;
}
