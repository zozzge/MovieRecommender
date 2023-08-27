
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>

#include<vector>
#include<unordered_map>
#include <algorithm>
#include <windows.h>

using namespace std;


struct similarity_rate {
	double similarity;
	double rating;
};

unordered_map<int, int> mapTopUsers;								
unordered_map<int, int> mapTopMovies;								

unordered_map<int, string> mapMovies;								
unordered_map<int, vector<double>> mapUserMovieRatingsCache;		

unordered_map<int, vector<int>> mapMovieUser;						
unordered_map<int, unordered_map<int, double>> mapUserMovieRating;	

bool isNumber(std::string& str)
{
	for (char const& c : str) {
		if (std::isdigit(c) == 0)
			return false;
	}
	return true;
}

double CalculateCosineSimilarity(vector<double> vecX, vector<double> vecY)
{
	double sumOfX = 0;
	double sumOfY = 0;
	double sumOfXY = 0;

	for (int i = 0; i < vecX.size(); i++) {
		sumOfXY += vecX[i] * vecY[i];
		sumOfX += vecX[i] * vecX[i];
		sumOfY += vecY[i] * vecY[i];
	}

	return sumOfXY / (sqrt(sumOfX) * sqrt(sumOfY));
}

vector<double> GetUserMovieRatings(int aUserId)
{
	vector<double> userMovieRatings;

	// Try to find User in cache. If it exists reuse it...
	if (mapUserMovieRatingsCache.find(aUserId) != mapUserMovieRatingsCache.end()) {
		userMovieRatings = mapUserMovieRatingsCache[aUserId];
	}
	else {
		for (auto& movie_it : mapMovies) {
			int movie_id = movie_it.first;
			double rating = 0;

			if (mapUserMovieRating.find(aUserId) != mapUserMovieRating.end()) {
				unordered_map<int, double> rm = mapUserMovieRating[aUserId];

				if (rm.find(movie_id) != rm.end())
					rating = rm[movie_id];
			}

			userMovieRatings.push_back(rating);
		}
		mapUserMovieRatingsCache[aUserId] = userMovieRatings;
	}

	return userMovieRatings;
}

std::string GetCurrentDirectory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");

	return std::string(buffer).substr(0, pos);
}

int main(int argc, char* argv[])
{

	string appPath = GetCurrentDirectory();

	std::string filePathTrain = "C:\\Users\\Zeynep\\source\\repos\\movieRecommender_1\\Debug\\train.txt";
	std::string filePathTest = "C:\\Users\\Zeynep\\source\\repos\\movieRecommender_1\\Debug\\test.txt";


	std::cout << "Loading datas. Please wait..." << std::endl;

	int kUsers = 10;

	std::string fileLine;
	std::ifstream fileDataSet(filePathTrain);

	auto start = chrono::steady_clock::now();

	// Load dataset into maps...
	while (std::getline(fileDataSet, fileLine)) {

		std::stringstream ss(fileLine);

		std::string strUserId, strMovieId, strRating;
		std::getline(ss, strUserId, ',');
		std::getline(ss, strMovieId, ',');
		std::getline(ss, strRating, ',');

		if (isNumber(strUserId) && isNumber(strMovieId)) {
			int userId = stoi(strUserId);
			int movieId = stoi(strMovieId);
			double rating = stof(strRating);

			mapMovieUser[movieId].push_back(userId);
			mapUserMovieRating[userId][movieId] = rating;
			mapMovies[movieId] = "movie #" + std::to_string(movieId);

			mapTopUsers[userId]++;
			mapTopMovies[movieId]++;
		}
	}

	// Put Top Users to vector and sort descending ...
	vector<pair<int, int>>  vecTopUsers(mapTopUsers.begin(), mapTopUsers.end());
	sort(vecTopUsers.begin(), vecTopUsers.end(), [](auto& left, auto& right) { return left.second > right.second; });

	cout << "Top Users:\n";
	for (int i = 0; i < 10; i++)
		cout << "User " << vecTopUsers[i].first << " -> " << vecTopUsers[i].second << std::endl;
	cout << std::endl;


	// Put Top Movies to vector and sort descending ...
	vector<pair<int, int>>  vecTopMovies(mapTopMovies.begin(), mapTopMovies.end());
	sort(vecTopMovies.begin(), vecTopMovies.end(), [](auto& left, auto& right) { return left.second > right.second; });

	cout << "Top Movies:\n";
	for (int i = 0; i < 10; i++)
		cout << "Movie " << vecTopMovies[i].first << " -> " << vecTopMovies[i].second << std::endl;
	cout << std::endl;

	auto end = chrono::steady_clock::now();
	std::cout << "Elapsed time in seconds: " << chrono::duration_cast<chrono::seconds>(end - start).count() << " sec" << std::endl;
	std::cout << "------------------------------" << std::endl;
	std::cout << std::endl;



	ofstream filePrediction(appPath + "\\prediction.txt");
	filePrediction << "Id,Predicted\n";

	std::cout << "Id, Predicted" << std::endl;


	std::ifstream fileTestDataSet(filePathTest);

	// Train Movie Ratings ...
	while (std::getline(fileTestDataSet, fileLine)) {

		std::stringstream ss(fileLine);

		std::string strId, strUserId, strMovieId;
		std::getline(ss, strId, ',');
		std::getline(ss, strUserId, ',');
		std::getline(ss, strMovieId, ',');

		if (isNumber(strUserId) && isNumber(strMovieId)) {
			int predictUserId = stoi(strUserId);
			int predictMovieId = stoi(strMovieId);

			auto start = chrono::steady_clock::now();

			// Build rating map for all movies that rated by user ...
			vector<double> predictUserMovieRatings = GetUserMovieRatings(predictUserId);

			// Calculate similarity
			vector<similarity_rate> similarities;

			// Iterate all users that rated the movie before for similarity calculation ...
			for (int i = 0; i < mapMovieUser[predictMovieId].size(); i++) {
				int calculateUserId = mapMovieUser[predictMovieId][i];
				double rating = mapUserMovieRating[calculateUserId][predictMovieId];

				// Build rating map for all movies that rated by user ...
				vector<double> userMovieRatings = GetUserMovieRatings(calculateUserId);

				// Calculate similarity between to user based on ratings ...
				double similarity = CalculateCosineSimilarity(predictUserMovieRatings, userMovieRatings);

				similarity_rate sr;
				sr.similarity = similarity;
				sr.rating = rating;

				similarities.push_back(sr);

				
			}

			// Sort most similar to less...
			sort(similarities.begin(),
				similarities.end(),
				[](similarity_rate a, similarity_rate b) {
					return a.similarity > b.similarity;
				}
			);

			// Calculate airthmethic average for top K users ...
			double totalRating = 0;
			int similarityLen = kUsers;
			if (similarities.size() < kUsers)
				similarityLen = similarities.size();
			for (int i = 0; i < similarityLen; i++) {
				totalRating += similarities[i].rating;
			}

			double calculatedRating = totalRating / kUsers;

			filePrediction << strId << ", " << calculatedRating << std::endl;
			std::cout << strId << ", " << calculatedRating << std::endl;

			auto end = chrono::steady_clock::now();
			std::cout << "Elapsed time in seconds: " << chrono::duration_cast<chrono::seconds>(end - start).count() << " sec" << std::endl;

		}
	}

	filePrediction.close();

}

