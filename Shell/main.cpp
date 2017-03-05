#include <map> 
#include <iostream>
#include <stdio.h>
#include <string>
#include <cstring>
#include <time.h>
#include <vector>
#include <fstream>
#include <windows.h>

using namespace std;

// helpers
void initializeInstructions();
void initializePath();
string currentDateTime(string type);
string currentDir();
void printCurrentDir();
bool cursorInc();
bool cursorDec();
bool includes(vector<string> v, string elm);
bool findFile(string path, string name);
string findProgramInPaths(string name);
void handling(string str_instruction);

// Process instructions
void listProcess(string s1, string option);
void killProcess(string id, string option);
void suspendProcess(string id, string option);
void resumeProcess(string id, string option);
void startProcess(string name, string option);
void startBatFile(string name, string option);

// System instructions
void exitProgram();
void help();
void date();
void time();
void directory();

// Path instructions
void path(string option);
void addpath(string path);

typedef struct instruct {
	map<string, void(*)(string, string)> process;
	map<string, void(*)()> system;
	map<string, void(*)(string)> path;
} instruct;
typedef struct process {
	string name;
	string status; // [NULL, "Running", "Suspending"]
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
} process;

instruct instructions;
map<string, process> processes;
int foreground_process_id;
bool running_batch_file = false;

vector<string> paths;
vector<string> system_instructions = { "exit", "help", "date", "time", "dir" };
vector<string> process_instructions = { "list", "kill", "stop", "resume", "start", "batch" };
vector<string> path_instructions = { "path", "addpath" };

COORD block = {};
COORD current_coord = {};
HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_SCREEN_BUFFER_INFO screen_info;

void initializeInstructions() {
	instructions.process["list"] = listProcess;
	instructions.process["kill"] = killProcess;
	instructions.process["stop"] = suspendProcess;
	instructions.process["resume"] = resumeProcess;
	instructions.process["start"] = startProcess;
	instructions.process["batch"] = startBatFile;

	instructions.system["exit"] = exitProgram;
	instructions.system["help"] = help;
	instructions.system["date"] = date;
	instructions.system["time"] = time;
	instructions.system["dir"] = directory;

	instructions.path["path"] = path;
	instructions.path["addpath"] = addpath;
}

void initializePath() {
	paths.push_back(currentDir());
	paths.push_back("E:\\C++\\");
	paths.push_back("E:\\C++\\C\\Shell\\");
}

string currentDir() {
	char pBuf[256];
	size_t len = sizeof(pBuf);
	GetModuleFileName(NULL, pBuf, len);
	string dir = pBuf;
	for (int i = dir.length() - 1; i >= 0; i--) {
		if (dir[i] == '\\') {
			dir = dir.substr(0, i + 1);
			break;
		}
	}
	return dir;
}

string currentDateTime(string type) {
	time_t now = time(NULL);
	struct tm time;
	localtime_s(&time, &now);
	char buf[26];
	if (type == "date") {
		sprintf_s(buf, sizeof(buf), "%02d/%02d/%02d",
			time.tm_mday, time.tm_mon + 1, time.tm_year + 1900);
	}
	else if (type == "time") {
		sprintf_s(buf, 26, "%02d/%02d/%02d  %02d:%02d",
			time.tm_mday, time.tm_mon + 1, time.tm_year + 1900,
			time.tm_hour, time.tm_min);
	}
	return buf;
}

void printCurrentDir() {
	cout << "\n" << currentDir() << "\n$ ";
	GetConsoleScreenBufferInfo(hOutput, &screen_info);
	current_coord = screen_info.dwCursorPosition;

	block = { 2, screen_info.dwCursorPosition.Y };
}

bool cursorInc() {
	GetConsoleScreenBufferInfo(hOutput, &screen_info);
	current_coord = screen_info.dwCursorPosition;
	if (current_coord.X < 79) {
		current_coord.X += 1;
		SetConsoleCursorPosition(hOutput, current_coord);
	}
	else {
		current_coord.Y += 1;
		current_coord.X = 0;
		SetConsoleCursorPosition(hOutput, current_coord);
	}
	return true;
}

bool cursorDec() {
	GetConsoleScreenBufferInfo(hOutput, &screen_info);
	current_coord = screen_info.dwCursorPosition;
	if (current_coord.X > block.X) {
		current_coord.X -= 1;
		SetConsoleCursorPosition(hOutput, current_coord);
		return true;
	}
	return false;
}

bool includes(vector<string> v, string elm) {
	for (int i = 0; i < v.size(); i++) {
		if (!v[i].compare(elm)) return true;
	}
	return false;
}

bool findFile(string path, string name) {
	HANDLE hFind;
	WIN32_FIND_DATA data;
	hFind = FindFirstFile(path.append("\\*.*").c_str(), &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (data.cFileName == name && data.dwFileAttributes == 32) {
				return true;
			}
		} while (FindNextFile(hFind, &data));
	}
	return false;
}

string findProgramInPaths(string name) {
	for (int i = 0; i < paths.size(); i++) {
		if (findFile(paths[i], name)) {
			return paths[i] + name;
		}
	}
	return "";
}

void handling(string str_instruction) {
	// Convert string instruction to array of string
	string arr_instruction[5] = {};
	string option = "";
	int index = 0;
	for (int i = 0; i < sizeof(arr_instruction) / sizeof(arr_instruction[0]); i++) {
		int pos_space = str_instruction.find(' ', index);
		arr_instruction[i] = str_instruction.substr(
			index,
			pos_space - index
		);
		if (arr_instruction[i].find("--", 0) >= 0) {
			option = arr_instruction[i];
		}
		if (pos_space == -1)	break;
		index = pos_space + 1;
	}

	if (includes(system_instructions, arr_instruction[0])) {
		instructions.system[arr_instruction[0]]();
	}
	else if (includes(process_instructions, arr_instruction[0])) {
		instructions.process[arr_instruction[0]](arr_instruction[1], option);
	}
	else if (includes(path_instructions, arr_instruction[0])) {
		instructions.path[arr_instruction[0]](arr_instruction[1]);
	}
	else {
		if (arr_instruction[0].find(".bat") != string::npos)
			instructions.process["batch"](arr_instruction[0], option);
		else
			instructions.process["start"](arr_instruction[0], option);
	}
}

void exitProgram() {
	exit(0);
}

void help() {
	cout << "\nHelpppp";
}

void date() {
	cout << "\n" << currentDateTime("date") << '\n';
}

void time() {
	cout << "\n" << currentDateTime("time") << '\n';
}

void directory() {
	HANDLE hFind;
	WIN32_FIND_DATA data;
	hFind = FindFirstFile(currentDir().append("\\*.*").c_str(), &data);
	int num_files = 0, num_dirs = 0, total_size_files = 0;
	printf("\n");
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			SYSTEMTIME creationTime;
			FileTimeToSystemTime(&data.ftCreationTime, &creationTime);

			string type = {};
			if (data.dwFileAttributes == 16) {
				type = "<DIR>";
				num_dirs++;
			}
			else {
				num_files++;
				total_size_files += data.nFileSizeLow / 1024;
			}
			printf(
				"%02d/%02d/%04d   %02d:%02d\t%s\t%s\t%s\n",
				creationTime.wDay,
				creationTime.wMonth,
				creationTime.wYear,
				creationTime.wHour,
				creationTime.wMinute,
				type.c_str(),
				data.nFileSizeLow == 0 ? "" : to_string(data.nFileSizeLow / 1024).c_str(),
				data.cFileName
			);
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
	printf(
		"\n\t\t%d File(s)\t%d KB  \n\t\t%d Dir(s)\n", 
		num_files, total_size_files, num_dirs
	);
}

void path(string option) {
	cout << "\nPATH:\n";
	for (int i = 0; i < paths.size(); i++) {
		cout << i + 1 << ". " <<  paths[i] << "\n";
	}
}

void addpath(string path) {
	// addpath [directory]
	if (path[path.length() - 1] != '\\') path += "\\";
	for (int i = 0; i < path.length(); i++) {
		if (path[i] == '/') {
			path[i] = '\\';
		}
	}
	paths.push_back(path);
	cout << "\nSuccessfull add path " << path << "\n";
}

void listProcess(string s1, string option) {
	printf("\n%10s | %20s | %10s\n", "pId", "Name", "Status");
	DWORD exitcode;
	vector<string> terminated_process_ids;
	for (auto &e : processes) {
		GetExitCodeProcess(e.second.pi.hProcess, &exitcode);
		if (exitcode == STILL_ACTIVE) {
			printf("%10s | %20s | %10s\n", e.first.c_str(), e.second.name.c_str(),
				e.second.status.c_str());
		}
		else terminated_process_ids.push_back(e.first);
	}
	for (string id : terminated_process_ids) {
		processes.erase(id);
	}
}

void killProcess(string id, string option) {
	if (processes.count(id)) {
		TerminateProcess(processes[id].pi.hProcess, 0);
		processes.erase(id);
		if (atoi(id.c_str()) == foreground_process_id) foreground_process_id = 0;
	}
}

void suspendProcess(string id, string option) {
	if (processes.count(id) ) {
		if (SuspendThread(processes[id].pi.hThread) != -1) {
			processes[id].status = "Suspending";
		}
	} else cout << "\nCan not find process with id '" << id << "' \n";
}

void resumeProcess(string id, string option) {
	if (processes.count(id)) ResumeThread(processes[id].pi.hThread);
	else cout << "\nCan not find process with id '" << id << "' \n";
}

DWORD WINAPI checkForegroundProcessTerminated(LPVOID lpParam) {
	while (1) {
		DWORD exitcode;
		GetExitCodeProcess(processes[to_string(foreground_process_id)].pi.hProcess, &exitcode);
		if (exitcode != STILL_ACTIVE) {
			processes.erase(to_string(foreground_process_id));
			foreground_process_id = 0;
			printCurrentDir();
			return 0;
		}
		Sleep(100);
	}
	return 0;
}

void startProcess(string name, string option) {
	string full_path = findProgramInPaths(name);
	process *new_process = new process{ name, "Running" };
	int pr = CreateProcess(full_path.c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&(new_process->si),
		&(new_process->pi)
	);
	if (pr) {
		int id = new_process->pi.dwProcessId;
		processes[to_string(id)] = *new_process;
		if (option == "&") { // run background
			cout << "\n" << name << " is running background\n";
		}
		else { // run foreground
			cout << "\n" << name << " is running foreground\nPress Ctrl+C to stop\n";
			foreground_process_id = id;
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) checkForegroundProcessTerminated, NULL, 0, NULL);
		}
	}
	else cout << "\nError: command \"" << name << "\" not found\n";
}

DWORD WINAPI readAndRunFile(LPVOID lpParam) {
	string name = *((string*) lpParam);
	string full_path = findProgramInPaths(name);
	if (!full_path.length()) {
		running_batch_file = false;
		cout << "\nCan't find " << name << " in any of known paths\n";
		return 0;
	}
	ifstream file(full_path);
	string line;
	cout << "\nExecuting batch file" << "\n";
	while (getline(file, line)) {
		cout << "> " << line << "\n";
		handling(line);
		while (foreground_process_id) Sleep(50);
	}
	running_batch_file = false;
	file.close();
	return 0;
}

void startBatFile(string name, string option) {
	running_batch_file = true;
	string *file_name = new string(name);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) readAndRunFile, file_name, 0, NULL);
}

int main() {
	initializeInstructions();
	initializePath();
	printCurrentDir();
	string command;

	DWORD dwMode = 0x0;
	GetConsoleMode(hInput, &dwMode);
	// Remove ENABLE_PROCESSED_INPUT flag
	dwMode &= ~ENABLE_PROCESSED_INPUT;
	SetConsoleMode(hInput, dwMode);
	// Ctrl C can be read using ReadConsoleInput

	DWORD NumInputs = 0;
	DWORD InputsRead = 0;
	INPUT_RECORD irInput;
	WCHAR char_input;
	while (1) {
		GetNumberOfConsoleInputEvents(hInput, &NumInputs);
		ReadConsoleInput(hInput, &irInput, 1, &InputsRead);
		if (irInput.EventType == KEY_EVENT && irInput.Event.KeyEvent.bKeyDown) {
			char_input = irInput.Event.KeyEvent.uChar.UnicodeChar;
			switch (irInput.Event.KeyEvent.wVirtualKeyCode) {
			case VK_LEFT:
				if (foreground_process_id) break;
				cursorDec();
				break;
			case VK_RIGHT:
				if (foreground_process_id) break;
				cursorInc();
				break;
			case VK_RETURN:
				if (foreground_process_id) break;
				if(command.length()) handling(command);
				command = "\0";
				if (!foreground_process_id && !running_batch_file) printCurrentDir();
				break;
			case VK_BACK:
				if (foreground_process_id) break;
				if (cursorDec()) {
					cout << " \b";
					command.pop_back();
				}
				break;
			case VK_SHIFT:
			case VK_UP:
			case VK_DOWN:
				break;
			default:
				if (char_input == 3) { // Ctrl + C
					if (foreground_process_id) {
						cout << "Closing..." << "\n";
						killProcess(to_string(foreground_process_id), "");
						if (running_batch_file) Sleep(50);
						if (!running_batch_file) printCurrentDir();
					}
				}
				else {
					if (foreground_process_id) break;
					command += char_input;
					printf("%c", char_input);
				}
			}
		}
	}
	return 0;
}
