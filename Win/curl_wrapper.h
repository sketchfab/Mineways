#ifndef CURL_WRAPPER
#define CURL_WRAPPER

#include "curl/curl.h"

static size_t WriteMemoryCallback(char *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


namespace curl {
	typedef std::map<std::string, std::string> attributes;

	std::pair<int, std::string> post(const std::string& url,
		const attributes& files = attributes(),
		const attributes& parameters = attributes()) {
		CURL *curl;
		CURLcode res;
		long http_code;
		std::string response;
		struct curl_httppost *formpost = NULL;
		struct curl_httppost *lastptr = NULL;

		http_code = 0;

		if (url.empty()) {
			std::cout << "Missing url parameter in post parameters" << std::endl;
			return std::pair<int, std::string>(-1, std::string());
		}
		if (files.empty()) {
			std::cout << "Missing files in post parameters" << std::endl;
			return std::pair<int, std::string>(-2, std::string());
		}
		
		for (attributes::const_iterator file = files.begin(); file != files.end(); ++file) {
			std::cout << file->first.c_str();
			curl_formadd(&formpost,
				&lastptr,
				CURLFORM_COPYNAME, file->first.c_str(),
				CURLFORM_FILE, file->second.c_str(),
				CURLFORM_END);
		}

		for (attributes::const_iterator parameter = parameters.begin(); parameter != parameters.end(); ++parameter) {
			if (!parameter->second.empty()) {
				curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, parameter->first.c_str(),
					CURLFORM_COPYCONTENTS, parameter->second.c_str(),
					CURLFORM_END);
			}
		}

		curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

			res = curl_easy_perform(curl);

			if (res != CURLE_OK) {
				std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
				std::cerr << "Model upload returned http code: " << http_code << std::endl;
			}
			else {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			}

			curl_easy_cleanup(curl);
			curl_formfree(formpost);
		}
		return std::pair<int, std::string>(http_code, response);
	}

	std::pair<int, std::string> get(const std::string& url,
		const attributes& parameters = attributes()) {
		CURL *curl;
		CURLcode res;
		std::string model_url;
		std::string response;
		long http_code;

		if (url.empty()) {
			std::cout << "Missing url parameters in get parameters" << std::endl;
			return std::pair<int, std::string>(-1, std::string());
		}

		std::string options;
		for (attributes::const_iterator parameter = parameters.begin(); parameter != parameters.end(); ++parameter) {
			if (options.empty()) {
				options = "?";
			}
			else {
				options += "&";
			}
			options += parameter->first + "=" + parameter->second;
		}

		http_code = 0;
		curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, (url + options).c_str());
			curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

			res = curl_easy_perform(curl);

			if (res != CURLE_OK) {
				std::cerr << "curl_easy_perform failed:" << curl_easy_strerror(res) << std::endl;
				std::cerr << "Model polling returned http code: " << http_code << std::endl;
			}
			else {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			}

			curl_easy_cleanup(curl);
		}

		return std::pair<int, std::string>(http_code, response);
	}
};

#endif