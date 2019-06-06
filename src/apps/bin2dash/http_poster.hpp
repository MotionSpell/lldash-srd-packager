#include "lib_modules/utils/helper.hpp" // ModuleS
#include "lib_media/common/metadata_file.hpp"
#include "lib_modules/core/error.hpp"
#include "lib_utils/format.hpp"
#include "lib_utils/log_sink.hpp"
#include "lib_utils/tools.hpp" // safe_cast
#include "lib_media/out/http.hpp"
#include <string>
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std;

auto const deleteOnServer = true;

template<typename T, typename V>
bool exists(T const& container, V const& val) {
	return container.find(val) != container.end();
}

struct HttpSink2 : Modules::ModuleS {
		HttpSink2(Modules::KHost* host, string baseURL, int timeshiftBufferDepthInMs)
		: m_host(host),
			timeshiftBufferDepthInMs(timeshiftBufferDepthInMs), baseURL(baseURL) {
			done = false;
		}
		~HttpSink2() {
			done = true;
		}
		void processOne(Modules::Data data) override {
			auto const meta = safe_cast<const Modules::MetadataFile>(data->getMetadata());

			{
				lock_guard<mutex> lg(toEraseMx);
				for (auto url : toErase) {
					m_host->log(Debug, format("Closing HTTP connection for url: \"%s\"", url).c_str());
					zeroSizeConnections.erase(url);
				}
				toErase.clear();
			}

			auto const url = baseURL + meta->filename;

			HttpOutputConfig httpConfig {};
			httpConfig.url = url;

			auto onFinished = [&](Modules::Data data2) {
				auto const url2 = baseURL + safe_cast<const Modules::MetadataFile>(data2->getMetadata())->filename;
				m_host->log(Debug, format("Finished transfer for url: \"%s\" (done=%s)", url2, (bool)done).c_str());
				if (!done) {
					{
						lock_guard<mutex> lg(toEraseMx);
						toErase.push_back(url2);
					}
					if (deleteOnServer) {
						/*only delete media segments*/
						auto const sepPos = url2.find_last_of(".");
						if (sepPos == string::npos) {
							m_host->log(Error, format("Cannot find '.' separator in \"%s\". Won't be removed.", url2).c_str());
							return;
						}
						auto const ext = url2.substr(sepPos);
						if (ext == ".m4s") {
							asyncRemoteDelete(url2);
						}
					}
				}
			};

			if (exists(zeroSizeConnections, url)) {
				m_host->log(Debug, format("Continue transfer (%s bytes) for URL: \"%s\"", meta->filesize, url).c_str());
				if (meta->filesize) {
					zeroSizeConnections[url]->getInput(0)->push(data);
				}
				if (meta->EOS) {
					zeroSizeConnections[url]->getInput(0)->push(nullptr);
				}
				zeroSizeConnections[url]->process();
			} else {
				if (!meta->EOS) {
					if (exists(zeroSizeConnections, url))
						throw Modules::error(format("Received zero-sized metadata but transfer is already initialized for URL: \"%s\"", url));

					m_host->log(Info, format("Initialize transfer for URL: \"%s\"", url).c_str());
					http = Modules::createModule<Modules::Out::HTTP>(&Modules::NullHost, httpConfig);
					ConnectOutput(http->getOutput(0), onFinished);

					http->getInput(0)->push(data);
					http->process();
					zeroSizeConnections[url] = move(http);
				} else {
					m_host->log(Debug, format("Pushing (%s bytes) to new URL: \"%s\"", meta->filesize, url).c_str());
					http = Modules::createModule<Modules::Out::HTTP>(&Modules::NullHost, httpConfig);
					ConnectOutput(http->getOutput(0), onFinished);
					http->getInput(0)->push(data);
					auto th = thread([](unique_ptr<Modules::Out::HTTP> http) {
						http->getInput(0)->push(nullptr);
						http->process();
					}, move(http));
					th.detach();
				}
			}
		}

	private:

		void asyncRemoteDelete(string url2) {
			auto remoteDelete = [url2,this]() {
				if (!timeshiftBufferDepthInMs)
          return;

				this_thread::sleep_for(chrono::milliseconds(timeshiftBufferDepthInMs));

				auto cmd = format("curl -X DELETE %s", url2);
				if(system(cmd.c_str()) != 0) {
					m_host->log(Warning, format("command %s failed", cmd).c_str());
				}
			};
			auto th = thread(remoteDelete);
			th.detach();
		}

		Modules::KHost* const m_host;
		atomic_bool done;
		unique_ptr<Modules::Out::HTTP> http;
		map<string, shared_ptr<Modules::Out::HTTP>> zeroSizeConnections;
		vector<string> toErase;
		mutex toEraseMx;
		int timeshiftBufferDepthInMs;
		const string baseURL, userAgent;
		const vector<string> headers;
};

