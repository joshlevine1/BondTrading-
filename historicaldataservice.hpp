/**
 * historicaldataservice.hpp
 * historicaldataservice.hpp
 *
 * @author Breman Thuraisingham
 * Defines the data types and Service for historical data.
 *
 * @author Breman Thuraisingham
 */
#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"
#include "riskservice.hpp"
#include "executionservice.hpp"
#include "streamingservice.hpp"
#include "inquiryservice.hpp"
#include "pricingservice.hpp"
#include <fstream>
#include <string>
#include <chrono>


using namespace std;

/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataService : Service<string,T>
{

public:

  // Persist data to a store
  virtual void PersistData(string persistKey, const T& data) = 0;

};

template<typename T>
class FileConnector : public Connector<T>
{
private: 
  string filename;

public: 
  FileConnector(const string& _filename);
  void Publish(T& data) override;
  //void SetFilename(const string& filename) {filename = filename;}

};

template<typename T>
FileConnector<T>::FileConnector(const string& _filename) 
{
  filename = _filename;
}

template<typename T>
inline void FileConnector<T>::Publish(T& data) 
{
  ofstream outFile(filename, ios::app);
  if (outFile.is_open()) {
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    outFile << put_time(localtime(&time_t_now), "%Y-%m-%d %H:%M:%S")
            << "." << setfill('0') << setw(3) << ms.count() << " "
            << data << endl;
    outFile.close();

    // auto now = chrono::system_clock::now();
    // auto time_t_now = chrono::system_clock::to_time_t(now);
    // auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // stringstream logData;
    // logData << put_time(localtime(&time_t_now), "%Y-%m-%d %H:%M:%S")
    //         << "." << setfill('0') << setw(3) << ms.count() << " "
    //         << data << endl;

    // cout << "Writing to File: " << logData.str();
    // outFile << logData.str();
    // outFile.close();
  }
  else {
    cerr << "Error: Could not open file " << filename << " for writing. " << endl;
  }
}

// template<typename T>
// class BondHistoricalDataService : public HistoricalDataService<T>
// {
// private: 
//   FileConnector<T>* connector;
//   vector<ServiceListener<T>*> listeners;

// public: 
//   BondHistoricalDataService(const string& filename);
//   ~BondHistoricalDataService();
//   void PersistData(string persistkey, const T& data) override;
//   T& GetData(string persistKey) override;
//   void AddListener(ServiceListener<T>* listener) override;
//   const vector<ServiceListener<T>*>& GetListeners() const override;
//   void OnMessage(T& data) override;

// };

// template<typename T>
// BondHistoricalDataService<T>::BondHistoricalDataService(const string& filename)
// {
//   connector = new FileConnector<T>(filename);
// }

// template<typename T>
// BondHistoricalDataService<T>::~BondHistoricalDataService()
// {
//   delete connector;
// }

// template<typename T>
// inline void BondHistoricalDataService<T>::PersistData(string persistkey, const T& data)
// {
//   connector->Publish(const_cast<T&>(data));
// }

// template<typename T>
// inline T& BondHistoricalDataService<T>::GetData(string persistKey)
// {
//   throw logic_error("GetData not implemented for BondHistoricalDataService.");
// }

// template<typename T>
// inline void BondHistoricalDataService<T>::AddListener(ServiceListener<T>* listener)
// {
//   listeners.push_back(listener);
// }

// template<typename T>
// inline const vector<ServiceListener<T>*>& BondHistoricalDataService<T>::GetListeners() const
// {
//   return listeners;
// }

// template<typename T>
// inline void BondHistoricalDataService<T>::OnMessage(T& data)
// {
//   PersistData("", data);
// }

class BondPositionHistoricalDataService : public ServiceListener<Position<Bond>>
{
private: 
  FileConnector<string>* connector;
public: 
  BondPositionHistoricalDataService() {
    connector = new FileConnector<string>("positions.txt");
  }

  ~BondPositionHistoricalDataService()
  {
    delete connector;
  }

  void PersistData(string persistKey, const Position<Bond>& position)
  {
    stringstream ss;

    const Bond& bond = position.GetProduct();
    string productId = bond.GetProductId();

    for (const auto& bookPosition : position.GetPositions()) {
        ss << "Product: " << productId << ", " 
          << "Book: " << bookPosition.first << ", " 
          << "Quantity: " << bookPosition.second << endl;
    }
    ss << "Product: " << productId << ", " 
      << "Aggregate Position: " << position.GetAggregatePosition() << endl;

    string ss_string = ss.str();
    

    connector->Publish(ss_string);
      
      
  }

  void ProcessAdd(Position<Bond>& data) override 
  {
    PersistData("position.txt", data);
  }

  void ProcessUpdate(Position<Bond>& data) override
  {
    PersistData("position.txt", data);
  }

  void ProcessRemove(Position<Bond>& data) override {}

};

class BondRiskHistoricalDataService : public ServiceListener<PV01<Bond>>
{

private: 
  FileConnector<string>* connector;
public: 
  BondRiskHistoricalDataService() {
    connector = new FileConnector<string>("risk.txt");
  }

  ~BondRiskHistoricalDataService() 
  {
    delete connector;
  }

  void PersistData(string persistKey, const PV01<Bond>& pv01)
  {
    stringstream ss;
    const Bond& bond = pv01.GetProduct();

    // cout << "Persisting Risk Data - Product: " << bond.GetProductId()
    //      << ", PV01: " << pv01.GetPV01()
    //      << ", Quantity: " << pv01.GetQuantity() << endl;

    ss << "Product: " << bond.GetProductId() << ", "
        << "PV01: " << pv01.GetPV01() << ", "
        << "Quantity: " << pv01.GetQuantity() << endl;
    
    string ss_string = ss.str();
    connector->Publish(ss_string);

  }
  

  void PersistBucketedRisk(const string& persistKey, const PV01<BucketedSector<Bond>>& bucketedRisk) 
  {
    stringstream ss;
    const BucketedSector<Bond> bucket = bucketedRisk.GetProduct();

    ss << "Bucket Sector: " << bucket.GetName() << ", "
            << "Total PV01: " << bucketedRisk.GetPV01() << ", "
            << "Total Quantity: " << bucketedRisk.GetQuantity() << endl;
    string ss_string = ss.str();
    connector->Publish(ss_string);

  }
  void ProcessAdd(PV01<Bond>& data) override 
  {
    PersistData("risk.txt", data);
    //PersistBucketedRisk("risk.txt", data)
  }

  void ProcessUpdate(PV01<Bond>& data) override
  {
    PersistData("risk.txt", data);
    //PersistBucketedRisk("risk.txt", data);
  }

  void ProcessRemove(PV01<Bond>& data) override {}

};

class BondStreamingHistoricalDataService : public ServiceListener<PriceStream<Bond>>
{

private: 
  FileConnector<string>* connector;
public: 
  BondStreamingHistoricalDataService() {
    connector = new FileConnector<string>("streaming.txt");
  }

  ~BondStreamingHistoricalDataService() 
  {
    delete connector;
  }

  void PersistData(string persistKey, const PriceStream<Bond>& stream) 
  {

    const Bond& bond = stream.GetProduct();
    string productId = bond.GetProductId();

    auto bidStream = stream.GetBidOrder();
    auto offerStream = stream.GetOfferOrder();

    stringstream ss;
    ss << "Streaming for product: " << productId << endl;
    ss << " Bid price: " << bidStream.GetPrice() 
        << ", Bid visible quantity: " << bidStream.GetVisibleQuantity()
        << ", Bid hidden quantity: " << bidStream.GetHiddenQuantity() << endl;
    ss << "Offer price " << offerStream.GetPrice() 
        << ", Offer visible quantity: " << offerStream.GetVisibleQuantity() 
        << ", offer hidden quantity: " << offerStream.GetHiddenQuantity() << endl;
    string ss_string = ss.str();
    connector->Publish(ss_string);

  }

  void ProcessAdd(PriceStream<Bond>& data) override 
  {
    PersistData("stream.txt", data);
  }

  void ProcessUpdate(PriceStream<Bond>& data) override
  {
    PersistData("stream.txt", data);
  }

  void ProcessRemove(PriceStream<Bond>& data) override {}
};

class BondInquiryHistoricalDataService : public ServiceListener<Inquiry<Bond>>
{

private: 
  FileConnector<string>* connector;
public: 
  BondInquiryHistoricalDataService() {
    connector = new FileConnector<string>("allinquiries.txt");
  }

  ~BondInquiryHistoricalDataService() 
  {
    delete connector;
  }

  void PersistData(string persistKey, const Inquiry<Bond>& inquiry) 
  {

    const Bond& bond = inquiry.GetProduct();
    InquiryState state = inquiry.GetState();
    double price = inquiry.GetPrice();

    stringstream ss;
    
    ss << "Inquiry for Bond: " << bond.GetProductId() 
        << ", inquiry ID: " << inquiry.GetInquiryId()
        << ", Quantity: " << inquiry.GetQuantity()
        << ", price: " << inquiry.GetPrice()
        << ", side: " << (inquiry.GetSide() == BUY ? "BUY" : "SELL")
        << ", state: " << (state == RECEIVED ? "RECEIVED" : 
                          state == QUOTED ? "QUOTED" :
                          state == DONE ? "DONE" : 
                          state == REJECTED ? "REJECTED" : "UNKNOWN")
        << endl;
        
    string ss_string = ss.str();
    connector->Publish(ss_string);

  }

  void ProcessAdd(Inquiry<Bond>& data) override 
  {
    PersistData("allinquiries.txt", data);
  }

  void ProcessUpdate(Inquiry<Bond>& data) override
  {
    PersistData("allinquiries.txt", data);
  }

  void ProcessRemove(Inquiry<Bond>& data) override {}
};

class BondExecutionHistoricalDataService : public ServiceListener<Trade<Bond>>
{

private: 
  FileConnector<string>* connector;
public: 
  BondExecutionHistoricalDataService() {
    connector = new FileConnector<string>("executions.txt");
  }

  ~BondExecutionHistoricalDataService() 
  {
    delete connector;
  }

  void PersistData(string persistKey, const Trade<Bond>& trade) 
  {
    const Bond& bond = trade.GetProduct();
    string tradeID = trade.GetTradeId();
    Side side = trade.GetSide();
    double price = trade.GetPrice();
    double quantity = trade.GetQuantity();
    string book = trade.GetBook();


    stringstream ss;
  
    ss << "Product: " << bond.GetProductId() 
        << ", Trade ID: " << tradeID 
        << ", Quantity: " << quantity 
        << ", Book: " << book 
        << ", price: " << price 
        << ", side: " << (side == BUY ? "BUY" : "SELL") << endl;
        
        
    string ss_string = ss.str();
    connector->Publish(ss_string);

  }

  void ProcessAdd(Trade<Bond>& data) override 
  {
    PersistData("executions.txt", data);
  }

  void ProcessUpdate(Trade<Bond>& data) override
  {
    PersistData("executions.txt", data);
  }

  void ProcessRemove(Trade<Bond>& data) override {}
};

// class BondPricingHistoricalDataService : public BondHistoricalDataService<Price<Bond>> 
// {

// private: 
//   FileConnector<string>* connector;
// public: 
//   BondPricingHistoricalDataService(const string& ) : BondHistoricalDataService<Price<Bond>>(filename) {
//     connector = new FileConnector<string>();
//   }

//   ~BondPricingHistoricalDataService() 
//   {
//     delete connector;
//   }

//   void PersistData(string persistKey, const Price<Bond>& price) override
//   {

//     const Bond& bond = price.GetProduct();
//     //string productId = bond.GetProductId();
//     double mid = price.GetMid();
//     double spread = price.GetBidOfferSpread();

//     stringstream ss;
    
//     ss << "Product: " << bond.GetProductId() 
//         << ", Mid: " << mid 
//         << ", Bid-Offer Spread: " << spread 
//         << endl;
        
//     string ss_string = ss.str();
//     connector->Publish(ss_string);

//   }
// };


#endif
