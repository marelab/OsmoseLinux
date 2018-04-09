
/* ToDo:
 * - Manueller Refill Aqua Sensor Aus
 * - Date Time Anzeige
 * - Statistik Seite
 *
 */
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "wiringPi/wiringPi.h"
#include "spdlog/spdlog.h"
#include "json/json.h"
#include "json/writer.h"
#include "json/reader.h"

#include "MessageBus.h"
#include "OsmoseAnlage.h"
#include "ConfigRegister.h"
#include "mail.h"


// Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// Added for the default_resource example
#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>

#ifdef HAVE_OPENSSL
#include "crypto.hpp"
#endif


namespace spd = spdlog;

// Added for the json-example:
using namespace boost::property_tree;


#include "websocket/server_ws.hpp"
#include "httpserver/server_http.hpp"

using namespace std;

size_t q_size = 4; //queue size must be power of 2

using WsServer 		= SimpleWeb::SocketServer<SimpleWeb::WS>;
using HttpServer	= SimpleWeb::Server<SimpleWeb::HTTP>;



#define IGNORE_CHANGE_BELOW_USEC 4000000

MessageBus messageBus;

struct timeval last_change_Aqua;
static volatile int state;
ConfigRegister *config;
Statistik	   *statistik;

OsmoseAnlage 	osmose(&messageBus);
mail			mailer(&messageBus);

string 			osmoseLinuxDir;

WsServer 		server;
HttpServer 		httpserver;

OsmoseModus 	anlagenState;


////////////////////////////////////////////
// Schaltet alles ab
////////////////////////////////////////////
void AllOff()
{
	auto logger = spdlog::get("logger");
	logger->debug("AllOff()");
}

void thread_io() {

	OsmoseRefillState oldRefillState,newRefillState;
	OsmoseProductionState oldProductionState,newProductionState;
	auto logger = spdlog::get("logger");

	///const mail mailer = osmose.getStatistik().getMailer();
	/// mailer.SendMail("Osmose Neustart","Das OS wurde neu gestartet und damit auch die Osmose Software");


	//config.readConfig();

	if (osmose.getOsmoseModus()== modus_automatik )
		anlagenState = modus_automatik;

	if (osmose.getOsmoseModus()== modus_manuel ){
		anlagenState = modus_manuel;
		AllOff();
	}



	for (;;) {
		// Baue status json um ggf bei connection zum
		// client es zu versenden
		osmose.CheckSensoren();
		ptree root;
		root.put("cmd", "status");
		root.put("refillPump", 				osmose.isRefillPump());
		root.put("osmosePump",   			osmose.isOsmosePump());
		root.put("cleanVent",				osmose.isCleanVent());
		root.put("frischVent",				osmose.isFrischVent());
		root.put("sensorTop", 				osmose.isSensorTop() );
		root.put("sensorBottom", 			osmose.isSensorBottom() );
		root.put("sensorAqua", 				osmose.isSensorAqua() );
		root.put("cleanRuntime", 	 		osmose.getCleanRuntime() );
		root.put("runtimeForCleaning", 		osmose.getRuntimeForCleaning() );
		root.put("cycleTime", 	 			osmose.getCycleTime());
		root.put("osmoseRuntime", 	 		osmose.getOsmoseRuntime() );
		root.put("osmoseModus",      		osmose.getOsmoseModus());


		oldProductionState = osmose.getOsmoseProductionLastState();
		newProductionState = osmose.stateMachineOsmose();
		root.put("modus", 	newProductionState	);

		if (oldProductionState!=newProductionState)
		{
			logger->info("OsmoseProduction changed to: " + osmose.osmoseStateToString(newProductionState));
		}

		oldRefillState = osmose.getRefillState();
		newRefillState = osmose.stateMachineRefill();
		if (oldRefillState!=newRefillState)
		{
			logger->info("RefillState changed to: " + osmose.refillStateToString(newRefillState));
		}

		root.put("refillState",				newRefillState);
		root.put("refillRuntime",			osmose.getRefillRuntime());

		// Message System Delivery Messages
		messageBus.notify();

		auto send_stream = make_shared<WsServer::SendStream>();
		std::stringstream ss;

		//ss.clear();
		write_json(ss, root);


		*send_stream << ss.str();

		//cout << "Server send : " << ss.str() <<  endl;
		// echo_all.get_connections() can also be used to solely receive connections on this endpoint
		for (auto &a_connection : server.get_connections())
			a_connection->send(send_stream);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	}
}

void SetupToJson(){
	string configJsonOsmose;

	configJsonOsmose = osmose.getConfigAsJsonString();

	auto logger = spdlog::get("logger");

	//std::stringstream ss;

	auto send_stream = make_shared<WsServer::SendStream>();
	//*send_stream << ss.str();
	*send_stream << configJsonOsmose;

	logger->debug("SetupToJson() Server send : " + configJsonOsmose );

	cout << configJsonOsmose << endl;

	//echo_all.get_connections() can also be used to solely receive connections on this endpoint
	for (auto &a_connection : server.get_connections())
		a_connection->send(send_stream);

}

void WriteConfig(ptree *pt){
    auto jsonValue = pt->get<int>("sensorAquaDbounce");
	osmose.setSensorAquaDbounce(jsonValue);
	jsonValue = pt->get<int>("sensorTopDbounce");
	osmose.setSensorTopDbounce(jsonValue);
	jsonValue = pt->get<int>("sensorBottomDbounce");
	osmose.setSensorBottomDbounce(jsonValue);
	jsonValue = pt->get<int>("osmoseFailTime");
	osmose.setOsmoseFailTime(jsonValue);
	jsonValue = pt->get<int>("refillFailTime");
	osmose.setRefillFaillTime(jsonValue);
	jsonValue = pt->get<int>("runtimeForCleaning");
	osmose.setRuntimeForCleaning(jsonValue);
	jsonValue = pt->get<int>("osmoseModus");
	osmose.setOsmoseModus((OsmoseModus)jsonValue);
	jsonValue = pt->get<int>("osmoseRuntimeToClean");
	osmose.setOsmoseRuntimeToClean((OsmoseModus)jsonValue);


	jsonValue = pt->get<int>("IoPinSensorAqua");
	osmose.getAqua()->PORT = jsonValue;
	jsonValue = pt->get<int>("IoPinSensorBottom");
	osmose.getBottom()->PORT = jsonValue;
	jsonValue = pt->get<int>("IoPinSensorTop");
	osmose.getTop()->PORT = jsonValue;


	jsonValue = pt->get<int>("IOpumpRefillPort");
	osmose.getIOpumpRefill()->setPort(jsonValue);
	jsonValue = pt->get<int>("IOrefillManuelPort");
	osmose.getIOrefillManuel()->setPort(jsonValue);
	jsonValue = pt->get<int>("IOsmoseManuelPort");
	osmose.getIOsmoseManuel()->setPort(jsonValue);
	jsonValue = pt->get<int>("IOventCleanPort");
	osmose.getIOventClean()->setPort(jsonValue);
	jsonValue = pt->get<int>("IOventFreshWaterPort");
	osmose.getIOventFreshWater()->setPort(jsonValue);

	config->writeConfig();
	config->readConfig(); // Sorgt dafür das alle Objekte neu angelegt werden
}

void command(ptree *pt){
	auto logger = spdlog::get("logger");
	auto cmd = pt->get<string>("cmd");

	if (cmd=="writeConfig"){
		// Alles statis zwischen speichern & alles ausschalten
		logger->debug("write osmoseLinux config file...:");
		WriteConfig(pt);
		// Alles statis zwischen zurückholenm & fortfahren

	}
	else if (cmd=="readConfig"){  // Lesen des OsmoseAnlage Objectes
		logger->debug("read osmoseLinux config file...:");
		config->readConfig();
		SetupToJson(); // WebSockets Clients informieren
	}
	else if (cmd=="status"){
	}
	else if (cmd=="readStatistik"){
		auto send_stream = make_shared<WsServer::SendStream>();
		*send_stream << statistik->PrintStatistikQueue();
		for (auto &a_connection : server.get_connections())
			a_connection->send(send_stream);

	}
	else if (cmd=="reset"){  // Reset der Anlage
		cout << "reset der osmose anlage..." << endl;
		if (osmose.getOsmoseProductionLastState()==serror){
			logger->debug("Reset manual OsmoseProduction");
			osmose.resetOsmoseProduction();
		}
		if (osmose.getOsmoseRefillState()==refill_error){
			logger->debug("Reset manual RefillProduction");
			osmose.resetRefillProduction();
		}
	}
}





void InitOsmose(){
	wiringPiSetup();
	config->addObj(&osmose);
	config->readConfig();
}

int main (int argc, char *argv[])
{

	spd::set_async_mode(q_size);

	//TODO: Stable parameter parsing at startup
	osmoseLinuxDir = "";
	if (argc == 2){
		osmoseLinuxDir = argv[1];
	}

	std::shared_ptr<spdlog::logger>  logger = spd::rotating_logger_mt("logger", osmoseLinuxDir +"logs/osmoselinux.log", 1048576 * 2, 3);
	spd::set_level(spd::level::debug); //Set global log level to info
	//TODO:FLUSH LOGGER NUR ÜBER CONFIG DA SONST IMMER SOFORT GESCHRIEBEN WIRD SSDCARD KILL
	logger->flush_on(spd::level::info);

	if (argc == 2) {
		logger->info("marelab osmoselinux started with: '" + osmoseLinuxDir + "'");
	} else {
		logger->info("marelab osmoselinux started without paramters");
	}


	server.config.port = 8080;
	httpserver.config.port = 8090;
	config 		= new ConfigRegister(osmoseLinuxDir);
	statistik 	= new Statistik(&messageBus);

	InitOsmose();


	// Default GET-example. If no other matches, this anonymous function will be called.
	// Will respond with content in the web/-directory, and its subdirectories.
	// Default file: index.html
	// Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
	httpserver.default_resource["GET"] =
			[](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
				try {
					auto web_root_path = boost::filesystem::canonical(osmoseLinuxDir + "web");
					auto path = boost::filesystem::canonical(web_root_path / request->path);
					//cout << "web_root_path" << web_root_path.string() << endl;
					//cout << "PATH" << path.string()  << endl;
					// Check if path is within web_root_path
					if(distance(web_root_path.begin(), web_root_path.end()) > distance(path.begin(), path.end()) || !equal(web_root_path.begin(), web_root_path.end(), path.begin()))
					throw invalid_argument("path must be within root path");
					if(boost::filesystem::is_directory(path))
					path /= "index.html";

					SimpleWeb::CaseInsensitiveMultimap header;

					// Uncomment the following line to enable Cache-Control
					//header.emplace("Cache-Control", "max-age=86400");



			auto ifs = make_shared<ifstream>();
			//cout << "PATHT TO OPEN->" << path.string()<< endl;
			ifs->open(path.string(), ifstream::in | ios::binary | ios::ate);

			if(*ifs) {
				auto length = ifs->tellg();
				ifs->seekg(0, ios::beg);

				header.emplace("Content-Length", to_string(length));
				response->write(header);

				// Trick to define a recursive function within this scope (for example purposes)
				class FileServer {
				public:
					static void read_and_send(const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs) {
						// Read and send 128 KB at a time
						static vector<char> buffer(131072);// Safe when server is running on one thread
						streamsize read_length;
						if((read_length = ifs->read(&buffer[0], static_cast<streamsize>(buffer.size())).gcount()) > 0) {
							response->write(&buffer[0], read_length);
							if(read_length == static_cast<streamsize>(buffer.size())) {
								response->send([response, ifs](const SimpleWeb::error_code &ec) {
											if(!ec)
											read_and_send(response, ifs);
											else
											cerr << "Connection interrupted" << endl;
										});
							}
						}
					}
				};
				FileServer::read_and_send(response, ifs);
			}
			else
			throw invalid_argument("could not read file");
		}
		catch(const exception &e) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what());
		}
	};

	httpserver.on_error = [](shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code &ec) {
	    // Handle errors here
	    // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
		cout << "Server: wscmd Closed connection " << ec.message() << endl;
	  };

	  thread httpserver_thread([&httpserver]() {
	    // Start server
		  auto logger = spdlog::get("logger");
		  logger->info("Http Server started");
		  httpserver.start();
	  });

	  // Wait for server to start so that the client can connect
	  this_thread::sleep_for(std::chrono::milliseconds(1000));





	  // WebSocketServer
	//////////////////////////////////
	auto &wscmd = server.endpoint["^/wscmd/?$"];

	wscmd.on_message =
			[](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
				auto message_str = message->string();
				 ptree pt;
				 std::stringstream ss;
				// cout << "recv json: " << message_str << endl;
				 cout << "Server: Message received: \"" << message_str << "\" from " << connection.get() << endl;
				 ss << message_str << std::endl;
				 read_json(ss, pt);
				 command(&pt);

				//cout << "Server: Sending message \"" << message_str << "\" to " << connection.get() << endl;

				auto send_stream = make_shared<WsServer::SendStream>();
				*send_stream << message_str;
				// connection->send is an asynchronous function
				connection->send(send_stream, [](const SimpleWeb::error_code &ec) {
							if(ec) {
								cout << "Server: Error sending message. " <<
								// See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
								"Error: " << ec << ", error message: " << ec.message() << endl;
							}
						});
			};

	wscmd.on_open = [](shared_ptr<WsServer::Connection> connection) {
		cout << "Server: wscmd Opened connection " << connection.get() << endl;
	};

	// See RFC 6455 7.4.1. for status codes
	wscmd.on_close =
			[](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
				cout << "Server: wscmd Closed connection " << connection.get() << " with status code " << status << endl;
			};

	// See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
	wscmd.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
				cout << "Server: wscmd Error in connection " << connection.get() << ". "
				<< "Error: " << ec << ", error message: " << ec.message() << endl;
			};

	  thread server_thread([&server]() {
	    // Start WS-server
		  auto logger = spdlog::get("logger");
		  logger->info("WebSocket Server started");
	    server.start();

	  });
	  // Wait for server to start so that the client can connect
	  this_thread::sleep_for(std::chrono::milliseconds(1000));




	  // IO Thread
	  ////////////////////////////////////////
	  //std::cout << "IO thread started..." << endl;
	  logger->info("OsmoseAnlage thread started");
	  std::thread IO_thread(thread_io);



	  server_thread.join();
	  logger->info("WebSocket Server end");
	  httpserver_thread.join();
	  logger->info("Http Server end");
	  IO_thread.join();
	  logger->info("IO Server end");
	  spd::drop_all();


    return 0;
}

