#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;

class SymbolTicker {
private:
    std::string symbol;
    int period;
    double bidPrice;
    double bidQty;
    double askPrice;
    double askQty;
    static int objCount;
    std::mutex mtx;

    void fetchData() {
        while (!symbol.empty() && period > 0) {
            http_client client(U("https://fapi.binance.com"));
            uri_builder builder(U("/fapi/v1/ticker/bookTicker"));
            builder.append_query(U("symbol"), symbol);
            auto response = client.request(methods::GET, builder.to_string()).get();
            if (response.status_code() == status_codes::OK) {
                auto data = response.extract_json().get();
                bidPrice = std::stod(data[U("bidPrice")].as_string());
                bidQty = std::stod(data[U("bidQty")].as_string());
                askPrice = std::stod(data[U("askPrice")].as_string());
                askQty = std::stod(data[U("askQty")].as_string());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(period));
        }
    }

public:
    SymbolTicker() : symbol(""), period(0), bidPrice(0), bidQty(0), askPrice(0), askQty(0) {
        ++objCount;
    }

    SymbolTicker(const std::string& symbol, int period) 
        : symbol(symbol), period(period), bidPrice(0), bidQty(0), askPrice(0), askQty(0) {
        ++objCount;
        std::thread(&SymbolTicker::fetchData, this).detach();
    }

    SymbolTicker(const SymbolTicker& other) 
        : symbol(other.symbol), period(other.period), bidPrice(other.bidPrice), bidQty(other.bidQty),
          askPrice(other.askPrice), askQty(other.askQty) {
        ++objCount;
    }

    SymbolTicker(SymbolTicker&& other) noexcept 
        : symbol(std::move(other.symbol)), period(other.period), bidPrice(other.bidPrice), bidQty(other.bidQty),
          askPrice(other.askPrice), askQty(other.askQty) {
        ++objCount;
    }

    ~SymbolTicker() {
        --objCount;
    }

    static int getObjCount() {
        return objCount;
    }

    friend std::ostream& operator<<(std::ostream& os, const SymbolTicker& ticker) {
        std::lock_guard<std::mutex> lock(ticker.mtx);
        os << ticker.symbol << std::endl;
        os << "bid price: " << ticker.bidPrice << std::endl;
        os << "ask price: " << ticker.askPrice << std::endl;
        os << "bid qty: " << ticker.bidQty << std::endl;
        os << "ask qty: " << ticker.askQty << std::endl;
        return os;
    }

    void setSymbol(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(mtx);
        this->symbol = symbol;
    }
};

int SymbolTicker::objCount = 0;

int main() {
    SymbolTicker test("ETHUSDT", 500);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << test << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << test << std::endl;

    SymbolTicker test2 = test;
    std::cout << test2 << std::endl;
    std::cout << test << std::endl;
    test2.setSymbol("AVAXUSDT");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << test2 << std::endl;
    std::cout << test << std::endl;
    std::cout << "Object count: " << SymbolTicker::getObjCount() << std::endl;

    return 0;
}
