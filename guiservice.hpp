#ifndef GUI_SERVICE_HPP
#define GUI_SERVICE_HPP

#include <unordered_map>
#include <fstream> 
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "soa.hpp"
#include "products.hpp"
#include "pricingservice.hpp"



class GUIService : public ServiceListener<Price<Bond>>
{
private: 
    unordered_map<string, Price<Bond>> priceMap;
    //vector<ServiceListener<Price<Bond>>*> listeners;
    string filename;
    ofstream file;

    chrono::steady_clock::time_point lastUpdateTime;
    int printCount;
    string latestPriceID;

    string GetCurrentTime();

    string ConvertToFrac(double price);

public: 
    GUIService(string filename = "gui.txt");
    ~GUIService();

    Price<Bond>& GetData(string productId);
    void PriceUpdate(const Price<Bond>& price);

    void ProcessAdd(Price<Bond> &price) override;
    void ProcessRemove(Price<Bond> &price) override;
    void ProcessUpdate(Price<Bond> &price) override;
};

GUIService::GUIService(string filename) : filename(filename), printCount(0) 
{
    file.open(filename, ios::out);
    if (!file.is_open()) {
        cerr << "could not open file" << filename << endl;
    }

    lastUpdateTime = chrono::steady_clock::now();
}

GUIService::~GUIService()
{
    if (file.is_open()) {
        file.close();
    }
}

inline Price<Bond>& GUIService::GetData(string productId) 
{
    auto it = priceMap.find(productId);
    if (it != priceMap.end()) {
        return it->second;
    }
    throw runtime_error("Price key not found: " + productId);
}


void GUIService::PriceUpdate(const Price<Bond> &price) 
{
    string productId = price.GetProduct().GetProductId();
    priceMap.emplace(productId, price);

    latestPriceID = productId;

    auto now = chrono::steady_clock::now();
    auto msSinceLast = chrono::duration_cast<chrono::milliseconds>(now - lastUpdateTime).count();

    if (printCount < 100 && msSinceLast >= 300) {
        string fracMid = ConvertToFrac(price.GetMid());
        string fracSpread = ConvertToFrac(price.GetBidOfferSpread());
        if (file.is_open()) {
            file << GetCurrentTime() << " "
                    << price.GetProduct().GetProductId() << " "
                    << fixed << setprecision(6) 
                    << fracMid << " "
                    << fracSpread << endl;
        }
        else { 
            cerr << "Could not open file" << endl;
        }
        ++printCount;
        lastUpdateTime = now;
    }


}

void GUIService::ProcessAdd(Price<Bond> &price) 
{
    cout << "Process Add called for product" << endl;
    PriceUpdate(price);
}

void GUIService::ProcessRemove(Price<Bond> &price) 
{

}

void GUIService::ProcessUpdate(Price<Bond> &price) 
{
    cout << "Process update called for product " << endl;
    PriceUpdate(price);
}

string GUIService::GetCurrentTime()
{
    auto now = chrono::system_clock::now();
    auto timeT = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    tm bt;
#ifdef _WIN32
    localtime_s(&bt, &timeT);
#else 
    localtime_r(&timeT, &bt);
#endif

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &bt);
    ostringstream oss; 

    oss << buf << "." << setw(3) << setfill('0') << ms.count();
    return oss.str();
}

string GUIService::ConvertToFrac(double price) 
{
    int whole = static_cast<int>(price);
    double frac = price - static_cast<double>(whole);

    double totalTicks = round(frac * 256.0);
    int ticks256 = static_cast<int>(totalTicks);

    if (ticks256 == 256) {
        ticks256 = 0;
        ++whole;
    }

    int ticks32 = ticks256 / 8;
    int subTicks = ticks256 % 8;
    ostringstream oss;
    oss << whole << "-";
    if (ticks32 < 10) oss << "0";
    oss << ticks32;

    if (subTicks == 4) {
        oss << "+";
    }

    else if (subTicks != 0) {
        oss << subTicks;
    }

    return oss.str();

}



#endif
