#include <libwebsockets.h>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <json/json.h>
#include <thread>
#include <cstring>  // Needed for memset()

std::vector<lws*> clients;
std::mutex clients_mutex;

struct Actuator {
    int id;
    std::string name;
    std::string status;
    std::map<std::string, double> readData;
    std::map<std::string, double> writeData;
};

std::map<int, Actuator> Actuators;
std::map<std::string, double> Instruments;

// Initialize Actuators & Instruments
void initializeActuators() {
    for (int i = 1; i <= 4; i++) {
        Actuators[i] = {
            i, "Actuator " + std::to_string(i), "Active",
            { {"Actual Position", i}, {"Actual Velocity", i}, {"Joint Angle", i}, {"Actual Torque", i} },
            { {"Target Position", 150 + i}, {"Target Velocity", 60 + i}, {"Control Word", 80 + i}, {"Target Torque", 40 + i} }
        };
    }
    Instruments = { {"Pitch", 10}, {"Yaw", 20}, {"Roll", 30}, {"Pinch", 4} };
}

// Convert Actuators & Instruments to JSON
std::string getJsonData() {
    Json::Value root;
    Json::StreamWriterBuilder writer;

    for (const auto& [id, actuator] : Actuators) {
        Json::Value act;
        act["id"] = actuator.id;
        act["name"] = actuator.name;
        act["status"] = actuator.status;

        for (const auto& [key, value] : actuator.readData) {
            act["readData"][key] = value;
        }
        for (const auto& [key, value] : actuator.writeData) {
            act["writeData"][key] = value;
        }
        root["Actuators"][std::to_string(id)] = act;
    }

    for (const auto& [key, value] : Instruments) {
        root["Instruments"][key] = value;
    }

    return Json::writeString(writer, root);
}

// Send Data to All Clients
void broadcastData() {
    std::string jsonData = getJsonData();
    size_t message_length = jsonData.length();
    
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it = clients.begin(); it != clients.end();) {
        lws* client = *it;
        
        if (!client) {
            it = clients.erase(it);  // Remove invalid clients
            continue;
        }

        // Allocate buffer with LWS_PRE padding
        size_t buffer_size = LWS_PRE + message_length;
        unsigned char* buffer = new unsigned char[buffer_size];
        memset(buffer, 0, buffer_size);
        memcpy(buffer + LWS_PRE, jsonData.c_str(), message_length);

        int bytes_written = lws_write(client, buffer + LWS_PRE, message_length, LWS_WRITE_TEXT);
        delete[] buffer;  // ✅ Prevent memory leaks

        if (bytes_written < 0) {
            std::cerr << "Error writing to client, removing from list\n";
            it = clients.erase(it);  // Remove the invalid client
        } else {
            ++it;
        }
    }
}

// WebSocket Callback
static int callback_ws(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            std::cout << "Client connected\n";
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.push_back(wsi);
            }
            break;

        case LWS_CALLBACK_RECEIVE:
            if (len > 0) {
                std::string message(static_cast<char*>(in), len);
                std::cout << "Received: " << message << "\n";

                try {
                    Json::CharReaderBuilder reader;
                    Json::Value jsonData;
                    std::string errs;
                    std::istringstream s(message);
                    if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
                        std::cerr << "JSON Parsing Error: " << errs << "\n";
                        break;
                    }

                    if (jsonData["type"].asString() == "updateWriteData") {
                        int instrumentId = jsonData["instrumentId"].asInt();
                        std::string key = jsonData["key"].asString();
                        double value = jsonData["value"].asDouble();

                        if (Actuators.find(instrumentId) != Actuators.end()) {
                            Actuators[instrumentId].writeData[key] = value;
                        }
                    } else if (jsonData["type"].asString() == "updateInstrument") {
                        std::string instrumentKey = jsonData["instrumentKey"].asString();
                        double value = jsonData["value"].asDouble();

                        if (Instruments.find(instrumentKey) != Instruments.end()) {
                            Instruments[instrumentKey] = value;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON: " << e.what() << "\n";
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:
            std::cout << "Client disconnected\n";
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                auto it = std::find(clients.begin(), clients.end(), wsi);
                if (it != clients.end()) {
                    clients.erase(it);
                }
            }
            break;

        default:
            break;
    }
    return 0;
}

// Timer to Update Data Every 1 Second
void updateLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        double actualPosition = (std::rand() % 100) + (std::rand() % 100) / 100.0;
        for (auto& [id, actuator] : Actuators) {
            actuator.readData["Actual Position"] = actualPosition;
            actuator.readData["Actual Velocity"] = actualPosition;
            actuator.readData["Joint Angle"] = actualPosition;
            actuator.readData["Actual Torque"] = actualPosition;
        }
        for (auto& [key, value] : Instruments) {
            value = actualPosition;
        }

        broadcastData();
    }
}

// WebSocket Protocols
static struct lws_protocols protocols[] = {
    { "ws", callback_ws, 0, 65536 },
    { NULL, NULL, 0, 0 }
};

// Main Function
int main() {
    initializeActuators();

    struct lws_context_creation_info info = {};
    info.port = 5002;
    info.protocols = protocols;

    struct lws_context* context = lws_create_context(&info);
    if (!context) {
        std::cerr << "WebSocket context creation failed\n";
        return -1;
    }

    std::thread updateThread(updateLoop);
    updateThread.detach();

    std::cout << "WebSocket server running on port 5002\n";
    while (true) {
        lws_service(context, 0);
    }

    lws_context_destroy(context);
    return 0;
}

