#include <iostream>
#include "pricingservice.hpp"
#include "streamingservice.hpp"
#include "algostreamingservice.hpp"
#include "historicaldataservice.hpp"
#include "products.hpp"  
#include "productservice.hpp"
#include "guiservice.hpp"
#include "inquiryservice.hpp"
#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "riskservice.hpp"
#include "marketdataservice.hpp"
#include "algoexecutionservice.hpp"
#include "executionservice.hpp"

int main()
{
    // 1) Create services
    BondProductService* bondProductService = new BondProductService();

    // Manually add Bonds to BondProductService
    bondProductService->Add(Bond("T2Y", CUSIP, "TICKER1", 0.02f, {2024, 12, 22}));
    bondProductService->Add(Bond("T3Y", CUSIP, "TICKER2", 0.025f, {2025, 6, 15}));
    bondProductService->Add(Bond("T5Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
    bondProductService->Add(Bond("T7Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
    bondProductService->Add(Bond("T10Y", CUSIP, "TICKER4", 0.035f, {2030, 1, 20}));
    bondProductService->Add(Bond("T20Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
    bondProductService->Add(Bond("T30Y", CUSIP, "TICKER5", 0.04f, {2050, 5, 10}));

    // Services
    BondPricingService* bondPricingService = new BondPricingService();
    BondAlgoStreamingService* bondAlgoStreamingService = new BondAlgoStreamingService();
    BondStreamingService* bondStreamingService = new BondStreamingService(nullptr); // Replace nullptr with an appropriate connector
    BondStreamingHistoricalDataService* bondStreamingHistoricalService = new BondStreamingHistoricalDataService();
    GUIService* gui = new GUIService("gui.txt");
    BondInquiryService* inquiryService = new BondInquiryService();
    BondInquiryHistoricalDataService* inquiryHistoricalService = new BondInquiryHistoricalDataService();
    BondTradeBookingService* bondTradeBookingService = new BondTradeBookingService();
    BondPositionService* bondPositionService = new BondPositionService();
    BondRiskService* bondRiskService = new BondRiskService(*bondPricingService);
    BondPositionHistoricalDataService* positionHistoricalService = new BondPositionHistoricalDataService();
    BondRiskHistoricalDataService* riskHistoricalService = new BondRiskHistoricalDataService();
    BondMarketDataService* bondMarketDataService = new BondMarketDataService();
    BondAlgoExecutionService* algoExecutionService = new BondAlgoExecutionService();
    BondExecutionService* bondExecutionService = new BondExecutionService();
    BondExecutionHistoricalDataService* executionHistoricalService = new BondExecutionHistoricalDataService();

    // 2) Register Listeners
    bondPricingService->AddListener(bondAlgoStreamingService);
    bondAlgoStreamingService->AddListener(bondStreamingService);
    bondStreamingService->AddListener(bondStreamingHistoricalService);
    bondPricingService->AddListener(gui);
    inquiryService->AddListener(inquiryHistoricalService);
    bondTradeBookingService->AddListener(bondPositionService);
    bondPositionService->AddListener(bondRiskService);
    bondPositionService->AddListener(positionHistoricalService);
    bondRiskService->AddListener(riskHistoricalService);
    bondMarketDataService->AddListener(algoExecutionService);
    algoExecutionService->AddListener(bondExecutionService);
    bondExecutionService->AddListener(executionHistoricalService);
    bondExecutionService->AddListener(bondTradeBookingService);

    // 3) Create and link connectors
    BondPricingConnector* pricingConnector = new BondPricingConnector(bondPricingService, "prices.txt", bondProductService);
    pricingConnector->Subscribe();

    InquiryConnector* inquiryConnector = new InquiryConnector(inquiryService, bondProductService, "inquiries.txt");
    inquiryConnector->Subscribe();

    TradeBookingServiceConnector* tradeBookingConnector = new TradeBookingServiceConnector(bondTradeBookingService, *bondProductService);
    tradeBookingConnector->ReadFile("trades.txt");

    MarketDataConnector* marketDataConnector = new MarketDataConnector(bondMarketDataService, bondProductService, "marketdata.txt");
    marketDataConnector->Subscribe();

    // Indicate completion
    std::cout << "All processes completed. Check output files for results." << std::endl;

    // 4) Clean up
    delete bondPricingService;
    delete bondAlgoStreamingService;
    delete bondStreamingService;
    delete bondStreamingHistoricalService;
    delete gui;
    delete inquiryService;
    delete inquiryHistoricalService;
    delete bondTradeBookingService;
    delete bondPositionService;
    delete bondRiskService;
    delete positionHistoricalService;
    delete riskHistoricalService;
    delete bondMarketDataService;
    delete algoExecutionService;
    delete bondExecutionService;
    delete executionHistoricalService;
    delete bondProductService;
    delete pricingConnector;
    delete inquiryConnector;
    delete tradeBookingConnector;
    delete marketDataConnector;

    return 0;
}
