#ifndef ALGO_STREAMING_SERVICE_HPP
#define ALGO_STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"
#include "executionservice.hpp"
#include "streamingservice.hpp"
#include "pricingservice.hpp"


class AlgoStream 
{
private: 
    
    PriceStream<Bond> priceStream;
    

public: 
    AlgoStream(const Bond& product, const PriceStreamOrder& bidOrder, const PriceStreamOrder& offerOrder)
             : priceStream(product, bidOrder, offerOrder) {}
    AlgoStream() : priceStream(Bond(), PriceStreamOrder(0.0, 0, 0, BID), PriceStreamOrder(0.0, 0, 0, OFFER)) {}
    
    const PriceStream<Bond>& GetPriceStream() const {
        return priceStream;
    }

};

class BondAlgoStreamingService : public ServiceListener<Price<Bond>>
{
private: 
    struct SizeTracker {
        bool toggle;
        SizeTracker() : toggle(true){}
    };

    unordered_map<string, AlgoStream> algoStreamMap;
    vector<ServiceListener<PriceStream<Bond>>*> listeners;
    unordered_map<string, SizeTracker> sizeTrackers;

public: 
    
    AlgoStream& GetData(string productId) ;
    void OnMessage(AlgoStream& data) ;
    void AddListener(ServiceListener<PriceStream<Bond>>* listener);
    //const vector<ServiceListener<AlgoStream>*>& GetListeners() const ;
    //void AddListener(ServiceListener<AlgoStream>* listener) ;
    const vector<ServiceListener<PriceStream<Bond>>*>& GetListeners();
    void ProcessPrice(Price<Bond>& price);

    void ProcessAdd(Price<Bond>& price) override;
    void ProcessRemove(Price<Bond>& price) override;
    void ProcessUpdate(Price<Bond>& price) override;

};

void BondAlgoStreamingService::ProcessAdd(Price<Bond>& price) 
{
    ProcessPrice(price);
}

void BondAlgoStreamingService::ProcessRemove(Price<Bond>& price) 
{
}

void BondAlgoStreamingService::ProcessUpdate(Price<Bond>& price)
{
    ProcessPrice(price);
}

AlgoStream& BondAlgoStreamingService::GetData(string productId) 
{
    auto it = algoStreamMap.find(productId);
  if (it != algoStreamMap.end()) {
    return it->second;
  }
  throw runtime_error("Stream key not found: " + productId);
}

void BondAlgoStreamingService::OnMessage(AlgoStream& data) {}

void BondAlgoStreamingService::AddListener(ServiceListener<PriceStream<Bond>>* listener)
{
    listeners.push_back(listener);
}

//const vector<ServiceListener<AlgoStream>*>& BondAlgoStreamingService::GetListeners() const {}

//void BondAlgoStreamingService::AddListener(ServiceListener<AlgoStream>* listener) {}

const vector<ServiceListener<PriceStream<Bond>>*>& BondAlgoStreamingService::GetListeners() 
{
    return listeners;
}

void BondAlgoStreamingService::ProcessPrice(Price<Bond>& price) 
{
    string productId = price.GetProduct().GetProductId();
    bool isNew = (algoStreamMap.find(productId) == algoStreamMap.end());
    if (isNew) {
        PriceStreamOrder bidOrder(price.GetMid() - price.GetBidOfferSpread() / 2.0, 1000000, 2000000, BID);
        PriceStreamOrder offerOrder(price.GetMid() + price.GetBidOfferSpread() / 2.0, 1000000, 2000000, OFFER);
        algoStreamMap.emplace(make_pair(productId, AlgoStream(price.GetProduct(), bidOrder, offerOrder)));
        sizeTrackers.emplace(make_pair(productId, SizeTracker()));
    }
    
    AlgoStream& algoStream = algoStreamMap[productId];
    SizeTracker& tracker = sizeTrackers[productId];
    long newVisibleSize = tracker.toggle ? 1000000 : 2000000;
    tracker.toggle = !tracker.toggle;

    long newHiddenSize = newVisibleSize * 2;

    PriceStreamOrder newBidOrder(price.GetMid() - price.GetBidOfferSpread() / 2.0, newVisibleSize, newHiddenSize, BID);
    PriceStreamOrder newOfferOrder(price.GetMid() + price.GetBidOfferSpread() / 2.0, newVisibleSize, newHiddenSize, OFFER);

    algoStream = AlgoStream(price.GetProduct(), newBidOrder, newOfferOrder);
    PriceStream<Bond> priceStream = algoStream.GetPriceStream();

    if (isNew) {
        for (auto listener : listeners) {
            listener->ProcessAdd(priceStream);
        }
    }
    else {
        for (auto listener : listeners) {
            listener->ProcessUpdate(priceStream);
        }
    }


}



#endif
