#include <iostream>
#include "pricingservice.hpp"
#include "streamingservice.hpp"
#include "algostreamingservice.hpp"
#include "historicaldataservice.hpp"
#include "products.hpp"  
#include "productservice.hpp"
#include "guiservice.hpp"

int main()
{
    
    BondPricingService* bondPricingService = new BondPricingService();
    BondAlgoStreamingService* bondAlgoStreamingService = new BondAlgoStreamingService();
    
    
    Connector<PriceStream<Bond>>* streamingConnector = nullptr; 
    auto* bondStreamingService = new BondStreamingService(streamingConnector);

    GUIService* gui = new GUIService("gui.txt");

    
    BondStreamingHistoricalDataService* bondStreamingHistoricalService = new BondStreamingHistoricalDataService();

    
    bondPricingService->AddListener(bondAlgoStreamingService);
    
    
    bondAlgoStreamingService->AddListener(bondStreamingService);
    
    
    bondStreamingService->AddListener(bondStreamingHistoricalService);

    bondPricingService->AddListener(gui);
    
    
    BondProductService* bondProductService = new BondProductService(); 

    //BondProductService* bondProductService = new BondProductService();
        
        
        bondProductService->Add(Bond("T2Y", CUSIP, "TICKER1", 0.02f, {2024, 12, 22}));
        bondProductService->Add(Bond("T3Y", CUSIP, "TICKER2", 0.025f, {2025, 6, 15}));
        bondProductService->Add(Bond("T5Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T7Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T10Y", CUSIP, "TICKER4", 0.035f, {2030, 1, 20}));
        bondProductService->Add(Bond("T20Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T30Y", CUSIP, "TICKER5", 0.04f, {2050, 5, 10}));
    BondPricingConnector* pricingConnector = new BondPricingConnector(bondPricingService, std::string("prices.txt"), bondProductService);

    
    pricingConnector->Subscribe();


    std::cout << "Finished processing prices. Check streaming.txt for output." << std::endl;

    delete pricingConnector;
    delete bondProductService;
    delete bondStreamingHistoricalService;
    delete bondStreamingService;
    delete bondAlgoStreamingService;
    delete bondPricingService;
    delete gui;

    
    return 0;
}
