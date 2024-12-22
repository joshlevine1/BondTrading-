/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "products.hpp"
#include "productservice.hpp"
#include <fstream>
#include <unordered_map>
//#include "executionservice.hpp"

// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */

using namespace std;

template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side);

  // Get the product
  const T& GetProduct() const;

  // Get the trade ID
  const string& GetTradeId() const;

  // Get the mid price
  double GetPrice() const;

  // Get the book
  const string& GetBook() const;

  // Get the quantity
  long GetQuantity() const;

  // Get the side
  Side GetSide() const;

private:
  T product;
  string tradeId;
  double price;
  string book;
  long quantity;
  Side side;

};

/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingService : public Service<string,Trade <T> >
{

public:

  // Book the trade
  virtual void BookTrade(const Trade<T> &trade) = 0;

};

template<typename T>
Trade<T>::Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side) :
  product(_product)
{
  tradeId = _tradeId;
  price = _price;
  book = _book;
  quantity = _quantity;
  side = _side;
}

template<typename T>
const T& Trade<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& Trade<T>::GetTradeId() const
{
  return tradeId;
}

template<typename T>
double Trade<T>::GetPrice() const
{
  return price;
}

template<typename T>
const string& Trade<T>::GetBook() const
{
  return book;
}

template<typename T>
long Trade<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
Side Trade<T>::GetSide() const
{
  return side;
}

template<typename T>
void TradeBookingService<T>::BookTrade(const Trade<T> &trade)
{
}

class BondTradeBookingService : public TradeBookingService<Bond>, public ServiceListener<Trade<Bond>>
{
private: 
  unordered_map<string, Trade<Bond>> tradeMap;
  vector<ServiceListener<Trade<Bond>>*> listeners;
  //BondExecutionService* executionService;

public: 
  Trade<Bond>& GetData(string key) override;
  void OnMessage(Trade<Bond> &data) override;
  void AddListener(ServiceListener<Trade<Bond>> *listener) override;
  const vector<ServiceListener<Trade<Bond>>*>& GetListeners() const override;
  void BookTrade(const Trade<Bond> &trade) override; 

  void ProcessAdd(Trade<Bond>& data) override;
  void ProcessRemove(Trade<Bond>& data) override;
  void ProcessUpdate(Trade<Bond> &data) override;
  
};


inline Trade<Bond>& BondTradeBookingService::GetData(string key)
{
  auto it = tradeMap.find(key);
  if (it != tradeMap.end()) {
    return it->second;
  }
  throw runtime_error("Trade ID not found: " + key);
  
}

inline void BondTradeBookingService::OnMessage(Trade<Bond> &data) 
{
  string tradeID = data.GetTradeId();
  auto it = tradeMap.find(tradeID);
  bool isNew = (it == tradeMap.end());

  tradeMap.emplace(tradeID, data);

  if (isNew) {
    for (auto listener : listeners) {
      listener->ProcessAdd(data);
    }
  }
  else {
    for (auto listener : listeners) {
      listener->ProcessUpdate(data);
    }
  }
}

inline void BondTradeBookingService::AddListener(ServiceListener<Trade<Bond>> *listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<Trade<Bond>>*>& BondTradeBookingService::GetListeners() const 
{
  return listeners;
}

inline void BondTradeBookingService::BookTrade(const Trade<Bond> &trade)
{
  Trade<Bond> t(trade.GetProduct(), trade.GetTradeId(), trade.GetPrice(), trade.GetBook(), trade.GetQuantity(), trade.GetSide());
  OnMessage(t);
}

inline void BondTradeBookingService::ProcessAdd(Trade<Bond> &data)
{
  // cout << "Execution add received - Trade ID: " 
  //     << data.GetTradeId() << ", side: " << ((data.GetSide() == BUY) ? "BUY" : "SELL")
  //     << ", quantity: " << data.GetQuantity() << endl;
  BookTrade(data);

}

inline void BondTradeBookingService::ProcessRemove(Trade<Bond> &data)
{
  // cout << "Execution remove received - Trade ID: " 
  //       << data.GetTradeId() << endl;
  auto it = tradeMap.find(data.GetTradeId());
  if (it != tradeMap.end()) {
    tradeMap.erase(it);
    for (auto listener : listeners) {
      listener->ProcessRemove(data);
    }
  }

}

inline void BondTradeBookingService::ProcessUpdate(Trade<Bond> &data)
{
  // cout << "Execution update received - Trade ID: " 
  //     << data.GetTradeId() << ", side: " << ((data.GetSide() == BUY) ? "BUY" : "SELL")
  //     << ", quantity: " << data.GetQuantity() << endl;
  BookTrade(data);
}

class TradeBookingServiceConnector : public Connector<Trade<Bond>>
{
private: 
  BondTradeBookingService* tradeBookingService;
  BondProductService &bondProductService;

public: 
  TradeBookingServiceConnector(BondTradeBookingService* _service, BondProductService &bondProductService)
    : tradeBookingService(_service), bondProductService(bondProductService) {}
  void ReadFile(const string &filename);
  void Publish(Trade<Bond> &data) override {}

};

// TradeBookingServiceConnector::TradeBookingServiceConnector(BondTradeBookingService* _service, BondProductService &bondProductService)
//     : tradeBookingService(_service), bondProductService(bondProductService) {}

void TradeBookingServiceConnector::ReadFile(const string& filename)
{
    ifstream file("/Users/joshlevine/Desktop/tradingsystem/trades.txt");
    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        string productId, tradeId, book, sideStr;
        double price;
        long quantity;
        Side side;

        // Assuming trade.txt format: ProductId,TradeId,Price,Book,Quantity,Side (BUY/SELL)
        getline(ss, productId, ',');
        getline(ss, tradeId, ',');
        ss >> price;
        ss.ignore(); 
        getline(ss, book, ',');
        ss >> quantity;
        ss.ignore(); 
        getline(ss, sideStr, ',');

        
        side = (sideStr == "BUY") ? BUY : SELL;

        // Create a Bond product (assuming Bond constructor takes a productId)
        Bond bond = bondProductService.GetData(productId);

        // Create the Trade object
        Trade<Bond> trade(bond, tradeId, price, book, quantity, side);

        // Publish the trade to the service
        tradeBookingService->OnMessage(trade);
    }

    file.close();
}

#endif
