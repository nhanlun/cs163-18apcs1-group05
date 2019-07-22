#include "Search.h"

Search::Search()
{
	system("md Process");
	system("md Data");

	if (!LoadSynonym())
		std::cerr << "Can't open synonym file\n";
	if (!LoadStopWord())
		std::cerr << "Can't open stop word file\n";

	if (!LoadListOfFile())
	{
		CreateIndex();
		trie.SaveTrie();
		numIndex.SaveNumIndex();
		SaveListOfFile();
	}
	else
	{
#ifdef CalcTime
		auto startTime = clock();
#endif
		numIndex.LoadNumIndex();
		trie.LoadTrie();
#ifdef CalcTime
		auto endTime = clock();
		std::cerr << "Load Trie and numIndex Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s\n";
#endif
	}
}

Search::~Search()
{
}

bool Search::IsStopWord(const std::string& word)
{
	if (stopWord.count(word) != 0) return true;
	return false;
}

bool Search::IsDelimeter(const char& c)
{
	if (delimeter.count(c) != 0) return true;
	else return false;
}

bool Search::IsWeirdWord(const std::string& word)
{
	for (auto c : word)
	{
		if (c < 0 || c > 255) return true;
	}
	return false;
}

bool Search::LoadSynonym()
{
	std::ifstream in("Process\\synonym.txt");
	if (!in.is_open()) return false;
	std::string word;
	while (std::getline(in, word) && word != "")
	{
		std::string listOfWord;
		std::getline(in, listOfWord);
		std::stringstream ss(listOfWord);

		std::string wordInList;
		while (ss >> wordInList) synonym[word].push_back(wordInList);
	}
	in.close();
	return true;
}

void Search::Run()
{
}

void Search::ReadSingleFile(const std::string & fileName, std::vector<std::string>& tokenVector)
{
	std::ifstream inFile(fileName);
	if (inFile.is_open()) {
		std::string fileData;
		//read everything into string
		std::string token;
		std::set<std::string> tokenSet;
		while (inFile >> token)
		{
			while ((int)token.size() > 0 && IsDelimeter(token[0]))
				token.erase(0, 1);
			while ((int)token.size() > 0 && IsDelimeter(token.back()))
				token.pop_back();
			if (IsStopWord(token)) continue;
			if (!token.empty())
				tokenSet.insert(token);
		}
		
		for (auto i : tokenSet) tokenVector.push_back(i);
	}
	else
		std::cout << "File " << fileName << " is not found\n";
	inFile.close();
}

void Search::GetFilename(const std::string rootDirectory, std::vector<std::string>& pathVector)
{
	std::stringstream ss;
	for (auto & entry : std::experimental::filesystem::directory_iterator(rootDirectory)) {
		ss << entry.path();
		std::string token;
		std::string path;
		while (ss >> token)
			path += token + " ";
		path.pop_back();
		pathVector.push_back(path);
		ss.clear();
	}
	return;
}

bool Search::LoadStopWord()
{
	std::ifstream fin;
	fin.open("Process/stopword.txt");
	if (!fin.is_open()) return false;
	std::string word;
	while (!fin.eof())
	{
		fin >> word;
		stopWord.insert(word);
	}
	fin.close();
	return true;
}

std::vector<std::string> Search::RemoveStopWord(const std::vector<std::string>& words)
{
	std::vector<std::string> afterRemove;
	afterRemove.clear();

	std::string word;

	for (int i = 0; i <(int) words.size(); ++i)
	{
		word = words[i];
		Tolower(word);
		if (stopWord.find(word) == stopWord.end())
			afterRemove.push_back(word);
	}
	return afterRemove;
}

std::vector<std::string> Search::RemoveWeirdWord(const std::vector<std::string>& words)
{
	std::vector<std::string> answer;
	for (auto word : words)
	{
		if (!IsWeirdWord(word)) answer.push_back(word);
	}
	return answer;
}


bool Search::CreateIndex()
{
#ifdef CalcTime
	auto startTime = clock();
#endif
	GetFilename("Data", theFullListOfFile);

	if (theFullListOfFile.empty())
		return false;

	int cnt = 0;
	for (auto i : theFullListOfFile)
	{
		std::vector<std::string> wordsInFile;
		wordsInFile.clear();
		ReadSingleFile(i, wordsInFile);

		//if (wordsInFile.empty())
		//{
		//	std::cout << "Can't load file\n";
		//	//return false;
		//}

		for (int j = 0; j < (int)wordsInFile.size(); ++j)
		{
			if (isNumberWithChar(wordsInFile[j]))
			{
				double val = stod(wordsInFile[j]);
				numIndex.AddNum(val, cnt);
			}
			else
			{
				if (!wordsInFile[j].empty()) trie.AddKey(wordsInFile[j], cnt);
			}
		}
		++cnt;
	}
#ifdef CalcTime
	auto endTime = clock();
	std::cerr << "Build Trie Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s\n";
#endif
	return true;
}

bool Search::LoadListOfFile()
{
	std::ifstream in("Process\\filename.txt");
	if (!in.is_open()) return false;
	int total;
	in >> total;
	std::string filename;
	std::getline(in, filename);
	for (int i = 0; i < total; ++i)
	{
		std::getline(in, filename);
		theFullListOfFile.push_back(filename);
	}
	in.close();
	return true;
}

void Search::SaveListOfFile()
{
	std::ofstream ou("Process\\filename.txt");
	ou << theFullListOfFile.size() << '\n';
	for (auto name : theFullListOfFile)
		ou << name << '\n';
	ou.close();
}


std::string Search::InputKey(int x, int y) {
	int key, len = 0;
	int moveCursor = -1;
	std::string resultStr;
	std::vector<std::string> lsHis;
	History h;

	Gotoxy(x, y);
	key = _getch();
	while (key != 13) {
		if (key == 8 && len > 0)
		{
			moveCursor = -1;
			--len;
			resultStr.pop_back();
			if (len > 110)
				OutOfRange(resultStr);
			else
				std::cout << "\b \b";
			lsHis=DisplayHistory(h.GetHistory(resultStr));
		}
		else if (key != 0 && key != 224 && key != 8)
		{
			moveCursor = -1;
			resultStr += (char)key;
			if (len > 110)
				OutOfRange(resultStr);
			else
				std::cout << (char)key;
			++len;
			if (resultStr != " ")
				lsHis=DisplayHistory(h.GetHistory(resultStr));
		}
		else if (key == 0 || key == 224)
		{
			int ex = _getch();
			int noHistory = lsHis.size();
			if (noHistory > 0) {
				if (ex == 72 && moveCursor > -1) {
					Gotoxy(x, y + moveCursor + 2);
					std::cout << lsHis[moveCursor--];
					Color(112);
					if (moveCursor != -1) {
						Gotoxy(x, y + moveCursor + 2);
						std::cout << lsHis[moveCursor];
					}
				}
				else if (ex == 80 && moveCursor<noHistory-1) {
					if (moveCursor != -1) {
						Gotoxy(x, y + moveCursor + 2);
						std::cout << lsHis[moveCursor];
					}
					Color(112);
					Gotoxy(x, y + ++moveCursor + 2);
					std::cout << lsHis[moveCursor];
				}
				Color(240);
			}
		}
		if (len < 111)
			Gotoxy(x + len, y);
		else
			Gotoxy(x + 111, y);
		key = _getch();
	}

	if (moveCursor != -1) {
		resultStr = lsHis[moveCursor];
		Gotoxy(x, y);
		std::cout << resultStr;
		for (int j = resultStr.length(); j < 113; ++j)
			std::cout << " ";
	}
	h.Add(resultStr);

	return resultStr;
}

//Extract command and split into smaller queries
std::string Search::InfixToPostfix(const std::string & query)
{

	std::string output;
	output.clear();
	std::string newquery = PreProcess(query);

	std::stringstream ss(newquery);
	std::string token, subquery;
	std::stack<std::string> st;

	while (ss >> token) {
		if (token == "AND" || token == "OR") {
			if (subquery.size()) {
				subquery.pop_back();
				subquery += ",";
			}
			output += subquery;
			subquery.clear();
			while (st.size() && st.top() != "(") {
				output += st.top() + ",";
				st.pop();
			}
			st.push(token);
		}
		else if (IsOpenBracket(token)) {
			st.push("(");
			token.erase(token.begin());
			if (token.size()) {
				token += " ";
			}
			subquery += token;
		}
		else if (IsCloseBracket(token)) {
			token.erase(token.end() - 1);
			subquery += token;
			if (subquery.size()) {
				subquery.pop_back();
				output += subquery + ",";
			}
			subquery.clear();
			while (st.size() && st.top() != "(") {
				output += st.top() + ",";
				st.pop();
			}
			st.pop();//pop the string "("
		}
		else if (IsExactQuery(token)) {//If it is exact query get everything between "" and add it to subquery 
			do {
				subquery += token + " ";
			} while (ss >> token && !IsExactQuery(token));
			subquery += token;
			output += subquery + ",";
			subquery.clear();
		}
		else {
			subquery += token + " ";
		}
	}

	while (st.size()) {
		output += st.top() + ",";
		st.pop();
	}
	output.pop_back();
	return output;
}

std::string Search::PreProcess(const std::string & query)
{
	std::string output(query);
	int bracket = 0, quote = 0;
	//open bracket +1, close bracket -1
	//open quote +1, close quote 0;
	for (int i = 0; i < (int)output.size(); i++) {
		if (output[i] == '(') {
			++bracket;
			output.insert(output.begin() + i + 1, ' ');
		}
		else if (output[i] == ')') {
			--bracket;
			output.insert(output.begin() + i, ' ');
			++i;
		}
		else if (output[i] == '\"') {
			quote = 1 - quote;
		}
	}
	for (int i = 0; i < bracket; i++) {
		output += " )";
	}
	if (quote == 1) {
		int i = 0;
		while (i < (int)output.size() && output[i] != ')')
			++i;
		output.insert(output.begin() + i, '\"');
	}
	return output;
}

bool Search::IsExactQuery(const std::string & query)
{
	if (query[0] == '\"' || query[query.size() - 1] == '\"')
		return true;
	return false;
}

bool Search::IsOpenBracket(const std::string & query)
{
	if (query[0] == '(')
		return true;
	return false;
}

bool Search::IsCloseBracket(const std::string & query)
{
	if (query[query.size() - 1] == ')')
		return true;
	return false;
}

void Search::SearchExact(std::string &str) {
	std::vector<std::string> words = splitSentence(str);
	std::vector<int> res;
	res.clear();
	if (!words.empty()) {
		res = trie.GetKey(words[0]);
		std::map<int, int> mp;
		for (std::vector<int>::iterator i = res.begin(); i != res.end();++i)
			mp[*i] = 1;
		for (std::vector<std::string>::iterator it = words.begin() + 1; it != words.end(); ++it) {
			res = trie.GetKey(*it);
			AddToMap(res, mp);
		}
		res.clear();
		int sz = words.size();
		for (std::map<int, int>::iterator it = mp.begin(); it != mp.end(); ++it)
			if (it->second == sz)
				res.push_back(it->first);
	}




}




