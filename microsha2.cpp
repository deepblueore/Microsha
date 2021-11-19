#include <stdio.h>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <errno.h>
#include <vector>
#include <string>
#include <cctype>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
#include <fnmatch.h>
#include <csignal>
#include <sys/wait.h>
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

char* double_size(char* array, size_t* size)
{
        int size_i = (int)*size;
        char* array_new = new char[size_i * 2];
        for (int i = 0; i < size_i; ++i) array_new[i] = array[i];
        delete array;
        *size = size_i * 2;
        return array_new;
}

char is_privileged()
{
	passwd* user = getpwuid(getuid());
        bool is_root = !(bool)std::strcmp(user->pw_name, "root");
        if (is_root) return '!';
	return '>';

}

std::string get_directory()
{
	errno = 0;
        size_t buf_size = 20;
        char* buffer = new char[buf_size];
        if (getcwd(buffer, buf_size) == NULL)
        {
        	while(errno == ERANGE)
                {
                	buffer = double_size(buffer, &buf_size);
                        errno = 0;
                        getcwd(buffer, buf_size);
                }
        }
	std::string directory;
	for (int i = 0; i < buf_size; ++i) directory.push_back(buffer[i]);
	return directory;
}

void do_cd(std::vector<std::string> buffer)
{
	if (buffer.size() == 1) 
	{
		if(chdir(getenv("HOME")) == -1) perror("chdir");
	}
	else
	{
		if(chdir(buffer[1].c_str()) == -1) fprintf(stderr, "no such directory\n");
	}
	return;
}

void perform(std::vector<std::string> element)
{
	std::vector<std::string> line;
	bool if_num_onei = false;
	for (std::vector<std::string>::iterator iter = element.begin(); iter != element.end(); ++iter)
	{
		if (*iter == "\n") break;
		else if (*iter == "<")
		{
			if (iter != element.end() - 1)
			{
				if_num_onei = false;
				++iter;
				if (*iter == "<" || *iter == ">" || *iter == "/" || *iter == "\\")
				{
					fprintf(stderr, "invalid output\n");
					return;
				}
				std::string tmpl;
				char prev_char = 0;
				for (int i = 0; i < (*iter).size(); ++i)
				{
					if ((*iter)[i] != '\\' || prev_char == '\\') tmpl.push_back((*iter)[i]);
					if ((*iter)[i] == '\\' && prev_char == '\\') prev_char = 0;
					else prev_char = (*iter)[i];
				}
				int fid = open((char*)tmpl.c_str(), O_RDWR, 0666);
				if (fid == -1)
				{
					perror("open");
					return;
				}
				if_num_onei = true;
				dup2(fid, 0);
				continue;
			}
			else
			{
				fprintf(stderr, "ERROR5");
				return;
			}
		}
		else if (*iter == ">")
		{
			if (iter != element.end() - 1)
                        {
                                if_num_onei = false;
                                ++iter;
                                if (*iter == "<" || *iter == ">" || *iter == "/" || *iter == "\\")
                                {
                                        fprintf(stderr, "invalid input");
                                        return;
                                }
                                std::string tmpl;
                                char prev_char = 0;
                                for (int i = 0; i < (*iter).size(); ++i)
                                {
                                        if ((*iter)[i] != '\\' || prev_char == '\\') tmpl.push_back((*iter)[i]);
                                        if ((*iter)[i] == '\\' && prev_char == '\\') prev_char = 0;
                                        else prev_char = (*iter)[i];
                                }
                                int fid = open((char*)tmpl.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
                                if (fid == -1)
                                {
                                        perror("open");
                                        return;
                                }
                                if_num_onei = true;
                                dup2(fid, 1);
                                continue;
                        }
                        else
                        {
                                fprintf(stderr, "there should be something after '>'\n");
                                return;
                        }

		}
		if (if_num_onei)
		{
			fprintf(stderr, "you should use only one file\n");
			return;
		}
		line.push_back(*iter);
	}
	if (!line.empty())
	{
		std::vector<char*> argv;
		for (std::vector<std::string>::iterator iter = line.begin(); iter != line.end(); ++iter) 
			argv.push_back((char*)(*iter).c_str());
		argv.push_back(NULL);
		execvp(argv[0], &argv[0]);
		fprintf(stderr, "unknown command\n");
		exit(1);
	}
	return;
}

void make_pipe(std::vector<std::vector<std::string>>& piped_input)
{
	int iter = 0;
	for (iter; iter <= piped_input.size(); ++iter)
	{
		int fd[2];
		pipe(fd);
		pid_t pid = fork();
		if (!pid)
		{
			dup2(fd[1], 1);
			close(fd[0]);
			if (piped_input[iter][0].size() == 1 && piped_input[iter][0] == "pwd")
			{
				std::string buffer = get_directory();
				printf("%s", buffer.c_str());
				return;
			}
			perform(piped_input[iter]);
		}
		dup2(fd[0], 0);
		close(fd[1]);
	}
	if (piped_input[iter][0].size() == 1 && piped_input[iter][0] == "pwd")
	{
		std::string buffer = get_directory();
		printf("%s", buffer.c_str());
		return;
	}
	perform(piped_input[iter]);
	return;
}

std::string eliminate_slashes(std::string buffer)
{
	int size = buffer.size();
	std::string buffer_eliminated;
	for (int i = 0; i < size; ++i)
	{
		buffer_eliminated.push_back(buffer.at(i));
		if (buffer.at(i) == '/')
		{
			do
			{
				++i;
			}while(i < size && buffer.at(i) == '/');
		}
	}
	return buffer_eliminated;
}

void replace(std::string buffer, std::string directory, std::vector<std::string>* parsed_input)
{
	std::vector<std::string> line;
	int iter = 0;
	bool if_root_dir = false;
	int buffer_size = buffer.size();
	std::string until_slash, after_slash, path;
	char symbol = buffer.at(0);
	if (buffer.at(0) == '/') if_root_dir = true;
	while (iter < buffer_size && symbol == '/')
	{
		++iter;
		if (iter < buffer_size) symbol = buffer.at(iter);
	}
	for (iter; symbol != '/' && iter < buffer_size; ++iter)
	{
		if (iter < buffer_size) symbol = buffer.at(iter);
		until_slash.push_back(symbol);
	}
	if (iter >= buffer_size) --iter;
	if (buffer.at(iter) == '/')
	{
		for (iter; iter < buffer_size; ++iter)
		{
			if (iter < buffer_size) symbol = buffer.at(iter);
			after_slash.push_back(symbol);
		}
		if (if_root_dir)
		{
			if (directory.empty()) path = '/';
			else path = directory;
		}
		else path = '.';
		if (until_slash == "." || until_slash == "..")
		{
			if (if_root_dir) path = directory + "/" + until_slash;
			else if (until_slash == ".") path = ".";
			else path = "..";
			if (after_slash.size() > 1) replace(after_slash, path, parsed_input);
			else parsed_input->push_back(path);
		}
		else
		{
			struct stat stat_buf;
			DIR* dir = opendir(path.c_str());
			if (!dir) return;
			for (dirent* dir_read = readdir(dir); dir_read; dir_read = readdir(dir))
			{
				if (std::string(dir_read->d_name) == "." || std::string(dir_read->d_name) == "..") continue;
				if (!fnmatch(until_slash.c_str(), dir_read->d_name, 0))
				{
					if (if_root_dir) path = directory + "/" + (std::string)dir_read->d_name;
					else path = (std::string)dir_read->d_name;
					if (stat(path.c_str(), &stat_buf) < 0) return ;
					if (S_ISDIR(stat_buf.st_mode)) line.push_back(std::string(dir_read->d_name));
				}
			}
			if (closedir(dir) < 0) fprintf(stderr, "can't close the file");
			if (!line.empty())
			{
				std::sort(line.begin(), line.end(), std::less<std::string>());
				for (std::vector<std::string>::iterator iter = line.begin(); iter != line.end();
						++iter)
				{
					if (if_root_dir) path = directory + "/" + *iter;
					else path = *iter;
					if (after_slash.size() > 1) replace(after_slash, path, parsed_input);
					else parsed_input->push_back(path);
				}
			}
		}
	}
	else
	{
		if (if_root_dir)
		{
			if (directory.empty()) path = '/';
			else path = directory;
		}
		else path = '.';
		if (until_slash == "." || until_slash == "..")
                {
                        if (if_root_dir) path = directory + "/" + until_slash;
                        else if (until_slash == ".") path = ".";
			else path = "..";
                        if (after_slash.size() > 1) replace(after_slash, path, parsed_input);
                        else parsed_input->push_back(path);
                }
		else
		{
			DIR* dir = opendir(path.c_str());
			if (!dir) return;
			for (dirent* dir_read = readdir(dir); dir_read; dir_read = readdir(dir))
			{
				if (dir_read->d_name[0] == '.') continue;
				else if (std::string(dir_read->d_name) == "." && std::string(dir_read->d_name) == "..") continue;
				if (!fnmatch(until_slash.c_str(), dir_read->d_name, 0)) line.push_back(std::string(dir_read->d_name));
			}
			if (!line.empty())
			{
				std::sort(line.begin(), line.end(), std::less<std::string>());
				if (if_root_dir)
				{
					for (std::vector<std::string>::iterator iter = line.begin(); iter != line.end();
							++iter)
					{
						path = directory + "/" + *iter;
						parsed_input->push_back(path);
					}
				}
				else
				{
					for (std::vector<std::string>::iterator iter = line.begin(); iter != line.end();
							++iter)
					{
						path = *iter;
						parsed_input->push_back(path);
					}
				}
			}
			closedir(dir);
		}
	}
	return;
}

int parser(std::string input, std::vector<std::string>* parsed_input)
{
	std::vector<std::string> line;
	char prev_sym = 0;
	int count_to = 0;
	int count_from = 0;
	int count_pipe = 0;
	for (int i = 0; i < input.size() && input[i] != '\n'; ++i)
	{
		int input_size = input.size();
		std::string buffer;
		char buf_sym = input.at(i);
		while (!isspace(buf_sym) && !(buf_sym == '<' && prev_sym != '\\') && !(buf_sym == '>' && prev_sym != '\\') && buf_sym != '?'
				&& buf_sym != '*' && buf_sym != '|' && i < input.size())
		{
			buffer.push_back(buf_sym);
			if (buf_sym == '\\' && prev_sym == '\\') prev_sym = 0;
			else prev_sym = buf_sym;
			++i;
			if (i < input_size) buf_sym = input.at(i);
		}
		std::string buffer_eliminated, str;
		char symbol;
		switch(input[i])
		{
			case '<':
				if (!count_from)
				{
					if (!buffer.empty()) line.push_back(buffer);
					line.push_back("<");
					++count_from;
				}
				else
				{
					fprintf(stderr, "too many '<'\n");
					return 0;
				}
				continue;
				break;
			case '>':
				if (!count_to)
				{
					if (!buffer.empty()) line.push_back(buffer);
					line.push_back(">");
					++count_to;
				}
				else
				{
					fprintf(stderr, "too many '>'\n");
					return 0;
				}
				continue;
				break;
			case '|':
				if (!buffer.empty()) line.push_back(buffer);
				line.push_back("|");
				++count_pipe;
				continue;
				break;
			case '?':
				{
					++i;
					if (i < input_size) symbol = input.at(i);
					buffer.push_back('?');
					while(i < input_size && !isspace(symbol) && symbol != '>' && symbol != '<' && symbol != '|' &&
						       	symbol != '*' && symbol != '?')
					{
						buffer.push_back(symbol);
						++i;
						if (i < input_size) symbol = input.at(i);
					}
					int line_size = line.size();
					buffer_eliminated = eliminate_slashes(buffer);
					str = "";
					replace(buffer_eliminated, str, &line);
					if (line_size == line.size())
					{
						fprintf(stderr, "no such directory\n");
						return 0;
					}
					continue;
					break;
				}
			case '*':
				//printf("in *\n");
				++i;
                                if (i < input_size) symbol = input.at(i);
                                buffer.push_back('*');
				//printf("succeed\n;");
                                while(i < input_size && !isspace(symbol) && symbol != '>' &&
					       	symbol != '<' && symbol != '|' && symbol != '*' && symbol != '?')
                                {
                                        buffer.push_back(symbol);
                                        ++i;
                                        if (i < input_size) symbol = input.at(i);
                                }
				//printf("obrab\n");
                                int line_size = line.size();
                                buffer_eliminated = eliminate_slashes(buffer);
                                str = "";
                                replace(buffer_eliminated, str, &line);
				//printf("again\n");
                                if (line_size == line.size())
                                {
                                        fprintf(stderr, "no such directory\n");
                                        return 0;
                                }
				continue;
				break;
		}
		if(buffer == "time" && line.size() == 0) continue;
		if (!buffer.empty()) line.push_back(buffer);
	}
	if (!line.empty())
	{
		for (std::vector<std::string>::iterator iter = line.begin(); iter != line.end();
			++iter) parsed_input->push_back(*iter);
		//printf("%s", (parsed_input)[0]);
		if((*parsed_input)[0] == "pwd" && parsed_input->size() == 1) return 1;
	}
	if (count_pipe) return 2;
	return 3;
}

int main()
{
	if(chdir(getenv("HOME")) == -1) perror("chdir");
	signal(SIGINT, SIG_IGN);
	char invite_symbol = is_privileged();
	do
	{
		std::string directory = get_directory();
		printf("[%s]%c ", directory.c_str(), invite_symbol);
		std::string console_input;
		std::getline(std::cin, console_input);
		std::string element;
		int counter = 0;
		while(isspace(console_input.at(counter))) ++counter;
		while(counter < console_input.size() && !isspace(console_input.at(counter)) && console_input.at(counter) != '>' &&
				console_input.at(counter) != '<' && console_input.at(counter) != '|')
		{
			element.push_back(console_input.at(counter));
			++counter;
		}
		if (element == "cd")
		{
			std::vector<std::string> argv;
			if (parser(console_input, &argv) != 0) do_cd(argv);
		}
		else
		{
			pid_t pid = fork();
			if (pid == 0)
			{	
				//printf("fork started\n");
				signal(SIGINT, SIG_DFL);
				std::vector<std::string> argv;
				int symbol_marker = parser(console_input, &argv);
				//printf("marker: %d\n", symbol_marker);
				switch(symbol_marker)
				{
					case 0:
						exit(1);
					case 1:
						{
							//printf("we are in case 1\n");
							std::string directory = get_directory();
							printf("%s\n", directory.c_str());
						}
						break;
					case 2:
						{
							std::vector<std::vector<std::string>> arguments;
							std::vector<std::string> line;
							for(std::vector<std::string>::iterator iter = argv.begin(); iter!=argv.end(); ++iter)
							{
								if(*iter == "|" && iter!=argv.begin() && iter!=argv.end())
								{
									arguments.push_back(line);
									line.clear();
									++iter;
								}
								line.push_back(*iter);
							}
							arguments.push_back(line);
							for(int i = 1; i < arguments.size() - 1; ++i)
							{
								for(std::vector<std::string>::iterator iter = arguments[i].begin(); 
										iter!=arguments[i].end(); ++iter)
								{
									if(*iter == "<" || *iter == ">") fprintf(stderr, "ERROR");
								}
							}
							if(!fork()) make_pipe(arguments);
							int status;
							wait(&status);
						}
						break;
					case 3:
						if (argv.size()) perform(argv);
						break;
				}
				exit(0);
			}
			int st;
			if(element == "time")
			{
				struct timeval start_time, end_time;
				struct rusage ch_start_utime, ch_end_utime;
				getrusage(RUSAGE_CHILDREN, &ch_start_utime);
				gettimeofday(&start_time, NULL);
				wait(&st);
				getrusage(RUSAGE_CHILDREN, &ch_end_utime);
				gettimeofday(&end_time, NULL);
				double u_time = ch_end_utime.ru_utime.tv_sec - ch_start_utime.ru_utime.tv_sec + (double)(ch_end_utime.ru_utime.tv_usec - ch_start_utime.ru_utime.tv_usec)/1000000;
				double s_time = ch_end_utime.ru_stime.tv_sec - ch_start_utime.ru_stime.tv_sec + (double)(ch_end_utime.ru_stime.tv_usec - ch_start_utime.ru_stime.tv_usec)/1000000;
				double r_time = end_time.tv_sec - start_time.tv_sec + (double)(end_time.tv_usec - start_time.tv_usec)/1000000;			
				printf("real  %.3fs\n", r_time);
				printf("user  %.3fs\n", u_time);
				printf("sys   %.3fs\n", s_time);
			}
		       	else wait(&st);
		}
	}while(1);
	return 0;
}

