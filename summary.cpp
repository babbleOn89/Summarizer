#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>

using namespace std;

//CURL helper

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//download webpage

string fetch_url(const string& url) {
    CURL* curl;
    CURLcode res;
    string html;

    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //pretend to be browser
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
            "Mozilla/5.0");
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            cerr << "Curl Error: "
                 << curl_easy_strerror(res)
                 << endl;
        }
        
        curl_easy_cleanup(curl);

    }
    
    return html;
}

//remove html tags

string strip_html(const string& text) {
    return regex_replace(text, regex("<[^>]*>"), "");
}

//remove URLs

string remove_urls(const string& text) {
    return regex_replace(text, regex("http[^\\s]+"), "");
}

//split into sentences

vector<string> split_sentences(const string& text) {
    vector<string> sentences;

    regex sentence_regex(R"([^.?!]+[.?!])");

    auto words_begin =
    sregex_iterator(text.begin(), text.end(), sentence_regex);

    auto words_end = sregex_iterator();

    for(auto i = words_begin; i != words_end; ++i) {
        sentences.push_back(i->str());
    }
    
    return sentences;
}

//split words

vector<string> split_words(const string& text) {
    vector<string> words;
    
    regex word_regex(R"(\b\w+\b)");

    auto begin =
        sregex_iterator(text.begin(), text.end(), word_regex);
    
    auto end = sregex_iterator();

    for(auto i = begin; i != end; ++i) {
        string word = i->str();

        transform(word.begin(), word.end(),
                  word.begin(), ::tolower);
        words.push_back(word);
    }

    return words;
}

//main summarizer

string summarize(const string& text, int num_sentences = 5) {

    vector<string> sentences = split_sentences(text);
    vector<string> words = split_words(text);

    map<string, int> frequency;

    //count word frequency
    for(const auto& word : words) {

        if(word.length() <= 2)
        continue;
        
        frequency[word]++;
    }

    vector<pair<int, string>> scored_sentences;

    //score sentences
    for(const auto& sentence : sentences) {

        vector<string> sentence_words = split_words(sentence);

        int score = 0;

        for(const auto& word : sentence_words) {
            score += frequency[word];
        }

        scored_sentences.push_back({score, sentence});
    }

    //sort highest score first
    sort(scored_sentences.begin(),
         scored_sentences.end(),
         greater<>());
    
    //build summary
    stringstream summary;

    for(int i = 0;
        i < num_sentences &&
        i < scored_sentences.size();
        i++) {
        
            summary << scored_sentences[i].second
                    << "\n\n";
        }

    return summary.str();
}

//MAIN

int main(int argc, char* argv[]) {

    if(argc != 2) {
        cout << "Usage: ./summary <URL" << endl;
        return 1;
    }

    string url = argv[1];
    
    cout << "\nFetching webpage...\n" <<endl;

    string html = fetch_url(url);

    string clean_text = strip_html(html);

    clean_text = remove_urls(clean_text);

    string summary = summarize(clean_text);

    cout <<"=== SUMMARY ===\n" << endl;

    cout << summary << endl;

    cout << "================" << endl;

    return 0;

}
