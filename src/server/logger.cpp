#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iomanip>

class Logger {
private:
    std::ofstream logFile;
    std::mutex logMutex;

    std::string getCurrentTime() {
        auto now = std::time(nullptr);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", std::localtime(&now));
        return std::string(buffer);
    }

public:
    Logger(const std::string& filename) {
        logFile.open(filename, std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo de log.");
        }
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::string formattedMessage = getCurrentTime() + " " + message;
        logFile << formattedMessage << std::endl;
    }
};
