/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "products.hpp"
#include <algorithm>
#include "productservice.hpp"
#include <fstream> 
#include <sstream>

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order(double _price, long _quantity, PricingSide _side);

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};

/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder);

  // Get the bid order
  const Order& GetBidOrder() const;

  // Get the offer order
  const Order& GetOfferOrder() const;

private:
  Order bidOrder;
  Order offerOrder;

};

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:

  // ctor for the order book
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);

  // Get the product
  const T& GetProduct() const;

  // Get the bid stack
  const vector<Order>& GetBidStack() const;

  // Get the offer stack
  const vector<Order>& GetOfferStack() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{

public:

  // Get the best bid/offer order
  virtual const BidOffer GetBestBidOffer(const string &productId) = 0;

  // Aggregate the order book
  virtual const OrderBook<T>& AggregateDepth(const string &productId) = 0;

};

Order::Order(double _price, long _quantity, PricingSide _side)
{
  price = _price;
  quantity = _quantity;
  side = _side;
}

double Order::GetPrice() const
{
  return price;
}
 
long Order::GetQuantity() const
{
  return quantity;
}
 
PricingSide Order::GetSide() const
{
  return side;
}

BidOffer::BidOffer(const Order &_bidOrder, const Order &_offerOrder) :
  bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

const Order& BidOffer::GetBidOrder() const
{
  return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
  return offerOrder;
}

template<typename T>
OrderBook<T>::OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack) :
  product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const T& OrderBook<T>::GetProduct() const
{
  return product;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetBidStack() const
{
  return bidStack;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetOfferStack() const
{
  return offerStack;
}

class BondMarketDataService : public MarketDataService<Bond>
{
private: 
  unordered_map<string, OrderBook<Bond>> orderBookMap;
  unordered_map<string, OrderBook<Bond>> aggregatedOrderBookMap;
  vector<ServiceListener<OrderBook<Bond>>*> listeners;

public: 
  OrderBook<Bond>& GetData(string key) override;
  void OnMessage(OrderBook<Bond> &data) override;
  void AddListener(ServiceListener<OrderBook<Bond>> *listener) override;
  const vector<ServiceListener<OrderBook<Bond>>*>& GetListeners() const override;
  BidOffer const GetBestBidOffer(const string &productId) override;
  const OrderBook<Bond>& AggregateDepth(const string &productId) override;

};

inline OrderBook<Bond>& BondMarketDataService::GetData(string key) 
{
  auto it = orderBookMap.find(key);
  if (it != orderBookMap.end()) {
      return it->second;
  } else {
      throw std::runtime_error("OrderBook not found for key: " + key);
  }
}

inline void BondMarketDataService::OnMessage(OrderBook<Bond> &data) 
{
  string productId = data.GetProduct().GetProductId();

  auto it = orderBookMap.find(productId);
  bool isNew = (it == orderBookMap.end());

  vector<Order> sortedBidStack = data.GetBidStack();
  sort(sortedBidStack.begin(), sortedBidStack.end(), 
  [](const Order &a, const Order &b) -> bool {
    return a.GetPrice() > b.GetPrice();
  });

  vector<Order> sortedOfferStack = data.GetOfferStack();
  sort(sortedOfferStack.begin(), sortedOfferStack.end(), 
  [](const Order &a, const Order &b) -> bool {
    return a.GetPrice() < b.GetPrice();
  });

  OrderBook<Bond> sortedOrderBook(data.GetProduct(), sortedBidStack, sortedOfferStack);

  orderBookMap.emplace(productId, sortedOrderBook);
  AggregateDepth(productId);


  for (auto listener : listeners) {
    if (isNew) {
      listener->ProcessAdd(data);
    }
    else {
      listener->ProcessUpdate(data);
    }
  }
}

inline void BondMarketDataService::AddListener(ServiceListener<OrderBook<Bond>> *listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<OrderBook<Bond>>*>& BondMarketDataService::GetListeners() const
{
  return listeners;
}

inline const BidOffer BondMarketDataService::GetBestBidOffer(const string &productId) 
{
  //const OrderBook<Bond> &orderBook = orderBookMap.at(productId);
  auto it = orderBookMap.find(productId);
  if (it != orderBookMap.end()) {
    const OrderBook<Bond> &orderBook = it->second;
    const vector<Order> &bidStack = orderBook.GetBidStack();
    const vector<Order> &offerStack = orderBook.GetOfferStack();
    return BidOffer(bidStack.front(), offerStack.front());
  }
  else {
    throw std::runtime_error("OrderBook not found for key: " + productId);
  }
  
}

inline const OrderBook<Bond>& BondMarketDataService::AggregateDepth(const string &productId)
{
  auto it = orderBookMap.find(productId);
    if (it == orderBookMap.end()) {
        throw runtime_error("Order Book not found for product ID: " + productId);
    }

    const OrderBook<Bond> &orderBook = it->second;
    // Aggregation logic optimized with unordered_map
    map<double, long, greater<double>> aggregatedBids;
    for (const Order &order : orderBook.GetBidStack()) {
        aggregatedBids[order.GetPrice()] += order.GetQuantity();
    }

    map<double, long, less<double>> aggregatedOffers;
    for (const Order &order : orderBook.GetOfferStack()) {
        aggregatedOffers[order.GetPrice()] += order.GetQuantity();
    }

    // Rebuild aggregated stacks
    vector<Order> newBidStack;
    for (const auto &entry : aggregatedBids) {
        newBidStack.emplace_back(entry.first, entry.second, BID);
    }

    vector<Order> newOfferStack;
    for (const auto &entry : aggregatedOffers) {
        newOfferStack.emplace_back(entry.first, entry.second, OFFER);
    }

    aggregatedOrderBookMap.emplace(productId, OrderBook<Bond>(orderBook.GetProduct(), newBidStack, newOfferStack));
    return aggregatedOrderBookMap.at(productId);

}

class MarketDataConnector : public Connector<OrderBook<Bond>>
{
private:
  BondMarketDataService* marketDataService;
  BondProductService *productService;
  string filename;

public: 
  MarketDataConnector(BondMarketDataService* marketDataService, BondProductService *productService, const string& file) 
                  : marketDataService(marketDataService), productService(productService), filename(file) {}
  
  void Publish(OrderBook<Bond> &data) override;
  void Subscribe();
  double ConvertToDouble(const string& fraction) const;

};

// MarketDataConnector::MarketDataConnector(BondMarketDataService* marketDataService, BondProductService &productService, const string& file) 
//                   : marketDataService(marketDataService), productService(productService), filename(file) {}

inline double MarketDataConnector::ConvertToDouble(const string& fraction) const 
{
  // 100-25+

  size_t dashPos = fraction.find("-");
  if (dashPos == string::npos){ return stod(fraction);}
  string intPart = fraction.substr(0, dashPos);
  string fracPart = fraction.substr(dashPos + 1);

  int integerPart = stoi(intPart);
  int xy = 0;
  int z = 0;

  if (fracPart.size() == 2) {
    xy = 0;
    z = stoi(fracPart);
  }

  else if (fracPart.size() > 2) {
    string xyStr = fracPart.substr(0, 2);
    xy = stoi(xyStr);

    char thirdChar = fracPart[2];
    if (thirdChar == '+') {
      z = 4;
    }
    else if (isdigit(thirdChar)) {
      z = thirdChar - '0';
    }

  }
  else {
    throw runtime_error("Invalid format: " + fraction);
  }

  double total256 = 8.0 * xy + z;
  double fractionDecimal  = total256 / 256.0;
  double result = static_cast<double>(integerPart) + fractionDecimal;
  return result;

}


void MarketDataConnector::Publish(OrderBook<Bond> &data){}

void MarketDataConnector::Subscribe() 
{

  ifstream file("/Users/joshlevine/Desktop/tradingsystem/marketdata.txt"); 
  if (!file.is_open()) {
    cerr << "Could not open file " << filename << endl;
    return; 
  }

  string line; 
  while (getline(file, line)) {
    if (line.empty()) continue;
    stringstream ss(line);
    string productId;
    getline(ss, productId, ',');
    if (productId.empty()) {
      cerr << "empty product ID line" << line << endl;
      continue;
    }

    vector<Order> bidStack, offerStack;
    for (int i = 0; i < 5; ++i) {
      string priceFraction, quantStr;
      getline(ss, priceFraction, ',');
      getline(ss, quantStr, ',');
      if (priceFraction.empty() || quantStr.empty()) {
        cerr << "error parsing bid stack for " << productId << endl;
        break;
      }
      double price = ConvertToDouble(priceFraction);
      long quantity = stol(quantStr);
      bidStack.emplace_back(price, quantity, BID);
    }
    for (int i = 0; i < 5; ++i) {
      string priceFraction, quantStr;
      getline(ss, priceFraction, ',');
      getline(ss, quantStr, ',');
      if (priceFraction.empty() || quantStr.empty()) {
        cerr << "error parsing offer stack for " << productId << endl;
        break;
      }
      double price = ConvertToDouble(priceFraction);
      long quantity = stol(quantStr);
      offerStack.emplace_back(price, quantity, OFFER);
    }
    try {
      Bond bond = productService->GetData(productId);
      OrderBook<Bond> orderBook(bond, bidStack, offerStack);
      marketDataService->OnMessage(orderBook);
    } catch (const exception& e) {
      cerr << productId << "not found in BondProdctService " << e.what() << endl;

    }

  }
  file.close();
      
  
}


#endif

