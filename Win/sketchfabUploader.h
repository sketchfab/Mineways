#include "stdafx.h"
#include "curl/picojson.h"
#include "curl_wrapper.h"

#define SKETCHFAB_SERVER "https://sketchfab.com"
#define SKETCHFAB_MODELS SKETCHFAB_SERVER "/models"
#define SKETCHFAB_MODELS_API SKETCHFAB_SERVER "/v2/models"


class SketchfabV2Uploader {
public:
	SketchfabV2Uploader(const std::string& token) : _token(token)
	{}

	std::string upload(const std::string& filepath,
		const std::string& name = std::string(),
		const std::string& description = std::string(),
		const std::string& tags = std::string()) const {
		std::string modelid;
		std::map<std::string, std::string> parameters, files;

		parameters["token"] = _token;
		parameters["name"] = name;
		parameters["tags"] = tags;
		parameters["description"] = description;
		parameters["private"] = "1";
		parameters["source"] = "mineways";
		files["modelFile"] = filepath;

		std::pair<int, std::string> response = curl::post(SKETCHFAB_MODELS_API, files, parameters);
		// Upload v2 returns a status 201 not 200
		if (response.first == 201) {
			modelid = get_json_key(response.second, "uid");
		}
		else {
			std::cerr << "Upload error: '" << get_json_key(response.second, "detail") << "'" << std::endl;
		}

		return modelid;
	}

	std::string status(const std::string& modelid) const {
		std::pair<int, std::string> response = curl::get(model_status_url(modelid));

		if (response.first == 200) {
			// 'SUCCEEDED': model correctly processed and viewable at model_url(modelid)
			// 'FAILED': model processing failed
			// 'PENDING': model waiting to be processed
			// 'PROCESSING': model being currently processed
			return get_json_key(response.second, "processing");
		}
		return std::string();
	}

	std::string poll(const std::string& modelid, unsigned int timeout = 60, unsigned int poll_delay = 1) {
		bool continue_polling = true;
		std::string model_status;
		unsigned int poll_time = 0;

		do {
			model_status = status(modelid);
			if (poll_time >= timeout) {
				continue_polling = false;
				model_status = "POLL_TIMEOUT";
			}
			else if (model_status == "SUCCEEDED" || model_status == "FAILED") {
				continue_polling = false;
			}
			else {
				poll_time += poll_delay;
				Sleep(poll_delay);
			}
		} while (continue_polling);

		return model_status;
	}

	std::string model_url(const std::string& modelid) const {
		return std::string(SKETCHFAB_MODELS) + "/" + modelid;
	}

	std::string model_status_url(const std::string& modelid) const {
		return std::string(SKETCHFAB_MODELS_API) + "/" + modelid + "/status?token=" + _token;
	}

protected:
	std::string get_json_key(const std::string& json, const std::string& key) const {
		picojson::value v;
		std::string err = picojson::parse(v, json);

		if (v.is<picojson::object>()) {
			picojson::object obj = v.get<picojson::object>();
			return obj[key].to_str();
		}
		return std::string();
	}

private:
	std::string _token;
};
