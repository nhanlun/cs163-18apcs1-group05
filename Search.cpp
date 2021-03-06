#include "Search.h"

Search::Search()
{
	system("md Process");
	system("md Data");
	system("md NewData");

#ifdef CalcTime
	auto startTime = clock();
#endif
	if (!LoadSynonym())
		std::cerr << "Can't open synonym file\n";
	if (!LoadStopWord())
		std::cerr << "Can't open stop word file\n";

	bool loadListOfFile = LoadListOfFile();
	bool loadTrie = trie.LoadTrie();
	bool loadNumIndex = numIndex.LoadNumIndex();
#ifdef CalcTime
	auto endTime = clock();
	std::cerr << "Load Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << '\n';
#endif

	if (!(loadListOfFile && loadTrie && loadNumIndex))
	{
#ifdef CalcTime
		auto startTime = clock();
#endif
		CreateIndex();
		SaveListOfFile();
		trie.SaveTrie();
		numIndex.SaveNumIndex();
#ifdef CalcTime
		auto endTime = clock();
		std::cerr << "Build Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << '\n';
#endif
	}

	if (CreateIndexForNewFile())
	{
#ifdef CalcTime
		auto startTime = clock();
#endif
		SaveListOfFile();
		trie.SaveTrie();
		numIndex.SaveNumIndex();
#ifdef CalcTime
		auto endTime = clock();
		std::cerr << "Add New File Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s\n";
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
	FrontEnd();
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

	std::string query = "";
	while (InputKey(query))
	{
		std::string processedQuery = InfixToPostfix(query);
		//std::cout << "May qua no thoat roi chu khong la no bay mau cmnr" << "\n";
			
		if (!processedQuery.empty())
		{
			std::vector<int> result = Process(processedQuery);

			std::vector<Document> docs;
			int total = min((int)result.size(), 5);
			if (total == 0)
				NoResult();
			else
			{
				std::vector <std::string> content, title;
				std::vector <std::string> phrases;
				SplitQuery(query, title, content);
				//title is vector contains all words need to be displayed in the title
				//contente is the vector contains all words need to be displayed in the content
				//phrases are the union of two above vectors
				OR(phrases, content);
				OR(phrases, title);
				OR(title, content);

				result = Ranking(result, phrases);

				if (!result.empty())
				{
					for (int i = 0; i < total; ++i)
						docs.push_back(Document(theFullListOfFile[result[i]]));


					for (auto& doc : docs)
					{
						doc.ReadFile();
						doc.getWordsIntitle(title);
						doc.GetParagraphForShowing(phrases);
					}

					while(true)
					{
						int x = 20, y = 31, step = 2;
						std::vector<int> cor;
						cor.clear();
						for (auto& doc : docs)
						{
							cor.push_back(y);
							doc.DisplayResult(x, y);
							y += step;
						}

						int chosen = ChooseLink(total, cor);
						if (chosen != -1) {
							docs[chosen].DisplayFile();
							_getch();
						}
						if (chosen == -1)
							break;
						system("cls");
						SearchScreen();
						Gotoxy(28, 21);
						std::cout << query;
					}
				}
				else
					NoResult();
			}
		}
		SearchScreen();
		query.clear();
	}

	ExitScreen();
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
			Tolower(token);
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

bool Search::CreateIndexForNewFile()
{
	std::vector<std::string> listOfNewFile;
	GetFilename("NewData", listOfNewFile);

	if (listOfNewFile.empty())
		return false;

	int cnt = (int)theFullListOfFile.size();

	for (auto& i : listOfNewFile)
	{
		i.erase(0, 8);
		std::string command = "move NewData\\" + i + " Data\\" + i;
		system(command.c_str());
		i = "Data\\" + i;
		theFullListOfFile.push_back(i);
	}

	for (auto i : listOfNewFile)
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


bool Search::InputKey(std::string &resultStr) {
	int x = 28, y = 21;
	int key, len = 0;
	int moveCursor = -1;
	std::vector<std::string> lsHis;
	History h;

	Gotoxy(x, y);
	key = _getch();
	while (key != 13 || len == 0 && moveCursor == -1) 
	{
		if (key == 27)
			return false;
		if (key == 8 && len > 0)
		{
			moveCursor = -1;
			--len;
			resultStr.pop_back();
			if (len > 110)
				OutOfRange(resultStr);
			else
				std::cout << "\b \b";
			lsHis = DisplayHistory(h.GetHistory(resultStr));
		}
		else if (key != 0 && key != 224 && key != 8 && key != 13)
		{
			moveCursor = -1;
			resultStr += (char)key;
			if (len > 110)
				OutOfRange(resultStr);
			else
				std::cout << (char)key;
			++len;
			if (resultStr != " ")
				lsHis = DisplayHistory(h.GetHistory(resultStr));
		}
		else if (key == 0 || key == 224)
		{
			int ex = _getch();
			int noHistory = (int)lsHis.size();
			if (noHistory > 0) {
				Color(8);
				if (ex == 72 && moveCursor > -1) {
					Gotoxy(x, y + moveCursor + 2);
					std::cout << lsHis[moveCursor--];
					Color(6);
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
					Color(6);
					Gotoxy(x, y + ++moveCursor + 2);
					std::cout << lsHis[moveCursor];
				}
				Color(15);
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
		for (int j = (int)resultStr.length(); j < 113; ++j)
			std::cout << " ";
	}
	h.Add(resultStr);

	return true;
}

void Search::GetFileNameByInt(const std::vector<int>& toGet, std::vector<std::string>& fileName)
{
	fileName.clear();
	for (auto i : toGet)
	{
		std::string file = theFullListOfFile[i];
		fileName.push_back(file);
	}
}

void Search::Debug(std::vector<int> &v)
{
	for (auto i : v) {
		std::cout << theFullListOfFile[i] << std::endl;
	}
	return;
}

std::vector<int> Search::Ranking(std::vector<int>& finalList, std::vector<std::string>& subQuery)
{
	std::set<int> fileList;
	AddToSet(finalList, fileList);

	std::vector<int> result;
	std::map<int, int> rank;

	for (auto& query : subQuery)
	{
		std::vector<std::string> words = splitSentence(query);
		for (auto& word : words)
			Tolower(word);
		words = RemoveStopWord(words);

		for (auto word : words)
		{
			if (!isNumberWithChar(word))
			{
				result = trie.GetKey(word);
				for (auto& fileIndex : result)
					if (fileList.count(fileIndex) != 0)
						++rank[fileIndex];
			}
			else
			{
				double num = stod(word);
				result.clear();
				bool haveNum = SearchNumber(num, result);
				if (haveNum)
				{
					for (auto& fileIndex : result)
						if (fileList.count(fileIndex) != 0)
							++rank[fileIndex];
				}
			}
		}
	}

	std::vector<std::pair<int, int> > score;
	for (auto& fileAndScore : rank)
	{
		int fileIndex = fileAndScore.first;
		int numberOfOccurence = fileAndScore.second;
		score.push_back(std::make_pair(numberOfOccurence, fileIndex));
	}
	std::sort(score.begin(), score.end(), std::greater<std::pair<int, int> >());

	result.clear();
	for (auto i : score)
		result.push_back(i.second);

	return result;
}

void Search::Test100Query()
{
	std::ifstream in("Process\\test_100_query.txt");
	if (!in)
		return;
	std::string query;
	auto startTime = clock();
	while (std::getline(in, query))
	{
		std::string processedQuery = InfixToPostfix(query);

		if (!processedQuery.empty())
		{
			std::vector<int> result = Process(processedQuery);

			std::vector<Document> docs;
			int total = min((int)result.size(), 5);
			if (total == 0)
				//NoResult();
			{
				//std::cout << query << " No result\n";
			}
			else
			{
				std::vector <std::string> content, title;
				std::vector <std::string> phrases;
				SplitQuery(query, title, content);
				//title is vector contains all words need to be displayed in the title
				//contente is the vector contains all words need to be displayed in the content
				//phrases are the union of two above vectors
				OR(phrases, content);
				OR(phrases, title);
				OR(title, content);

				result = Ranking(result, phrases);

				if (!result.empty())
				{
					for (int i = 0; i < total; ++i)
						docs.push_back(Document(theFullListOfFile[result[i]]));


					for (auto& doc : docs)
					{
						doc.ReadFile();
						doc.getWordsIntitle(title);
						doc.GetParagraphForShowing(phrases);
					}

					int x = 20, y = 31, step = 2;
					std::vector<int> cor;
					cor.clear();
					for (auto& doc : docs)
					{
						cor.push_back(y);
						doc.DisplayResult(x, y);
						y += step;
					}
				}
				else
				{
					//NoResult();
					std::cout << query << " No result\n";
				}
			}
		}
		SearchScreen();
		query.clear();
	}
	auto endTime = clock();
	in.close();
	std::cout << "Total Time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << '\n';
	system("pause");
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
			subquery += token + ' ';
			while (token.back() != '\"')
			{
				ss >> token;
				subquery += token + ' ';
			}
			subquery.pop_back();
			/*do 
			{
				subquery += token + " ";
			} while (ss >> token && !IsExactQuery(token));*/
			/*if (token[0] == '\"') 
			{
				subquery.pop_back();
			}
			else 
				subquery += token;*/
			output += subquery + ",";
			subquery.clear();
		}
		else {
			subquery += token + " ";
		}
	}

	if (subquery.size()) {
		subquery.pop_back();
		output += subquery + ",";
	}

	while (st.size()) {
		output += st.top() + ",";
		st.pop();
	}
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
		int i = (int)output.size() - 1;
		for (i; i >= 0; i--) {
			if (output[i] == '\"')
				break;
		}
		std::string fakeoutput(output, i, std::string::npos);
		std::stringstream iss(fakeoutput);
		fakeoutput.clear();
		std::string token;
		while (iss >> token) {
			if (token == "AND" || token == "OR") {
				break;
			}
			fakeoutput += token + " ";
		}

		fakeoutput.pop_back();
		int insertPos = (int)fakeoutput.size() + i;
		output.insert(output.begin() + insertPos, '\"');
	}

	return output;
}

void Search::SplitQuery(const std::string& query, std::vector<std::string> &intitle, std::vector<std::string> &content)
{
	std::string tmp = InfixToPostfix(query);
	std::stringstream ss(tmp);
	std::string subquery;
	while (std::getline(ss, subquery, ',')) {
		TrimQuery(subquery, intitle, content);
	// std::vector <std::string> output;
	// output.clear();
	// std::string newquery = PreProcess(query);

	// std::stringstream ss(newquery);
	// std::string token, subquery;
	// std::stack<std::string> st;

	// while (ss >> token) {
	// 	if (token == "AND" || token == "OR") {

// 			/*subquery = token;

// 			if (subquery.size()) {
// 				subquery.pop_back();
// 			}
// 			TrimQuery(subquery);
// 			output.push_back(subquery);
// 			subquery.clear();*/
// 		}
// 		else if (IsOpenBracket(token)) {
// 			token.erase(token.begin());
// 			if (token.size()) {
// 				token += " ";
// 			}
// 			subquery += token;
// 		}
// 		else if (IsCloseBracket(token)) {
// 			token.erase(token.end() - 1);
// 			subquery += token;
// 			if (subquery.size()) {
// 				subquery.pop_back();
// 				TrimQuery(subquery);
// 				output.push_back(subquery);
// 			}
// 			subquery.clear();
// 		}
// 		else if (IsExactQuery(token)) {//If it is exact query get everything between "" and add it to subquery 
// 			subquery += token + ' ';
// 			while (token.back() != '\"')
// 			{
// 				ss >> token;
// 				subquery += token + ' ';
// 			}
// 			subquery.pop_back();
// 			/*do {
// 				subquery += token + " ";
// 			} while (ss >> token && !IsExactQuery(token));
// 			if (token[0] == '\"') {
// 				subquery.pop_back();
// 			}
// 			else
// 				subquery += token;*/
// 			subquery.erase(0, 1);
// 			subquery.pop_back();
// 			output.push_back(subquery) ;
// 			subquery.clear();
// 		}
// 		else {
// 			subquery += token + " ";
// 		}
// 	}

// 	if (subquery.size()) {
// 		subquery.pop_back();
// 		TrimQuery(subquery);
// 		output.push_back(subquery);
	}
}

void Search::TrimQuery(std::string &query, std::vector <std::string> &intitle, std::vector <std::string> &content)
{
	std::stringstream ss(query);
	query.clear();
	std::string token;
	while (ss >> token) {
		for (int i = 0; i < (int)token.size(); i++) {
			if (token[i] == '$' || token[i] == '%')
				token.erase(i, 1);
		}
		query += token + " ";
	}
	if (query.size() && query.back() == ' ')
		query.pop_back();
	if (IsRangeQuery(query)) {
		//int i = (int)query.find("..");
		//if (i != query.npos)
		//	query.erase(i, 2);
		//Something big is missing lol :v
		double lo, hi;
		PreProcessRangeQuery(query, lo, hi);
		numIndex.GetValueInRange(lo, hi, content);
	}
	else if (IsPlusQuery(query)) {
		int i = (int)query.find("+");
		if (i != query.npos)
			query.erase(i, 1);
		content.push_back(query);
	}
	else if (IsMinusQuery(query)) {
		int i = (int)query.find("-");
		if (i != query.npos)
			query.erase(i);//erase from the - character till end
		if (query.back() == ' ')
			query.pop_back();
		content.push_back(query);
	}
	else if (IsExactQuery(query)) {
		query.pop_back();
		query.erase(0, 1);
		content.push_back(query);
	}
	else if (IsIntitleQuery(query)) {
		query.erase(0, 8);
		intitle.push_back(query);
	}
	else if (IsPlaceHolderQuery(query)) {
		int i = query.find(" * ");
		if (i != query.npos) {
			std::string left(query, 0, i);
			if (i + 3 < (int)query.size()) {
				std::string right(query.begin() + i + 3, query.end());
				content.push_back(left);
				content.push_back(right);
			}
		}
	}
	else if (query[0] == '~') {
		query.erase(0, 1);
		if (synonym.count(query)) {
			for (auto syn : synonym[query]) {
				content.push_back(syn);
			}
		}
		content.push_back(query);
	}
	else if (query != "AND" && query != "OR")
		content.push_back(query);
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

bool Search::IsIntitleQuery(const std::string & query)
{
	std::string cmp(query, 0, 8);
	return !cmp.compare("Intitle:");
}

bool Search::IsPlaceHolderQuery(const std::string & query)
{
	return (query.find(" * ") != query.npos);
}

bool Search::IsPlusQuery(const std::string & query)
{
	return query.find(" +") != query.npos;
}

bool Search::IsMinusQuery(const std::string & query)
{
	int i = (int)query.find(" -");
	return (i != query.npos && i+1 < (int)query.size() && query[i + 1] != ' ');
}

bool Search::IsRangeQuery(const std::string & query)
{
	return (query.find("..") != query.npos);
}

std::vector<int> Search::SearchExact(const std::string &phrase) {
	std::vector<std::string> words = splitSentence(phrase);
	std::vector<int> res;
	res.clear();

	if (words.empty()) return res;

	for (auto& word : words) 
		Tolower(word);
	words = RemoveStopWord(words);
	std::map<int, int> mp;

	for (auto word : words) 
	{
		if (!isNumberWithChar(word))
		{
			res = trie.GetKey(word);
			AddToMap(res, mp);
		}
		else
		{
			double num = stod(word);
			res.clear();
			bool haveNum = SearchNumber(num, res);
			if (haveNum) AddToMap(res, mp);
		}
	}

	res.clear();
	int sz = (int)words.size();
	for (auto i : mp) if (i.second == sz)
	{
		if (HaveExactString(i.first, phrase)) // check if this file has exact string
			res.push_back(i.first);
	}

	return res;
}

std::vector<int> Search::SearchNormal(const std::string & phrase)
{
	std::vector<std::string> words = splitSentence(phrase);
	std::vector<int> res;
	res.clear();

	if (words.empty()) return res;

	for (auto& word : words)
		Tolower(word);
	words = RemoveStopWord(words);

	std::set<int> s;

	for (auto word : words)
	{
		if (!isNumberWithCharExtended(word))
		{
			//Search synonym
			if (word[0] == '~') {
				word.erase(0, 1);
				res = SearchSynonym(word);
				AddToSet(res, s);
			}
			else {
				//normal search
				res = trie.GetKey(word);
				AddToSet(res, s);
			}
		}
		else
		{
			if (IsRangeQuery(word)) {
				double l, r;
				PreProcessRangeQuery(word, l, r);
				SearchRange(l, r, res);
				AddToSet(res, s);
			}
			else{
				while (!isdigit(word[0]))
					word.erase(0, 1);
				while (!isdigit(word[word.size() - 1]))
					word.pop_back();
				double number = stod(word);
				SearchNumber(number, res);
				AddToSet(res, s);
			}
		}
	}
	res.clear();
	for (auto i : s) 
		res.push_back(i);

	return res;

}

bool Search::HaveExactString(const int& pos, const std::string& phrase)
{
	Document doc;
	doc.SetFileName(theFullListOfFile[pos]);
	doc.ReadFile();
	if (doc.SearchForPhraseInContent(phrase) != -1 || doc.SearchForPhraseInTitle(phrase) != -1) return true;
	return false;
}

bool Search::SearchNumber(const double & key,std::vector<int>& result)
{
	numIndex.GetNumKey(result, key);
	if (result.empty())
		return false;
	return true;
	
}

bool Search::SearchRange(const double & key1, const double & key2,std::vector<int>& result)
{
	numIndex.GetRange(result, key1, key2);
	if (result.empty())
		return false;
	return true;
}

void Search::PreProcessRangeQuery(const std::string query, double & lo, double & hi)
{
	int i = (int)query.find("..");
	std::string left(query, 0, i);
	while (!isdigit(left[0]))
		left.erase(0, 1);
	while (!isdigit(left[left.size() - 1]))
		left.erase(left.end()-1);
	lo = std::stod(left);

	std::string right(query, i+2, (int)query.size()-1);
	while (!isdigit(right[0]))
		right.erase(0, 1);
	while (!isdigit(right[right.size() - 1]))
		right.erase(right.end()-1);
	hi = std::stod(right);
	return;
}


std::vector<int> Search::SearchSynonym(const std::string &phrase) {
	std::vector<std::string> syn;
	if (synonym.count(phrase)) 
		syn = synonym[phrase];
	syn.push_back(phrase);

	std::vector<int> res;
	std::set<int> st;
	for (auto i : syn) {
		res = trie.GetKey(i);
		for (auto j : res)
			st.insert(j);
	}
	res.clear();
	for (auto i : st)
		res.push_back(i);
	return res;
}

std::vector<int> Search::SearchPlus(const std::string & phrase)
{
	int i = (int)phrase.find("+");
	std::string left(phrase, 0, i);
	std::string tmp(phrase.begin() + i + 1, phrase.end());
	left += tmp;
	std::vector <int> res = SearchExact(left);
	return res;
}

std::vector<int> Search::SearchMinus(const std::string & phrase)
{
	int i = (int)phrase.find("-");
	std::string left(phrase, 0, i);
	left.pop_back();
	std::vector<int> res = SearchExact(left);
	std::string tmp(phrase.begin() + i + 1, phrase.end());
	left += " " + tmp;
	std::vector<int> complement = SearchExact(left);
	NOT(res, complement);
	return res;
}

std::vector<int> Search::SearchPlaceHolder(const std::string & phrase)
{
	//Extract 2 part of the query
	int i = phrase.find(" * ");
	if (i != phrase.npos) {
		std::string left(phrase, 0, i);
		if (i + 3 < (int)phrase.size()) {
			std::string right(phrase.begin() + i + 3, phrase.end());
			//Search exact 2 strings
			std::vector<int> v1 = SearchExact(left);
			std::vector<int> v2 = SearchExact(right);
			AND(v1, v2);
			return v1;
		}
	}
	else
		return std::vector<int>();
}

std::vector<int> Search::SearchIntitle(const std::string & phrase)
{
	std::vector<int> res;
	std::vector<std::string> words = splitSentence(phrase);
	auto tmp = SearchNormal(phrase);

	std::vector<Document> docVector;
	for (auto i : tmp) {
		docVector.push_back(Document(theFullListOfFile[i]));
	}

	for (int i = 0; i < (int)tmp.size(); i++) {
		docVector[i].ReadFile();
		for (auto word : words) {
			if (docVector[i].SearchForPhraseInTitle(word) != -1) {
				res.push_back(tmp[i]);
				break;
			}
		}
	}
	return res;
}

int Search::SwitchQuery(const std::string & subquery) {
	if (IsExactQuery(subquery))
		return 1;
	if (IsIntitleQuery(subquery))
		return 2;
	if (IsPlaceHolderQuery(subquery))
		return 3;
	if (IsPlusQuery(subquery))
		return 4;
	if (IsMinusQuery(subquery))
		return 5;
	return 0;
}

std::vector <int> Search::Process(const std::string &query) {
	//This function implement stack by using a vector
	//st is a stack of vector <int> 
	std::vector <std::vector <int>> st;
	st.clear();
	std::stringstream ss(query);
	std::string subquery;
	while (getline(ss, subquery, ',')) {
		if (subquery != "AND" && subquery != "OR") {
			int u = SwitchQuery(subquery);
			std::vector <int> res;
			switch (u) {
			case 1://Exact query 
				//Process Exact query here
				subquery.pop_back();
				subquery.erase(0,1);
				res = SearchExact(subquery);
				break;
			case 2://Intitle query
				//Process Intitle query here
				subquery.erase(0, 8);
				res = SearchIntitle(subquery);
				break;
			case 3://Placeholder query
				//Process placeholder here
				res = SearchPlaceHolder(subquery);
				break;
			case 4://Plus query
				//Process plus query
				res = SearchPlus(subquery);
				break;
			case 5://Minus query
				//Process minus query
				res = SearchMinus(subquery);
				break;
			default:
				res = SearchNormal(subquery);
				break;
			}
			st.push_back(res);
		}
		else {
			/*std::vector <int> v1 = st.back();
			st.pop_back();
			std::vector <int> v2 = st.back();
			st.pop_back();*/
			std::vector<int> v1, v2;
			if (!st.empty())
			{
				v1 = st.back();
				st.pop_back();
			}
			if (!st.empty())
			{
				v2 = st.back();
				st.pop_back();
			}
			if (subquery == "AND")
				AND(v1, v2);
			else if (subquery == "OR")
				OR(v1, v2);
			st.push_back(v1);
		}
	}
	if (!st.empty())
		return st.back();
	else
		return std::vector<int>();
}