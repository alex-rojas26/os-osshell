#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cctype>

bool fileExecutableExists(std::string file_path);
void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);

int main (int argc, char **argv)
{
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Create list to store history
    std::vector<std::string> history;

    // Load history from file if it exists
    std::ifstream inputFile("input_history.txt");
    std::string line;
    while(inputFile && std::getline(inputFile, line)){
        if(!line.empty()){
            history.push_back(line);
        }
    }
    inputFile.close();

    // Create variables for storing command user types
    std::string user_command;               // to store command user types in
    std::string dotslash = "./";
    std::vector<std::string> command_list;  // to store `user_command` split into its variour parameters
    char **command_list_exec;               // to store `command_list` converted to an array of character arrays

    //String stream to store history permenantly
    std::stringstream buffer;
    for(const auto& h : history) buffer << h << "\n";

    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

    // Repeat:
    //  Print prompt for user input: "osshell> " (no newline)
    //  Get user input for next command
    //  If command is `exit` exit loop / quit program
    //  If command is `history` print previous N commands
    //  For all other commands, check if an executable by that name is in one of the PATH directories
    //   If yes, execute it
    //   If no, print error statement: "<command_name>: Error command not found" (do include newline)

    while(true){
        std::cout << "osshell> ";
        std::getline(std::cin, user_command);
        if(user_command.empty()) continue;

        buffer << user_command << "\n";

        if(user_command == "exit"){
            if(history.size() == 128) history.erase(history.begin());
            history.push_back("exit");
            std::cout << std::endl;
            break; // quit shell
        }
        //history with optional parameter
        if(user_command.rfind("history",0) == 0){
            splitString(user_command, ' ', command_list);

            if(command_list.size() == 1){
                // Print all history
                int start = (history.size() > 128) ? history.size() - 128 : 0;
                for(int i=start; i<history.size(); i++){
                    std::cout << "  " << (i+1) << ": " << history[i] << std::endl;
                }
                if(history.size() == 128) history.erase(history.begin());
                history.push_back(user_command);
            }
            else if(command_list.size() == 2){
                std::string arg = command_list[1];
                if(arg == "clear"){
                    history.clear(); // don't log "history clear" command
                    buffer.str(""); // clear file buffer
                    buffer.clear();
                    continue;
                }
                else{
                    bool validNumber = true;
                    for(char c : arg) if(!isdigit(c)) validNumber = false;

                    if(!validNumber){
                        std::cout << "Error: history expects an integer > 0 (or 'clear')" << std::endl;
                        if(history.size() == 128) history.erase(history.begin());
                        history.push_back(user_command);
                        continue;
                    }
                    int n = std::stoi(arg);
                    if(n <= 0){
                        std::cout << "Error: history expects an integer > 0 (or 'clear')" << std::endl;
                        if(history.size() == 128) history.erase(history.begin());
                        history.push_back(user_command);
                        continue;
                    }
                    int start = (history.size() > n) ? history.size() - n : 0;
                    for(int i=start; i<history.size(); i++){
                        std::cout << "  " << (i+1) << ": " << history[i] << std::endl;
                    }
                    if(history.size() == 128) history.erase(history.begin());
                    history.push_back(user_command);
                }
            }
            else{
                std::cout << "Error: history expects an integer > 0 (or 'clear')" << std::endl;
                if(history.size() == 128) history.erase(history.begin());
                history.push_back(user_command);
            }
        }
        //Executable
        else if(user_command.find('/') != std::string::npos) {
            splitString(user_command, ' ', command_list);
            vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
            // std::cout << "local command" << std::endl;
            std::string path = command_list[0];
            if(fileExecutableExists(path)){
                    // std::cout << path << " exists"<< std::endl;
                    pid_t pid = fork();
                    if(pid == 0){
                        execv(path.c_str(), command_list_exec);
                    }
                    else if(pid > 0){
                        wait(NULL);
                        freeArrayOfCharArrays(command_list_exec, command_list.size()+1);
                    }
                }
            else{
                std::cout << user_command << ": Error command not found" << std::endl;
            }
            if(history.size() == 128) history.erase(history.begin());
            history.push_back(user_command);
        }
        else {
            splitString(user_command, ' ', command_list);
            vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);

            bool foundPath = false;
            for (const auto& dir : os_path_list)
            {
                std::string path = dir + "/" + command_list[0];
                //if you want to test this and the fileExecutableExists is not yet implemented
                // put a i == 3 || before the fileExecut... and test with the ls command
                if(fileExecutableExists(path)){
                    foundPath = true;
                    pid_t pid = fork();
                    if(pid == 0){
                        execv(path.c_str(), command_list_exec);
                    }
                    else if(pid > 0){
                        wait(NULL);
                        freeArrayOfCharArrays(command_list_exec, command_list.size()+1);
                    }
                    break;
                }
            }
            if(!foundPath){
                std::cout << user_command << ": Error command not found" << std::endl;
                freeArrayOfCharArrays(command_list_exec, command_list.size()+1);
            }
            if(history.size() == 128) history.erase(history.begin());
            history.push_back(user_command);
        }
    }

    
    /************************************************************************************
     *   Example code - remove in actual program                                        *
     ************************************************************************************/
    // Shows how to loop over the directories in the PATH environment variable
    /*
    int i;
    for (i = 0; i < os_path_list.size(); i++)
    {
        printf("PATH[%2d]: %s\n", i, os_path_list[i].c_str());
    }
    printf("------\n");
    
    // Shows how to split a command and prepare for the execv() function
    std::string example_command = "ls -lh";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");

    // Second example command - reuse the `command_list` and `command_list_exec` variables
    example_command = "echo \"Hello world\" I am alive!";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");
    */
    /************************************************************************************
     *   End example code                                                               *
     ************************************************************************************/
    
    //txt file with all inputs
    std::ofstream outputFile("input_history.txt");
    if (outputFile.is_open()) {
        outputFile << buffer.str();
        outputFile.close();
    }

    return 0;
}

/*
   file_path: path to a file
   RETURN: true/false - whether or not that file exists AND is executable
*/
bool fileExecutableExists(std::string file_path)
{   
    bool exists = false;
    struct stat fileproperties;
    // check if `file_path` exists
    // if so, ensure it is not a directory and that it has executable permission
    // how to check for executable permission
    if (stat(file_path.c_str(), &fileproperties) == 0) {
        // std::cout << "first" << std::endl;
        if (!S_ISDIR(fileproperties.st_mode) && (fileproperties.st_mode & S_IXUSR)) {
            // std::cout << "second" << std::endl;
            exists = true;
        }
    }

    return exists;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
    delete[] array;
}
