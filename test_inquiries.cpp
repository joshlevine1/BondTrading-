#include <iostream>

#include "inquiryservice.hpp"
#include "historicaldataservice.hpp"
#include "products.hpp"  
#include "productservice.hpp"


int main()
{
    
    BondInquiryService* inquiryService = new BondInquiryService();

    
    BondInquiryHistoricalDataService* hist_inq = new BondInquiryHistoricalDataService();

    
    inquiryService->AddListener(hist_inq);
    
    
    BondProductService* bondProductService = new BondProductService(); 

    //BondProductService* bondProductService = new BondProductService();
        
        
        bondProductService->Add(Bond("T2Y", CUSIP, "TICKER1", 0.02f, {2024, 12, 22}));
        bondProductService->Add(Bond("T3Y", CUSIP, "TICKER2", 0.025f, {2025, 6, 15}));
        bondProductService->Add(Bond("T5Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T7Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T10Y", CUSIP, "TICKER4", 0.035f, {2030, 1, 20}));
        bondProductService->Add(Bond("T20Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T30Y", CUSIP, "TICKER5", 0.04f, {2050, 5, 10}));
    InquiryConnector* connector = new InquiryConnector(inquiryService, bondProductService, std::string("inquiries.txt"));

    
    connector->Subscribe();

    std::cout << "Finished processing prices. Check streaming.txt for output." << std::endl;

    delete inquiryService;
    delete hist_inq;
    delete bondProductService;
    delete connector;

    
    return 0;
}
