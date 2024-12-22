/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "productservice.hpp"
#include "soa.hpp"
#include "products.hpp"


/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price(const T &_product, double _mid, double _bidOfferSpread);
  Price();

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

private:
  T product;
  double mid;
  double bidOfferSpread;

};

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PricingService : public Service<string,Price <T> >
{
};

template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
Price<T>::Price() : product(T()), mid(0.0), bidOfferSpread(0.0) { }


template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}

class BondPricingService : public PricingService<Bond>
{
private: 
  unordered_map<string, Price<Bond>> bondPriceMap;
  vector<ServiceListener<Price<Bond>>*> listeners;

public:
  Price<Bond>& GetData(string key) override;

  void OnMessage(Price<Bond> &data) override;

  void AddListener(ServiceListener<Price<Bond>> *listener) override;

  const vector<ServiceListener<Price<Bond>>*>& GetListeners() const override;

};

inline Price<Bond>& BondPricingService::GetData(string key)
  {
    auto it = bondPriceMap.find(key);
    if (it != bondPriceMap.end()) {
      return it->second;
    }
    throw runtime_error("Price key not found: " + key);
  }

inline void BondPricingService::OnMessage(Price<Bond> &data)
{
  string productId = data.GetProduct().GetProductId();

  auto it = bondPriceMap.find(productId);
  if (it == bondPriceMap.end()) 
  {
    bondPriceMap.insert(make_pair(productId, data));
    for (auto listener : listeners) 
    {
      listener->ProcessAdd(data);
    }
  }
  else 
  {
    bondPriceMap.erase(it);
    bondPriceMap.insert(make_pair(productId, data));
    for (auto listener : listeners) 
    {
      listener->ProcessUpdate(data);
    }
  }

    
}

inline void BondPricingService::AddListener(ServiceListener<Price<Bond>> *listener) 
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<Price<Bond>>*>& BondPricingService::GetListeners() const 
{
  return listeners;
}


class BondPricingConnector : public Connector<Price<Bond>>
{
private: 
  BondPricingService* service;
  string filename;
  BondProductService* bondProductService;

  double ConvertToDouble(const string &fraction) const;

public: 
  BondPricingConnector(BondPricingService* service, const string& file, BondProductService* bondProductService) 
              : service(service), filename(file), bondProductService(bondProductService) {}
              
  void Subscribe();

  void Publish(Price<Bond> &data) override {}
};

//BondPricingConnector::~

inline void BondPricingConnector::Subscribe() 
{
  ifstream file("/Users/joshlevine/Desktop/tradingsystem/prices.txt");
  if (!file.is_open()) {
    cerr << "failed to open file " << filename << endl;
    return;
  }

  string line;
  while (getline(file, line)) {
    if (line.empty()) continue;
    istringstream iss(line);
    
    string productId, midFraction, spreadFraction, timeStamp;
    iss >> productId >> midFraction >> spreadFraction >> timeStamp;

    Bond bond = bondProductService->GetData(productId);
    double mid = ConvertToDouble(midFraction);
    double spread = ConvertToDouble(spreadFraction);

    Price<Bond> bondPrice(bond, mid, spread);

    service->OnMessage(bondPrice);

  }
  file.close();

}

inline double BondPricingConnector::ConvertToDouble(const string& fraction) const 
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



#endif
