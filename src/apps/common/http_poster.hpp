#include "lib_media/out/http.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <vector>

//TODO: ensure when and how this is needed
auto const deleteOnServer = true;

template<typename T, typename V>
bool exists(T const& container, V const& val) {
	return container.find(val) != container.end();
}

class HttpPoster : public Modules::ModuleS {
public:
	HttpPoster(std::string baseURL) : baseURL(baseURL) {
		addInput(new Modules::Input<Modules::DataBase>(this));
		done = false;
	}
	~HttpPoster() {
		done = true;
	}
	void process(Modules::Data data) override {
		for(auto url : toErase) {
			log(Debug, format("Closing HTTP connection for url: \"%s\"", url).c_str());
			zeroSizeConnections.erase(url);
		}
		toErase.clear();

		auto const meta = safe_cast<const Modules::MetadataFile>(data->getMetadata());
		auto const url = baseURL + meta->filename;

		if (meta->filesize == 0 && !meta->EOS) {
			if (exists(zeroSizeConnections, url))
				throw error(format("Received zero-sized metadata but transfer is already initialized for URL: \"%s\"", url));

			log(Info, format("Initialize transfer for URL: \"%s\"", url).c_str());
			http = Modules::create<Modules::Out::HTTP>(url, Modules::Out::HTTP::Chunked);
			auto onFinished = [&](Modules::Data data2) {
				auto const url2 = baseURL + safe_cast<const Modules::MetadataFile>(data2->getMetadata())->filename;
				log(Debug, format("Finished transfer for url: \"%s\" (done=%s)", url2, (bool)done).c_str());
				if (!done) {
					toErase.push_back(url2);
					if (deleteOnServer) {
						asyncRemoteDelete(url2);
					}
				}
			};
			ConnectOutput(http.get(), onFinished);

			http->getInput(0)->push(data);
			http->getInput(0)->process();
			zeroSizeConnections[url] = move(http);
		} else {
			if (exists(zeroSizeConnections, url)) {
				log(Debug, format("Continue transfer (%s bytes) for URL: \"%s\"", meta->filesize, url).c_str());
				if (meta->filesize) {
					zeroSizeConnections[url]->getInput(0)->push(data);
				}
				if (meta->EOS) {
					zeroSizeConnections[url]->getInput(0)->push(nullptr);
				}
				zeroSizeConnections[url]->getInput(0)->process();
			} else {
				log(Debug, format("Pushing (%s bytes) to new URL: \"%s\"", meta->filesize, url).c_str());
				http = Modules::create<Modules::Out::HTTP>(url, Modules::Out::HTTP::Chunked);
				http->getInput(0)->push(data);
				auto th = std::thread([](std::unique_ptr<Modules::Out::HTTP> http) {
					http->getInput(0)->push(nullptr);
					http->getInput(0)->process();
				}, move(http));
				th.detach();
			}
		}
	}

private:
	void asyncRemoteDelete(std::string url2) {
		if (deleteOnServer) {
			auto remoteDelete = [url2, this]() {
				auto cmd = format("curl -X DELETE %s", url2);
				if (system(cmd.c_str()) != 0) {
					log(Debug, format("command %s failed", cmd).c_str());
				}
			};
			auto th = std::thread(remoteDelete);
			th.detach();
		}
	}
	
	std::atomic_bool done;
	std::unique_ptr<Modules::Out::HTTP> http;
	std::map<std::string, std::shared_ptr<Modules::Out::HTTP>> zeroSizeConnections;
	std::vector<std::string> toErase;
	const std::string baseURL, userAgent;
	const std::vector<std::string> headers;
};

