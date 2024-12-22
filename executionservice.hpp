/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "marketdataservice.hpp"
#include <fstream>
#include "tradebookingservice.hpp"

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

namespace Execution {
enum ExecutionState { EXECUTED, CANCELLED, REJECTED };
}


/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder
{

public:

  // ctor for an order
  ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder);

  // Get the product
  const T& GetProduct() const;

  // Get the order ID
  const string& GetOrderId() const;

  // Get the order type on this order
  OrderType GetOrderType() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity
  long GetHiddenQuantity() const;

  // Get the parent order ID
  const string& GetParentOrderId() const;

  // Is child order?
  bool IsChildOrder() const;

  PricingSide GetSide() const;


private:
  T product;
  PricingSide side;
  string orderId;
  OrderType orderType;
  double price;
  double visibleQuantity;
  double hiddenQuantity;
  string parentOrderId;
  bool isChildOrder;

};

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class ExecutionService : public Service<string,ExecutionOrder <T> >
{

public:

  // Execute an order on a market
  virtual void ExecuteOrder(const ExecutionOrder<T>& order, Market market) = 0;

};

template<typename T>
ExecutionOrder<T>::ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
  product(_product)
{
  side = _side;
  orderId = _orderId;
  orderType = _orderType;
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  parentOrderId = _parentOrderId;
  isChildOrder = _isChildOrder;
}

template<typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& ExecutionOrder<T>::GetOrderId() const
{
  return orderId;
}

template<typename T>
PricingSide ExecutionOrder<T>::GetSide() const
{
  return side;
}

template<typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
  return orderType;
}

template<typename T>
double ExecutionOrder<T>::GetPrice() const
{
  return price;
}

template<typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
  return visibleQuantity;
}

template<typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
const string& ExecutionOrder<T>::GetParentOrderId() const
{
  return parentOrderId;
}

template<typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
  return isChildOrder;
}

// class BondExecutionConnector : public Connector<ExecutionOrder<Bond>>
// {
// private: 
//   string filename;

// public: 
//   BondExecutionConnector(const string& _filename);
//   void Publish(ExecutionOrder<Bond>& executionOrder) override;

// };

// BondExecutionConnector::BondExecutionConnector(const string& _filename)
// {
//   filename = _filename;
// }

// void BondExecutionConnector::Publish(ExecutionOrder<Bond>& executionOrder)
// {
//   ofstream outFile(filename, ios::app);
//   if (outFile.is_open()) {
//     outFile << "Order ID: " << executionOrder.GetOrderId() << ", "
//             << "Product: " << executionOrder.GetProduct().GetProductId() << ", "
//             << "Side: " << (executionOrder.GetSide() == BID ? "BUY" : "SELL")// fix
//             << "Order Type: ";
//     switch (executionOrder.GetOrderType()) {
//       case FOK: outFile << "FOK"; break;
//       case IOC: outFile << "IOC"; break;
//       case MARKET: outFile << "MARKET"; break;
//       case LIMIT: outFile << "LIMIT"; break;
//       case STOP: outFile << "STOP"; break;
//       default: outFile << "Unknown"; break;
//     }
//     outFile << "Price: " << executionOrder.GetPrice() << ", "
//             << "Visible Quantity: " << executionOrder.GetVisibleQuantity() << ", "
//             << "Hidden Quantity: " << executionOrder.GetHiddenQuantity() << ", "
//             << "Parent Order ID: " << executionOrder.GetParentOrderId() << ", "
//             << "Is Child Order: " << (executionOrder.IsChildOrder() ? "YES" : "NO") << ", "
//             << "State: ";
//     outFile << endl;
//     outFile.close();
//   }
//   else {
//     cerr << "Error: Could not open file " << filename << " to write. " << endl;
//   }

// }

class BondExecutionService : public ServiceListener<ExecutionOrder<Bond>>
{
private: 
  unordered_map<string, ExecutionOrder<Bond>> executionMap;
  vector<ServiceListener<Trade<Bond>>*> listeners;
  // Connector<ExecutionOrder<Bond>>* connector;
  //TradeBookingService<Bond>* tradeBookingService;
  vector<string> tradeBooks = {"TRSY1", "TRSY2", "TRSY3"};
  size_t currentTradeBookIndex = 0;

public: 
  // BondExecutionService(Connector<ExecutionOrder<Bond>>* _connector);
  // ~BondExecutionService();
  ExecutionOrder<Bond>& GetData(string key);
  void OnMessage(ExecutionOrder<Bond> &data);
  void AddListener(ServiceListener<Trade<Bond>>* listener);
  const vector<ServiceListener<Trade<Bond>>*>& GetListeners();
  void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market);

  void ProcessAdd(ExecutionOrder<Bond>& data) override;
  void ProcessRemove(ExecutionOrder<Bond>& data) override;
  void ProcessUpdate(ExecutionOrder<Bond>& data) override;

};

// BondExecutionService::BondExecutionService(Connector<ExecutionOrder<Bond>>* _connector)
// {
//   connector = _connector;
//   currentTradeBookIndex = 0;
// }

// BondExecutionService::~BondExecutionService()
// {
//   delete connector;
// }

inline ExecutionOrder<Bond>& BondExecutionService::GetData(string key)
{
  auto it = executionMap.find(key);
  if (it != executionMap.end()) {
      return it->second;
  } else {
      throw std::runtime_error("ExecutionOrder not found for key: " + key);
  }
}

inline void BondExecutionService::OnMessage(ExecutionOrder<Bond> &data)
{
}

inline void BondExecutionService::AddListener(ServiceListener<Trade<Bond>>* listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<Trade<Bond>>*>& BondExecutionService::GetListeners() 
{
  return listeners;
}

inline void BondExecutionService::ExecuteOrder(const ExecutionOrder<Bond>& order, Market market)
{
  // bool executedFully = false;
  // long executedQuantity = 0;

  long executionQuant = (order.GetVisibleQuantity() > 0) ? order.GetVisibleQuantity() : order.GetHiddenQuantity();

  if (order.IsChildOrder()) {
    string productId = order.GetParentOrderId();
    auto it = executionMap.find(productId);
    if (it == executionMap.end()) {
      cerr << "Parent Order not found for child order " << order.GetOrderId() << endl;
      return;
    }
  }
  else {
    executionMap.emplace(order.GetOrderId(), order);
  }

  Execution::ExecutionState executionState = Execution::REJECTED;
  for (auto Currmarket : {BROKERTEC, ESPEED, CME}) {
    switch (order.GetOrderType()) {
      case FOK: 
        if (executionQuant >= order.GetVisibleQuantity()) {
          executionState = Execution::EXECUTED;
        }
        else {
          executionState = Execution::CANCELLED;
        }
        break;
      
      case IOC: 
        executionState = (executionQuant > 0) ? Execution::EXECUTED : Execution::CANCELLED;
        break; 
      case MARKET: 
        executionState = Execution::EXECUTED;
        break;
      // case LIMIT:
      //   executionState = (order.GetPrice() >= )
    }
    if (executionState == Execution::CANCELLED) {
      cerr << "Order Cancelled. Order ID " << order.GetOrderId() << endl;
      return;
    }
    else if (executionState == Execution::EXECUTED) {
      break;
    }
    cerr << "Order rejected in market " << Currmarket << ", Order ID: " << order.GetOrderId() << endl;
  }

  if (executionState == Execution::EXECUTED) {
    string orderId = order.GetOrderId();
    auto it = executionMap.find(orderId);
    bool isNew = (it == executionMap.end());
    executionMap.emplace(orderId, order);

    string tradeBook = tradeBooks[currentTradeBookIndex];
    currentTradeBookIndex = (currentTradeBookIndex + 1) % tradeBooks.size();

    string tradeId = "TRADE_" + order.GetOrderId();

    Side orderSide = (order.GetSide() == BID ? BUY : SELL);

    Trade<Bond> trade(order.GetProduct(), tradeId, order.GetPrice(), tradeBook, order.GetVisibleQuantity(), orderSide);

    for (auto listener : listeners) { 
      if (isNew) {
        listener->ProcessAdd(trade);
      }
      else {
        listener->ProcessUpdate(trade);
      }
    }
  }

}

inline void BondExecutionService::ProcessAdd(ExecutionOrder<Bond>& data)
{
    
    ExecuteOrder(data, BROKERTEC); 
}

inline void BondExecutionService::ProcessRemove(ExecutionOrder<Bond>& data)
{
    std::string orderId = data.GetOrderId();
    auto it = executionMap.find(orderId);
    if (it != executionMap.end()) {
        executionMap.erase(it);
    }
}

inline void BondExecutionService::ProcessUpdate(ExecutionOrder<Bond>& data)
{
    
    ExecuteOrder(data, BROKERTEC);
}


#endif
