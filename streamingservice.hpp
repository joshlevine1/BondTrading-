/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"

/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder
{

public:

  // ctor for an order
  PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side);

  // The side on this order
  PricingSide GetSide() const;

  // Get the price on this order
  double GetPrice() const;

  // Get the visible quantity on this order
  long GetVisibleQuantity() const;

  // Get the hidden quantity on this order
  long GetHiddenQuantity() const;

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};

/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template<typename T>
class PriceStream
{

public:

  // ctor
  PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder);


  // Get the product
  const T& GetProduct() const;

  // Get the bid order
  const PriceStreamOrder& GetBidOrder() const;

  // Get the offer order
  const PriceStreamOrder& GetOfferOrder() const;

private:
  T product;
  PriceStreamOrder bidOrder;
  PriceStreamOrder offerOrder;

};

/**
 * Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class StreamingService : public Service<string,PriceStream <T> >
{

public:

  // Publish two-way prices
  virtual void PublishPrice(PriceStream<T>& priceStream) = 0;

};

PriceStreamOrder::PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side)
{
  price = _price;
  visibleQuantity = _visibleQuantity;
  hiddenQuantity = _hiddenQuantity;
  side = _side;
}

double PriceStreamOrder::GetPrice() const
{
  return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
  return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
  return hiddenQuantity;
}

template<typename T>
PriceStream<T>::PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder) :
  product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

template<typename T>
const T& PriceStream<T>::GetProduct() const
{
  return product;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
  return bidOrder;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
  return offerOrder;
}

class BondStreamingService : public StreamingService<Bond>, public ServiceListener<PriceStream<Bond>>
{
private: 
  unordered_map<string, PriceStream<Bond>> priceStreamMap;
  vector<ServiceListener<PriceStream<Bond>>*> listeners;
  Connector<PriceStream<Bond>>* connector;

public: 
  BondStreamingService(Connector<PriceStream<Bond>>* _connector);
  PriceStream<Bond>& GetData(string key) override;
  void OnMessage(PriceStream<Bond>& data) override;
  void AddListener(ServiceListener<PriceStream<Bond>>* listener) override;
  const vector<ServiceListener<PriceStream<Bond>>*>& GetListeners() const override;
  void PublishPrice(PriceStream<Bond>& priceStream) override;

  void ProcessAdd(PriceStream<Bond> &price) override;
  void ProcessRemove(PriceStream<Bond> &price) override;
  void ProcessUpdate(PriceStream<Bond> &price) override;

};

void BondStreamingService::ProcessAdd(PriceStream<Bond> &price)
{
  PublishPrice(price);
}
void BondStreamingService::ProcessRemove(PriceStream<Bond> &price) {}
void BondStreamingService::ProcessUpdate(PriceStream<Bond> &price) 
{
  PublishPrice(price);
}

BondStreamingService::BondStreamingService(Connector<PriceStream<Bond>>* _connector)
{
  connector = _connector;
}

inline PriceStream<Bond>& BondStreamingService::GetData(string key)
{
  auto it = priceStreamMap.find(key);
  if (it != priceStreamMap.end()) {
      return it->second;
  } else {
      throw std::runtime_error("PriceStream not found for key: " + key);
  }
}

inline void BondStreamingService::OnMessage(PriceStream<Bond>& data)
{
}

inline void BondStreamingService::AddListener(ServiceListener<PriceStream<Bond>>* listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<PriceStream<Bond>>*>& BondStreamingService::GetListeners() const
{
  return listeners;
}

inline void BondStreamingService::PublishPrice(PriceStream<Bond>& priceStream)
{
  string productId = priceStream.GetProduct().GetProductId();
  bool isNew = (priceStreamMap.find(productId) == priceStreamMap.end());

  priceStreamMap.emplace(productId, priceStream);
  for (auto listener : listeners) {
    if (isNew) {
      listener->ProcessAdd(priceStream);
    }
    else {
      listener->ProcessUpdate(priceStream);
    }
  }

  if (connector) {
    connector->Publish(priceStream);
  }
}


#endif
